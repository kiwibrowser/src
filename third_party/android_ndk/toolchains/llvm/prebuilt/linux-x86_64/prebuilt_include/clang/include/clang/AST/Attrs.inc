/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Attribute classes' definitions                                             *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef LLVM_CLANG_ATTR_CLASSES_INC
#define LLVM_CLANG_ATTR_CLASSES_INC

class AMDGPUFlatWorkGroupSizeAttr : public InheritableAttr {
unsigned min;

unsigned max;

public:
  static AMDGPUFlatWorkGroupSizeAttr *CreateImplicit(ASTContext &Ctx, unsigned Min, unsigned Max, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AMDGPUFlatWorkGroupSizeAttr(Loc, Ctx, Min, Max, 0);
    A->setImplicit(true);
    return A;
  }

  AMDGPUFlatWorkGroupSizeAttr(SourceRange R, ASTContext &Ctx
              , unsigned Min
              , unsigned Max
              , unsigned SI
             )
    : InheritableAttr(attr::AMDGPUFlatWorkGroupSize, R, SI, false, false)
              , min(Min)
              , max(Max)
  {
  }

  AMDGPUFlatWorkGroupSizeAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getMin() const {
    return min;
  }

  unsigned getMax() const {
    return max;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::AMDGPUFlatWorkGroupSize; }
};

class AMDGPUNumSGPRAttr : public InheritableAttr {
unsigned numSGPR;

public:
  static AMDGPUNumSGPRAttr *CreateImplicit(ASTContext &Ctx, unsigned NumSGPR, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AMDGPUNumSGPRAttr(Loc, Ctx, NumSGPR, 0);
    A->setImplicit(true);
    return A;
  }

  AMDGPUNumSGPRAttr(SourceRange R, ASTContext &Ctx
              , unsigned NumSGPR
              , unsigned SI
             )
    : InheritableAttr(attr::AMDGPUNumSGPR, R, SI, false, false)
              , numSGPR(NumSGPR)
  {
  }

  AMDGPUNumSGPRAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getNumSGPR() const {
    return numSGPR;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::AMDGPUNumSGPR; }
};

class AMDGPUNumVGPRAttr : public InheritableAttr {
unsigned numVGPR;

public:
  static AMDGPUNumVGPRAttr *CreateImplicit(ASTContext &Ctx, unsigned NumVGPR, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AMDGPUNumVGPRAttr(Loc, Ctx, NumVGPR, 0);
    A->setImplicit(true);
    return A;
  }

  AMDGPUNumVGPRAttr(SourceRange R, ASTContext &Ctx
              , unsigned NumVGPR
              , unsigned SI
             )
    : InheritableAttr(attr::AMDGPUNumVGPR, R, SI, false, false)
              , numVGPR(NumVGPR)
  {
  }

  AMDGPUNumVGPRAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getNumVGPR() const {
    return numVGPR;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::AMDGPUNumVGPR; }
};

class AMDGPUWavesPerEUAttr : public InheritableAttr {
unsigned min;

unsigned max;

public:
  static AMDGPUWavesPerEUAttr *CreateImplicit(ASTContext &Ctx, unsigned Min, unsigned Max, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AMDGPUWavesPerEUAttr(Loc, Ctx, Min, Max, 0);
    A->setImplicit(true);
    return A;
  }

  AMDGPUWavesPerEUAttr(SourceRange R, ASTContext &Ctx
              , unsigned Min
              , unsigned Max
              , unsigned SI
             )
    : InheritableAttr(attr::AMDGPUWavesPerEU, R, SI, false, false)
              , min(Min)
              , max(Max)
  {
  }

  AMDGPUWavesPerEUAttr(SourceRange R, ASTContext &Ctx
              , unsigned Min
              , unsigned SI
             )
    : InheritableAttr(attr::AMDGPUWavesPerEU, R, SI, false, false)
              , min(Min)
              , max()
  {
  }

  AMDGPUWavesPerEUAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getMin() const {
    return min;
  }

  unsigned getMax() const {
    return max;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::AMDGPUWavesPerEU; }
};

class ARMInterruptAttr : public InheritableAttr {
public:
  enum InterruptType {
    IRQ,
    FIQ,
    SWI,
    ABORT,
    UNDEF,
    Generic
  };
private:
  InterruptType interrupt;

public:
  static ARMInterruptAttr *CreateImplicit(ASTContext &Ctx, InterruptType Interrupt, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ARMInterruptAttr(Loc, Ctx, Interrupt, 0);
    A->setImplicit(true);
    return A;
  }

  ARMInterruptAttr(SourceRange R, ASTContext &Ctx
              , InterruptType Interrupt
              , unsigned SI
             )
    : InheritableAttr(attr::ARMInterrupt, R, SI, false, false)
              , interrupt(Interrupt)
  {
  }

  ARMInterruptAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ARMInterrupt, R, SI, false, false)
              , interrupt(InterruptType(0))
  {
  }

  ARMInterruptAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  InterruptType getInterrupt() const {
    return interrupt;
  }

  static bool ConvertStrToInterruptType(StringRef Val, InterruptType &Out) {
    Optional<InterruptType> R = llvm::StringSwitch<Optional<InterruptType>>(Val)
      .Case("IRQ", ARMInterruptAttr::IRQ)
      .Case("FIQ", ARMInterruptAttr::FIQ)
      .Case("SWI", ARMInterruptAttr::SWI)
      .Case("ABORT", ARMInterruptAttr::ABORT)
      .Case("UNDEF", ARMInterruptAttr::UNDEF)
      .Case("", ARMInterruptAttr::Generic)
      .Default(Optional<InterruptType>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertInterruptTypeToStr(InterruptType Val) {
    switch(Val) {
    case ARMInterruptAttr::IRQ: return "IRQ";
    case ARMInterruptAttr::FIQ: return "FIQ";
    case ARMInterruptAttr::SWI: return "SWI";
    case ARMInterruptAttr::ABORT: return "ABORT";
    case ARMInterruptAttr::UNDEF: return "UNDEF";
    case ARMInterruptAttr::Generic: return "";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::ARMInterrupt; }
};

class AVRInterruptAttr : public InheritableAttr {
public:
  static AVRInterruptAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AVRInterruptAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  AVRInterruptAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::AVRInterrupt, R, SI, false, false)
  {
  }

  AVRInterruptAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::AVRInterrupt; }
};

class AVRSignalAttr : public InheritableAttr {
public:
  static AVRSignalAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AVRSignalAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  AVRSignalAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::AVRSignal, R, SI, false, false)
  {
  }

  AVRSignalAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::AVRSignal; }
};

class AbiTagAttr : public Attr {
  unsigned tags_Size;
  StringRef *tags_;

public:
  static AbiTagAttr *CreateImplicit(ASTContext &Ctx, StringRef *Tags, unsigned TagsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AbiTagAttr(Loc, Ctx, Tags, TagsSize, 0);
    A->setImplicit(true);
    return A;
  }

  AbiTagAttr(SourceRange R, ASTContext &Ctx
              , StringRef *Tags, unsigned TagsSize
              , unsigned SI
             )
    : Attr(attr::AbiTag, R, SI, false, false)
              , tags_Size(TagsSize), tags_(new (Ctx, 16) StringRef[tags_Size])
  {
    for (size_t I = 0, E = tags_Size; I != E;
         ++I) {
      StringRef Ref = Tags[I];
      if (!Ref.empty()) {
        char *Mem = new (Ctx, 1) char[Ref.size()];
        std::memcpy(Mem, Ref.data(), Ref.size());
        tags_[I] = StringRef(Mem, Ref.size());
      }
    }
  }

  AbiTagAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : Attr(attr::AbiTag, R, SI, false, false)
              , tags_Size(0), tags_(nullptr)
  {
  }

  AbiTagAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  typedef StringRef* tags_iterator;
  tags_iterator tags_begin() const { return tags_; }
  tags_iterator tags_end() const { return tags_ + tags_Size; }
  unsigned tags_size() const { return tags_Size; }
  llvm::iterator_range<tags_iterator> tags() const { return llvm::make_range(tags_begin(), tags_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::AbiTag; }
};

class AcquireCapabilityAttr : public InheritableAttr {
  unsigned args_Size;
  Expr * *args_;

public:
  enum Spelling {
    GNU_acquire_capability = 0,
    CXX11_clang_acquire_capability = 1,
    GNU_acquire_shared_capability = 2,
    CXX11_clang_acquire_shared_capability = 3,
    GNU_exclusive_lock_function = 4,
    GNU_shared_lock_function = 5
  };

  static AcquireCapabilityAttr *CreateImplicit(ASTContext &Ctx, Spelling S, Expr * *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AcquireCapabilityAttr(Loc, Ctx, Args, ArgsSize, S);
    A->setImplicit(true);
    return A;
  }

  AcquireCapabilityAttr(SourceRange R, ASTContext &Ctx
              , Expr * *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::AcquireCapability, R, SI, true, true)
              , args_Size(ArgsSize), args_(new (Ctx, 16) Expr *[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  AcquireCapabilityAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::AcquireCapability, R, SI, true, true)
              , args_Size(0), args_(nullptr)
  {
  }

  AcquireCapabilityAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_acquire_capability;
    case 1: return CXX11_clang_acquire_capability;
    case 2: return GNU_acquire_shared_capability;
    case 3: return CXX11_clang_acquire_shared_capability;
    case 4: return GNU_exclusive_lock_function;
    case 5: return GNU_shared_lock_function;
  }
  }
  bool isShared() const { return SpellingListIndex == 2 ||
    SpellingListIndex == 3 ||
    SpellingListIndex == 5; }
  typedef Expr ** args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::AcquireCapability; }
};

class AcquiredAfterAttr : public InheritableAttr {
  unsigned args_Size;
  Expr * *args_;

public:
  static AcquiredAfterAttr *CreateImplicit(ASTContext &Ctx, Expr * *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AcquiredAfterAttr(Loc, Ctx, Args, ArgsSize, 0);
    A->setImplicit(true);
    return A;
  }

  AcquiredAfterAttr(SourceRange R, ASTContext &Ctx
              , Expr * *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::AcquiredAfter, R, SI, true, true)
              , args_Size(ArgsSize), args_(new (Ctx, 16) Expr *[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  AcquiredAfterAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::AcquiredAfter, R, SI, true, true)
              , args_Size(0), args_(nullptr)
  {
  }

  AcquiredAfterAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  typedef Expr ** args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::AcquiredAfter; }
};

class AcquiredBeforeAttr : public InheritableAttr {
  unsigned args_Size;
  Expr * *args_;

public:
  static AcquiredBeforeAttr *CreateImplicit(ASTContext &Ctx, Expr * *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AcquiredBeforeAttr(Loc, Ctx, Args, ArgsSize, 0);
    A->setImplicit(true);
    return A;
  }

  AcquiredBeforeAttr(SourceRange R, ASTContext &Ctx
              , Expr * *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::AcquiredBefore, R, SI, true, true)
              , args_Size(ArgsSize), args_(new (Ctx, 16) Expr *[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  AcquiredBeforeAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::AcquiredBefore, R, SI, true, true)
              , args_Size(0), args_(nullptr)
  {
  }

  AcquiredBeforeAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  typedef Expr ** args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::AcquiredBefore; }
};

class AliasAttr : public Attr {
unsigned aliaseeLength;
char *aliasee;

public:
  static AliasAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Aliasee, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AliasAttr(Loc, Ctx, Aliasee, 0);
    A->setImplicit(true);
    return A;
  }

  AliasAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Aliasee
              , unsigned SI
             )
    : Attr(attr::Alias, R, SI, false, false)
              , aliaseeLength(Aliasee.size()),aliasee(new (Ctx, 1) char[aliaseeLength])
  {
      if (!Aliasee.empty())
        std::memcpy(aliasee, Aliasee.data(), aliaseeLength);
  }

  AliasAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getAliasee() const {
    return llvm::StringRef(aliasee, aliaseeLength);
  }
  unsigned getAliaseeLength() const {
    return aliaseeLength;
  }
  void setAliasee(ASTContext &C, llvm::StringRef S) {
    aliaseeLength = S.size();
    this->aliasee = new (C, 1) char [aliaseeLength];
    if (!S.empty())
      std::memcpy(this->aliasee, S.data(), aliaseeLength);
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::Alias; }
};

class AlignMac68kAttr : public InheritableAttr {
public:
  static AlignMac68kAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AlignMac68kAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  AlignMac68kAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::AlignMac68k, R, SI, false, false)
  {
  }

  AlignMac68kAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::AlignMac68k; }
};

class AlignValueAttr : public Attr {
Expr * alignment;

public:
  static AlignValueAttr *CreateImplicit(ASTContext &Ctx, Expr * Alignment, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AlignValueAttr(Loc, Ctx, Alignment, 0);
    A->setImplicit(true);
    return A;
  }

  AlignValueAttr(SourceRange R, ASTContext &Ctx
              , Expr * Alignment
              , unsigned SI
             )
    : Attr(attr::AlignValue, R, SI, false, false)
              , alignment(Alignment)
  {
  }

  AlignValueAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Expr * getAlignment() const {
    return alignment;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::AlignValue; }
};

class AlignedAttr : public InheritableAttr {
bool isalignmentExpr;
union {
Expr *alignmentExpr;
TypeSourceInfo *alignmentType;
};

public:
  enum Spelling {
    GNU_aligned = 0,
    CXX11_gnu_aligned = 1,
    Declspec_align = 2,
    Keyword_alignas = 3,
    Keyword_Alignas = 4
  };

  static AlignedAttr *CreateImplicit(ASTContext &Ctx, Spelling S, bool IsAlignmentExpr, void *Alignment, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AlignedAttr(Loc, Ctx, IsAlignmentExpr, Alignment, S);
    A->setImplicit(true);
    return A;
  }

  AlignedAttr(SourceRange R, ASTContext &Ctx
              , bool IsAlignmentExpr, void *Alignment
              , unsigned SI
             )
    : InheritableAttr(attr::Aligned, R, SI, false, false)
              , isalignmentExpr(IsAlignmentExpr)
  {
    if (isalignmentExpr)
       alignmentExpr = reinterpret_cast<Expr *>(Alignment);
    else
       alignmentType = reinterpret_cast<TypeSourceInfo *>(Alignment);
  }

  AlignedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Aligned, R, SI, false, false)
              , isalignmentExpr(false)
  {
  }

  AlignedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_aligned;
    case 1: return CXX11_gnu_aligned;
    case 2: return Declspec_align;
    case 3: return Keyword_alignas;
    case 4: return Keyword_Alignas;
  }
  }
  bool isGNU() const { return SpellingListIndex == 0 ||
    SpellingListIndex == 1; }
  bool isC11() const { return SpellingListIndex == 4; }
  bool isAlignas() const { return SpellingListIndex == 3 ||
    SpellingListIndex == 4; }
  bool isDeclspec() const { return SpellingListIndex == 2; }
  bool isAlignmentDependent() const;
  unsigned getAlignment(ASTContext &Ctx) const;
  bool isAlignmentExpr() const {
    return isalignmentExpr;
  }
  Expr *getAlignmentExpr() const {
    assert(isalignmentExpr);
    return alignmentExpr;
  }
  TypeSourceInfo *getAlignmentType() const {
    assert(!isalignmentExpr);
    return alignmentType;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::Aligned; }
};

class AllocAlignAttr : public InheritableAttr {
int paramIndex;

public:
  static AllocAlignAttr *CreateImplicit(ASTContext &Ctx, int ParamIndex, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AllocAlignAttr(Loc, Ctx, ParamIndex, 0);
    A->setImplicit(true);
    return A;
  }

  AllocAlignAttr(SourceRange R, ASTContext &Ctx
              , int ParamIndex
              , unsigned SI
             )
    : InheritableAttr(attr::AllocAlign, R, SI, false, false)
              , paramIndex(ParamIndex)
  {
  }

  AllocAlignAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  int getParamIndex() const {
    return paramIndex;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::AllocAlign; }
};

class AllocSizeAttr : public InheritableAttr {
int elemSizeParam;

int numElemsParam;

public:
  static AllocSizeAttr *CreateImplicit(ASTContext &Ctx, int ElemSizeParam, int NumElemsParam, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AllocSizeAttr(Loc, Ctx, ElemSizeParam, NumElemsParam, 0);
    A->setImplicit(true);
    return A;
  }

  AllocSizeAttr(SourceRange R, ASTContext &Ctx
              , int ElemSizeParam
              , int NumElemsParam
              , unsigned SI
             )
    : InheritableAttr(attr::AllocSize, R, SI, false, false)
              , elemSizeParam(ElemSizeParam)
              , numElemsParam(NumElemsParam)
  {
  }

  AllocSizeAttr(SourceRange R, ASTContext &Ctx
              , int ElemSizeParam
              , unsigned SI
             )
    : InheritableAttr(attr::AllocSize, R, SI, false, false)
              , elemSizeParam(ElemSizeParam)
              , numElemsParam()
  {
  }

  AllocSizeAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  int getElemSizeParam() const {
    return elemSizeParam;
  }

  int getNumElemsParam() const {
    return numElemsParam;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::AllocSize; }
};

class AlwaysInlineAttr : public InheritableAttr {
public:
  enum Spelling {
    GNU_always_inline = 0,
    CXX11_gnu_always_inline = 1,
    Keyword_forceinline = 2
  };

  static AlwaysInlineAttr *CreateImplicit(ASTContext &Ctx, Spelling S, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AlwaysInlineAttr(Loc, Ctx, S);
    A->setImplicit(true);
    return A;
  }

  AlwaysInlineAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::AlwaysInline, R, SI, false, false)
  {
  }

  AlwaysInlineAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_always_inline;
    case 1: return CXX11_gnu_always_inline;
    case 2: return Keyword_forceinline;
  }
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::AlwaysInline; }
};

class AnalyzerNoReturnAttr : public InheritableAttr {
public:
  static AnalyzerNoReturnAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AnalyzerNoReturnAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  AnalyzerNoReturnAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::AnalyzerNoReturn, R, SI, false, false)
  {
  }

  AnalyzerNoReturnAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::AnalyzerNoReturn; }
};

class AnnotateAttr : public InheritableParamAttr {
unsigned annotationLength;
char *annotation;

public:
  static AnnotateAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Annotation, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AnnotateAttr(Loc, Ctx, Annotation, 0);
    A->setImplicit(true);
    return A;
  }

  AnnotateAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Annotation
              , unsigned SI
             )
    : InheritableParamAttr(attr::Annotate, R, SI, false, false)
              , annotationLength(Annotation.size()),annotation(new (Ctx, 1) char[annotationLength])
  {
      if (!Annotation.empty())
        std::memcpy(annotation, Annotation.data(), annotationLength);
  }

  AnnotateAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getAnnotation() const {
    return llvm::StringRef(annotation, annotationLength);
  }
  unsigned getAnnotationLength() const {
    return annotationLength;
  }
  void setAnnotation(ASTContext &C, llvm::StringRef S) {
    annotationLength = S.size();
    this->annotation = new (C, 1) char [annotationLength];
    if (!S.empty())
      std::memcpy(this->annotation, S.data(), annotationLength);
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::Annotate; }
};

class AnyX86InterruptAttr : public InheritableAttr {
public:
  static AnyX86InterruptAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AnyX86InterruptAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  AnyX86InterruptAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::AnyX86Interrupt, R, SI, false, false)
  {
  }

  AnyX86InterruptAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::AnyX86Interrupt; }
};

class ArcWeakrefUnavailableAttr : public InheritableAttr {
public:
  static ArcWeakrefUnavailableAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ArcWeakrefUnavailableAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ArcWeakrefUnavailableAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ArcWeakrefUnavailable, R, SI, false, false)
  {
  }

  ArcWeakrefUnavailableAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ArcWeakrefUnavailable; }
};

class ArgumentWithTypeTagAttr : public InheritableAttr {
IdentifierInfo * argumentKind;

unsigned argumentIdx;

unsigned typeTagIdx;

bool isPointer;

public:
  enum Spelling {
    GNU_argument_with_type_tag = 0,
    GNU_pointer_with_type_tag = 1
  };

  static ArgumentWithTypeTagAttr *CreateImplicit(ASTContext &Ctx, Spelling S, IdentifierInfo * ArgumentKind, unsigned ArgumentIdx, unsigned TypeTagIdx, bool IsPointer, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ArgumentWithTypeTagAttr(Loc, Ctx, ArgumentKind, ArgumentIdx, TypeTagIdx, IsPointer, S);
    A->setImplicit(true);
    return A;
  }

  ArgumentWithTypeTagAttr(SourceRange R, ASTContext &Ctx
              , IdentifierInfo * ArgumentKind
              , unsigned ArgumentIdx
              , unsigned TypeTagIdx
              , bool IsPointer
              , unsigned SI
             )
    : InheritableAttr(attr::ArgumentWithTypeTag, R, SI, false, false)
              , argumentKind(ArgumentKind)
              , argumentIdx(ArgumentIdx)
              , typeTagIdx(TypeTagIdx)
              , isPointer(IsPointer)
  {
  }

  ArgumentWithTypeTagAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_argument_with_type_tag;
    case 1: return GNU_pointer_with_type_tag;
  }
  }
  IdentifierInfo * getArgumentKind() const {
    return argumentKind;
  }

  unsigned getArgumentIdx() const {
    return argumentIdx;
  }

  unsigned getTypeTagIdx() const {
    return typeTagIdx;
  }

  bool getIsPointer() const {
    return isPointer;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::ArgumentWithTypeTag; }
};

class AsmLabelAttr : public InheritableAttr {
unsigned labelLength;
char *label;

public:
  static AsmLabelAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Label, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AsmLabelAttr(Loc, Ctx, Label, 0);
    A->setImplicit(true);
    return A;
  }

  AsmLabelAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Label
              , unsigned SI
             )
    : InheritableAttr(attr::AsmLabel, R, SI, false, false)
              , labelLength(Label.size()),label(new (Ctx, 1) char[labelLength])
  {
      if (!Label.empty())
        std::memcpy(label, Label.data(), labelLength);
  }

  AsmLabelAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getLabel() const {
    return llvm::StringRef(label, labelLength);
  }
  unsigned getLabelLength() const {
    return labelLength;
  }
  void setLabel(ASTContext &C, llvm::StringRef S) {
    labelLength = S.size();
    this->label = new (C, 1) char [labelLength];
    if (!S.empty())
      std::memcpy(this->label, S.data(), labelLength);
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::AsmLabel; }
};

class AssertCapabilityAttr : public InheritableAttr {
Expr * expr;

public:
  enum Spelling {
    GNU_assert_capability = 0,
    CXX11_clang_assert_capability = 1,
    GNU_assert_shared_capability = 2,
    CXX11_clang_assert_shared_capability = 3
  };

  static AssertCapabilityAttr *CreateImplicit(ASTContext &Ctx, Spelling S, Expr * Expr, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AssertCapabilityAttr(Loc, Ctx, Expr, S);
    A->setImplicit(true);
    return A;
  }

  AssertCapabilityAttr(SourceRange R, ASTContext &Ctx
              , Expr * Expr
              , unsigned SI
             )
    : InheritableAttr(attr::AssertCapability, R, SI, true, true)
              , expr(Expr)
  {
  }

  AssertCapabilityAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_assert_capability;
    case 1: return CXX11_clang_assert_capability;
    case 2: return GNU_assert_shared_capability;
    case 3: return CXX11_clang_assert_shared_capability;
  }
  }
  bool isShared() const { return SpellingListIndex == 2 ||
    SpellingListIndex == 3; }
  Expr * getExpr() const {
    return expr;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::AssertCapability; }
};

class AssertExclusiveLockAttr : public InheritableAttr {
  unsigned args_Size;
  Expr * *args_;

public:
  static AssertExclusiveLockAttr *CreateImplicit(ASTContext &Ctx, Expr * *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AssertExclusiveLockAttr(Loc, Ctx, Args, ArgsSize, 0);
    A->setImplicit(true);
    return A;
  }

  AssertExclusiveLockAttr(SourceRange R, ASTContext &Ctx
              , Expr * *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::AssertExclusiveLock, R, SI, true, true)
              , args_Size(ArgsSize), args_(new (Ctx, 16) Expr *[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  AssertExclusiveLockAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::AssertExclusiveLock, R, SI, true, true)
              , args_Size(0), args_(nullptr)
  {
  }

  AssertExclusiveLockAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  typedef Expr ** args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::AssertExclusiveLock; }
};

class AssertSharedLockAttr : public InheritableAttr {
  unsigned args_Size;
  Expr * *args_;

public:
  static AssertSharedLockAttr *CreateImplicit(ASTContext &Ctx, Expr * *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AssertSharedLockAttr(Loc, Ctx, Args, ArgsSize, 0);
    A->setImplicit(true);
    return A;
  }

  AssertSharedLockAttr(SourceRange R, ASTContext &Ctx
              , Expr * *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::AssertSharedLock, R, SI, true, true)
              , args_Size(ArgsSize), args_(new (Ctx, 16) Expr *[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  AssertSharedLockAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::AssertSharedLock, R, SI, true, true)
              , args_Size(0), args_(nullptr)
  {
  }

  AssertSharedLockAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  typedef Expr ** args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::AssertSharedLock; }
};

class AssumeAlignedAttr : public InheritableAttr {
Expr * alignment;

Expr * offset;

public:
  static AssumeAlignedAttr *CreateImplicit(ASTContext &Ctx, Expr * Alignment, Expr * Offset, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AssumeAlignedAttr(Loc, Ctx, Alignment, Offset, 0);
    A->setImplicit(true);
    return A;
  }

  AssumeAlignedAttr(SourceRange R, ASTContext &Ctx
              , Expr * Alignment
              , Expr * Offset
              , unsigned SI
             )
    : InheritableAttr(attr::AssumeAligned, R, SI, false, false)
              , alignment(Alignment)
              , offset(Offset)
  {
  }

  AssumeAlignedAttr(SourceRange R, ASTContext &Ctx
              , Expr * Alignment
              , unsigned SI
             )
    : InheritableAttr(attr::AssumeAligned, R, SI, false, false)
              , alignment(Alignment)
              , offset()
  {
  }

  AssumeAlignedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Expr * getAlignment() const {
    return alignment;
  }

  Expr * getOffset() const {
    return offset;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::AssumeAligned; }
};

class AvailabilityAttr : public InheritableAttr {
IdentifierInfo * platform;

VersionTuple introduced;


VersionTuple deprecated;


VersionTuple obsoleted;


bool unavailable;

unsigned messageLength;
char *message;

bool strict;

unsigned replacementLength;
char *replacement;

public:
  static AvailabilityAttr *CreateImplicit(ASTContext &Ctx, IdentifierInfo * Platform, VersionTuple Introduced, VersionTuple Deprecated, VersionTuple Obsoleted, bool Unavailable, llvm::StringRef Message, bool Strict, llvm::StringRef Replacement, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) AvailabilityAttr(Loc, Ctx, Platform, Introduced, Deprecated, Obsoleted, Unavailable, Message, Strict, Replacement, 0);
    A->setImplicit(true);
    return A;
  }

  AvailabilityAttr(SourceRange R, ASTContext &Ctx
              , IdentifierInfo * Platform
              , VersionTuple Introduced
              , VersionTuple Deprecated
              , VersionTuple Obsoleted
              , bool Unavailable
              , llvm::StringRef Message
              , bool Strict
              , llvm::StringRef Replacement
              , unsigned SI
             )
    : InheritableAttr(attr::Availability, R, SI, false, true)
              , platform(Platform)
              , introduced(Introduced)
              , deprecated(Deprecated)
              , obsoleted(Obsoleted)
              , unavailable(Unavailable)
              , messageLength(Message.size()),message(new (Ctx, 1) char[messageLength])
              , strict(Strict)
              , replacementLength(Replacement.size()),replacement(new (Ctx, 1) char[replacementLength])
  {
      if (!Message.empty())
        std::memcpy(message, Message.data(), messageLength);
      if (!Replacement.empty())
        std::memcpy(replacement, Replacement.data(), replacementLength);
  }

  AvailabilityAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  IdentifierInfo * getPlatform() const {
    return platform;
  }

  VersionTuple getIntroduced() const {
    return introduced;
  }
  void setIntroduced(ASTContext &C, VersionTuple V) {
    introduced = V;
  }

  VersionTuple getDeprecated() const {
    return deprecated;
  }
  void setDeprecated(ASTContext &C, VersionTuple V) {
    deprecated = V;
  }

  VersionTuple getObsoleted() const {
    return obsoleted;
  }
  void setObsoleted(ASTContext &C, VersionTuple V) {
    obsoleted = V;
  }

  bool getUnavailable() const {
    return unavailable;
  }

  llvm::StringRef getMessage() const {
    return llvm::StringRef(message, messageLength);
  }
  unsigned getMessageLength() const {
    return messageLength;
  }
  void setMessage(ASTContext &C, llvm::StringRef S) {
    messageLength = S.size();
    this->message = new (C, 1) char [messageLength];
    if (!S.empty())
      std::memcpy(this->message, S.data(), messageLength);
  }

  bool getStrict() const {
    return strict;
  }

  llvm::StringRef getReplacement() const {
    return llvm::StringRef(replacement, replacementLength);
  }
  unsigned getReplacementLength() const {
    return replacementLength;
  }
  void setReplacement(ASTContext &C, llvm::StringRef S) {
    replacementLength = S.size();
    this->replacement = new (C, 1) char [replacementLength];
    if (!S.empty())
      std::memcpy(this->replacement, S.data(), replacementLength);
  }

static llvm::StringRef getPrettyPlatformName(llvm::StringRef Platform) {
    return llvm::StringSwitch<llvm::StringRef>(Platform)
             .Case("android", "Android")
             .Case("ios", "iOS")
             .Case("macos", "macOS")
             .Case("tvos", "tvOS")
             .Case("watchos", "watchOS")
             .Case("ios_app_extension", "iOS (App Extension)")
             .Case("macos_app_extension", "macOS (App Extension)")
             .Case("tvos_app_extension", "tvOS (App Extension)")
             .Case("watchos_app_extension", "watchOS (App Extension)")
             .Default(llvm::StringRef());
} 

  static bool classof(const Attr *A) { return A->getKind() == attr::Availability; }
};

class BlocksAttr : public InheritableAttr {
public:
  enum BlockType {
    ByRef
  };
private:
  BlockType type;

public:
  static BlocksAttr *CreateImplicit(ASTContext &Ctx, BlockType Type, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) BlocksAttr(Loc, Ctx, Type, 0);
    A->setImplicit(true);
    return A;
  }

  BlocksAttr(SourceRange R, ASTContext &Ctx
              , BlockType Type
              , unsigned SI
             )
    : InheritableAttr(attr::Blocks, R, SI, false, false)
              , type(Type)
  {
  }

  BlocksAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  BlockType getType() const {
    return type;
  }

  static bool ConvertStrToBlockType(StringRef Val, BlockType &Out) {
    Optional<BlockType> R = llvm::StringSwitch<Optional<BlockType>>(Val)
      .Case("byref", BlocksAttr::ByRef)
      .Default(Optional<BlockType>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertBlockTypeToStr(BlockType Val) {
    switch(Val) {
    case BlocksAttr::ByRef: return "byref";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::Blocks; }
};

class C11NoReturnAttr : public InheritableAttr {
public:
  static C11NoReturnAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) C11NoReturnAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  C11NoReturnAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::C11NoReturn, R, SI, false, false)
  {
  }

  C11NoReturnAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::C11NoReturn; }
};

class CDeclAttr : public InheritableAttr {
public:
  static CDeclAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CDeclAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CDeclAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CDecl, R, SI, false, false)
  {
  }

  CDeclAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CDecl; }
};

class CFAuditedTransferAttr : public InheritableAttr {
public:
  static CFAuditedTransferAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CFAuditedTransferAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CFAuditedTransferAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CFAuditedTransfer, R, SI, false, false)
  {
  }

  CFAuditedTransferAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CFAuditedTransfer; }
};

class CFConsumedAttr : public InheritableParamAttr {
public:
  static CFConsumedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CFConsumedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CFConsumedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableParamAttr(attr::CFConsumed, R, SI, false, false)
  {
  }

  CFConsumedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CFConsumed; }
};

class CFReturnsNotRetainedAttr : public InheritableAttr {
public:
  static CFReturnsNotRetainedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CFReturnsNotRetainedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CFReturnsNotRetainedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CFReturnsNotRetained, R, SI, false, false)
  {
  }

  CFReturnsNotRetainedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CFReturnsNotRetained; }
};

class CFReturnsRetainedAttr : public InheritableAttr {
public:
  static CFReturnsRetainedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CFReturnsRetainedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CFReturnsRetainedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CFReturnsRetained, R, SI, false, false)
  {
  }

  CFReturnsRetainedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CFReturnsRetained; }
};

class CFUnknownTransferAttr : public InheritableAttr {
public:
  static CFUnknownTransferAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CFUnknownTransferAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CFUnknownTransferAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CFUnknownTransfer, R, SI, false, false)
  {
  }

  CFUnknownTransferAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CFUnknownTransfer; }
};

class CUDAConstantAttr : public InheritableAttr {
public:
  static CUDAConstantAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CUDAConstantAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CUDAConstantAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CUDAConstant, R, SI, false, false)
  {
  }

  CUDAConstantAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CUDAConstant; }
};

class CUDADeviceAttr : public InheritableAttr {
public:
  static CUDADeviceAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CUDADeviceAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CUDADeviceAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CUDADevice, R, SI, false, false)
  {
  }

  CUDADeviceAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CUDADevice; }
};

class CUDAGlobalAttr : public InheritableAttr {
public:
  static CUDAGlobalAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CUDAGlobalAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CUDAGlobalAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CUDAGlobal, R, SI, false, false)
  {
  }

  CUDAGlobalAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CUDAGlobal; }
};

class CUDAHostAttr : public InheritableAttr {
public:
  static CUDAHostAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CUDAHostAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CUDAHostAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CUDAHost, R, SI, false, false)
  {
  }

  CUDAHostAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CUDAHost; }
};

class CUDAInvalidTargetAttr : public InheritableAttr {
public:
  static CUDAInvalidTargetAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CUDAInvalidTargetAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CUDAInvalidTargetAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CUDAInvalidTarget, R, SI, false, false)
  {
  }

  CUDAInvalidTargetAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CUDAInvalidTarget; }
};

class CUDALaunchBoundsAttr : public InheritableAttr {
Expr * maxThreads;

Expr * minBlocks;

public:
  static CUDALaunchBoundsAttr *CreateImplicit(ASTContext &Ctx, Expr * MaxThreads, Expr * MinBlocks, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CUDALaunchBoundsAttr(Loc, Ctx, MaxThreads, MinBlocks, 0);
    A->setImplicit(true);
    return A;
  }

  CUDALaunchBoundsAttr(SourceRange R, ASTContext &Ctx
              , Expr * MaxThreads
              , Expr * MinBlocks
              , unsigned SI
             )
    : InheritableAttr(attr::CUDALaunchBounds, R, SI, false, false)
              , maxThreads(MaxThreads)
              , minBlocks(MinBlocks)
  {
  }

  CUDALaunchBoundsAttr(SourceRange R, ASTContext &Ctx
              , Expr * MaxThreads
              , unsigned SI
             )
    : InheritableAttr(attr::CUDALaunchBounds, R, SI, false, false)
              , maxThreads(MaxThreads)
              , minBlocks()
  {
  }

  CUDALaunchBoundsAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Expr * getMaxThreads() const {
    return maxThreads;
  }

  Expr * getMinBlocks() const {
    return minBlocks;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::CUDALaunchBounds; }
};

class CUDASharedAttr : public InheritableAttr {
public:
  static CUDASharedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CUDASharedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CUDASharedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CUDAShared, R, SI, false, false)
  {
  }

  CUDASharedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CUDAShared; }
};

class CXX11NoReturnAttr : public InheritableAttr {
public:
  static CXX11NoReturnAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CXX11NoReturnAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CXX11NoReturnAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CXX11NoReturn, R, SI, false, false)
  {
  }

  CXX11NoReturnAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CXX11NoReturn; }
};

class CallableWhenAttr : public InheritableAttr {
public:
  enum ConsumedState {
    Unknown,
    Consumed,
    Unconsumed
  };
private:
  unsigned callableStates_Size;
  ConsumedState *callableStates_;

public:
  static CallableWhenAttr *CreateImplicit(ASTContext &Ctx, ConsumedState *CallableStates, unsigned CallableStatesSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CallableWhenAttr(Loc, Ctx, CallableStates, CallableStatesSize, 0);
    A->setImplicit(true);
    return A;
  }

  CallableWhenAttr(SourceRange R, ASTContext &Ctx
              , ConsumedState *CallableStates, unsigned CallableStatesSize
              , unsigned SI
             )
    : InheritableAttr(attr::CallableWhen, R, SI, false, false)
              , callableStates_Size(CallableStatesSize), callableStates_(new (Ctx, 16) ConsumedState[callableStates_Size])
  {
    std::copy(CallableStates, CallableStates + callableStates_Size, callableStates_);
  }

  CallableWhenAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CallableWhen, R, SI, false, false)
              , callableStates_Size(0), callableStates_(nullptr)
  {
  }

  CallableWhenAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  typedef ConsumedState* callableStates_iterator;
  callableStates_iterator callableStates_begin() const { return callableStates_; }
  callableStates_iterator callableStates_end() const { return callableStates_ + callableStates_Size; }
  unsigned callableStates_size() const { return callableStates_Size; }
  llvm::iterator_range<callableStates_iterator> callableStates() const { return llvm::make_range(callableStates_begin(), callableStates_end()); }


  static bool ConvertStrToConsumedState(StringRef Val, ConsumedState &Out) {
    Optional<ConsumedState> R = llvm::StringSwitch<Optional<ConsumedState>>(Val)
      .Case("unknown", CallableWhenAttr::Unknown)
      .Case("consumed", CallableWhenAttr::Consumed)
      .Case("unconsumed", CallableWhenAttr::Unconsumed)
      .Default(Optional<ConsumedState>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertConsumedStateToStr(ConsumedState Val) {
    switch(Val) {
    case CallableWhenAttr::Unknown: return "unknown";
    case CallableWhenAttr::Consumed: return "consumed";
    case CallableWhenAttr::Unconsumed: return "unconsumed";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::CallableWhen; }
};

class CapabilityAttr : public InheritableAttr {
unsigned nameLength;
char *name;

public:
  enum Spelling {
    GNU_capability = 0,
    CXX11_clang_capability = 1,
    GNU_shared_capability = 2,
    CXX11_clang_shared_capability = 3
  };

  static CapabilityAttr *CreateImplicit(ASTContext &Ctx, Spelling S, llvm::StringRef Name, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CapabilityAttr(Loc, Ctx, Name, S);
    A->setImplicit(true);
    return A;
  }

  CapabilityAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Name
              , unsigned SI
             )
    : InheritableAttr(attr::Capability, R, SI, false, false)
              , nameLength(Name.size()),name(new (Ctx, 1) char[nameLength])
  {
      if (!Name.empty())
        std::memcpy(name, Name.data(), nameLength);
  }

  CapabilityAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_capability;
    case 1: return CXX11_clang_capability;
    case 2: return GNU_shared_capability;
    case 3: return CXX11_clang_shared_capability;
  }
  }
  bool isShared() const { return SpellingListIndex == 2 ||
    SpellingListIndex == 3; }
  llvm::StringRef getName() const {
    return llvm::StringRef(name, nameLength);
  }
  unsigned getNameLength() const {
    return nameLength;
  }
  void setName(ASTContext &C, llvm::StringRef S) {
    nameLength = S.size();
    this->name = new (C, 1) char [nameLength];
    if (!S.empty())
      std::memcpy(this->name, S.data(), nameLength);
  }


    bool isMutex() const { return getName().equals_lower("mutex"); }
    bool isRole() const { return getName().equals_lower("role"); }
  

  static bool classof(const Attr *A) { return A->getKind() == attr::Capability; }
};

class CapturedRecordAttr : public InheritableAttr {
public:
  static CapturedRecordAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CapturedRecordAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CapturedRecordAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::CapturedRecord, R, SI, false, false)
  {
  }

  CapturedRecordAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CapturedRecord; }
};

class CarriesDependencyAttr : public InheritableParamAttr {
public:
  static CarriesDependencyAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CarriesDependencyAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CarriesDependencyAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableParamAttr(attr::CarriesDependency, R, SI, false, false)
  {
  }

  CarriesDependencyAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::CarriesDependency; }
};

class CleanupAttr : public InheritableAttr {
FunctionDecl * functionDecl;

public:
  static CleanupAttr *CreateImplicit(ASTContext &Ctx, FunctionDecl * FunctionDecl, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CleanupAttr(Loc, Ctx, FunctionDecl, 0);
    A->setImplicit(true);
    return A;
  }

  CleanupAttr(SourceRange R, ASTContext &Ctx
              , FunctionDecl * FunctionDecl
              , unsigned SI
             )
    : InheritableAttr(attr::Cleanup, R, SI, false, false)
              , functionDecl(FunctionDecl)
  {
  }

  CleanupAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  FunctionDecl * getFunctionDecl() const {
    return functionDecl;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::Cleanup; }
};

class ColdAttr : public InheritableAttr {
public:
  static ColdAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ColdAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ColdAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Cold, R, SI, false, false)
  {
  }

  ColdAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Cold; }
};

class CommonAttr : public InheritableAttr {
public:
  static CommonAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) CommonAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  CommonAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Common, R, SI, false, false)
  {
  }

  CommonAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Common; }
};

class ConstAttr : public InheritableAttr {
public:
  static ConstAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ConstAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ConstAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Const, R, SI, false, false)
  {
  }

  ConstAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Const; }
};

class ConstructorAttr : public InheritableAttr {
int priority;

public:
  static ConstructorAttr *CreateImplicit(ASTContext &Ctx, int Priority, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ConstructorAttr(Loc, Ctx, Priority, 0);
    A->setImplicit(true);
    return A;
  }

  ConstructorAttr(SourceRange R, ASTContext &Ctx
              , int Priority
              , unsigned SI
             )
    : InheritableAttr(attr::Constructor, R, SI, false, false)
              , priority(Priority)
  {
  }

  ConstructorAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Constructor, R, SI, false, false)
              , priority()
  {
  }

  ConstructorAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  int getPriority() const {
    return priority;
  }

  static const int DefaultPriority = 65535;



  static bool classof(const Attr *A) { return A->getKind() == attr::Constructor; }
};

class ConsumableAttr : public InheritableAttr {
public:
  enum ConsumedState {
    Unknown,
    Consumed,
    Unconsumed
  };
private:
  ConsumedState defaultState;

public:
  static ConsumableAttr *CreateImplicit(ASTContext &Ctx, ConsumedState DefaultState, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ConsumableAttr(Loc, Ctx, DefaultState, 0);
    A->setImplicit(true);
    return A;
  }

  ConsumableAttr(SourceRange R, ASTContext &Ctx
              , ConsumedState DefaultState
              , unsigned SI
             )
    : InheritableAttr(attr::Consumable, R, SI, false, false)
              , defaultState(DefaultState)
  {
  }

  ConsumableAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  ConsumedState getDefaultState() const {
    return defaultState;
  }

  static bool ConvertStrToConsumedState(StringRef Val, ConsumedState &Out) {
    Optional<ConsumedState> R = llvm::StringSwitch<Optional<ConsumedState>>(Val)
      .Case("unknown", ConsumableAttr::Unknown)
      .Case("consumed", ConsumableAttr::Consumed)
      .Case("unconsumed", ConsumableAttr::Unconsumed)
      .Default(Optional<ConsumedState>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertConsumedStateToStr(ConsumedState Val) {
    switch(Val) {
    case ConsumableAttr::Unknown: return "unknown";
    case ConsumableAttr::Consumed: return "consumed";
    case ConsumableAttr::Unconsumed: return "unconsumed";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::Consumable; }
};

class ConsumableAutoCastAttr : public InheritableAttr {
public:
  static ConsumableAutoCastAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ConsumableAutoCastAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ConsumableAutoCastAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ConsumableAutoCast, R, SI, false, false)
  {
  }

  ConsumableAutoCastAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ConsumableAutoCast; }
};

class ConsumableSetOnReadAttr : public InheritableAttr {
public:
  static ConsumableSetOnReadAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ConsumableSetOnReadAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ConsumableSetOnReadAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ConsumableSetOnRead, R, SI, false, false)
  {
  }

  ConsumableSetOnReadAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ConsumableSetOnRead; }
};

class ConvergentAttr : public InheritableAttr {
public:
  static ConvergentAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ConvergentAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ConvergentAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Convergent, R, SI, false, false)
  {
  }

  ConvergentAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Convergent; }
};

class DLLExportAttr : public InheritableAttr {
public:
  static DLLExportAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) DLLExportAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  DLLExportAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::DLLExport, R, SI, false, false)
  {
  }

  DLLExportAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::DLLExport; }
};

class DLLImportAttr : public InheritableAttr {
public:
  static DLLImportAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) DLLImportAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  DLLImportAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::DLLImport, R, SI, false, false)
  {
  }

  DLLImportAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::DLLImport; }
};

class DeprecatedAttr : public InheritableAttr {
unsigned messageLength;
char *message;

unsigned replacementLength;
char *replacement;

public:
  static DeprecatedAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Message, llvm::StringRef Replacement, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) DeprecatedAttr(Loc, Ctx, Message, Replacement, 0);
    A->setImplicit(true);
    return A;
  }

  DeprecatedAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Message
              , llvm::StringRef Replacement
              , unsigned SI
             )
    : InheritableAttr(attr::Deprecated, R, SI, false, false)
              , messageLength(Message.size()),message(new (Ctx, 1) char[messageLength])
              , replacementLength(Replacement.size()),replacement(new (Ctx, 1) char[replacementLength])
  {
      if (!Message.empty())
        std::memcpy(message, Message.data(), messageLength);
      if (!Replacement.empty())
        std::memcpy(replacement, Replacement.data(), replacementLength);
  }

  DeprecatedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Deprecated, R, SI, false, false)
              , messageLength(0),message(nullptr)
              , replacementLength(0),replacement(nullptr)
  {
  }

  DeprecatedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getMessage() const {
    return llvm::StringRef(message, messageLength);
  }
  unsigned getMessageLength() const {
    return messageLength;
  }
  void setMessage(ASTContext &C, llvm::StringRef S) {
    messageLength = S.size();
    this->message = new (C, 1) char [messageLength];
    if (!S.empty())
      std::memcpy(this->message, S.data(), messageLength);
  }

  llvm::StringRef getReplacement() const {
    return llvm::StringRef(replacement, replacementLength);
  }
  unsigned getReplacementLength() const {
    return replacementLength;
  }
  void setReplacement(ASTContext &C, llvm::StringRef S) {
    replacementLength = S.size();
    this->replacement = new (C, 1) char [replacementLength];
    if (!S.empty())
      std::memcpy(this->replacement, S.data(), replacementLength);
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::Deprecated; }
};

class DestructorAttr : public InheritableAttr {
int priority;

public:
  static DestructorAttr *CreateImplicit(ASTContext &Ctx, int Priority, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) DestructorAttr(Loc, Ctx, Priority, 0);
    A->setImplicit(true);
    return A;
  }

  DestructorAttr(SourceRange R, ASTContext &Ctx
              , int Priority
              , unsigned SI
             )
    : InheritableAttr(attr::Destructor, R, SI, false, false)
              , priority(Priority)
  {
  }

  DestructorAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Destructor, R, SI, false, false)
              , priority()
  {
  }

  DestructorAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  int getPriority() const {
    return priority;
  }

  static const int DefaultPriority = 65535;



  static bool classof(const Attr *A) { return A->getKind() == attr::Destructor; }
};

class DiagnoseIfAttr : public InheritableAttr {
Expr * cond;

unsigned messageLength;
char *message;

public:
  enum DiagnosticType {
    DT_Error,
    DT_Warning
  };
private:
  DiagnosticType diagnosticType;

bool argDependent;

FunctionDecl * parent;

public:
  static DiagnoseIfAttr *CreateImplicit(ASTContext &Ctx, Expr * Cond, llvm::StringRef Message, DiagnosticType DiagnosticType, bool ArgDependent, FunctionDecl * Parent, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) DiagnoseIfAttr(Loc, Ctx, Cond, Message, DiagnosticType, ArgDependent, Parent, 0);
    A->setImplicit(true);
    return A;
  }

  static DiagnoseIfAttr *CreateImplicit(ASTContext &Ctx, Expr * Cond, llvm::StringRef Message, DiagnosticType DiagnosticType, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) DiagnoseIfAttr(Loc, Ctx, Cond, Message, DiagnosticType, 0);
    A->setImplicit(true);
    return A;
  }

  DiagnoseIfAttr(SourceRange R, ASTContext &Ctx
              , Expr * Cond
              , llvm::StringRef Message
              , DiagnosticType DiagnosticType
              , bool ArgDependent
              , FunctionDecl * Parent
              , unsigned SI
             )
    : InheritableAttr(attr::DiagnoseIf, R, SI, true, true)
              , cond(Cond)
              , messageLength(Message.size()),message(new (Ctx, 1) char[messageLength])
              , diagnosticType(DiagnosticType)
              , argDependent(ArgDependent)
              , parent(Parent)
  {
      if (!Message.empty())
        std::memcpy(message, Message.data(), messageLength);
  }

  DiagnoseIfAttr(SourceRange R, ASTContext &Ctx
              , Expr * Cond
              , llvm::StringRef Message
              , DiagnosticType DiagnosticType
              , unsigned SI
             )
    : InheritableAttr(attr::DiagnoseIf, R, SI, true, true)
              , cond(Cond)
              , messageLength(Message.size()),message(new (Ctx, 1) char[messageLength])
              , diagnosticType(DiagnosticType)
              , argDependent()
              , parent()
  {
      if (!Message.empty())
        std::memcpy(message, Message.data(), messageLength);
  }

  DiagnoseIfAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Expr * getCond() const {
    return cond;
  }

  llvm::StringRef getMessage() const {
    return llvm::StringRef(message, messageLength);
  }
  unsigned getMessageLength() const {
    return messageLength;
  }
  void setMessage(ASTContext &C, llvm::StringRef S) {
    messageLength = S.size();
    this->message = new (C, 1) char [messageLength];
    if (!S.empty())
      std::memcpy(this->message, S.data(), messageLength);
  }

  DiagnosticType getDiagnosticType() const {
    return diagnosticType;
  }

  static bool ConvertStrToDiagnosticType(StringRef Val, DiagnosticType &Out) {
    Optional<DiagnosticType> R = llvm::StringSwitch<Optional<DiagnosticType>>(Val)
      .Case("error", DiagnoseIfAttr::DT_Error)
      .Case("warning", DiagnoseIfAttr::DT_Warning)
      .Default(Optional<DiagnosticType>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertDiagnosticTypeToStr(DiagnosticType Val) {
    switch(Val) {
    case DiagnoseIfAttr::DT_Error: return "error";
    case DiagnoseIfAttr::DT_Warning: return "warning";
    }
    llvm_unreachable("No enumerator with that value");
  }
  bool getArgDependent() const {
    return argDependent;
  }

  FunctionDecl * getParent() const {
    return parent;
  }


    bool isError() const { return diagnosticType == DT_Error; }
    bool isWarning() const { return diagnosticType == DT_Warning; }
  

  static bool classof(const Attr *A) { return A->getKind() == attr::DiagnoseIf; }
};

class DisableTailCallsAttr : public InheritableAttr {
public:
  static DisableTailCallsAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) DisableTailCallsAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  DisableTailCallsAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::DisableTailCalls, R, SI, false, false)
  {
  }

  DisableTailCallsAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::DisableTailCalls; }
};

class EmptyBasesAttr : public InheritableAttr {
public:
  static EmptyBasesAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) EmptyBasesAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  EmptyBasesAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::EmptyBases, R, SI, false, false)
  {
  }

  EmptyBasesAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::EmptyBases; }
};

class EnableIfAttr : public InheritableAttr {
Expr * cond;

unsigned messageLength;
char *message;

public:
  static EnableIfAttr *CreateImplicit(ASTContext &Ctx, Expr * Cond, llvm::StringRef Message, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) EnableIfAttr(Loc, Ctx, Cond, Message, 0);
    A->setImplicit(true);
    return A;
  }

  EnableIfAttr(SourceRange R, ASTContext &Ctx
              , Expr * Cond
              , llvm::StringRef Message
              , unsigned SI
             )
    : InheritableAttr(attr::EnableIf, R, SI, false, false)
              , cond(Cond)
              , messageLength(Message.size()),message(new (Ctx, 1) char[messageLength])
  {
      if (!Message.empty())
        std::memcpy(message, Message.data(), messageLength);
  }

  EnableIfAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Expr * getCond() const {
    return cond;
  }

  llvm::StringRef getMessage() const {
    return llvm::StringRef(message, messageLength);
  }
  unsigned getMessageLength() const {
    return messageLength;
  }
  void setMessage(ASTContext &C, llvm::StringRef S) {
    messageLength = S.size();
    this->message = new (C, 1) char [messageLength];
    if (!S.empty())
      std::memcpy(this->message, S.data(), messageLength);
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::EnableIf; }
};

class EnumExtensibilityAttr : public InheritableAttr {
public:
  enum Kind {
    Closed,
    Open
  };
private:
  Kind extensibility;

public:
  static EnumExtensibilityAttr *CreateImplicit(ASTContext &Ctx, Kind Extensibility, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) EnumExtensibilityAttr(Loc, Ctx, Extensibility, 0);
    A->setImplicit(true);
    return A;
  }

  EnumExtensibilityAttr(SourceRange R, ASTContext &Ctx
              , Kind Extensibility
              , unsigned SI
             )
    : InheritableAttr(attr::EnumExtensibility, R, SI, false, false)
              , extensibility(Extensibility)
  {
  }

  EnumExtensibilityAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Kind getExtensibility() const {
    return extensibility;
  }

  static bool ConvertStrToKind(StringRef Val, Kind &Out) {
    Optional<Kind> R = llvm::StringSwitch<Optional<Kind>>(Val)
      .Case("closed", EnumExtensibilityAttr::Closed)
      .Case("open", EnumExtensibilityAttr::Open)
      .Default(Optional<Kind>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertKindToStr(Kind Val) {
    switch(Val) {
    case EnumExtensibilityAttr::Closed: return "closed";
    case EnumExtensibilityAttr::Open: return "open";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::EnumExtensibility; }
};

class ExclusiveTrylockFunctionAttr : public InheritableAttr {
Expr * successValue;

  unsigned args_Size;
  Expr * *args_;

public:
  static ExclusiveTrylockFunctionAttr *CreateImplicit(ASTContext &Ctx, Expr * SuccessValue, Expr * *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ExclusiveTrylockFunctionAttr(Loc, Ctx, SuccessValue, Args, ArgsSize, 0);
    A->setImplicit(true);
    return A;
  }

  ExclusiveTrylockFunctionAttr(SourceRange R, ASTContext &Ctx
              , Expr * SuccessValue
              , Expr * *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::ExclusiveTrylockFunction, R, SI, true, true)
              , successValue(SuccessValue)
              , args_Size(ArgsSize), args_(new (Ctx, 16) Expr *[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  ExclusiveTrylockFunctionAttr(SourceRange R, ASTContext &Ctx
              , Expr * SuccessValue
              , unsigned SI
             )
    : InheritableAttr(attr::ExclusiveTrylockFunction, R, SI, true, true)
              , successValue(SuccessValue)
              , args_Size(0), args_(nullptr)
  {
  }

  ExclusiveTrylockFunctionAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Expr * getSuccessValue() const {
    return successValue;
  }

  typedef Expr ** args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::ExclusiveTrylockFunction; }
};

class ExternalSourceSymbolAttr : public InheritableAttr {
unsigned languageLength;
char *language;

unsigned definedInLength;
char *definedIn;

bool generatedDeclaration;

public:
  static ExternalSourceSymbolAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Language, llvm::StringRef DefinedIn, bool GeneratedDeclaration, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ExternalSourceSymbolAttr(Loc, Ctx, Language, DefinedIn, GeneratedDeclaration, 0);
    A->setImplicit(true);
    return A;
  }

  ExternalSourceSymbolAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Language
              , llvm::StringRef DefinedIn
              , bool GeneratedDeclaration
              , unsigned SI
             )
    : InheritableAttr(attr::ExternalSourceSymbol, R, SI, false, false)
              , languageLength(Language.size()),language(new (Ctx, 1) char[languageLength])
              , definedInLength(DefinedIn.size()),definedIn(new (Ctx, 1) char[definedInLength])
              , generatedDeclaration(GeneratedDeclaration)
  {
      if (!Language.empty())
        std::memcpy(language, Language.data(), languageLength);
      if (!DefinedIn.empty())
        std::memcpy(definedIn, DefinedIn.data(), definedInLength);
  }

  ExternalSourceSymbolAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ExternalSourceSymbol, R, SI, false, false)
              , languageLength(0),language(nullptr)
              , definedInLength(0),definedIn(nullptr)
              , generatedDeclaration()
  {
  }

  ExternalSourceSymbolAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getLanguage() const {
    return llvm::StringRef(language, languageLength);
  }
  unsigned getLanguageLength() const {
    return languageLength;
  }
  void setLanguage(ASTContext &C, llvm::StringRef S) {
    languageLength = S.size();
    this->language = new (C, 1) char [languageLength];
    if (!S.empty())
      std::memcpy(this->language, S.data(), languageLength);
  }

  llvm::StringRef getDefinedIn() const {
    return llvm::StringRef(definedIn, definedInLength);
  }
  unsigned getDefinedInLength() const {
    return definedInLength;
  }
  void setDefinedIn(ASTContext &C, llvm::StringRef S) {
    definedInLength = S.size();
    this->definedIn = new (C, 1) char [definedInLength];
    if (!S.empty())
      std::memcpy(this->definedIn, S.data(), definedInLength);
  }

  bool getGeneratedDeclaration() const {
    return generatedDeclaration;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::ExternalSourceSymbol; }
};

class FallThroughAttr : public StmtAttr {
public:
  static FallThroughAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) FallThroughAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  FallThroughAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : StmtAttr(attr::FallThrough, R, SI, false, false)
  {
  }

  FallThroughAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::FallThrough; }
};

class FastCallAttr : public InheritableAttr {
public:
  static FastCallAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) FastCallAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  FastCallAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::FastCall, R, SI, false, false)
  {
  }

  FastCallAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::FastCall; }
};

class FinalAttr : public InheritableAttr {
public:
  enum Spelling {
    Keyword_final = 0,
    Keyword_sealed = 1
  };

  static FinalAttr *CreateImplicit(ASTContext &Ctx, Spelling S, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) FinalAttr(Loc, Ctx, S);
    A->setImplicit(true);
    return A;
  }

  FinalAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Final, R, SI, false, false)
  {
  }

  FinalAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return Keyword_final;
    case 1: return Keyword_sealed;
  }
  }
  bool isSpelledAsSealed() const { return SpellingListIndex == 1; }


  static bool classof(const Attr *A) { return A->getKind() == attr::Final; }
};

class FlagEnumAttr : public InheritableAttr {
public:
  static FlagEnumAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) FlagEnumAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  FlagEnumAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::FlagEnum, R, SI, false, false)
  {
  }

  FlagEnumAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::FlagEnum; }
};

class FlattenAttr : public InheritableAttr {
public:
  static FlattenAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) FlattenAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  FlattenAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Flatten, R, SI, false, false)
  {
  }

  FlattenAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Flatten; }
};

class FormatAttr : public InheritableAttr {
IdentifierInfo * type;

int formatIdx;

int firstArg;

public:
  static FormatAttr *CreateImplicit(ASTContext &Ctx, IdentifierInfo * Type, int FormatIdx, int FirstArg, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) FormatAttr(Loc, Ctx, Type, FormatIdx, FirstArg, 0);
    A->setImplicit(true);
    return A;
  }

  FormatAttr(SourceRange R, ASTContext &Ctx
              , IdentifierInfo * Type
              , int FormatIdx
              , int FirstArg
              , unsigned SI
             )
    : InheritableAttr(attr::Format, R, SI, false, false)
              , type(Type)
              , formatIdx(FormatIdx)
              , firstArg(FirstArg)
  {
  }

  FormatAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  IdentifierInfo * getType() const {
    return type;
  }

  int getFormatIdx() const {
    return formatIdx;
  }

  int getFirstArg() const {
    return firstArg;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::Format; }
};

class FormatArgAttr : public InheritableAttr {
int formatIdx;

public:
  static FormatArgAttr *CreateImplicit(ASTContext &Ctx, int FormatIdx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) FormatArgAttr(Loc, Ctx, FormatIdx, 0);
    A->setImplicit(true);
    return A;
  }

  FormatArgAttr(SourceRange R, ASTContext &Ctx
              , int FormatIdx
              , unsigned SI
             )
    : InheritableAttr(attr::FormatArg, R, SI, false, false)
              , formatIdx(FormatIdx)
  {
  }

  FormatArgAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  int getFormatIdx() const {
    return formatIdx;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::FormatArg; }
};

class GNUInlineAttr : public InheritableAttr {
public:
  static GNUInlineAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) GNUInlineAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  GNUInlineAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::GNUInline, R, SI, false, false)
  {
  }

  GNUInlineAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::GNUInline; }
};

class GuardedByAttr : public InheritableAttr {
Expr * arg;

public:
  static GuardedByAttr *CreateImplicit(ASTContext &Ctx, Expr * Arg, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) GuardedByAttr(Loc, Ctx, Arg, 0);
    A->setImplicit(true);
    return A;
  }

  GuardedByAttr(SourceRange R, ASTContext &Ctx
              , Expr * Arg
              , unsigned SI
             )
    : InheritableAttr(attr::GuardedBy, R, SI, true, true)
              , arg(Arg)
  {
  }

  GuardedByAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Expr * getArg() const {
    return arg;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::GuardedBy; }
};

class GuardedVarAttr : public InheritableAttr {
public:
  static GuardedVarAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) GuardedVarAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  GuardedVarAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::GuardedVar, R, SI, false, false)
  {
  }

  GuardedVarAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::GuardedVar; }
};

class HotAttr : public InheritableAttr {
public:
  static HotAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) HotAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  HotAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Hot, R, SI, false, false)
  {
  }

  HotAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Hot; }
};

class IBActionAttr : public InheritableAttr {
public:
  static IBActionAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) IBActionAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  IBActionAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::IBAction, R, SI, false, false)
  {
  }

  IBActionAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::IBAction; }
};

class IBOutletAttr : public InheritableAttr {
public:
  static IBOutletAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) IBOutletAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  IBOutletAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::IBOutlet, R, SI, false, false)
  {
  }

  IBOutletAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::IBOutlet; }
};

class IBOutletCollectionAttr : public InheritableAttr {
TypeSourceInfo * interface_;

public:
  static IBOutletCollectionAttr *CreateImplicit(ASTContext &Ctx, TypeSourceInfo * Interface, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) IBOutletCollectionAttr(Loc, Ctx, Interface, 0);
    A->setImplicit(true);
    return A;
  }

  IBOutletCollectionAttr(SourceRange R, ASTContext &Ctx
              , TypeSourceInfo * Interface
              , unsigned SI
             )
    : InheritableAttr(attr::IBOutletCollection, R, SI, false, false)
              , interface_(Interface)
  {
  }

  IBOutletCollectionAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::IBOutletCollection, R, SI, false, false)
              , interface_()
  {
  }

  IBOutletCollectionAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  QualType getInterface() const {
    return interface_->getType();
  }  TypeSourceInfo * getInterfaceLoc() const {
    return interface_;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::IBOutletCollection; }
};

class IFuncAttr : public Attr {
unsigned resolverLength;
char *resolver;

public:
  static IFuncAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Resolver, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) IFuncAttr(Loc, Ctx, Resolver, 0);
    A->setImplicit(true);
    return A;
  }

  IFuncAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Resolver
              , unsigned SI
             )
    : Attr(attr::IFunc, R, SI, false, false)
              , resolverLength(Resolver.size()),resolver(new (Ctx, 1) char[resolverLength])
  {
      if (!Resolver.empty())
        std::memcpy(resolver, Resolver.data(), resolverLength);
  }

  IFuncAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getResolver() const {
    return llvm::StringRef(resolver, resolverLength);
  }
  unsigned getResolverLength() const {
    return resolverLength;
  }
  void setResolver(ASTContext &C, llvm::StringRef S) {
    resolverLength = S.size();
    this->resolver = new (C, 1) char [resolverLength];
    if (!S.empty())
      std::memcpy(this->resolver, S.data(), resolverLength);
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::IFunc; }
};

class InitPriorityAttr : public InheritableAttr {
unsigned priority;

public:
  static InitPriorityAttr *CreateImplicit(ASTContext &Ctx, unsigned Priority, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) InitPriorityAttr(Loc, Ctx, Priority, 0);
    A->setImplicit(true);
    return A;
  }

  InitPriorityAttr(SourceRange R, ASTContext &Ctx
              , unsigned Priority
              , unsigned SI
             )
    : InheritableAttr(attr::InitPriority, R, SI, false, false)
              , priority(Priority)
  {
  }

  InitPriorityAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getPriority() const {
    return priority;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::InitPriority; }
};

class InitSegAttr : public Attr {
unsigned sectionLength;
char *section;

public:
  static InitSegAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Section, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) InitSegAttr(Loc, Ctx, Section, 0);
    A->setImplicit(true);
    return A;
  }

  InitSegAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Section
              , unsigned SI
             )
    : Attr(attr::InitSeg, R, SI, false, false)
              , sectionLength(Section.size()),section(new (Ctx, 1) char[sectionLength])
  {
      if (!Section.empty())
        std::memcpy(section, Section.data(), sectionLength);
  }

  InitSegAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getSection() const {
    return llvm::StringRef(section, sectionLength);
  }
  unsigned getSectionLength() const {
    return sectionLength;
  }
  void setSection(ASTContext &C, llvm::StringRef S) {
    sectionLength = S.size();
    this->section = new (C, 1) char [sectionLength];
    if (!S.empty())
      std::memcpy(this->section, S.data(), sectionLength);
  }


  void printPrettyPragma(raw_ostream &OS, const PrintingPolicy &Policy) const {
    OS << '(' << getSection() << ')';
  }
  

  static bool classof(const Attr *A) { return A->getKind() == attr::InitSeg; }
};

class IntelOclBiccAttr : public InheritableAttr {
public:
  static IntelOclBiccAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) IntelOclBiccAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  IntelOclBiccAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::IntelOclBicc, R, SI, false, false)
  {
  }

  IntelOclBiccAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::IntelOclBicc; }
};

class InternalLinkageAttr : public InheritableAttr {
public:
  static InternalLinkageAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) InternalLinkageAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  InternalLinkageAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::InternalLinkage, R, SI, false, false)
  {
  }

  InternalLinkageAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::InternalLinkage; }
};

class LTOVisibilityPublicAttr : public InheritableAttr {
public:
  static LTOVisibilityPublicAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) LTOVisibilityPublicAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  LTOVisibilityPublicAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::LTOVisibilityPublic, R, SI, false, false)
  {
  }

  LTOVisibilityPublicAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::LTOVisibilityPublic; }
};

class LayoutVersionAttr : public InheritableAttr {
unsigned version;

public:
  static LayoutVersionAttr *CreateImplicit(ASTContext &Ctx, unsigned Version, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) LayoutVersionAttr(Loc, Ctx, Version, 0);
    A->setImplicit(true);
    return A;
  }

  LayoutVersionAttr(SourceRange R, ASTContext &Ctx
              , unsigned Version
              , unsigned SI
             )
    : InheritableAttr(attr::LayoutVersion, R, SI, false, false)
              , version(Version)
  {
  }

  LayoutVersionAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getVersion() const {
    return version;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::LayoutVersion; }
};

class LockReturnedAttr : public InheritableAttr {
Expr * arg;

public:
  static LockReturnedAttr *CreateImplicit(ASTContext &Ctx, Expr * Arg, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) LockReturnedAttr(Loc, Ctx, Arg, 0);
    A->setImplicit(true);
    return A;
  }

  LockReturnedAttr(SourceRange R, ASTContext &Ctx
              , Expr * Arg
              , unsigned SI
             )
    : InheritableAttr(attr::LockReturned, R, SI, true, false)
              , arg(Arg)
  {
  }

  LockReturnedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Expr * getArg() const {
    return arg;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::LockReturned; }
};

class LocksExcludedAttr : public InheritableAttr {
  unsigned args_Size;
  Expr * *args_;

public:
  static LocksExcludedAttr *CreateImplicit(ASTContext &Ctx, Expr * *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) LocksExcludedAttr(Loc, Ctx, Args, ArgsSize, 0);
    A->setImplicit(true);
    return A;
  }

  LocksExcludedAttr(SourceRange R, ASTContext &Ctx
              , Expr * *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::LocksExcluded, R, SI, true, true)
              , args_Size(ArgsSize), args_(new (Ctx, 16) Expr *[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  LocksExcludedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::LocksExcluded, R, SI, true, true)
              , args_Size(0), args_(nullptr)
  {
  }

  LocksExcludedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  typedef Expr ** args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::LocksExcluded; }
};

class LoopHintAttr : public Attr {
public:
  enum OptionType {
    Vectorize,
    VectorizeWidth,
    Interleave,
    InterleaveCount,
    Unroll,
    UnrollCount,
    Distribute
  };
private:
  OptionType option;

public:
  enum LoopHintState {
    Enable,
    Disable,
    Numeric,
    AssumeSafety,
    Full
  };
private:
  LoopHintState state;

Expr * value;

public:
  enum Spelling {
    Pragma_clang_loop = 0,
    Pragma_unroll = 1,
    Pragma_nounroll = 2
  };

  static LoopHintAttr *CreateImplicit(ASTContext &Ctx, Spelling S, OptionType Option, LoopHintState State, Expr * Value, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) LoopHintAttr(Loc, Ctx, Option, State, Value, S);
    A->setImplicit(true);
    return A;
  }

  LoopHintAttr(SourceRange R, ASTContext &Ctx
              , OptionType Option
              , LoopHintState State
              , Expr * Value
              , unsigned SI
             )
    : Attr(attr::LoopHint, R, SI, false, false)
              , option(Option)
              , state(State)
              , value(Value)
  {
  }

  LoopHintAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return Pragma_clang_loop;
    case 1: return Pragma_unroll;
    case 2: return Pragma_nounroll;
  }
  }
  OptionType getOption() const {
    return option;
  }

  static bool ConvertStrToOptionType(StringRef Val, OptionType &Out) {
    Optional<OptionType> R = llvm::StringSwitch<Optional<OptionType>>(Val)
      .Case("vectorize", LoopHintAttr::Vectorize)
      .Case("vectorize_width", LoopHintAttr::VectorizeWidth)
      .Case("interleave", LoopHintAttr::Interleave)
      .Case("interleave_count", LoopHintAttr::InterleaveCount)
      .Case("unroll", LoopHintAttr::Unroll)
      .Case("unroll_count", LoopHintAttr::UnrollCount)
      .Case("distribute", LoopHintAttr::Distribute)
      .Default(Optional<OptionType>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertOptionTypeToStr(OptionType Val) {
    switch(Val) {
    case LoopHintAttr::Vectorize: return "vectorize";
    case LoopHintAttr::VectorizeWidth: return "vectorize_width";
    case LoopHintAttr::Interleave: return "interleave";
    case LoopHintAttr::InterleaveCount: return "interleave_count";
    case LoopHintAttr::Unroll: return "unroll";
    case LoopHintAttr::UnrollCount: return "unroll_count";
    case LoopHintAttr::Distribute: return "distribute";
    }
    llvm_unreachable("No enumerator with that value");
  }
  LoopHintState getState() const {
    return state;
  }

  static bool ConvertStrToLoopHintState(StringRef Val, LoopHintState &Out) {
    Optional<LoopHintState> R = llvm::StringSwitch<Optional<LoopHintState>>(Val)
      .Case("enable", LoopHintAttr::Enable)
      .Case("disable", LoopHintAttr::Disable)
      .Case("numeric", LoopHintAttr::Numeric)
      .Case("assume_safety", LoopHintAttr::AssumeSafety)
      .Case("full", LoopHintAttr::Full)
      .Default(Optional<LoopHintState>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertLoopHintStateToStr(LoopHintState Val) {
    switch(Val) {
    case LoopHintAttr::Enable: return "enable";
    case LoopHintAttr::Disable: return "disable";
    case LoopHintAttr::Numeric: return "numeric";
    case LoopHintAttr::AssumeSafety: return "assume_safety";
    case LoopHintAttr::Full: return "full";
    }
    llvm_unreachable("No enumerator with that value");
  }
  Expr * getValue() const {
    return value;
  }


  static const char *getOptionName(int Option) {
    switch(Option) {
    case Vectorize: return "vectorize";
    case VectorizeWidth: return "vectorize_width";
    case Interleave: return "interleave";
    case InterleaveCount: return "interleave_count";
    case Unroll: return "unroll";
    case UnrollCount: return "unroll_count";
    case Distribute: return "distribute";
    }
    llvm_unreachable("Unhandled LoopHint option.");
  }

  void printPrettyPragma(raw_ostream &OS, const PrintingPolicy &Policy) const {
    unsigned SpellingIndex = getSpellingListIndex();
    // For "#pragma unroll" and "#pragma nounroll" the string "unroll" or
    // "nounroll" is already emitted as the pragma name.
    if (SpellingIndex == Pragma_nounroll)
      return;
    else if (SpellingIndex == Pragma_unroll) {
      OS << getValueString(Policy);
      return;
    }

    assert(SpellingIndex == Pragma_clang_loop && "Unexpected spelling");
    OS << getOptionName(option) << getValueString(Policy);
  }

  // Return a string containing the loop hint argument including the
  // enclosing parentheses.
  std::string getValueString(const PrintingPolicy &Policy) const {
    std::string ValueName;
    llvm::raw_string_ostream OS(ValueName);
    OS << "(";
    if (state == Numeric)
      value->printPretty(OS, nullptr, Policy);
    else if (state == Enable)
      OS << "enable";
    else if (state == Full)
      OS << "full";
    else if (state == AssumeSafety)
      OS << "assume_safety";
    else
      OS << "disable";
    OS << ")";
    return OS.str();
  }

  // Return a string suitable for identifying this attribute in diagnostics.
  std::string getDiagnosticName(const PrintingPolicy &Policy) const {
    unsigned SpellingIndex = getSpellingListIndex();
    if (SpellingIndex == Pragma_nounroll)
      return "#pragma nounroll";
    else if (SpellingIndex == Pragma_unroll)
      return "#pragma unroll" + (option == UnrollCount ? getValueString(Policy) : "");

    assert(SpellingIndex == Pragma_clang_loop && "Unexpected spelling");
    return getOptionName(option) + getValueString(Policy);
  }
  

  static bool classof(const Attr *A) { return A->getKind() == attr::LoopHint; }
};

class MSABIAttr : public InheritableAttr {
public:
  static MSABIAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) MSABIAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  MSABIAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::MSABI, R, SI, false, false)
  {
  }

  MSABIAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::MSABI; }
};

class MSInheritanceAttr : public InheritableAttr {
bool bestCase;

public:
  enum Spelling {
    Keyword_single_inheritance = 0,
    Keyword_multiple_inheritance = 1,
    Keyword_virtual_inheritance = 2,
    Keyword_unspecified_inheritance = 3
  };

  static MSInheritanceAttr *CreateImplicit(ASTContext &Ctx, Spelling S, bool BestCase, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) MSInheritanceAttr(Loc, Ctx, BestCase, S);
    A->setImplicit(true);
    return A;
  }

  MSInheritanceAttr(SourceRange R, ASTContext &Ctx
              , bool BestCase
              , unsigned SI
             )
    : InheritableAttr(attr::MSInheritance, R, SI, false, false)
              , bestCase(BestCase)
  {
  }

  MSInheritanceAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::MSInheritance, R, SI, false, false)
              , bestCase()
  {
  }

  MSInheritanceAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return Keyword_single_inheritance;
    case 1: return Keyword_multiple_inheritance;
    case 2: return Keyword_virtual_inheritance;
    case 3: return Keyword_unspecified_inheritance;
  }
  }
  bool getBestCase() const {
    return bestCase;
  }

  static const bool DefaultBestCase = true;


  static bool hasVBPtrOffsetField(Spelling Inheritance) {
    return Inheritance == Keyword_unspecified_inheritance;
  }

  // Only member pointers to functions need a this adjustment, since it can be
  // combined with the field offset for data pointers.
  static bool hasNVOffsetField(bool IsMemberFunction, Spelling Inheritance) {
    return IsMemberFunction && Inheritance >= Keyword_multiple_inheritance;
  }

  static bool hasVBTableOffsetField(Spelling Inheritance) {
    return Inheritance >= Keyword_virtual_inheritance;
  }

  static bool hasOnlyOneField(bool IsMemberFunction,
                              Spelling Inheritance) {
    if (IsMemberFunction)
      return Inheritance <= Keyword_single_inheritance;
    return Inheritance <= Keyword_multiple_inheritance;
  }
  

  static bool classof(const Attr *A) { return A->getKind() == attr::MSInheritance; }
};

class MSNoVTableAttr : public InheritableAttr {
public:
  static MSNoVTableAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) MSNoVTableAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  MSNoVTableAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::MSNoVTable, R, SI, false, false)
  {
  }

  MSNoVTableAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::MSNoVTable; }
};

class MSP430InterruptAttr : public InheritableAttr {
unsigned number;

public:
  static MSP430InterruptAttr *CreateImplicit(ASTContext &Ctx, unsigned Number, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) MSP430InterruptAttr(Loc, Ctx, Number, 0);
    A->setImplicit(true);
    return A;
  }

  MSP430InterruptAttr(SourceRange R, ASTContext &Ctx
              , unsigned Number
              , unsigned SI
             )
    : InheritableAttr(attr::MSP430Interrupt, R, SI, false, false)
              , number(Number)
  {
  }

  MSP430InterruptAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getNumber() const {
    return number;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::MSP430Interrupt; }
};

class MSStructAttr : public InheritableAttr {
public:
  static MSStructAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) MSStructAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  MSStructAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::MSStruct, R, SI, false, false)
  {
  }

  MSStructAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::MSStruct; }
};

class MSVtorDispAttr : public InheritableAttr {
unsigned vdm;

public:
  static MSVtorDispAttr *CreateImplicit(ASTContext &Ctx, unsigned Vdm, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) MSVtorDispAttr(Loc, Ctx, Vdm, 0);
    A->setImplicit(true);
    return A;
  }

  MSVtorDispAttr(SourceRange R, ASTContext &Ctx
              , unsigned Vdm
              , unsigned SI
             )
    : InheritableAttr(attr::MSVtorDisp, R, SI, false, false)
              , vdm(Vdm)
  {
  }

  MSVtorDispAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getVdm() const {
    return vdm;
  }


  enum Mode {
    Never,
    ForVBaseOverride,
    ForVFTable
  };

  Mode getVtorDispMode() const { return Mode(vdm); }
  

  static bool classof(const Attr *A) { return A->getKind() == attr::MSVtorDisp; }
};

class MaxFieldAlignmentAttr : public InheritableAttr {
unsigned alignment;

public:
  static MaxFieldAlignmentAttr *CreateImplicit(ASTContext &Ctx, unsigned Alignment, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) MaxFieldAlignmentAttr(Loc, Ctx, Alignment, 0);
    A->setImplicit(true);
    return A;
  }

  MaxFieldAlignmentAttr(SourceRange R, ASTContext &Ctx
              , unsigned Alignment
              , unsigned SI
             )
    : InheritableAttr(attr::MaxFieldAlignment, R, SI, false, false)
              , alignment(Alignment)
  {
  }

  MaxFieldAlignmentAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getAlignment() const {
    return alignment;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::MaxFieldAlignment; }
};

class MayAliasAttr : public InheritableAttr {
public:
  static MayAliasAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) MayAliasAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  MayAliasAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::MayAlias, R, SI, false, false)
  {
  }

  MayAliasAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::MayAlias; }
};

class MinSizeAttr : public InheritableAttr {
public:
  static MinSizeAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) MinSizeAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  MinSizeAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::MinSize, R, SI, false, false)
  {
  }

  MinSizeAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::MinSize; }
};

class Mips16Attr : public InheritableAttr {
public:
  static Mips16Attr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) Mips16Attr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  Mips16Attr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Mips16, R, SI, false, false)
  {
  }

  Mips16Attr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Mips16; }
};

class MipsInterruptAttr : public InheritableAttr {
public:
  enum InterruptType {
    sw0,
    sw1,
    hw0,
    hw1,
    hw2,
    hw3,
    hw4,
    hw5,
    eic
  };
private:
  InterruptType interrupt;

public:
  static MipsInterruptAttr *CreateImplicit(ASTContext &Ctx, InterruptType Interrupt, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) MipsInterruptAttr(Loc, Ctx, Interrupt, 0);
    A->setImplicit(true);
    return A;
  }

  MipsInterruptAttr(SourceRange R, ASTContext &Ctx
              , InterruptType Interrupt
              , unsigned SI
             )
    : InheritableAttr(attr::MipsInterrupt, R, SI, false, false)
              , interrupt(Interrupt)
  {
  }

  MipsInterruptAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  InterruptType getInterrupt() const {
    return interrupt;
  }

  static bool ConvertStrToInterruptType(StringRef Val, InterruptType &Out) {
    Optional<InterruptType> R = llvm::StringSwitch<Optional<InterruptType>>(Val)
      .Case("vector=sw0", MipsInterruptAttr::sw0)
      .Case("vector=sw1", MipsInterruptAttr::sw1)
      .Case("vector=hw0", MipsInterruptAttr::hw0)
      .Case("vector=hw1", MipsInterruptAttr::hw1)
      .Case("vector=hw2", MipsInterruptAttr::hw2)
      .Case("vector=hw3", MipsInterruptAttr::hw3)
      .Case("vector=hw4", MipsInterruptAttr::hw4)
      .Case("vector=hw5", MipsInterruptAttr::hw5)
      .Case("eic", MipsInterruptAttr::eic)
      .Case("", MipsInterruptAttr::eic)
      .Default(Optional<InterruptType>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertInterruptTypeToStr(InterruptType Val) {
    switch(Val) {
    case MipsInterruptAttr::sw0: return "vector=sw0";
    case MipsInterruptAttr::sw1: return "vector=sw1";
    case MipsInterruptAttr::hw0: return "vector=hw0";
    case MipsInterruptAttr::hw1: return "vector=hw1";
    case MipsInterruptAttr::hw2: return "vector=hw2";
    case MipsInterruptAttr::hw3: return "vector=hw3";
    case MipsInterruptAttr::hw4: return "vector=hw4";
    case MipsInterruptAttr::hw5: return "vector=hw5";
    case MipsInterruptAttr::eic: return "eic";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::MipsInterrupt; }
};

class ModeAttr : public Attr {
IdentifierInfo * mode;

public:
  static ModeAttr *CreateImplicit(ASTContext &Ctx, IdentifierInfo * Mode, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ModeAttr(Loc, Ctx, Mode, 0);
    A->setImplicit(true);
    return A;
  }

  ModeAttr(SourceRange R, ASTContext &Ctx
              , IdentifierInfo * Mode
              , unsigned SI
             )
    : Attr(attr::Mode, R, SI, false, false)
              , mode(Mode)
  {
  }

  ModeAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  IdentifierInfo * getMode() const {
    return mode;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::Mode; }
};

class NSConsumedAttr : public InheritableParamAttr {
public:
  static NSConsumedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NSConsumedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NSConsumedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableParamAttr(attr::NSConsumed, R, SI, false, false)
  {
  }

  NSConsumedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NSConsumed; }
};

class NSConsumesSelfAttr : public InheritableAttr {
public:
  static NSConsumesSelfAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NSConsumesSelfAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NSConsumesSelfAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NSConsumesSelf, R, SI, false, false)
  {
  }

  NSConsumesSelfAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NSConsumesSelf; }
};

class NSReturnsAutoreleasedAttr : public InheritableAttr {
public:
  static NSReturnsAutoreleasedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NSReturnsAutoreleasedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NSReturnsAutoreleasedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NSReturnsAutoreleased, R, SI, false, false)
  {
  }

  NSReturnsAutoreleasedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NSReturnsAutoreleased; }
};

class NSReturnsNotRetainedAttr : public InheritableAttr {
public:
  static NSReturnsNotRetainedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NSReturnsNotRetainedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NSReturnsNotRetainedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NSReturnsNotRetained, R, SI, false, false)
  {
  }

  NSReturnsNotRetainedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NSReturnsNotRetained; }
};

class NSReturnsRetainedAttr : public InheritableAttr {
public:
  static NSReturnsRetainedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NSReturnsRetainedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NSReturnsRetainedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NSReturnsRetained, R, SI, false, false)
  {
  }

  NSReturnsRetainedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NSReturnsRetained; }
};

class NakedAttr : public InheritableAttr {
public:
  static NakedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NakedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NakedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Naked, R, SI, false, false)
  {
  }

  NakedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Naked; }
};

class NoAliasAttr : public InheritableAttr {
public:
  static NoAliasAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoAliasAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NoAliasAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoAlias, R, SI, false, false)
  {
  }

  NoAliasAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NoAlias; }
};

class NoCommonAttr : public InheritableAttr {
public:
  static NoCommonAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoCommonAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NoCommonAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoCommon, R, SI, false, false)
  {
  }

  NoCommonAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NoCommon; }
};

class NoDebugAttr : public InheritableAttr {
public:
  static NoDebugAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoDebugAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NoDebugAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoDebug, R, SI, false, false)
  {
  }

  NoDebugAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NoDebug; }
};

class NoDuplicateAttr : public InheritableAttr {
public:
  static NoDuplicateAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoDuplicateAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NoDuplicateAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoDuplicate, R, SI, false, false)
  {
  }

  NoDuplicateAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NoDuplicate; }
};

class NoInlineAttr : public InheritableAttr {
public:
  static NoInlineAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoInlineAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NoInlineAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoInline, R, SI, false, false)
  {
  }

  NoInlineAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NoInline; }
};

class NoInstrumentFunctionAttr : public InheritableAttr {
public:
  static NoInstrumentFunctionAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoInstrumentFunctionAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NoInstrumentFunctionAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoInstrumentFunction, R, SI, false, false)
  {
  }

  NoInstrumentFunctionAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NoInstrumentFunction; }
};

class NoMips16Attr : public InheritableAttr {
public:
  static NoMips16Attr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoMips16Attr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NoMips16Attr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoMips16, R, SI, false, false)
  {
  }

  NoMips16Attr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NoMips16; }
};

class NoReturnAttr : public InheritableAttr {
public:
  static NoReturnAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoReturnAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NoReturnAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoReturn, R, SI, false, false)
  {
  }

  NoReturnAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NoReturn; }
};

class NoSanitizeAttr : public InheritableAttr {
  unsigned sanitizers_Size;
  StringRef *sanitizers_;

public:
  static NoSanitizeAttr *CreateImplicit(ASTContext &Ctx, StringRef *Sanitizers, unsigned SanitizersSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoSanitizeAttr(Loc, Ctx, Sanitizers, SanitizersSize, 0);
    A->setImplicit(true);
    return A;
  }

  NoSanitizeAttr(SourceRange R, ASTContext &Ctx
              , StringRef *Sanitizers, unsigned SanitizersSize
              , unsigned SI
             )
    : InheritableAttr(attr::NoSanitize, R, SI, false, false)
              , sanitizers_Size(SanitizersSize), sanitizers_(new (Ctx, 16) StringRef[sanitizers_Size])
  {
    for (size_t I = 0, E = sanitizers_Size; I != E;
         ++I) {
      StringRef Ref = Sanitizers[I];
      if (!Ref.empty()) {
        char *Mem = new (Ctx, 1) char[Ref.size()];
        std::memcpy(Mem, Ref.data(), Ref.size());
        sanitizers_[I] = StringRef(Mem, Ref.size());
      }
    }
  }

  NoSanitizeAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoSanitize, R, SI, false, false)
              , sanitizers_Size(0), sanitizers_(nullptr)
  {
  }

  NoSanitizeAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  typedef StringRef* sanitizers_iterator;
  sanitizers_iterator sanitizers_begin() const { return sanitizers_; }
  sanitizers_iterator sanitizers_end() const { return sanitizers_ + sanitizers_Size; }
  unsigned sanitizers_size() const { return sanitizers_Size; }
  llvm::iterator_range<sanitizers_iterator> sanitizers() const { return llvm::make_range(sanitizers_begin(), sanitizers_end()); }



    SanitizerMask getMask() const {
      SanitizerMask Mask = 0;
      for (auto SanitizerName : sanitizers()) {
        SanitizerMask ParsedMask =
            parseSanitizerValue(SanitizerName, /*AllowGroups=*/true);
        Mask |= expandSanitizerGroups(ParsedMask);
      }
      return Mask;
    }
  

  static bool classof(const Attr *A) { return A->getKind() == attr::NoSanitize; }
};

class NoSplitStackAttr : public InheritableAttr {
public:
  static NoSplitStackAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoSplitStackAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NoSplitStackAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoSplitStack, R, SI, false, false)
  {
  }

  NoSplitStackAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NoSplitStack; }
};

class NoThreadSafetyAnalysisAttr : public InheritableAttr {
public:
  static NoThreadSafetyAnalysisAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoThreadSafetyAnalysisAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NoThreadSafetyAnalysisAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoThreadSafetyAnalysis, R, SI, false, false)
  {
  }

  NoThreadSafetyAnalysisAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NoThreadSafetyAnalysis; }
};

class NoThrowAttr : public InheritableAttr {
public:
  static NoThrowAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NoThrowAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NoThrowAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NoThrow, R, SI, false, false)
  {
  }

  NoThrowAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NoThrow; }
};

class NonNullAttr : public InheritableParamAttr {
  unsigned args_Size;
  unsigned *args_;

public:
  static NonNullAttr *CreateImplicit(ASTContext &Ctx, unsigned *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NonNullAttr(Loc, Ctx, Args, ArgsSize, 0);
    A->setImplicit(true);
    return A;
  }

  NonNullAttr(SourceRange R, ASTContext &Ctx
              , unsigned *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableParamAttr(attr::NonNull, R, SI, false, true)
              , args_Size(ArgsSize), args_(new (Ctx, 16) unsigned[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  NonNullAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableParamAttr(attr::NonNull, R, SI, false, true)
              , args_Size(0), args_(nullptr)
  {
  }

  NonNullAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  typedef unsigned* args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }


bool isNonNull(unsigned idx) const {
    if (!args_size())
      return true;
    for (const auto &V : args())
      if (V == idx)
        return true;
    return false;
  } 

  static bool classof(const Attr *A) { return A->getKind() == attr::NonNull; }
};

class NotTailCalledAttr : public InheritableAttr {
public:
  static NotTailCalledAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) NotTailCalledAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  NotTailCalledAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::NotTailCalled, R, SI, false, false)
  {
  }

  NotTailCalledAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::NotTailCalled; }
};

class OMPCaptureNoInitAttr : public InheritableAttr {
public:
  static OMPCaptureNoInitAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) OMPCaptureNoInitAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  OMPCaptureNoInitAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::OMPCaptureNoInit, R, SI, false, false)
  {
  }

  OMPCaptureNoInitAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::OMPCaptureNoInit; }
};

class OMPDeclareSimdDeclAttr : public Attr {
public:
  enum BranchStateTy {
    BS_Undefined,
    BS_Inbranch,
    BS_Notinbranch
  };
private:
  BranchStateTy branchState;

Expr * simdlen;

  unsigned uniforms_Size;
  Expr * *uniforms_;

  unsigned aligneds_Size;
  Expr * *aligneds_;

  unsigned alignments_Size;
  Expr * *alignments_;

  unsigned linears_Size;
  Expr * *linears_;

  unsigned modifiers_Size;
  unsigned *modifiers_;

  unsigned steps_Size;
  Expr * *steps_;

public:
  static OMPDeclareSimdDeclAttr *CreateImplicit(ASTContext &Ctx, BranchStateTy BranchState, Expr * Simdlen, Expr * *Uniforms, unsigned UniformsSize, Expr * *Aligneds, unsigned AlignedsSize, Expr * *Alignments, unsigned AlignmentsSize, Expr * *Linears, unsigned LinearsSize, unsigned *Modifiers, unsigned ModifiersSize, Expr * *Steps, unsigned StepsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) OMPDeclareSimdDeclAttr(Loc, Ctx, BranchState, Simdlen, Uniforms, UniformsSize, Aligneds, AlignedsSize, Alignments, AlignmentsSize, Linears, LinearsSize, Modifiers, ModifiersSize, Steps, StepsSize, 0);
    A->setImplicit(true);
    return A;
  }

  OMPDeclareSimdDeclAttr(SourceRange R, ASTContext &Ctx
              , BranchStateTy BranchState
              , Expr * Simdlen
              , Expr * *Uniforms, unsigned UniformsSize
              , Expr * *Aligneds, unsigned AlignedsSize
              , Expr * *Alignments, unsigned AlignmentsSize
              , Expr * *Linears, unsigned LinearsSize
              , unsigned *Modifiers, unsigned ModifiersSize
              , Expr * *Steps, unsigned StepsSize
              , unsigned SI
             )
    : Attr(attr::OMPDeclareSimdDecl, R, SI, false, false)
              , branchState(BranchState)
              , simdlen(Simdlen)
              , uniforms_Size(UniformsSize), uniforms_(new (Ctx, 16) Expr *[uniforms_Size])
              , aligneds_Size(AlignedsSize), aligneds_(new (Ctx, 16) Expr *[aligneds_Size])
              , alignments_Size(AlignmentsSize), alignments_(new (Ctx, 16) Expr *[alignments_Size])
              , linears_Size(LinearsSize), linears_(new (Ctx, 16) Expr *[linears_Size])
              , modifiers_Size(ModifiersSize), modifiers_(new (Ctx, 16) unsigned[modifiers_Size])
              , steps_Size(StepsSize), steps_(new (Ctx, 16) Expr *[steps_Size])
  {
    std::copy(Uniforms, Uniforms + uniforms_Size, uniforms_);
    std::copy(Aligneds, Aligneds + aligneds_Size, aligneds_);
    std::copy(Alignments, Alignments + alignments_Size, alignments_);
    std::copy(Linears, Linears + linears_Size, linears_);
    std::copy(Modifiers, Modifiers + modifiers_Size, modifiers_);
    std::copy(Steps, Steps + steps_Size, steps_);
  }

  OMPDeclareSimdDeclAttr(SourceRange R, ASTContext &Ctx
              , BranchStateTy BranchState
              , Expr * Simdlen
              , unsigned SI
             )
    : Attr(attr::OMPDeclareSimdDecl, R, SI, false, false)
              , branchState(BranchState)
              , simdlen(Simdlen)
              , uniforms_Size(0), uniforms_(nullptr)
              , aligneds_Size(0), aligneds_(nullptr)
              , alignments_Size(0), alignments_(nullptr)
              , linears_Size(0), linears_(nullptr)
              , modifiers_Size(0), modifiers_(nullptr)
              , steps_Size(0), steps_(nullptr)
  {
  }

  OMPDeclareSimdDeclAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  BranchStateTy getBranchState() const {
    return branchState;
  }

  static bool ConvertStrToBranchStateTy(StringRef Val, BranchStateTy &Out) {
    Optional<BranchStateTy> R = llvm::StringSwitch<Optional<BranchStateTy>>(Val)
      .Case("", OMPDeclareSimdDeclAttr::BS_Undefined)
      .Case("inbranch", OMPDeclareSimdDeclAttr::BS_Inbranch)
      .Case("notinbranch", OMPDeclareSimdDeclAttr::BS_Notinbranch)
      .Default(Optional<BranchStateTy>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertBranchStateTyToStr(BranchStateTy Val) {
    switch(Val) {
    case OMPDeclareSimdDeclAttr::BS_Undefined: return "";
    case OMPDeclareSimdDeclAttr::BS_Inbranch: return "inbranch";
    case OMPDeclareSimdDeclAttr::BS_Notinbranch: return "notinbranch";
    }
    llvm_unreachable("No enumerator with that value");
  }
  Expr * getSimdlen() const {
    return simdlen;
  }

  typedef Expr ** uniforms_iterator;
  uniforms_iterator uniforms_begin() const { return uniforms_; }
  uniforms_iterator uniforms_end() const { return uniforms_ + uniforms_Size; }
  unsigned uniforms_size() const { return uniforms_Size; }
  llvm::iterator_range<uniforms_iterator> uniforms() const { return llvm::make_range(uniforms_begin(), uniforms_end()); }


  typedef Expr ** aligneds_iterator;
  aligneds_iterator aligneds_begin() const { return aligneds_; }
  aligneds_iterator aligneds_end() const { return aligneds_ + aligneds_Size; }
  unsigned aligneds_size() const { return aligneds_Size; }
  llvm::iterator_range<aligneds_iterator> aligneds() const { return llvm::make_range(aligneds_begin(), aligneds_end()); }


  typedef Expr ** alignments_iterator;
  alignments_iterator alignments_begin() const { return alignments_; }
  alignments_iterator alignments_end() const { return alignments_ + alignments_Size; }
  unsigned alignments_size() const { return alignments_Size; }
  llvm::iterator_range<alignments_iterator> alignments() const { return llvm::make_range(alignments_begin(), alignments_end()); }


  typedef Expr ** linears_iterator;
  linears_iterator linears_begin() const { return linears_; }
  linears_iterator linears_end() const { return linears_ + linears_Size; }
  unsigned linears_size() const { return linears_Size; }
  llvm::iterator_range<linears_iterator> linears() const { return llvm::make_range(linears_begin(), linears_end()); }


  typedef unsigned* modifiers_iterator;
  modifiers_iterator modifiers_begin() const { return modifiers_; }
  modifiers_iterator modifiers_end() const { return modifiers_ + modifiers_Size; }
  unsigned modifiers_size() const { return modifiers_Size; }
  llvm::iterator_range<modifiers_iterator> modifiers() const { return llvm::make_range(modifiers_begin(), modifiers_end()); }


  typedef Expr ** steps_iterator;
  steps_iterator steps_begin() const { return steps_; }
  steps_iterator steps_end() const { return steps_ + steps_Size; }
  unsigned steps_size() const { return steps_Size; }
  llvm::iterator_range<steps_iterator> steps() const { return llvm::make_range(steps_begin(), steps_end()); }



    void printPrettyPragma(raw_ostream & OS, const PrintingPolicy &Policy)
        const {
      if (getBranchState() != BS_Undefined)
        OS << ConvertBranchStateTyToStr(getBranchState()) << " ";
      if (auto *E = getSimdlen()) {
        OS << "simdlen(";
        E->printPretty(OS, nullptr, Policy);
        OS << ") ";
      }
      if (uniforms_size() > 0) {
        OS << "uniform";
        StringRef Sep = "(";
        for (auto *E : uniforms()) {
          OS << Sep;
          E->printPretty(OS, nullptr, Policy);
          Sep = ", ";
        }
        OS << ") ";
      }
      alignments_iterator NI = alignments_begin();
      for (auto *E : aligneds()) {
        OS << "aligned(";
        E->printPretty(OS, nullptr, Policy);
        if (*NI) {
          OS << ": ";
          (*NI)->printPretty(OS, nullptr, Policy);
        }
        OS << ") ";
        ++NI;
      }
      steps_iterator I = steps_begin();
      modifiers_iterator MI = modifiers_begin();
      for (auto *E : linears()) {
        OS << "linear(";
        if (*MI != OMPC_LINEAR_unknown)
          OS << getOpenMPSimpleClauseTypeName(OMPC_linear, *MI) << "(";
        E->printPretty(OS, nullptr, Policy);
        if (*MI != OMPC_LINEAR_unknown)
          OS << ")";
        if (*I) {
          OS << ": ";
          (*I)->printPretty(OS, nullptr, Policy);
        }
        OS << ") ";
        ++I;
        ++MI;
      }
    }
  

  static bool classof(const Attr *A) { return A->getKind() == attr::OMPDeclareSimdDecl; }
};

class OMPDeclareTargetDeclAttr : public Attr {
public:
  enum MapTypeTy {
    MT_To,
    MT_Link
  };
private:
  MapTypeTy mapType;

public:
  static OMPDeclareTargetDeclAttr *CreateImplicit(ASTContext &Ctx, MapTypeTy MapType, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) OMPDeclareTargetDeclAttr(Loc, Ctx, MapType, 0);
    A->setImplicit(true);
    return A;
  }

  OMPDeclareTargetDeclAttr(SourceRange R, ASTContext &Ctx
              , MapTypeTy MapType
              , unsigned SI
             )
    : Attr(attr::OMPDeclareTargetDecl, R, SI, false, false)
              , mapType(MapType)
  {
  }

  OMPDeclareTargetDeclAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  MapTypeTy getMapType() const {
    return mapType;
  }

  static bool ConvertStrToMapTypeTy(StringRef Val, MapTypeTy &Out) {
    Optional<MapTypeTy> R = llvm::StringSwitch<Optional<MapTypeTy>>(Val)
      .Case("to", OMPDeclareTargetDeclAttr::MT_To)
      .Case("link", OMPDeclareTargetDeclAttr::MT_Link)
      .Default(Optional<MapTypeTy>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertMapTypeTyToStr(MapTypeTy Val) {
    switch(Val) {
    case OMPDeclareTargetDeclAttr::MT_To: return "to";
    case OMPDeclareTargetDeclAttr::MT_Link: return "link";
    }
    llvm_unreachable("No enumerator with that value");
  }

    void printPrettyPragma(raw_ostream &OS, const PrintingPolicy &Policy) const {
      // Use fake syntax because it is for testing and debugging purpose only.
      if (getMapType() != MT_To)
        OS << ConvertMapTypeTyToStr(getMapType()) << " ";
    }
  

  static bool classof(const Attr *A) { return A->getKind() == attr::OMPDeclareTargetDecl; }
};

class OMPThreadPrivateDeclAttr : public InheritableAttr {
public:
  static OMPThreadPrivateDeclAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) OMPThreadPrivateDeclAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  OMPThreadPrivateDeclAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::OMPThreadPrivateDecl, R, SI, false, false)
  {
  }

  OMPThreadPrivateDeclAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::OMPThreadPrivateDecl; }
};

class ObjCBoxableAttr : public Attr {
public:
  static ObjCBoxableAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCBoxableAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCBoxableAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : Attr(attr::ObjCBoxable, R, SI, false, false)
  {
  }

  ObjCBoxableAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCBoxable; }
};

class ObjCBridgeAttr : public InheritableAttr {
IdentifierInfo * bridgedType;

public:
  static ObjCBridgeAttr *CreateImplicit(ASTContext &Ctx, IdentifierInfo * BridgedType, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCBridgeAttr(Loc, Ctx, BridgedType, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCBridgeAttr(SourceRange R, ASTContext &Ctx
              , IdentifierInfo * BridgedType
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCBridge, R, SI, false, false)
              , bridgedType(BridgedType)
  {
  }

  ObjCBridgeAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  IdentifierInfo * getBridgedType() const {
    return bridgedType;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCBridge; }
};

class ObjCBridgeMutableAttr : public InheritableAttr {
IdentifierInfo * bridgedType;

public:
  static ObjCBridgeMutableAttr *CreateImplicit(ASTContext &Ctx, IdentifierInfo * BridgedType, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCBridgeMutableAttr(Loc, Ctx, BridgedType, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCBridgeMutableAttr(SourceRange R, ASTContext &Ctx
              , IdentifierInfo * BridgedType
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCBridgeMutable, R, SI, false, false)
              , bridgedType(BridgedType)
  {
  }

  ObjCBridgeMutableAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  IdentifierInfo * getBridgedType() const {
    return bridgedType;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCBridgeMutable; }
};

class ObjCBridgeRelatedAttr : public InheritableAttr {
IdentifierInfo * relatedClass;

IdentifierInfo * classMethod;

IdentifierInfo * instanceMethod;

public:
  static ObjCBridgeRelatedAttr *CreateImplicit(ASTContext &Ctx, IdentifierInfo * RelatedClass, IdentifierInfo * ClassMethod, IdentifierInfo * InstanceMethod, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCBridgeRelatedAttr(Loc, Ctx, RelatedClass, ClassMethod, InstanceMethod, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCBridgeRelatedAttr(SourceRange R, ASTContext &Ctx
              , IdentifierInfo * RelatedClass
              , IdentifierInfo * ClassMethod
              , IdentifierInfo * InstanceMethod
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCBridgeRelated, R, SI, false, false)
              , relatedClass(RelatedClass)
              , classMethod(ClassMethod)
              , instanceMethod(InstanceMethod)
  {
  }

  ObjCBridgeRelatedAttr(SourceRange R, ASTContext &Ctx
              , IdentifierInfo * RelatedClass
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCBridgeRelated, R, SI, false, false)
              , relatedClass(RelatedClass)
              , classMethod()
              , instanceMethod()
  {
  }

  ObjCBridgeRelatedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  IdentifierInfo * getRelatedClass() const {
    return relatedClass;
  }

  IdentifierInfo * getClassMethod() const {
    return classMethod;
  }

  IdentifierInfo * getInstanceMethod() const {
    return instanceMethod;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCBridgeRelated; }
};

class ObjCDesignatedInitializerAttr : public Attr {
public:
  static ObjCDesignatedInitializerAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCDesignatedInitializerAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCDesignatedInitializerAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : Attr(attr::ObjCDesignatedInitializer, R, SI, false, false)
  {
  }

  ObjCDesignatedInitializerAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCDesignatedInitializer; }
};

class ObjCExceptionAttr : public InheritableAttr {
public:
  static ObjCExceptionAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCExceptionAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCExceptionAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCException, R, SI, false, false)
  {
  }

  ObjCExceptionAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCException; }
};

class ObjCExplicitProtocolImplAttr : public InheritableAttr {
public:
  static ObjCExplicitProtocolImplAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCExplicitProtocolImplAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCExplicitProtocolImplAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCExplicitProtocolImpl, R, SI, false, false)
  {
  }

  ObjCExplicitProtocolImplAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCExplicitProtocolImpl; }
};

class ObjCIndependentClassAttr : public InheritableAttr {
public:
  static ObjCIndependentClassAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCIndependentClassAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCIndependentClassAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCIndependentClass, R, SI, false, false)
  {
  }

  ObjCIndependentClassAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCIndependentClass; }
};

class ObjCMethodFamilyAttr : public InheritableAttr {
public:
  enum FamilyKind {
    OMF_None,
    OMF_alloc,
    OMF_copy,
    OMF_init,
    OMF_mutableCopy,
    OMF_new
  };
private:
  FamilyKind family;

public:
  static ObjCMethodFamilyAttr *CreateImplicit(ASTContext &Ctx, FamilyKind Family, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCMethodFamilyAttr(Loc, Ctx, Family, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCMethodFamilyAttr(SourceRange R, ASTContext &Ctx
              , FamilyKind Family
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCMethodFamily, R, SI, false, false)
              , family(Family)
  {
  }

  ObjCMethodFamilyAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  FamilyKind getFamily() const {
    return family;
  }

  static bool ConvertStrToFamilyKind(StringRef Val, FamilyKind &Out) {
    Optional<FamilyKind> R = llvm::StringSwitch<Optional<FamilyKind>>(Val)
      .Case("none", ObjCMethodFamilyAttr::OMF_None)
      .Case("alloc", ObjCMethodFamilyAttr::OMF_alloc)
      .Case("copy", ObjCMethodFamilyAttr::OMF_copy)
      .Case("init", ObjCMethodFamilyAttr::OMF_init)
      .Case("mutableCopy", ObjCMethodFamilyAttr::OMF_mutableCopy)
      .Case("new", ObjCMethodFamilyAttr::OMF_new)
      .Default(Optional<FamilyKind>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertFamilyKindToStr(FamilyKind Val) {
    switch(Val) {
    case ObjCMethodFamilyAttr::OMF_None: return "none";
    case ObjCMethodFamilyAttr::OMF_alloc: return "alloc";
    case ObjCMethodFamilyAttr::OMF_copy: return "copy";
    case ObjCMethodFamilyAttr::OMF_init: return "init";
    case ObjCMethodFamilyAttr::OMF_mutableCopy: return "mutableCopy";
    case ObjCMethodFamilyAttr::OMF_new: return "new";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCMethodFamily; }
};

class ObjCNSObjectAttr : public InheritableAttr {
public:
  static ObjCNSObjectAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCNSObjectAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCNSObjectAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCNSObject, R, SI, false, false)
  {
  }

  ObjCNSObjectAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCNSObject; }
};

class ObjCPreciseLifetimeAttr : public InheritableAttr {
public:
  static ObjCPreciseLifetimeAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCPreciseLifetimeAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCPreciseLifetimeAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCPreciseLifetime, R, SI, false, false)
  {
  }

  ObjCPreciseLifetimeAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCPreciseLifetime; }
};

class ObjCRequiresPropertyDefsAttr : public InheritableAttr {
public:
  static ObjCRequiresPropertyDefsAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCRequiresPropertyDefsAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCRequiresPropertyDefsAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCRequiresPropertyDefs, R, SI, false, false)
  {
  }

  ObjCRequiresPropertyDefsAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCRequiresPropertyDefs; }
};

class ObjCRequiresSuperAttr : public InheritableAttr {
public:
  static ObjCRequiresSuperAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCRequiresSuperAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCRequiresSuperAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCRequiresSuper, R, SI, false, false)
  {
  }

  ObjCRequiresSuperAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCRequiresSuper; }
};

class ObjCReturnsInnerPointerAttr : public InheritableAttr {
public:
  static ObjCReturnsInnerPointerAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCReturnsInnerPointerAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCReturnsInnerPointerAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCReturnsInnerPointer, R, SI, false, false)
  {
  }

  ObjCReturnsInnerPointerAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCReturnsInnerPointer; }
};

class ObjCRootClassAttr : public InheritableAttr {
public:
  static ObjCRootClassAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCRootClassAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCRootClassAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCRootClass, R, SI, false, false)
  {
  }

  ObjCRootClassAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCRootClass; }
};

class ObjCRuntimeNameAttr : public Attr {
unsigned metadataNameLength;
char *metadataName;

public:
  static ObjCRuntimeNameAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef MetadataName, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCRuntimeNameAttr(Loc, Ctx, MetadataName, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCRuntimeNameAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef MetadataName
              , unsigned SI
             )
    : Attr(attr::ObjCRuntimeName, R, SI, false, false)
              , metadataNameLength(MetadataName.size()),metadataName(new (Ctx, 1) char[metadataNameLength])
  {
      if (!MetadataName.empty())
        std::memcpy(metadataName, MetadataName.data(), metadataNameLength);
  }

  ObjCRuntimeNameAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getMetadataName() const {
    return llvm::StringRef(metadataName, metadataNameLength);
  }
  unsigned getMetadataNameLength() const {
    return metadataNameLength;
  }
  void setMetadataName(ASTContext &C, llvm::StringRef S) {
    metadataNameLength = S.size();
    this->metadataName = new (C, 1) char [metadataNameLength];
    if (!S.empty())
      std::memcpy(this->metadataName, S.data(), metadataNameLength);
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCRuntimeName; }
};

class ObjCRuntimeVisibleAttr : public Attr {
public:
  static ObjCRuntimeVisibleAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCRuntimeVisibleAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCRuntimeVisibleAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : Attr(attr::ObjCRuntimeVisible, R, SI, false, false)
  {
  }

  ObjCRuntimeVisibleAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCRuntimeVisible; }
};

class ObjCSubclassingRestrictedAttr : public InheritableAttr {
public:
  static ObjCSubclassingRestrictedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ObjCSubclassingRestrictedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ObjCSubclassingRestrictedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ObjCSubclassingRestricted, R, SI, false, false)
  {
  }

  ObjCSubclassingRestrictedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ObjCSubclassingRestricted; }
};

class OpenCLAccessAttr : public Attr {
public:
  enum Spelling {
    Keyword_read_only = 0,
    Keyword_write_only = 2,
    Keyword_read_write = 4
  };

  static OpenCLAccessAttr *CreateImplicit(ASTContext &Ctx, Spelling S, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) OpenCLAccessAttr(Loc, Ctx, S);
    A->setImplicit(true);
    return A;
  }

  OpenCLAccessAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : Attr(attr::OpenCLAccess, R, SI, false, false)
  {
  }

  OpenCLAccessAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return Keyword_read_only;
    case 1: return Keyword_read_only;
    case 2: return Keyword_write_only;
    case 3: return Keyword_write_only;
    case 4: return Keyword_read_write;
    case 5: return Keyword_read_write;
  }
  }
  bool isReadOnly() const { return SpellingListIndex == 0 ||
    SpellingListIndex == 1; }
  bool isReadWrite() const { return SpellingListIndex == 4 ||
    SpellingListIndex == 5; }
  bool isWriteOnly() const { return SpellingListIndex == 2 ||
    SpellingListIndex == 3; }


  static bool classof(const Attr *A) { return A->getKind() == attr::OpenCLAccess; }
};

class OpenCLKernelAttr : public InheritableAttr {
public:
  static OpenCLKernelAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) OpenCLKernelAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  OpenCLKernelAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::OpenCLKernel, R, SI, false, false)
  {
  }

  OpenCLKernelAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::OpenCLKernel; }
};

class OpenCLUnrollHintAttr : public InheritableAttr {
unsigned unrollHint;

public:
  static OpenCLUnrollHintAttr *CreateImplicit(ASTContext &Ctx, unsigned UnrollHint, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) OpenCLUnrollHintAttr(Loc, Ctx, UnrollHint, 0);
    A->setImplicit(true);
    return A;
  }

  OpenCLUnrollHintAttr(SourceRange R, ASTContext &Ctx
              , unsigned UnrollHint
              , unsigned SI
             )
    : InheritableAttr(attr::OpenCLUnrollHint, R, SI, false, false)
              , unrollHint(UnrollHint)
  {
  }

  OpenCLUnrollHintAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getUnrollHint() const {
    return unrollHint;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::OpenCLUnrollHint; }
};

class OptimizeNoneAttr : public InheritableAttr {
public:
  static OptimizeNoneAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) OptimizeNoneAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  OptimizeNoneAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::OptimizeNone, R, SI, false, false)
  {
  }

  OptimizeNoneAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::OptimizeNone; }
};

class OverloadableAttr : public Attr {
public:
  static OverloadableAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) OverloadableAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  OverloadableAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : Attr(attr::Overloadable, R, SI, false, false)
  {
  }

  OverloadableAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Overloadable; }
};

class OverrideAttr : public InheritableAttr {
public:
  static OverrideAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) OverrideAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  OverrideAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Override, R, SI, false, false)
  {
  }

  OverrideAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Override; }
};

class OwnershipAttr : public InheritableAttr {
IdentifierInfo * module;

  unsigned args_Size;
  unsigned *args_;

public:
  enum Spelling {
    GNU_ownership_holds = 0,
    GNU_ownership_returns = 1,
    GNU_ownership_takes = 2
  };

  static OwnershipAttr *CreateImplicit(ASTContext &Ctx, Spelling S, IdentifierInfo * Module, unsigned *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) OwnershipAttr(Loc, Ctx, Module, Args, ArgsSize, S);
    A->setImplicit(true);
    return A;
  }

  OwnershipAttr(SourceRange R, ASTContext &Ctx
              , IdentifierInfo * Module
              , unsigned *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::Ownership, R, SI, false, false)
              , module(Module)
              , args_Size(ArgsSize), args_(new (Ctx, 16) unsigned[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  OwnershipAttr(SourceRange R, ASTContext &Ctx
              , IdentifierInfo * Module
              , unsigned SI
             )
    : InheritableAttr(attr::Ownership, R, SI, false, false)
              , module(Module)
              , args_Size(0), args_(nullptr)
  {
  }

  OwnershipAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_ownership_holds;
    case 1: return GNU_ownership_returns;
    case 2: return GNU_ownership_takes;
  }
  }
  bool isHolds() const { return SpellingListIndex == 0; }
  bool isReturns() const { return SpellingListIndex == 1; }
  bool isTakes() const { return SpellingListIndex == 2; }
  IdentifierInfo * getModule() const {
    return module;
  }

  typedef unsigned* args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }



    enum OwnershipKind { Holds, Returns, Takes };
    OwnershipKind getOwnKind() const {
      return isHolds() ? Holds :
             isTakes() ? Takes :
             Returns;
    }
  

  static bool classof(const Attr *A) { return A->getKind() == attr::Ownership; }
};

class PackedAttr : public InheritableAttr {
public:
  static PackedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) PackedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  PackedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Packed, R, SI, false, false)
  {
  }

  PackedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Packed; }
};

class ParamTypestateAttr : public InheritableAttr {
public:
  enum ConsumedState {
    Unknown,
    Consumed,
    Unconsumed
  };
private:
  ConsumedState paramState;

public:
  static ParamTypestateAttr *CreateImplicit(ASTContext &Ctx, ConsumedState ParamState, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ParamTypestateAttr(Loc, Ctx, ParamState, 0);
    A->setImplicit(true);
    return A;
  }

  ParamTypestateAttr(SourceRange R, ASTContext &Ctx
              , ConsumedState ParamState
              , unsigned SI
             )
    : InheritableAttr(attr::ParamTypestate, R, SI, false, false)
              , paramState(ParamState)
  {
  }

  ParamTypestateAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  ConsumedState getParamState() const {
    return paramState;
  }

  static bool ConvertStrToConsumedState(StringRef Val, ConsumedState &Out) {
    Optional<ConsumedState> R = llvm::StringSwitch<Optional<ConsumedState>>(Val)
      .Case("unknown", ParamTypestateAttr::Unknown)
      .Case("consumed", ParamTypestateAttr::Consumed)
      .Case("unconsumed", ParamTypestateAttr::Unconsumed)
      .Default(Optional<ConsumedState>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertConsumedStateToStr(ConsumedState Val) {
    switch(Val) {
    case ParamTypestateAttr::Unknown: return "unknown";
    case ParamTypestateAttr::Consumed: return "consumed";
    case ParamTypestateAttr::Unconsumed: return "unconsumed";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::ParamTypestate; }
};

class PascalAttr : public InheritableAttr {
public:
  static PascalAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) PascalAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  PascalAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Pascal, R, SI, false, false)
  {
  }

  PascalAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Pascal; }
};

class PassObjectSizeAttr : public InheritableParamAttr {
int type;

public:
  static PassObjectSizeAttr *CreateImplicit(ASTContext &Ctx, int Type, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) PassObjectSizeAttr(Loc, Ctx, Type, 0);
    A->setImplicit(true);
    return A;
  }

  PassObjectSizeAttr(SourceRange R, ASTContext &Ctx
              , int Type
              , unsigned SI
             )
    : InheritableParamAttr(attr::PassObjectSize, R, SI, false, false)
              , type(Type)
  {
  }

  PassObjectSizeAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  int getType() const {
    return type;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::PassObjectSize; }
};

class PcsAttr : public InheritableAttr {
public:
  enum PCSType {
    AAPCS,
    AAPCS_VFP
  };
private:
  PCSType pCS;

public:
  static PcsAttr *CreateImplicit(ASTContext &Ctx, PCSType PCS, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) PcsAttr(Loc, Ctx, PCS, 0);
    A->setImplicit(true);
    return A;
  }

  PcsAttr(SourceRange R, ASTContext &Ctx
              , PCSType PCS
              , unsigned SI
             )
    : InheritableAttr(attr::Pcs, R, SI, false, false)
              , pCS(PCS)
  {
  }

  PcsAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  PCSType getPCS() const {
    return pCS;
  }

  static bool ConvertStrToPCSType(StringRef Val, PCSType &Out) {
    Optional<PCSType> R = llvm::StringSwitch<Optional<PCSType>>(Val)
      .Case("aapcs", PcsAttr::AAPCS)
      .Case("aapcs-vfp", PcsAttr::AAPCS_VFP)
      .Default(Optional<PCSType>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertPCSTypeToStr(PCSType Val) {
    switch(Val) {
    case PcsAttr::AAPCS: return "aapcs";
    case PcsAttr::AAPCS_VFP: return "aapcs-vfp";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::Pcs; }
};

class PreserveAllAttr : public InheritableAttr {
public:
  static PreserveAllAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) PreserveAllAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  PreserveAllAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::PreserveAll, R, SI, false, false)
  {
  }

  PreserveAllAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::PreserveAll; }
};

class PreserveMostAttr : public InheritableAttr {
public:
  static PreserveMostAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) PreserveMostAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  PreserveMostAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::PreserveMost, R, SI, false, false)
  {
  }

  PreserveMostAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::PreserveMost; }
};

class PtGuardedByAttr : public InheritableAttr {
Expr * arg;

public:
  static PtGuardedByAttr *CreateImplicit(ASTContext &Ctx, Expr * Arg, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) PtGuardedByAttr(Loc, Ctx, Arg, 0);
    A->setImplicit(true);
    return A;
  }

  PtGuardedByAttr(SourceRange R, ASTContext &Ctx
              , Expr * Arg
              , unsigned SI
             )
    : InheritableAttr(attr::PtGuardedBy, R, SI, true, true)
              , arg(Arg)
  {
  }

  PtGuardedByAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Expr * getArg() const {
    return arg;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::PtGuardedBy; }
};

class PtGuardedVarAttr : public InheritableAttr {
public:
  static PtGuardedVarAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) PtGuardedVarAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  PtGuardedVarAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::PtGuardedVar, R, SI, false, false)
  {
  }

  PtGuardedVarAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::PtGuardedVar; }
};

class PureAttr : public InheritableAttr {
public:
  static PureAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) PureAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  PureAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Pure, R, SI, false, false)
  {
  }

  PureAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Pure; }
};

class RegCallAttr : public InheritableAttr {
public:
  static RegCallAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) RegCallAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  RegCallAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::RegCall, R, SI, false, false)
  {
  }

  RegCallAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::RegCall; }
};

class ReleaseCapabilityAttr : public InheritableAttr {
  unsigned args_Size;
  Expr * *args_;

public:
  enum Spelling {
    GNU_release_capability = 0,
    CXX11_clang_release_capability = 1,
    GNU_release_shared_capability = 2,
    CXX11_clang_release_shared_capability = 3,
    GNU_release_generic_capability = 4,
    CXX11_clang_release_generic_capability = 5,
    GNU_unlock_function = 6
  };

  static ReleaseCapabilityAttr *CreateImplicit(ASTContext &Ctx, Spelling S, Expr * *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ReleaseCapabilityAttr(Loc, Ctx, Args, ArgsSize, S);
    A->setImplicit(true);
    return A;
  }

  ReleaseCapabilityAttr(SourceRange R, ASTContext &Ctx
              , Expr * *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::ReleaseCapability, R, SI, true, true)
              , args_Size(ArgsSize), args_(new (Ctx, 16) Expr *[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  ReleaseCapabilityAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ReleaseCapability, R, SI, true, true)
              , args_Size(0), args_(nullptr)
  {
  }

  ReleaseCapabilityAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_release_capability;
    case 1: return CXX11_clang_release_capability;
    case 2: return GNU_release_shared_capability;
    case 3: return CXX11_clang_release_shared_capability;
    case 4: return GNU_release_generic_capability;
    case 5: return CXX11_clang_release_generic_capability;
    case 6: return GNU_unlock_function;
  }
  }
  bool isShared() const { return SpellingListIndex == 2 ||
    SpellingListIndex == 3; }
  bool isGeneric() const { return SpellingListIndex == 4 ||
    SpellingListIndex == 5 ||
    SpellingListIndex == 6; }
  typedef Expr ** args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::ReleaseCapability; }
};

class RenderScriptKernelAttr : public Attr {
public:
  static RenderScriptKernelAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) RenderScriptKernelAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  RenderScriptKernelAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : Attr(attr::RenderScriptKernel, R, SI, false, false)
  {
  }

  RenderScriptKernelAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::RenderScriptKernel; }
};

class ReqdWorkGroupSizeAttr : public InheritableAttr {
unsigned xDim;

unsigned yDim;

unsigned zDim;

public:
  static ReqdWorkGroupSizeAttr *CreateImplicit(ASTContext &Ctx, unsigned XDim, unsigned YDim, unsigned ZDim, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ReqdWorkGroupSizeAttr(Loc, Ctx, XDim, YDim, ZDim, 0);
    A->setImplicit(true);
    return A;
  }

  ReqdWorkGroupSizeAttr(SourceRange R, ASTContext &Ctx
              , unsigned XDim
              , unsigned YDim
              , unsigned ZDim
              , unsigned SI
             )
    : InheritableAttr(attr::ReqdWorkGroupSize, R, SI, false, false)
              , xDim(XDim)
              , yDim(YDim)
              , zDim(ZDim)
  {
  }

  ReqdWorkGroupSizeAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getXDim() const {
    return xDim;
  }

  unsigned getYDim() const {
    return yDim;
  }

  unsigned getZDim() const {
    return zDim;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::ReqdWorkGroupSize; }
};

class RequireConstantInitAttr : public InheritableAttr {
public:
  static RequireConstantInitAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) RequireConstantInitAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  RequireConstantInitAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::RequireConstantInit, R, SI, false, false)
  {
  }

  RequireConstantInitAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::RequireConstantInit; }
};

class RequiresCapabilityAttr : public InheritableAttr {
  unsigned args_Size;
  Expr * *args_;

public:
  enum Spelling {
    GNU_requires_capability = 0,
    CXX11_clang_requires_capability = 1,
    GNU_exclusive_locks_required = 2,
    GNU_requires_shared_capability = 3,
    CXX11_clang_requires_shared_capability = 4,
    GNU_shared_locks_required = 5
  };

  static RequiresCapabilityAttr *CreateImplicit(ASTContext &Ctx, Spelling S, Expr * *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) RequiresCapabilityAttr(Loc, Ctx, Args, ArgsSize, S);
    A->setImplicit(true);
    return A;
  }

  RequiresCapabilityAttr(SourceRange R, ASTContext &Ctx
              , Expr * *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::RequiresCapability, R, SI, true, true)
              , args_Size(ArgsSize), args_(new (Ctx, 16) Expr *[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  RequiresCapabilityAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::RequiresCapability, R, SI, true, true)
              , args_Size(0), args_(nullptr)
  {
  }

  RequiresCapabilityAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_requires_capability;
    case 1: return CXX11_clang_requires_capability;
    case 2: return GNU_exclusive_locks_required;
    case 3: return GNU_requires_shared_capability;
    case 4: return CXX11_clang_requires_shared_capability;
    case 5: return GNU_shared_locks_required;
  }
  }
  bool isShared() const { return SpellingListIndex == 3 ||
    SpellingListIndex == 5 ||
    SpellingListIndex == 4; }
  typedef Expr ** args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::RequiresCapability; }
};

class RestrictAttr : public InheritableAttr {
public:
  enum Spelling {
    Declspec_restrict = 0,
    GNU_malloc = 1,
    CXX11_gnu_malloc = 2
  };

  static RestrictAttr *CreateImplicit(ASTContext &Ctx, Spelling S, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) RestrictAttr(Loc, Ctx, S);
    A->setImplicit(true);
    return A;
  }

  RestrictAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Restrict, R, SI, false, false)
  {
  }

  RestrictAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return Declspec_restrict;
    case 1: return GNU_malloc;
    case 2: return CXX11_gnu_malloc;
  }
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::Restrict; }
};

class ReturnTypestateAttr : public InheritableAttr {
public:
  enum ConsumedState {
    Unknown,
    Consumed,
    Unconsumed
  };
private:
  ConsumedState state;

public:
  static ReturnTypestateAttr *CreateImplicit(ASTContext &Ctx, ConsumedState State, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ReturnTypestateAttr(Loc, Ctx, State, 0);
    A->setImplicit(true);
    return A;
  }

  ReturnTypestateAttr(SourceRange R, ASTContext &Ctx
              , ConsumedState State
              , unsigned SI
             )
    : InheritableAttr(attr::ReturnTypestate, R, SI, false, false)
              , state(State)
  {
  }

  ReturnTypestateAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  ConsumedState getState() const {
    return state;
  }

  static bool ConvertStrToConsumedState(StringRef Val, ConsumedState &Out) {
    Optional<ConsumedState> R = llvm::StringSwitch<Optional<ConsumedState>>(Val)
      .Case("unknown", ReturnTypestateAttr::Unknown)
      .Case("consumed", ReturnTypestateAttr::Consumed)
      .Case("unconsumed", ReturnTypestateAttr::Unconsumed)
      .Default(Optional<ConsumedState>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertConsumedStateToStr(ConsumedState Val) {
    switch(Val) {
    case ReturnTypestateAttr::Unknown: return "unknown";
    case ReturnTypestateAttr::Consumed: return "consumed";
    case ReturnTypestateAttr::Unconsumed: return "unconsumed";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::ReturnTypestate; }
};

class ReturnsNonNullAttr : public InheritableAttr {
public:
  static ReturnsNonNullAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ReturnsNonNullAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ReturnsNonNullAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ReturnsNonNull, R, SI, false, false)
  {
  }

  ReturnsNonNullAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ReturnsNonNull; }
};

class ReturnsTwiceAttr : public InheritableAttr {
public:
  static ReturnsTwiceAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ReturnsTwiceAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ReturnsTwiceAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ReturnsTwice, R, SI, false, false)
  {
  }

  ReturnsTwiceAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ReturnsTwice; }
};

class ScopedLockableAttr : public InheritableAttr {
public:
  static ScopedLockableAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ScopedLockableAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ScopedLockableAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ScopedLockable, R, SI, false, false)
  {
  }

  ScopedLockableAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ScopedLockable; }
};

class SectionAttr : public InheritableAttr {
unsigned nameLength;
char *name;

public:
  enum Spelling {
    GNU_section = 0,
    CXX11_gnu_section = 1,
    Declspec_allocate = 2
  };

  static SectionAttr *CreateImplicit(ASTContext &Ctx, Spelling S, llvm::StringRef Name, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) SectionAttr(Loc, Ctx, Name, S);
    A->setImplicit(true);
    return A;
  }

  SectionAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Name
              , unsigned SI
             )
    : InheritableAttr(attr::Section, R, SI, false, false)
              , nameLength(Name.size()),name(new (Ctx, 1) char[nameLength])
  {
      if (!Name.empty())
        std::memcpy(name, Name.data(), nameLength);
  }

  SectionAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_section;
    case 1: return CXX11_gnu_section;
    case 2: return Declspec_allocate;
  }
  }
  llvm::StringRef getName() const {
    return llvm::StringRef(name, nameLength);
  }
  unsigned getNameLength() const {
    return nameLength;
  }
  void setName(ASTContext &C, llvm::StringRef S) {
    nameLength = S.size();
    this->name = new (C, 1) char [nameLength];
    if (!S.empty())
      std::memcpy(this->name, S.data(), nameLength);
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::Section; }
};

class SelectAnyAttr : public InheritableAttr {
public:
  static SelectAnyAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) SelectAnyAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  SelectAnyAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::SelectAny, R, SI, false, false)
  {
  }

  SelectAnyAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::SelectAny; }
};

class SentinelAttr : public InheritableAttr {
int sentinel;

int nullPos;

public:
  static SentinelAttr *CreateImplicit(ASTContext &Ctx, int Sentinel, int NullPos, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) SentinelAttr(Loc, Ctx, Sentinel, NullPos, 0);
    A->setImplicit(true);
    return A;
  }

  SentinelAttr(SourceRange R, ASTContext &Ctx
              , int Sentinel
              , int NullPos
              , unsigned SI
             )
    : InheritableAttr(attr::Sentinel, R, SI, false, false)
              , sentinel(Sentinel)
              , nullPos(NullPos)
  {
  }

  SentinelAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Sentinel, R, SI, false, false)
              , sentinel()
              , nullPos()
  {
  }

  SentinelAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  int getSentinel() const {
    return sentinel;
  }

  static const int DefaultSentinel = 0;

  int getNullPos() const {
    return nullPos;
  }

  static const int DefaultNullPos = 0;



  static bool classof(const Attr *A) { return A->getKind() == attr::Sentinel; }
};

class SetTypestateAttr : public InheritableAttr {
public:
  enum ConsumedState {
    Unknown,
    Consumed,
    Unconsumed
  };
private:
  ConsumedState newState;

public:
  static SetTypestateAttr *CreateImplicit(ASTContext &Ctx, ConsumedState NewState, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) SetTypestateAttr(Loc, Ctx, NewState, 0);
    A->setImplicit(true);
    return A;
  }

  SetTypestateAttr(SourceRange R, ASTContext &Ctx
              , ConsumedState NewState
              , unsigned SI
             )
    : InheritableAttr(attr::SetTypestate, R, SI, false, false)
              , newState(NewState)
  {
  }

  SetTypestateAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  ConsumedState getNewState() const {
    return newState;
  }

  static bool ConvertStrToConsumedState(StringRef Val, ConsumedState &Out) {
    Optional<ConsumedState> R = llvm::StringSwitch<Optional<ConsumedState>>(Val)
      .Case("unknown", SetTypestateAttr::Unknown)
      .Case("consumed", SetTypestateAttr::Consumed)
      .Case("unconsumed", SetTypestateAttr::Unconsumed)
      .Default(Optional<ConsumedState>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertConsumedStateToStr(ConsumedState Val) {
    switch(Val) {
    case SetTypestateAttr::Unknown: return "unknown";
    case SetTypestateAttr::Consumed: return "consumed";
    case SetTypestateAttr::Unconsumed: return "unconsumed";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::SetTypestate; }
};

class SharedTrylockFunctionAttr : public InheritableAttr {
Expr * successValue;

  unsigned args_Size;
  Expr * *args_;

public:
  static SharedTrylockFunctionAttr *CreateImplicit(ASTContext &Ctx, Expr * SuccessValue, Expr * *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) SharedTrylockFunctionAttr(Loc, Ctx, SuccessValue, Args, ArgsSize, 0);
    A->setImplicit(true);
    return A;
  }

  SharedTrylockFunctionAttr(SourceRange R, ASTContext &Ctx
              , Expr * SuccessValue
              , Expr * *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::SharedTrylockFunction, R, SI, true, true)
              , successValue(SuccessValue)
              , args_Size(ArgsSize), args_(new (Ctx, 16) Expr *[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  SharedTrylockFunctionAttr(SourceRange R, ASTContext &Ctx
              , Expr * SuccessValue
              , unsigned SI
             )
    : InheritableAttr(attr::SharedTrylockFunction, R, SI, true, true)
              , successValue(SuccessValue)
              , args_Size(0), args_(nullptr)
  {
  }

  SharedTrylockFunctionAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Expr * getSuccessValue() const {
    return successValue;
  }

  typedef Expr ** args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::SharedTrylockFunction; }
};

class StdCallAttr : public InheritableAttr {
public:
  static StdCallAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) StdCallAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  StdCallAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::StdCall, R, SI, false, false)
  {
  }

  StdCallAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::StdCall; }
};

class SuppressAttr : public StmtAttr {
  unsigned diagnosticIdentifiers_Size;
  StringRef *diagnosticIdentifiers_;

public:
  static SuppressAttr *CreateImplicit(ASTContext &Ctx, StringRef *DiagnosticIdentifiers, unsigned DiagnosticIdentifiersSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) SuppressAttr(Loc, Ctx, DiagnosticIdentifiers, DiagnosticIdentifiersSize, 0);
    A->setImplicit(true);
    return A;
  }

  SuppressAttr(SourceRange R, ASTContext &Ctx
              , StringRef *DiagnosticIdentifiers, unsigned DiagnosticIdentifiersSize
              , unsigned SI
             )
    : StmtAttr(attr::Suppress, R, SI, false, false)
              , diagnosticIdentifiers_Size(DiagnosticIdentifiersSize), diagnosticIdentifiers_(new (Ctx, 16) StringRef[diagnosticIdentifiers_Size])
  {
    for (size_t I = 0, E = diagnosticIdentifiers_Size; I != E;
         ++I) {
      StringRef Ref = DiagnosticIdentifiers[I];
      if (!Ref.empty()) {
        char *Mem = new (Ctx, 1) char[Ref.size()];
        std::memcpy(Mem, Ref.data(), Ref.size());
        diagnosticIdentifiers_[I] = StringRef(Mem, Ref.size());
      }
    }
  }

  SuppressAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : StmtAttr(attr::Suppress, R, SI, false, false)
              , diagnosticIdentifiers_Size(0), diagnosticIdentifiers_(nullptr)
  {
  }

  SuppressAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  typedef StringRef* diagnosticIdentifiers_iterator;
  diagnosticIdentifiers_iterator diagnosticIdentifiers_begin() const { return diagnosticIdentifiers_; }
  diagnosticIdentifiers_iterator diagnosticIdentifiers_end() const { return diagnosticIdentifiers_ + diagnosticIdentifiers_Size; }
  unsigned diagnosticIdentifiers_size() const { return diagnosticIdentifiers_Size; }
  llvm::iterator_range<diagnosticIdentifiers_iterator> diagnosticIdentifiers() const { return llvm::make_range(diagnosticIdentifiers_begin(), diagnosticIdentifiers_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::Suppress; }
};

class SwiftCallAttr : public InheritableAttr {
public:
  static SwiftCallAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) SwiftCallAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  SwiftCallAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::SwiftCall, R, SI, false, false)
  {
  }

  SwiftCallAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::SwiftCall; }
};

class SwiftContextAttr : public ParameterABIAttr {
public:
  static SwiftContextAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) SwiftContextAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  SwiftContextAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : ParameterABIAttr(attr::SwiftContext, R, SI, false, false)
  {
  }

  SwiftContextAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::SwiftContext; }
};

class SwiftErrorResultAttr : public ParameterABIAttr {
public:
  static SwiftErrorResultAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) SwiftErrorResultAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  SwiftErrorResultAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : ParameterABIAttr(attr::SwiftErrorResult, R, SI, false, false)
  {
  }

  SwiftErrorResultAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::SwiftErrorResult; }
};

class SwiftIndirectResultAttr : public ParameterABIAttr {
public:
  static SwiftIndirectResultAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) SwiftIndirectResultAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  SwiftIndirectResultAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : ParameterABIAttr(attr::SwiftIndirectResult, R, SI, false, false)
  {
  }

  SwiftIndirectResultAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::SwiftIndirectResult; }
};

class SysVABIAttr : public InheritableAttr {
public:
  static SysVABIAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) SysVABIAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  SysVABIAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::SysVABI, R, SI, false, false)
  {
  }

  SysVABIAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::SysVABI; }
};

class TLSModelAttr : public InheritableAttr {
unsigned modelLength;
char *model;

public:
  static TLSModelAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Model, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) TLSModelAttr(Loc, Ctx, Model, 0);
    A->setImplicit(true);
    return A;
  }

  TLSModelAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Model
              , unsigned SI
             )
    : InheritableAttr(attr::TLSModel, R, SI, false, false)
              , modelLength(Model.size()),model(new (Ctx, 1) char[modelLength])
  {
      if (!Model.empty())
        std::memcpy(model, Model.data(), modelLength);
  }

  TLSModelAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getModel() const {
    return llvm::StringRef(model, modelLength);
  }
  unsigned getModelLength() const {
    return modelLength;
  }
  void setModel(ASTContext &C, llvm::StringRef S) {
    modelLength = S.size();
    this->model = new (C, 1) char [modelLength];
    if (!S.empty())
      std::memcpy(this->model, S.data(), modelLength);
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::TLSModel; }
};

class TargetAttr : public InheritableAttr {
unsigned featuresStrLength;
char *featuresStr;

public:
  static TargetAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef FeaturesStr, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) TargetAttr(Loc, Ctx, FeaturesStr, 0);
    A->setImplicit(true);
    return A;
  }

  TargetAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef FeaturesStr
              , unsigned SI
             )
    : InheritableAttr(attr::Target, R, SI, false, false)
              , featuresStrLength(FeaturesStr.size()),featuresStr(new (Ctx, 1) char[featuresStrLength])
  {
      if (!FeaturesStr.empty())
        std::memcpy(featuresStr, FeaturesStr.data(), featuresStrLength);
  }

  TargetAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getFeaturesStr() const {
    return llvm::StringRef(featuresStr, featuresStrLength);
  }
  unsigned getFeaturesStrLength() const {
    return featuresStrLength;
  }
  void setFeaturesStr(ASTContext &C, llvm::StringRef S) {
    featuresStrLength = S.size();
    this->featuresStr = new (C, 1) char [featuresStrLength];
    if (!S.empty())
      std::memcpy(this->featuresStr, S.data(), featuresStrLength);
  }


    typedef std::pair<std::vector<std::string>, StringRef> ParsedTargetAttr;
    ParsedTargetAttr parse() const {
      ParsedTargetAttr Ret;
      SmallVector<StringRef, 1> AttrFeatures;
      getFeaturesStr().split(AttrFeatures, ",");

      // Grab the various features and prepend a "+" to turn on the feature to
      // the backend and add them to our existing set of features.
      for (auto &Feature : AttrFeatures) {
        // Go ahead and trim whitespace rather than either erroring or
        // accepting it weirdly.
        Feature = Feature.trim();

        // We don't support cpu tuning this way currently.
        // TODO: Support the fpmath option. It will require checking
        // overall feature validity for the function with the rest of the
        // attributes on the function.
        if (Feature.startswith("fpmath=") || Feature.startswith("tune="))
	  continue;

        // While we're here iterating check for a different target cpu.
        if (Feature.startswith("arch="))
          Ret.second = Feature.split("=").second.trim();
        else if (Feature.startswith("no-"))
          Ret.first.push_back("-" + Feature.split("-").second.str());
        else
          Ret.first.push_back("+" + Feature.str());
      }
      return Ret;
    }
  

  static bool classof(const Attr *A) { return A->getKind() == attr::Target; }
};

class TestTypestateAttr : public InheritableAttr {
public:
  enum ConsumedState {
    Consumed,
    Unconsumed
  };
private:
  ConsumedState testState;

public:
  static TestTypestateAttr *CreateImplicit(ASTContext &Ctx, ConsumedState TestState, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) TestTypestateAttr(Loc, Ctx, TestState, 0);
    A->setImplicit(true);
    return A;
  }

  TestTypestateAttr(SourceRange R, ASTContext &Ctx
              , ConsumedState TestState
              , unsigned SI
             )
    : InheritableAttr(attr::TestTypestate, R, SI, false, false)
              , testState(TestState)
  {
  }

  TestTypestateAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  ConsumedState getTestState() const {
    return testState;
  }

  static bool ConvertStrToConsumedState(StringRef Val, ConsumedState &Out) {
    Optional<ConsumedState> R = llvm::StringSwitch<Optional<ConsumedState>>(Val)
      .Case("consumed", TestTypestateAttr::Consumed)
      .Case("unconsumed", TestTypestateAttr::Unconsumed)
      .Default(Optional<ConsumedState>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertConsumedStateToStr(ConsumedState Val) {
    switch(Val) {
    case TestTypestateAttr::Consumed: return "consumed";
    case TestTypestateAttr::Unconsumed: return "unconsumed";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::TestTypestate; }
};

class ThisCallAttr : public InheritableAttr {
public:
  static ThisCallAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ThisCallAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ThisCallAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::ThisCall, R, SI, false, false)
  {
  }

  ThisCallAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::ThisCall; }
};

class ThreadAttr : public Attr {
public:
  static ThreadAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) ThreadAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  ThreadAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : Attr(attr::Thread, R, SI, false, false)
  {
  }

  ThreadAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Thread; }
};

class TransparentUnionAttr : public InheritableAttr {
public:
  static TransparentUnionAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) TransparentUnionAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  TransparentUnionAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::TransparentUnion, R, SI, false, false)
  {
  }

  TransparentUnionAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::TransparentUnion; }
};

class TryAcquireCapabilityAttr : public InheritableAttr {
Expr * successValue;

  unsigned args_Size;
  Expr * *args_;

public:
  enum Spelling {
    GNU_try_acquire_capability = 0,
    CXX11_clang_try_acquire_capability = 1,
    GNU_try_acquire_shared_capability = 2,
    CXX11_clang_try_acquire_shared_capability = 3
  };

  static TryAcquireCapabilityAttr *CreateImplicit(ASTContext &Ctx, Spelling S, Expr * SuccessValue, Expr * *Args, unsigned ArgsSize, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) TryAcquireCapabilityAttr(Loc, Ctx, SuccessValue, Args, ArgsSize, S);
    A->setImplicit(true);
    return A;
  }

  TryAcquireCapabilityAttr(SourceRange R, ASTContext &Ctx
              , Expr * SuccessValue
              , Expr * *Args, unsigned ArgsSize
              , unsigned SI
             )
    : InheritableAttr(attr::TryAcquireCapability, R, SI, true, true)
              , successValue(SuccessValue)
              , args_Size(ArgsSize), args_(new (Ctx, 16) Expr *[args_Size])
  {
    std::copy(Args, Args + args_Size, args_);
  }

  TryAcquireCapabilityAttr(SourceRange R, ASTContext &Ctx
              , Expr * SuccessValue
              , unsigned SI
             )
    : InheritableAttr(attr::TryAcquireCapability, R, SI, true, true)
              , successValue(SuccessValue)
              , args_Size(0), args_(nullptr)
  {
  }

  TryAcquireCapabilityAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_try_acquire_capability;
    case 1: return CXX11_clang_try_acquire_capability;
    case 2: return GNU_try_acquire_shared_capability;
    case 3: return CXX11_clang_try_acquire_shared_capability;
  }
  }
  bool isShared() const { return SpellingListIndex == 2 ||
    SpellingListIndex == 3; }
  Expr * getSuccessValue() const {
    return successValue;
  }

  typedef Expr ** args_iterator;
  args_iterator args_begin() const { return args_; }
  args_iterator args_end() const { return args_ + args_Size; }
  unsigned args_size() const { return args_Size; }
  llvm::iterator_range<args_iterator> args() const { return llvm::make_range(args_begin(), args_end()); }




  static bool classof(const Attr *A) { return A->getKind() == attr::TryAcquireCapability; }
};

class TypeTagForDatatypeAttr : public InheritableAttr {
IdentifierInfo * argumentKind;

TypeSourceInfo * matchingCType;

bool layoutCompatible;

bool mustBeNull;

public:
  static TypeTagForDatatypeAttr *CreateImplicit(ASTContext &Ctx, IdentifierInfo * ArgumentKind, TypeSourceInfo * MatchingCType, bool LayoutCompatible, bool MustBeNull, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) TypeTagForDatatypeAttr(Loc, Ctx, ArgumentKind, MatchingCType, LayoutCompatible, MustBeNull, 0);
    A->setImplicit(true);
    return A;
  }

  TypeTagForDatatypeAttr(SourceRange R, ASTContext &Ctx
              , IdentifierInfo * ArgumentKind
              , TypeSourceInfo * MatchingCType
              , bool LayoutCompatible
              , bool MustBeNull
              , unsigned SI
             )
    : InheritableAttr(attr::TypeTagForDatatype, R, SI, false, false)
              , argumentKind(ArgumentKind)
              , matchingCType(MatchingCType)
              , layoutCompatible(LayoutCompatible)
              , mustBeNull(MustBeNull)
  {
  }

  TypeTagForDatatypeAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  IdentifierInfo * getArgumentKind() const {
    return argumentKind;
  }

  QualType getMatchingCType() const {
    return matchingCType->getType();
  }  TypeSourceInfo * getMatchingCTypeLoc() const {
    return matchingCType;
  }

  bool getLayoutCompatible() const {
    return layoutCompatible;
  }

  bool getMustBeNull() const {
    return mustBeNull;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::TypeTagForDatatype; }
};

class TypeVisibilityAttr : public InheritableAttr {
public:
  enum VisibilityType {
    Default,
    Hidden,
    Protected
  };
private:
  VisibilityType visibility;

public:
  static TypeVisibilityAttr *CreateImplicit(ASTContext &Ctx, VisibilityType Visibility, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) TypeVisibilityAttr(Loc, Ctx, Visibility, 0);
    A->setImplicit(true);
    return A;
  }

  TypeVisibilityAttr(SourceRange R, ASTContext &Ctx
              , VisibilityType Visibility
              , unsigned SI
             )
    : InheritableAttr(attr::TypeVisibility, R, SI, false, false)
              , visibility(Visibility)
  {
  }

  TypeVisibilityAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  VisibilityType getVisibility() const {
    return visibility;
  }

  static bool ConvertStrToVisibilityType(StringRef Val, VisibilityType &Out) {
    Optional<VisibilityType> R = llvm::StringSwitch<Optional<VisibilityType>>(Val)
      .Case("default", TypeVisibilityAttr::Default)
      .Case("hidden", TypeVisibilityAttr::Hidden)
      .Case("internal", TypeVisibilityAttr::Hidden)
      .Case("protected", TypeVisibilityAttr::Protected)
      .Default(Optional<VisibilityType>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertVisibilityTypeToStr(VisibilityType Val) {
    switch(Val) {
    case TypeVisibilityAttr::Default: return "default";
    case TypeVisibilityAttr::Hidden: return "hidden";
    case TypeVisibilityAttr::Protected: return "protected";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::TypeVisibility; }
};

class UnavailableAttr : public InheritableAttr {
unsigned messageLength;
char *message;

public:
  enum ImplicitReason {
    IR_None,
    IR_ARCForbiddenType,
    IR_ForbiddenWeak,
    IR_ARCForbiddenConversion,
    IR_ARCInitReturnsUnrelated,
    IR_ARCFieldWithOwnership
  };
private:
  ImplicitReason implicitReason;

public:
  static UnavailableAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Message, ImplicitReason ImplicitReason, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) UnavailableAttr(Loc, Ctx, Message, ImplicitReason, 0);
    A->setImplicit(true);
    return A;
  }

  static UnavailableAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Message, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) UnavailableAttr(Loc, Ctx, Message, 0);
    A->setImplicit(true);
    return A;
  }

  UnavailableAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Message
              , ImplicitReason ImplicitReason
              , unsigned SI
             )
    : InheritableAttr(attr::Unavailable, R, SI, false, false)
              , messageLength(Message.size()),message(new (Ctx, 1) char[messageLength])
              , implicitReason(ImplicitReason)
  {
      if (!Message.empty())
        std::memcpy(message, Message.data(), messageLength);
  }

  UnavailableAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Message
              , unsigned SI
             )
    : InheritableAttr(attr::Unavailable, R, SI, false, false)
              , messageLength(Message.size()),message(new (Ctx, 1) char[messageLength])
              , implicitReason(ImplicitReason(0))
  {
      if (!Message.empty())
        std::memcpy(message, Message.data(), messageLength);
  }

  UnavailableAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Unavailable, R, SI, false, false)
              , messageLength(0),message(nullptr)
              , implicitReason(ImplicitReason(0))
  {
  }

  UnavailableAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getMessage() const {
    return llvm::StringRef(message, messageLength);
  }
  unsigned getMessageLength() const {
    return messageLength;
  }
  void setMessage(ASTContext &C, llvm::StringRef S) {
    messageLength = S.size();
    this->message = new (C, 1) char [messageLength];
    if (!S.empty())
      std::memcpy(this->message, S.data(), messageLength);
  }

  ImplicitReason getImplicitReason() const {
    return implicitReason;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::Unavailable; }
};

class UnusedAttr : public InheritableAttr {
public:
  enum Spelling {
    CXX11_maybe_unused = 0,
    GNU_unused = 1,
    CXX11_gnu_unused = 2
  };

  static UnusedAttr *CreateImplicit(ASTContext &Ctx, Spelling S, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) UnusedAttr(Loc, Ctx, S);
    A->setImplicit(true);
    return A;
  }

  UnusedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Unused, R, SI, false, false)
  {
  }

  UnusedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return CXX11_maybe_unused;
    case 1: return GNU_unused;
    case 2: return CXX11_gnu_unused;
  }
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::Unused; }
};

class UsedAttr : public InheritableAttr {
public:
  static UsedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) UsedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  UsedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Used, R, SI, false, false)
  {
  }

  UsedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Used; }
};

class UuidAttr : public InheritableAttr {
unsigned guidLength;
char *guid;

public:
  static UuidAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Guid, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) UuidAttr(Loc, Ctx, Guid, 0);
    A->setImplicit(true);
    return A;
  }

  UuidAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Guid
              , unsigned SI
             )
    : InheritableAttr(attr::Uuid, R, SI, false, false)
              , guidLength(Guid.size()),guid(new (Ctx, 1) char[guidLength])
  {
      if (!Guid.empty())
        std::memcpy(guid, Guid.data(), guidLength);
  }

  UuidAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getGuid() const {
    return llvm::StringRef(guid, guidLength);
  }
  unsigned getGuidLength() const {
    return guidLength;
  }
  void setGuid(ASTContext &C, llvm::StringRef S) {
    guidLength = S.size();
    this->guid = new (C, 1) char [guidLength];
    if (!S.empty())
      std::memcpy(this->guid, S.data(), guidLength);
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::Uuid; }
};

class VecReturnAttr : public InheritableAttr {
public:
  static VecReturnAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) VecReturnAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  VecReturnAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::VecReturn, R, SI, false, false)
  {
  }

  VecReturnAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::VecReturn; }
};

class VecTypeHintAttr : public InheritableAttr {
TypeSourceInfo * typeHint;

public:
  static VecTypeHintAttr *CreateImplicit(ASTContext &Ctx, TypeSourceInfo * TypeHint, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) VecTypeHintAttr(Loc, Ctx, TypeHint, 0);
    A->setImplicit(true);
    return A;
  }

  VecTypeHintAttr(SourceRange R, ASTContext &Ctx
              , TypeSourceInfo * TypeHint
              , unsigned SI
             )
    : InheritableAttr(attr::VecTypeHint, R, SI, false, false)
              , typeHint(TypeHint)
  {
  }

  VecTypeHintAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  QualType getTypeHint() const {
    return typeHint->getType();
  }  TypeSourceInfo * getTypeHintLoc() const {
    return typeHint;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::VecTypeHint; }
};

class VectorCallAttr : public InheritableAttr {
public:
  static VectorCallAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) VectorCallAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  VectorCallAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::VectorCall, R, SI, false, false)
  {
  }

  VectorCallAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::VectorCall; }
};

class VisibilityAttr : public InheritableAttr {
public:
  enum VisibilityType {
    Default,
    Hidden,
    Protected
  };
private:
  VisibilityType visibility;

public:
  static VisibilityAttr *CreateImplicit(ASTContext &Ctx, VisibilityType Visibility, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) VisibilityAttr(Loc, Ctx, Visibility, 0);
    A->setImplicit(true);
    return A;
  }

  VisibilityAttr(SourceRange R, ASTContext &Ctx
              , VisibilityType Visibility
              , unsigned SI
             )
    : InheritableAttr(attr::Visibility, R, SI, false, false)
              , visibility(Visibility)
  {
  }

  VisibilityAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  VisibilityType getVisibility() const {
    return visibility;
  }

  static bool ConvertStrToVisibilityType(StringRef Val, VisibilityType &Out) {
    Optional<VisibilityType> R = llvm::StringSwitch<Optional<VisibilityType>>(Val)
      .Case("default", VisibilityAttr::Default)
      .Case("hidden", VisibilityAttr::Hidden)
      .Case("internal", VisibilityAttr::Hidden)
      .Case("protected", VisibilityAttr::Protected)
      .Default(Optional<VisibilityType>());
    if (R) {
      Out = *R;
      return true;
    }
    return false;
  }

  static const char *ConvertVisibilityTypeToStr(VisibilityType Val) {
    switch(Val) {
    case VisibilityAttr::Default: return "default";
    case VisibilityAttr::Hidden: return "hidden";
    case VisibilityAttr::Protected: return "protected";
    }
    llvm_unreachable("No enumerator with that value");
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::Visibility; }
};

class WarnUnusedAttr : public InheritableAttr {
public:
  static WarnUnusedAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) WarnUnusedAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  WarnUnusedAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::WarnUnused, R, SI, false, false)
  {
  }

  WarnUnusedAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::WarnUnused; }
};

class WarnUnusedResultAttr : public InheritableAttr {
public:
  enum Spelling {
    CXX11_nodiscard = 0,
    CXX11_clang_warn_unused_result = 1,
    GNU_warn_unused_result = 2,
    CXX11_gnu_warn_unused_result = 3
  };

  static WarnUnusedResultAttr *CreateImplicit(ASTContext &Ctx, Spelling S, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) WarnUnusedResultAttr(Loc, Ctx, S);
    A->setImplicit(true);
    return A;
  }

  WarnUnusedResultAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::WarnUnusedResult, R, SI, false, false)
  {
  }

  WarnUnusedResultAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return CXX11_nodiscard;
    case 1: return CXX11_clang_warn_unused_result;
    case 2: return GNU_warn_unused_result;
    case 3: return CXX11_gnu_warn_unused_result;
  }
  }


  static bool classof(const Attr *A) { return A->getKind() == attr::WarnUnusedResult; }
};

class WeakAttr : public InheritableAttr {
public:
  static WeakAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) WeakAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  WeakAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::Weak, R, SI, false, false)
  {
  }

  WeakAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::Weak; }
};

class WeakImportAttr : public InheritableAttr {
public:
  static WeakImportAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) WeakImportAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  WeakImportAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::WeakImport, R, SI, false, false)
  {
  }

  WeakImportAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::WeakImport; }
};

class WeakRefAttr : public InheritableAttr {
unsigned aliaseeLength;
char *aliasee;

public:
  static WeakRefAttr *CreateImplicit(ASTContext &Ctx, llvm::StringRef Aliasee, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) WeakRefAttr(Loc, Ctx, Aliasee, 0);
    A->setImplicit(true);
    return A;
  }

  WeakRefAttr(SourceRange R, ASTContext &Ctx
              , llvm::StringRef Aliasee
              , unsigned SI
             )
    : InheritableAttr(attr::WeakRef, R, SI, false, false)
              , aliaseeLength(Aliasee.size()),aliasee(new (Ctx, 1) char[aliaseeLength])
  {
      if (!Aliasee.empty())
        std::memcpy(aliasee, Aliasee.data(), aliaseeLength);
  }

  WeakRefAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::WeakRef, R, SI, false, false)
              , aliaseeLength(0),aliasee(nullptr)
  {
  }

  WeakRefAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  llvm::StringRef getAliasee() const {
    return llvm::StringRef(aliasee, aliaseeLength);
  }
  unsigned getAliaseeLength() const {
    return aliaseeLength;
  }
  void setAliasee(ASTContext &C, llvm::StringRef S) {
    aliaseeLength = S.size();
    this->aliasee = new (C, 1) char [aliaseeLength];
    if (!S.empty())
      std::memcpy(this->aliasee, S.data(), aliaseeLength);
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::WeakRef; }
};

class WorkGroupSizeHintAttr : public InheritableAttr {
unsigned xDim;

unsigned yDim;

unsigned zDim;

public:
  static WorkGroupSizeHintAttr *CreateImplicit(ASTContext &Ctx, unsigned XDim, unsigned YDim, unsigned ZDim, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) WorkGroupSizeHintAttr(Loc, Ctx, XDim, YDim, ZDim, 0);
    A->setImplicit(true);
    return A;
  }

  WorkGroupSizeHintAttr(SourceRange R, ASTContext &Ctx
              , unsigned XDim
              , unsigned YDim
              , unsigned ZDim
              , unsigned SI
             )
    : InheritableAttr(attr::WorkGroupSizeHint, R, SI, false, false)
              , xDim(XDim)
              , yDim(YDim)
              , zDim(ZDim)
  {
  }

  WorkGroupSizeHintAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getXDim() const {
    return xDim;
  }

  unsigned getYDim() const {
    return yDim;
  }

  unsigned getZDim() const {
    return zDim;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::WorkGroupSizeHint; }
};

class X86ForceAlignArgPointerAttr : public InheritableAttr {
public:
  static X86ForceAlignArgPointerAttr *CreateImplicit(ASTContext &Ctx, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) X86ForceAlignArgPointerAttr(Loc, Ctx, 0);
    A->setImplicit(true);
    return A;
  }

  X86ForceAlignArgPointerAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::X86ForceAlignArgPointer, R, SI, false, false)
  {
  }

  X86ForceAlignArgPointerAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;


  static bool classof(const Attr *A) { return A->getKind() == attr::X86ForceAlignArgPointer; }
};

class XRayInstrumentAttr : public InheritableAttr {
public:
  enum Spelling {
    GNU_xray_always_instrument = 0,
    CXX11_clang_xray_always_instrument = 1,
    GNU_xray_never_instrument = 2,
    CXX11_clang_xray_never_instrument = 3
  };

  static XRayInstrumentAttr *CreateImplicit(ASTContext &Ctx, Spelling S, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) XRayInstrumentAttr(Loc, Ctx, S);
    A->setImplicit(true);
    return A;
  }

  XRayInstrumentAttr(SourceRange R, ASTContext &Ctx
              , unsigned SI
             )
    : InheritableAttr(attr::XRayInstrument, R, SI, false, false)
  {
  }

  XRayInstrumentAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  Spelling getSemanticSpelling() const {
  switch (SpellingListIndex) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_xray_always_instrument;
    case 1: return CXX11_clang_xray_always_instrument;
    case 2: return GNU_xray_never_instrument;
    case 3: return CXX11_clang_xray_never_instrument;
  }
  }
  bool alwaysXRayInstrument() const { return SpellingListIndex == 0 ||
    SpellingListIndex == 1; }
  bool neverXRayInstrument() const { return SpellingListIndex == 2 ||
    SpellingListIndex == 3; }


  static bool classof(const Attr *A) { return A->getKind() == attr::XRayInstrument; }
};

class XRayLogArgsAttr : public InheritableAttr {
unsigned argumentCount;

public:
  static XRayLogArgsAttr *CreateImplicit(ASTContext &Ctx, unsigned ArgumentCount, SourceRange Loc = SourceRange()) {
    auto *A = new (Ctx) XRayLogArgsAttr(Loc, Ctx, ArgumentCount, 0);
    A->setImplicit(true);
    return A;
  }

  XRayLogArgsAttr(SourceRange R, ASTContext &Ctx
              , unsigned ArgumentCount
              , unsigned SI
             )
    : InheritableAttr(attr::XRayLogArgs, R, SI, false, false)
              , argumentCount(ArgumentCount)
  {
  }

  XRayLogArgsAttr *clone(ASTContext &C) const;
  void printPretty(raw_ostream &OS,
                   const PrintingPolicy &Policy) const;
  const char *getSpelling() const;
  unsigned getArgumentCount() const {
    return argumentCount;
  }



  static bool classof(const Attr *A) { return A->getKind() == attr::XRayLogArgs; }
};

#endif // LLVM_CLANG_ATTR_CLASSES_INC
