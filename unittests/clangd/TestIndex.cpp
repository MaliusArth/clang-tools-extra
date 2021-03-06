//===-- IndexHelpers.cpp ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TestIndex.h"

namespace clang {
namespace clangd {

Symbol symbol(llvm::StringRef QName) {
  Symbol Sym;
  Sym.ID = SymbolID(QName.str());
  size_t Pos = QName.rfind("::");
  if (Pos == llvm::StringRef::npos) {
    Sym.Name = QName;
    Sym.Scope = "";
  } else {
    Sym.Name = QName.substr(Pos + 2);
    Sym.Scope = QName.substr(0, Pos + 2);
  }
  return Sym;
}

std::shared_ptr<std::vector<const Symbol *>>
generateSymbols(std::vector<std::string> QualifiedNames,
                std::weak_ptr<SlabAndPointers> *WeakSymbols) {
  SymbolSlab::Builder Slab;
  for (llvm::StringRef QName : QualifiedNames)
    Slab.insert(symbol(QName));

  auto Storage = std::make_shared<SlabAndPointers>();
  Storage->Slab = std::move(Slab).build();
  for (const auto &Sym : Storage->Slab)
    Storage->Pointers.push_back(&Sym);
  if (WeakSymbols)
    *WeakSymbols = Storage;
  auto *Pointers = &Storage->Pointers;
  return {std::move(Storage), Pointers};
}

std::shared_ptr<std::vector<const Symbol *>>
generateNumSymbols(int Begin, int End,
                   std::weak_ptr<SlabAndPointers> *WeakSymbols) {
  std::vector<std::string> Names;
  for (int i = Begin; i <= End; i++)
    Names.push_back(std::to_string(i));
  return generateSymbols(Names, WeakSymbols);
}

std::string getQualifiedName(const Symbol &Sym) {
  return (Sym.Scope + Sym.Name).str();
}

std::vector<std::string> match(const SymbolIndex &I,
                               const FuzzyFindRequest &Req, bool *Incomplete) {
  std::vector<std::string> Matches;
  bool IsIncomplete = I.fuzzyFind(Req, [&](const Symbol &Sym) {
    Matches.push_back(clang::clangd::getQualifiedName(Sym));
  });
  if (Incomplete)
    *Incomplete = IsIncomplete;
  return Matches;
}

// Returns qualified names of symbols with any of IDs in the index.
std::vector<std::string> lookup(const SymbolIndex &I,
                                llvm::ArrayRef<SymbolID> IDs) {
  LookupRequest Req;
  Req.IDs.insert(IDs.begin(), IDs.end());
  std::vector<std::string> Results;
  I.lookup(Req, [&](const Symbol &Sym) {
    Results.push_back(getQualifiedName(Sym));
  });
  return Results;
}

} // namespace clangd
} // namespace clang
