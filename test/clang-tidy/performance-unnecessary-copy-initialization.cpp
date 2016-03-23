// RUN: %check_clang_tidy %s performance-unnecessary-copy-initialization %t

struct ExpensiveToCopyType {
  ExpensiveToCopyType();
  virtual ~ExpensiveToCopyType();
  const ExpensiveToCopyType &reference() const;
  void nonConstMethod();
  bool constMethod() const;
};

struct TrivialToCopyType {
  const TrivialToCopyType &reference() const;
};

struct WeirdCopyCtorType {
  WeirdCopyCtorType();
  WeirdCopyCtorType(const WeirdCopyCtorType &w, bool oh_yes = true);

  void nonConstMethod();
  bool constMethod() const;
};

ExpensiveToCopyType global_expensive_to_copy_type;

const ExpensiveToCopyType &ExpensiveTypeReference();
const TrivialToCopyType &TrivialTypeReference();

void mutate(ExpensiveToCopyType &);
void mutate(ExpensiveToCopyType *);
void useAsConstPointer(const ExpensiveToCopyType *);
void useAsConstReference(const ExpensiveToCopyType &);
void useByValue(ExpensiveToCopyType);

void PositiveFunctionCall() {
  const auto AutoAssigned = ExpensiveTypeReference();
  // CHECK-MESSAGES: [[@LINE-1]]:14: warning: the const qualified variable 'AutoAssigned' is copy-constructed from a const reference; consider making it a const reference [performance-unnecessary-copy-initialization]
  // CHECK-FIXES: const auto& AutoAssigned = ExpensiveTypeReference();
  const auto AutoCopyConstructed(ExpensiveTypeReference());
  // CHECK-MESSAGES: [[@LINE-1]]:14: warning: the const qualified variable
  // CHECK-FIXES: const auto& AutoCopyConstructed(ExpensiveTypeReference());
  const ExpensiveToCopyType VarAssigned = ExpensiveTypeReference();
  // CHECK-MESSAGES: [[@LINE-1]]:29: warning: the const qualified variable
  // CHECK-FIXES:   const ExpensiveToCopyType& VarAssigned = ExpensiveTypeReference();
  const ExpensiveToCopyType VarCopyConstructed(ExpensiveTypeReference());
  // CHECK-MESSAGES: [[@LINE-1]]:29: warning: the const qualified variable
  // CHECK-FIXES: const ExpensiveToCopyType& VarCopyConstructed(ExpensiveTypeReference());
}

void PositiveMethodCallConstReferenceParam(const ExpensiveToCopyType &Obj) {
  const auto AutoAssigned = Obj.reference();
  // CHECK-MESSAGES: [[@LINE-1]]:14: warning: the const qualified variable
  // CHECK-FIXES: const auto& AutoAssigned = Obj.reference();
  const auto AutoCopyConstructed(Obj.reference());
  // CHECK-MESSAGES: [[@LINE-1]]:14: warning: the const qualified variable
  // CHECK-FIXES: const auto& AutoCopyConstructed(Obj.reference());
  const ExpensiveToCopyType VarAssigned = Obj.reference();
  // CHECK-MESSAGES: [[@LINE-1]]:29: warning: the const qualified variable
  // CHECK-FIXES: const ExpensiveToCopyType& VarAssigned = Obj.reference();
  const ExpensiveToCopyType VarCopyConstructed(Obj.reference());
  // CHECK-MESSAGES: [[@LINE-1]]:29: warning: the const qualified variable
  // CHECK-FIXES: const ExpensiveToCopyType& VarCopyConstructed(Obj.reference());
}

void PositiveMethodCallConstParam(const ExpensiveToCopyType Obj) {
  const auto AutoAssigned = Obj.reference();
  // CHECK-MESSAGES: [[@LINE-1]]:14: warning: the const qualified variable
  // CHECK-FIXES: const auto& AutoAssigned = Obj.reference();
  const auto AutoCopyConstructed(Obj.reference());
  // CHECK-MESSAGES: [[@LINE-1]]:14: warning: the const qualified variable
  // CHECK-FIXES: const auto& AutoCopyConstructed(Obj.reference());
  const ExpensiveToCopyType VarAssigned = Obj.reference();
  // CHECK-MESSAGES: [[@LINE-1]]:29: warning: the const qualified variable
  // CHECK-FIXES: const ExpensiveToCopyType& VarAssigned = Obj.reference();
  const ExpensiveToCopyType VarCopyConstructed(Obj.reference());
  // CHECK-MESSAGES: [[@LINE-1]]:29: warning: the const qualified variable
  // CHECK-FIXES: const ExpensiveToCopyType& VarCopyConstructed(Obj.reference());
}

void PositiveMethodCallConstPointerParam(const ExpensiveToCopyType *const Obj) {
  const auto AutoAssigned = Obj->reference();
  // CHECK-MESSAGES: [[@LINE-1]]:14: warning: the const qualified variable
  // CHECK-FIXES: const auto& AutoAssigned = Obj->reference();
  const auto AutoCopyConstructed(Obj->reference());
  // CHECK-MESSAGES: [[@LINE-1]]:14: warning: the const qualified variable
  // CHECK-FIXES: const auto& AutoCopyConstructed(Obj->reference());
  const ExpensiveToCopyType VarAssigned = Obj->reference();
  // CHECK-MESSAGES: [[@LINE-1]]:29: warning: the const qualified variable
  // CHECK-FIXES: const ExpensiveToCopyType& VarAssigned = Obj->reference();
  const ExpensiveToCopyType VarCopyConstructed(Obj->reference());
  // CHECK-MESSAGES: [[@LINE-1]]:29: warning: the const qualified variable
  // CHECK-FIXES: const ExpensiveToCopyType& VarCopyConstructed(Obj->reference());
}

void PositiveLocalConstValue() {
  const ExpensiveToCopyType Obj;
  const auto UnnecessaryCopy = Obj.reference();
  // CHECK-MESSAGES: [[@LINE-1]]:14: warning: the const qualified variable
  // CHECK-FIXES: const auto& UnnecessaryCopy = Obj.reference();
}

void PositiveLocalConstRef() {
  const ExpensiveToCopyType Obj;
  const ExpensiveToCopyType &ConstReference = Obj.reference();
  const auto UnnecessaryCopy = ConstReference.reference();
  // CHECK-MESSAGES: [[@LINE-1]]:14: warning: the const qualified variable
  // CHECK-FIXES: const auto& UnnecessaryCopy = ConstReference.reference();
}

void PositiveLocalConstPointer() {
  const ExpensiveToCopyType Obj;
  const ExpensiveToCopyType *const ConstPointer = &Obj;
  const auto UnnecessaryCopy = ConstPointer->reference();
  // CHECK-MESSAGES: [[@LINE-1]]:14: warning: the const qualified variable
  // CHECK-FIXES: const auto& UnnecessaryCopy = ConstPointer->reference();
}

void NegativeFunctionCallTrivialType() {
  const auto AutoAssigned = TrivialTypeReference();
  const auto AutoCopyConstructed(TrivialTypeReference());
  const TrivialToCopyType VarAssigned = TrivialTypeReference();
  const TrivialToCopyType VarCopyConstructed(TrivialTypeReference());
}

void NegativeStaticLocalVar(const ExpensiveToCopyType &Obj) {
  static const auto StaticVar = Obj.reference();
}

void PositiveFunctionCallExpensiveTypeNonConstVariable() {
  auto AutoAssigned = ExpensiveTypeReference();
  // CHECK-MESSAGES: [[@LINE-1]]:8: warning: the variable 'AutoAssigned' is copy-constructed from a const reference but is only used as const reference; consider making it a const reference [performance-unnecessary-copy-initialization]
  // CHECK-FIXES: const auto& AutoAssigned = ExpensiveTypeReference();
  auto AutoCopyConstructed(ExpensiveTypeReference());
  // CHECK-MESSAGES: [[@LINE-1]]:8: warning: the variable
  // CHECK-FIXES: const auto& AutoCopyConstructed(ExpensiveTypeReference());
  ExpensiveToCopyType VarAssigned = ExpensiveTypeReference();
  // CHECK-MESSAGES: [[@LINE-1]]:23: warning: the variable
  // CHECK-FIXES: const ExpensiveToCopyType& VarAssigned = ExpensiveTypeReference();
  ExpensiveToCopyType VarCopyConstructed(ExpensiveTypeReference());
  // CHECK-MESSAGES: [[@LINE-1]]:23: warning: the variable
  // CHECK-FIXES: const ExpensiveToCopyType& VarCopyConstructed(ExpensiveTypeReference());
}

void positiveNonConstVarInCodeBlock(const ExpensiveToCopyType &Obj) {
  {
    auto Assigned = Obj.reference();
    // CHECK-MESSAGES: [[@LINE-1]]:10: warning: the variable
    // CHECK-FIXES: const auto& Assigned = Obj.reference();
    Assigned.reference();
    useAsConstReference(Assigned);
    useByValue(Assigned);
  }
}

void negativeNonConstVarWithNonConstUse(const ExpensiveToCopyType &Obj) {
  {
    auto NonConstInvoked = Obj.reference();
    // CHECK-FIXES: auto NonConstInvoked = Obj.reference();
    NonConstInvoked.nonConstMethod();
  }
  {
    auto Reassigned = Obj.reference();
    // CHECK-FIXES: auto Reassigned = Obj.reference();
    Reassigned = ExpensiveToCopyType();
  }
  {
    auto MutatedByReference = Obj.reference();
    // CHECK-FIXES: auto MutatedByReference = Obj.reference();
    mutate(MutatedByReference);
  }
  {
    auto MutatedByPointer = Obj.reference();
    // CHECK-FIXES: auto MutatedByPointer = Obj.reference();
    mutate(&MutatedByPointer);
  }
}

void NegativeMethodCallNonConstRef(ExpensiveToCopyType &Obj) {
  const auto AutoAssigned = Obj.reference();
  const auto AutoCopyConstructed(Obj.reference());
  const ExpensiveToCopyType VarAssigned = Obj.reference();
  const ExpensiveToCopyType VarCopyConstructed(Obj.reference());
}

void NegativeMethodCallNonConst(ExpensiveToCopyType Obj) {
  const auto AutoAssigned = Obj.reference();
  const auto AutoCopyConstructed(Obj.reference());
  const ExpensiveToCopyType VarAssigned = Obj.reference();
  const ExpensiveToCopyType VarCopyConstructed(Obj.reference());
}

void NegativeMethodCallNonConstPointer(ExpensiveToCopyType *const Obj) {
  const auto AutoAssigned = Obj->reference();
  const auto AutoCopyConstructed(Obj->reference());
  const ExpensiveToCopyType VarAssigned = Obj->reference();
  const ExpensiveToCopyType VarCopyConstructed(Obj->reference());
}

void NegativeObjIsNotParam() {
  ExpensiveToCopyType Obj;
  const auto AutoAssigned = Obj.reference();
  const auto AutoCopyConstructed(Obj.reference());
  ExpensiveToCopyType VarAssigned = Obj.reference();
  ExpensiveToCopyType VarCopyConstructed(Obj.reference());
}

struct NegativeConstructor {
  NegativeConstructor(const ExpensiveToCopyType &Obj) : Obj(Obj) {}
  ExpensiveToCopyType Obj;
};

#define UNNECESSARY_COPY_INIT_IN_MACRO_BODY(TYPE)                              \
  void functionWith##TYPE(const TYPE &T) {                                     \
    auto AssignedInMacro = T.reference();                                      \
  }                                                                            \
// Ensure fix is not applied.
// CHECK-FIXES: auto AssignedInMacro = T.reference();

UNNECESSARY_COPY_INIT_IN_MACRO_BODY(ExpensiveToCopyType)
// CHECK-MESSAGES: [[@LINE-1]]:1: warning: the variable 'AssignedInMacro' is copy-constructed

#define UNNECESSARY_COPY_INIT_IN_MACRO_ARGUMENT(ARGUMENT) ARGUMENT

void PositiveMacroArgument(const ExpensiveToCopyType &Obj) {
  UNNECESSARY_COPY_INIT_IN_MACRO_ARGUMENT(auto CopyInMacroArg = Obj.reference());
  // CHECK-MESSAGES: [[@LINE-1]]:48: warning: the variable 'CopyInMacroArg' is copy-constructed
  // Ensure fix is not applied.
  // CHECK-FIXES: auto CopyInMacroArg = Obj.reference()
}

void PositiveLocalCopyConstMethodInvoked() {
  ExpensiveToCopyType orig;
  ExpensiveToCopyType copy_1 = orig;
  // CHECK-MESSAGES: [[@LINE-1]]:23: warning: local copy 'copy_1' of the variable 'orig' is never modified; consider avoiding the copy [performance-unnecessary-copy-initialization]
  // CHECK-FIXES: const ExpensiveToCopyType& copy_1 = orig;
  copy_1.constMethod();
  orig.constMethod();
}

void PositiveLocalCopyUsingExplicitCopyCtor() {
  ExpensiveToCopyType orig;
  ExpensiveToCopyType copy_2(orig);
  // CHECK-MESSAGES: [[@LINE-1]]:23: warning: local copy 'copy_2'
  // CHECK-FIXES: const ExpensiveToCopyType& copy_2(orig);
  copy_2.constMethod();
  orig.constMethod();
}

void PositiveLocalCopyCopyIsArgument(const ExpensiveToCopyType &orig) {
  ExpensiveToCopyType copy_3 = orig;
  // CHECK-MESSAGES: [[@LINE-1]]:23: warning: local copy 'copy_3'
  // CHECK-FIXES: const ExpensiveToCopyType& copy_3 = orig;
  copy_3.constMethod();
}

void PositiveLocalCopyUsedAsConstRef() {
  ExpensiveToCopyType orig;
  ExpensiveToCopyType copy_4 = orig;
  // CHECK-MESSAGES: [[@LINE-1]]:23: warning: local copy 'copy_4'
  // CHECK-FIXES: const ExpensiveToCopyType& copy_4 = orig;
  useAsConstReference(orig);
}

void PositiveLocalCopyTwice() {
  ExpensiveToCopyType orig;
  ExpensiveToCopyType copy_5 = orig;
  // CHECK-MESSAGES: [[@LINE-1]]:23: warning: local copy 'copy_5'
  // CHECK-FIXES: const ExpensiveToCopyType& copy_5 = orig;
  ExpensiveToCopyType copy_6 = copy_5;
  // CHECK-MESSAGES: [[@LINE-1]]:23: warning: local copy 'copy_6'
  // CHECK-FIXES: const ExpensiveToCopyType& copy_6 = copy_5;
  copy_5.constMethod();
  copy_6.constMethod();
  orig.constMethod();
}


void PositiveLocalCopyWeirdCopy() {
  WeirdCopyCtorType orig;
  WeirdCopyCtorType weird_1(orig);
  // CHECK-MESSAGES: [[@LINE-1]]:21: warning: local copy 'weird_1'
  // CHECK-FIXES: const WeirdCopyCtorType& weird_1(orig);
  weird_1.constMethod();

  WeirdCopyCtorType weird_2 = orig;
  // CHECK-MESSAGES: [[@LINE-1]]:21: warning: local copy 'weird_2'
  // CHECK-FIXES: const WeirdCopyCtorType& weird_2 = orig;
  weird_2.constMethod();
}

void NegativeLocalCopySimpleTypes() {
  int i1 = 0;
  int i2 = i1;
}

void NegativeLocalCopyCopyIsModified() {
  ExpensiveToCopyType orig;
  ExpensiveToCopyType neg_copy_1 = orig;
  neg_copy_1.nonConstMethod();
}

void NegativeLocalCopyOriginalIsModified() {
  ExpensiveToCopyType orig;
  ExpensiveToCopyType neg_copy_2 = orig;
  orig.nonConstMethod();
}

void NegativeLocalCopyUsedAsRefArg() {
  ExpensiveToCopyType orig;
  ExpensiveToCopyType neg_copy_3 = orig;
  mutate(neg_copy_3);
}

void NegativeLocalCopyUsedAsPointerArg() {
  ExpensiveToCopyType orig;
  ExpensiveToCopyType neg_copy_4 = orig;
  mutate(&neg_copy_4);
}

void NegativeLocalCopyCopyFromGlobal() {
  ExpensiveToCopyType neg_copy_5 = global_expensive_to_copy_type;
}

void NegativeLocalCopyCopyToStatic() {
  ExpensiveToCopyType orig;
  static ExpensiveToCopyType neg_copy_6 = orig;
}

void NegativeLocalCopyNonConstInForLoop() {
  ExpensiveToCopyType orig;
  for (ExpensiveToCopyType neg_copy_7 = orig; orig.constMethod();
       orig.nonConstMethod()) {
    orig.constMethod();
  }
}

void NegativeLocalCopyWeirdNonCopy() {
  WeirdCopyCtorType orig;
  WeirdCopyCtorType neg_weird_1(orig, false);
  WeirdCopyCtorType neg_weird_2(orig, true);
}