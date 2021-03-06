//===-- DexIndexTests.cpp  ----------------------------*- C++ -*-----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "TestIndex.h"
#include "index/Index.h"
#include "index/Merge.h"
#include "index/dex/DexIndex.h"
#include "index/dex/Iterator.h"
#include "index/dex/Token.h"
#include "index/dex/Trigram.h"
#include "llvm/Support/ScopedPrinter.h"
#include "llvm/Support/raw_ostream.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>

using ::testing::ElementsAre;
using ::testing::UnorderedElementsAre;

namespace clang {
namespace clangd {
namespace dex {
namespace {

std::vector<DocID> consumeIDs(Iterator &It) {
  auto IDAndScore = consume(It);
  std::vector<DocID> IDs(IDAndScore.size());
  for (size_t I = 0; I < IDAndScore.size(); ++I)
    IDs[I] = IDAndScore[I].first;
  return IDs;
}

TEST(DexIndexIterators, DocumentIterator) {
  const PostingList L = {4, 7, 8, 20, 42, 100};
  auto DocIterator = create(L);

  EXPECT_EQ(DocIterator->peek(), 4U);
  EXPECT_FALSE(DocIterator->reachedEnd());

  DocIterator->advance();
  EXPECT_EQ(DocIterator->peek(), 7U);
  EXPECT_FALSE(DocIterator->reachedEnd());

  DocIterator->advanceTo(20);
  EXPECT_EQ(DocIterator->peek(), 20U);
  EXPECT_FALSE(DocIterator->reachedEnd());

  DocIterator->advanceTo(65);
  EXPECT_EQ(DocIterator->peek(), 100U);
  EXPECT_FALSE(DocIterator->reachedEnd());

  DocIterator->advanceTo(420);
  EXPECT_TRUE(DocIterator->reachedEnd());
}

TEST(DexIndexIterators, AndWithEmpty) {
  const PostingList L0;
  const PostingList L1 = {0, 5, 7, 10, 42, 320, 9000};

  auto AndEmpty = createAnd(create(L0));
  EXPECT_TRUE(AndEmpty->reachedEnd());

  auto AndWithEmpty = createAnd(create(L0), create(L1));
  EXPECT_TRUE(AndWithEmpty->reachedEnd());

  EXPECT_THAT(consumeIDs(*AndWithEmpty), ElementsAre());
}

TEST(DexIndexIterators, AndTwoLists) {
  const PostingList L0 = {0, 5, 7, 10, 42, 320, 9000};
  const PostingList L1 = {0, 4, 7, 10, 30, 60, 320, 9000};

  auto And = createAnd(create(L1), create(L0));

  EXPECT_FALSE(And->reachedEnd());
  EXPECT_THAT(consumeIDs(*And), ElementsAre(0U, 7U, 10U, 320U, 9000U));

  And = createAnd(create(L0), create(L1));

  And->advanceTo(0);
  EXPECT_EQ(And->peek(), 0U);
  And->advanceTo(5);
  EXPECT_EQ(And->peek(), 7U);
  And->advanceTo(10);
  EXPECT_EQ(And->peek(), 10U);
  And->advanceTo(42);
  EXPECT_EQ(And->peek(), 320U);
  And->advanceTo(8999);
  EXPECT_EQ(And->peek(), 9000U);
  And->advanceTo(9001);
}

TEST(DexIndexIterators, AndThreeLists) {
  const PostingList L0 = {0, 5, 7, 10, 42, 320, 9000};
  const PostingList L1 = {0, 4, 7, 10, 30, 60, 320, 9000};
  const PostingList L2 = {1, 4, 7, 11, 30, 60, 320, 9000};

  auto And = createAnd(create(L0), create(L1), create(L2));
  EXPECT_EQ(And->peek(), 7U);
  And->advanceTo(300);
  EXPECT_EQ(And->peek(), 320U);
  And->advanceTo(100000);

  EXPECT_TRUE(And->reachedEnd());
}

TEST(DexIndexIterators, OrWithEmpty) {
  const PostingList L0;
  const PostingList L1 = {0, 5, 7, 10, 42, 320, 9000};

  auto OrEmpty = createOr(create(L0));
  EXPECT_TRUE(OrEmpty->reachedEnd());

  auto OrWithEmpty = createOr(create(L0), create(L1));
  EXPECT_FALSE(OrWithEmpty->reachedEnd());

  EXPECT_THAT(consumeIDs(*OrWithEmpty),
              ElementsAre(0U, 5U, 7U, 10U, 42U, 320U, 9000U));
}

TEST(DexIndexIterators, OrTwoLists) {
  const PostingList L0 = {0, 5, 7, 10, 42, 320, 9000};
  const PostingList L1 = {0, 4, 7, 10, 30, 60, 320, 9000};

  auto Or = createOr(create(L0), create(L1));

  EXPECT_FALSE(Or->reachedEnd());
  EXPECT_EQ(Or->peek(), 0U);
  Or->advance();
  EXPECT_EQ(Or->peek(), 4U);
  Or->advance();
  EXPECT_EQ(Or->peek(), 5U);
  Or->advance();
  EXPECT_EQ(Or->peek(), 7U);
  Or->advance();
  EXPECT_EQ(Or->peek(), 10U);
  Or->advance();
  EXPECT_EQ(Or->peek(), 30U);
  Or->advanceTo(42);
  EXPECT_EQ(Or->peek(), 42U);
  Or->advanceTo(300);
  EXPECT_EQ(Or->peek(), 320U);
  Or->advanceTo(9000);
  EXPECT_EQ(Or->peek(), 9000U);
  Or->advanceTo(9001);
  EXPECT_TRUE(Or->reachedEnd());

  Or = createOr(create(L0), create(L1));

  EXPECT_THAT(consumeIDs(*Or),
              ElementsAre(0U, 4U, 5U, 7U, 10U, 30U, 42U, 60U, 320U, 9000U));
}

TEST(DexIndexIterators, OrThreeLists) {
  const PostingList L0 = {0, 5, 7, 10, 42, 320, 9000};
  const PostingList L1 = {0, 4, 7, 10, 30, 60, 320, 9000};
  const PostingList L2 = {1, 4, 7, 11, 30, 60, 320, 9000};

  auto Or = createOr(create(L0), create(L1), create(L2));

  EXPECT_FALSE(Or->reachedEnd());
  EXPECT_EQ(Or->peek(), 0U);

  Or->advance();
  EXPECT_EQ(Or->peek(), 1U);

  Or->advance();
  EXPECT_EQ(Or->peek(), 4U);

  Or->advanceTo(7);

  Or->advanceTo(59);
  EXPECT_EQ(Or->peek(), 60U);

  Or->advanceTo(9001);
  EXPECT_TRUE(Or->reachedEnd());
}

// FIXME(kbobyrev): The testcase below is similar to what is expected in real
// queries. It should be updated once new iterators (such as boosting, limiting,
// etc iterators) appear. However, it is not exhaustive and it would be
// beneficial to implement automatic generation (e.g. fuzzing) of query trees
// for more comprehensive testing.
TEST(DexIndexIterators, QueryTree) {
  //
  //                      +-----------------+
  //                      |And Iterator:1, 5|
  //                      +--------+--------+
  //                               |
  //                               |
  //                 +-------------+----------------------+
  //                 |                                    |
  //                 |                                    |
  //      +----------v----------+              +----------v------------+
  //      |And Iterator: 1, 5, 9|              |Or Iterator: 0, 1, 3, 5|
  //      +----------+----------+              +----------+------------+
  //                 |                                    |
  //          +------+-----+                    +---------------------+
  //          |            |                    |         |           |
  //  +-------v-----+ +----+---+             +--v--+  +---v----+ +----v---+
  //  |1, 3, 5, 8, 9| |Boost: 2|             |Empty|  |Boost: 3| |Boost: 4|
  //  +-------------+ +----+---+             +-----+  +---+----+ +----+---+
  //                       |                              |           |
  //                  +----v-----+                      +-v--+    +---v---+
  //                  |1, 5, 7, 9|                      |1, 5|    |0, 3, 5|
  //                  +----------+                      +----+    +-------+
  //
  const PostingList L0 = {1, 3, 5, 8, 9};
  const PostingList L1 = {1, 5, 7, 9};
  const PostingList L3;
  const PostingList L4 = {1, 5};
  const PostingList L5 = {0, 3, 5};

  // Root of the query tree: [1, 5]
  auto Root = createAnd(
      // Lower And Iterator: [1, 5, 9]
      createAnd(create(L0), createBoost(create(L1), 2U)),
      // Lower Or Iterator: [0, 1, 5]
      createOr(create(L3), createBoost(create(L4), 3U),
               createBoost(create(L5), 4U)));

  EXPECT_FALSE(Root->reachedEnd());
  EXPECT_EQ(Root->peek(), 1U);
  Root->advanceTo(0);
  // Advance multiple times. Shouldn't do anything.
  Root->advanceTo(1);
  Root->advanceTo(0);
  EXPECT_EQ(Root->peek(), 1U);
  auto ElementBoost = Root->consume();
  EXPECT_THAT(ElementBoost, 6);
  Root->advance();
  EXPECT_EQ(Root->peek(), 5U);
  Root->advanceTo(5);
  EXPECT_EQ(Root->peek(), 5U);
  ElementBoost = Root->consume();
  EXPECT_THAT(ElementBoost, 8);
  Root->advanceTo(9000);
  EXPECT_TRUE(Root->reachedEnd());
}

TEST(DexIndexIterators, StringRepresentation) {
  const PostingList L0 = {4, 7, 8, 20, 42, 100};
  const PostingList L1 = {1, 3, 5, 8, 9};
  const PostingList L2 = {1, 5, 7, 9};
  const PostingList L3 = {0, 5};
  const PostingList L4 = {0, 1, 5};
  const PostingList L5;

  EXPECT_EQ(llvm::to_string(*(create(L0))), "[{4}, 7, 8, 20, 42, 100, END]");

  auto Nested = createAnd(createAnd(create(L1), create(L2)),
                          createOr(create(L3), create(L4), create(L5)));

  EXPECT_EQ(llvm::to_string(*Nested),
            "(& (& [{1}, 3, 5, 8, 9, END] [{1}, 5, 7, 9, END]) (| [0, {5}, "
            "END] [0, {1}, 5, END] [{END}]))");
}

TEST(DexIndexIterators, Limit) {
  const PostingList L0 = {3, 6, 7, 20, 42, 100};
  const PostingList L1 = {1, 3, 5, 6, 7, 30, 100};
  const PostingList L2 = {0, 3, 5, 7, 8, 100};

  auto DocIterator = createLimit(create(L0), 42);
  EXPECT_THAT(consumeIDs(*DocIterator), ElementsAre(3, 6, 7, 20, 42, 100));

  DocIterator = createLimit(create(L0), 3);
  EXPECT_THAT(consumeIDs(*DocIterator), ElementsAre(3, 6, 7));

  DocIterator = createLimit(create(L0), 0);
  EXPECT_THAT(consumeIDs(*DocIterator), ElementsAre());

  auto AndIterator =
      createAnd(createLimit(createTrue(9000), 343), createLimit(create(L0), 2),
                createLimit(create(L1), 3), createLimit(create(L2), 42));
  EXPECT_THAT(consumeIDs(*AndIterator), ElementsAre(3, 7));
}

TEST(DexIndexIterators, True) {
  auto TrueIterator = createTrue(0U);
  EXPECT_TRUE(TrueIterator->reachedEnd());
  EXPECT_THAT(consumeIDs(*TrueIterator), ElementsAre());

  PostingList L0 = {1, 2, 5, 7};
  TrueIterator = createTrue(7U);
  EXPECT_THAT(TrueIterator->peek(), 0);
  auto AndIterator = createAnd(create(L0), move(TrueIterator));
  EXPECT_FALSE(AndIterator->reachedEnd());
  EXPECT_THAT(consumeIDs(*AndIterator), ElementsAre(1, 2, 5));
}

TEST(DexIndexIterators, Boost) {
  auto BoostIterator = createBoost(createTrue(5U), 42U);
  EXPECT_FALSE(BoostIterator->reachedEnd());
  auto ElementBoost = BoostIterator->consume();
  EXPECT_THAT(ElementBoost, 42U);

  const PostingList L0 = {2, 4};
  const PostingList L1 = {1, 4};
  auto Root = createOr(createTrue(5U), createBoost(create(L0), 2U),
                       createBoost(create(L1), 3U));

  ElementBoost = Root->consume();
  EXPECT_THAT(ElementBoost, Iterator::DEFAULT_BOOST_SCORE);
  Root->advance();
  EXPECT_THAT(Root->peek(), 1U);
  ElementBoost = Root->consume();
  EXPECT_THAT(ElementBoost, 3);

  Root->advance();
  EXPECT_THAT(Root->peek(), 2U);
  ElementBoost = Root->consume();
  EXPECT_THAT(ElementBoost, 2);

  Root->advanceTo(4);
  ElementBoost = Root->consume();
  EXPECT_THAT(ElementBoost, 3);
}

testing::Matcher<std::vector<Token>>
trigramsAre(std::initializer_list<std::string> Trigrams) {
  std::vector<Token> Tokens;
  for (const auto &Symbols : Trigrams) {
    Tokens.push_back(Token(Token::Kind::Trigram, Symbols));
  }
  return testing::UnorderedElementsAreArray(Tokens);
}

TEST(DexIndexTrigrams, IdentifierTrigrams) {
  EXPECT_THAT(generateIdentifierTrigrams("X86"),
              trigramsAre({"x86", "x$$", "x8$", "$$$"}));

  EXPECT_THAT(generateIdentifierTrigrams("nl"),
              trigramsAre({"nl$", "n$$", "$$$"}));

  EXPECT_THAT(generateIdentifierTrigrams("n"), trigramsAre({"n$$", "$$$"}));

  EXPECT_THAT(generateIdentifierTrigrams("clangd"),
              trigramsAre({"c$$", "cl$", "cla", "lan", "ang", "ngd", "$$$"}));

  EXPECT_THAT(generateIdentifierTrigrams("abc_def"),
              trigramsAre({"a$$", "abc", "abd", "ade", "bcd", "bde", "cde",
                           "def", "ab$", "ad$", "$$$"}));

  EXPECT_THAT(generateIdentifierTrigrams("a_b_c_d_e_"),
              trigramsAre({"a$$", "a_$", "a_b", "abc", "abd", "acd", "ace",
                           "bcd", "bce", "bde", "cde", "ab$", "$$$"}));

  EXPECT_THAT(generateIdentifierTrigrams("unique_ptr"),
              trigramsAre({"u$$", "uni", "unp", "upt", "niq", "nip", "npt",
                           "iqu", "iqp", "ipt", "que", "qup", "qpt", "uep",
                           "ept", "ptr", "un$", "up$", "$$$"}));

  EXPECT_THAT(generateIdentifierTrigrams("TUDecl"),
              trigramsAre({"t$$", "tud", "tde", "ude", "dec", "ecl", "tu$",
                           "td$", "$$$"}));

  EXPECT_THAT(generateIdentifierTrigrams("IsOK"),
              trigramsAre({"i$$", "iso", "iok", "sok", "is$", "io$", "$$$"}));

  EXPECT_THAT(
      generateIdentifierTrigrams("abc_defGhij__klm"),
      trigramsAre({"a$$", "abc", "abd", "abg", "ade", "adg", "adk", "agh",
                   "agk", "bcd", "bcg", "bde", "bdg", "bdk", "bgh", "bgk",
                   "cde", "cdg", "cdk", "cgh", "cgk", "def", "deg", "dek",
                   "dgh", "dgk", "dkl", "efg", "efk", "egh", "egk", "ekl",
                   "fgh", "fgk", "fkl", "ghi", "ghk", "gkl", "hij", "hik",
                   "hkl", "ijk", "ikl", "jkl", "klm", "ab$", "ad$", "$$$"}));
}

TEST(DexIndexTrigrams, QueryTrigrams) {
  EXPECT_THAT(generateQueryTrigrams("c"), trigramsAre({"c$$"}));
  EXPECT_THAT(generateQueryTrigrams("cl"), trigramsAre({"cl$"}));
  EXPECT_THAT(generateQueryTrigrams("cla"), trigramsAre({"cla"}));

  EXPECT_THAT(generateQueryTrigrams("_"), trigramsAre({"_$$"}));
  EXPECT_THAT(generateQueryTrigrams("__"), trigramsAre({"__$"}));
  EXPECT_THAT(generateQueryTrigrams("___"), trigramsAre({"___"}));

  EXPECT_THAT(generateQueryTrigrams("X86"), trigramsAre({"x86"}));

  EXPECT_THAT(generateQueryTrigrams("clangd"),
              trigramsAre({"cla", "lan", "ang", "ngd"}));

  EXPECT_THAT(generateQueryTrigrams("abc_def"),
              trigramsAre({"abc", "bcd", "cde", "def"}));

  EXPECT_THAT(generateQueryTrigrams("a_b_c_d_e_"),
              trigramsAre({"abc", "bcd", "cde"}));

  EXPECT_THAT(generateQueryTrigrams("unique_ptr"),
              trigramsAre({"uni", "niq", "iqu", "que", "uep", "ept", "ptr"}));

  EXPECT_THAT(generateQueryTrigrams("TUDecl"),
              trigramsAre({"tud", "ude", "dec", "ecl"}));

  EXPECT_THAT(generateQueryTrigrams("IsOK"), trigramsAre({"iso", "sok"}));

  EXPECT_THAT(generateQueryTrigrams("abc_defGhij__klm"),
              trigramsAre({"abc", "bcd", "cde", "def", "efg", "fgh", "ghi",
                           "hij", "ijk", "jkl", "klm"}));
}

TEST(DexIndex, Lookup) {
  DexIndex I;
  I.build(generateSymbols({"ns::abc", "ns::xyz"}));
  EXPECT_THAT(lookup(I, SymbolID("ns::abc")), UnorderedElementsAre("ns::abc"));
  EXPECT_THAT(lookup(I, {SymbolID("ns::abc"), SymbolID("ns::xyz")}),
              UnorderedElementsAre("ns::abc", "ns::xyz"));
  EXPECT_THAT(lookup(I, {SymbolID("ns::nonono"), SymbolID("ns::xyz")}),
              UnorderedElementsAre("ns::xyz"));
  EXPECT_THAT(lookup(I, SymbolID("ns::nonono")), UnorderedElementsAre());
}

TEST(DexIndex, FuzzyFind) {
  DexIndex Index;
  Index.build(generateSymbols({"ns::ABC", "ns::BCD", "::ABC", "ns::nested::ABC",
                               "other::ABC", "other::A"}));
  FuzzyFindRequest Req;
  Req.Query = "ABC";
  Req.Scopes = {"ns::"};
  EXPECT_THAT(match(Index, Req), UnorderedElementsAre("ns::ABC"));
  Req.Scopes = {"ns::", "ns::nested::"};
  EXPECT_THAT(match(Index, Req),
              UnorderedElementsAre("ns::ABC", "ns::nested::ABC"));
  Req.Query = "A";
  Req.Scopes = {"other::"};
  EXPECT_THAT(match(Index, Req),
              UnorderedElementsAre("other::A", "other::ABC"));
  Req.Query = "";
  Req.Scopes = {};
  EXPECT_THAT(match(Index, Req),
              UnorderedElementsAre("ns::ABC", "ns::BCD", "::ABC",
                                   "ns::nested::ABC", "other::ABC",
                                   "other::A"));
}

TEST(DexIndexTest, FuzzyMatchQ) {
  DexIndex I;
  I.build(
      generateSymbols({"LaughingOutLoud", "LionPopulation", "LittleOldLady"}));
  FuzzyFindRequest Req;
  Req.Query = "lol";
  Req.MaxCandidateCount = 2;
  EXPECT_THAT(match(I, Req),
              UnorderedElementsAre("LaughingOutLoud", "LittleOldLady"));
}

TEST(DexIndexTest, DexIndexSymbolsRecycled) {
  DexIndex I;
  std::weak_ptr<SlabAndPointers> Symbols;
  I.build(generateNumSymbols(0, 10, &Symbols));
  FuzzyFindRequest Req;
  Req.Query = "7";
  EXPECT_THAT(match(I, Req), UnorderedElementsAre("7"));

  EXPECT_FALSE(Symbols.expired());
  // Release old symbols.
  I.build(generateNumSymbols(0, 0));
  EXPECT_TRUE(Symbols.expired());
}

// FIXME(kbobyrev): This test is different for DexIndex and MemIndex: while
// MemIndex manages response deduplication, DexIndex simply returns all matched
// symbols which means there might be equivalent symbols in the response.
// Before drop-in replacement of MemIndex with DexIndex happens, FileIndex
// should handle deduplication instead.
TEST(DexIndexTest, DexIndexDeduplicate) {
  auto Symbols = generateNumSymbols(0, 10);

  // Inject duplicates.
  auto Sym = symbol("7");
  Symbols->push_back(&Sym);
  Symbols->push_back(&Sym);
  Symbols->push_back(&Sym);

  FuzzyFindRequest Req;
  Req.Query = "7";
  DexIndex I;
  I.build(std::move(Symbols));
  auto Matches = match(I, Req);
  EXPECT_EQ(Matches.size(), 4u);
}

TEST(DexIndexTest, DexIndexLimitedNumMatches) {
  DexIndex I;
  I.build(generateNumSymbols(0, 100));
  FuzzyFindRequest Req;
  Req.Query = "5";
  Req.MaxCandidateCount = 3;
  bool Incomplete;
  auto Matches = match(I, Req, &Incomplete);
  EXPECT_EQ(Matches.size(), Req.MaxCandidateCount);
  EXPECT_TRUE(Incomplete);
}

TEST(DexIndexTest, FuzzyMatch) {
  DexIndex I;
  I.build(
      generateSymbols({"LaughingOutLoud", "LionPopulation", "LittleOldLady"}));
  FuzzyFindRequest Req;
  Req.Query = "lol";
  Req.MaxCandidateCount = 2;
  EXPECT_THAT(match(I, Req),
              UnorderedElementsAre("LaughingOutLoud", "LittleOldLady"));
}

TEST(DexIndexTest, MatchQualifiedNamesWithoutSpecificScope) {
  DexIndex I;
  I.build(generateSymbols({"a::y1", "b::y2", "y3"}));
  FuzzyFindRequest Req;
  Req.Query = "y";
  EXPECT_THAT(match(I, Req), UnorderedElementsAre("a::y1", "b::y2", "y3"));
}

TEST(DexIndexTest, MatchQualifiedNamesWithGlobalScope) {
  DexIndex I;
  I.build(generateSymbols({"a::y1", "b::y2", "y3"}));
  FuzzyFindRequest Req;
  Req.Query = "y";
  Req.Scopes = {""};
  EXPECT_THAT(match(I, Req), UnorderedElementsAre("y3"));
}

TEST(DexIndexTest, MatchQualifiedNamesWithOneScope) {
  DexIndex I;
  I.build(generateSymbols({"a::y1", "a::y2", "a::x", "b::y2", "y3"}));
  FuzzyFindRequest Req;
  Req.Query = "y";
  Req.Scopes = {"a::"};
  EXPECT_THAT(match(I, Req), UnorderedElementsAre("a::y1", "a::y2"));
}

TEST(DexIndexTest, MatchQualifiedNamesWithMultipleScopes) {
  DexIndex I;
  I.build(generateSymbols({"a::y1", "a::y2", "a::x", "b::y3", "y3"}));
  FuzzyFindRequest Req;
  Req.Query = "y";
  Req.Scopes = {"a::", "b::"};
  EXPECT_THAT(match(I, Req), UnorderedElementsAre("a::y1", "a::y2", "b::y3"));
}

TEST(DexIndexTest, NoMatchNestedScopes) {
  DexIndex I;
  I.build(generateSymbols({"a::y1", "a::b::y2"}));
  FuzzyFindRequest Req;
  Req.Query = "y";
  Req.Scopes = {"a::"};
  EXPECT_THAT(match(I, Req), UnorderedElementsAre("a::y1"));
}

TEST(DexIndexTest, IgnoreCases) {
  DexIndex I;
  I.build(generateSymbols({"ns::ABC", "ns::abc"}));
  FuzzyFindRequest Req;
  Req.Query = "AB";
  Req.Scopes = {"ns::"};
  EXPECT_THAT(match(I, Req), UnorderedElementsAre("ns::ABC", "ns::abc"));
}

TEST(DexIndexTest, Lookup) {
  DexIndex I;
  I.build(generateSymbols({"ns::abc", "ns::xyz"}));
  EXPECT_THAT(lookup(I, SymbolID("ns::abc")), UnorderedElementsAre("ns::abc"));
  EXPECT_THAT(lookup(I, {SymbolID("ns::abc"), SymbolID("ns::xyz")}),
              UnorderedElementsAre("ns::abc", "ns::xyz"));
  EXPECT_THAT(lookup(I, {SymbolID("ns::nonono"), SymbolID("ns::xyz")}),
              UnorderedElementsAre("ns::xyz"));
  EXPECT_THAT(lookup(I, SymbolID("ns::nonono")), UnorderedElementsAre());
}

} // namespace
} // namespace dex
} // namespace clangd
} // namespace clang
