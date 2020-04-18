/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Parsed attribute helpers                                                   *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

static bool defaultAppertainsTo(Sema &, const AttributeList &,const Decl *) {
  return true;
}

static bool defaultDiagnoseLangOpts(Sema &, const AttributeList &) {
  return true;
}

static bool defaultTargetRequirements(const TargetInfo &) {
  return true;
}

static unsigned defaultSpellingIndexToSemanticSpelling(const AttributeList &Attr) {
  return UINT_MAX;
}

static bool checkAMDGPUFlatWorkGroupSizeAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedKernelFunction;
    return false;
  }
  return true;
}

static bool checkAMDGPUNumSGPRAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedKernelFunction;
    return false;
  }
  return true;
}

static bool checkAMDGPUNumVGPRAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedKernelFunction;
    return false;
  }
  return true;
}

static bool checkAMDGPUWavesPerEUAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedKernelFunction;
    return false;
  }
  return true;
}

static bool isTargetarmthumbarmebthumbebavrx86x86_64msp430mipsmipsel(const TargetInfo &Target) {
  const llvm::Triple &T = Target.getTriple();
  return (T.getArch() == llvm::Triple::arm || T.getArch() == llvm::Triple::thumb || T.getArch() == llvm::Triple::armeb || T.getArch() == llvm::Triple::thumbeb || T.getArch() == llvm::Triple::avr || T.getArch() == llvm::Triple::x86 || T.getArch() == llvm::Triple::x86_64 || T.getArch() == llvm::Triple::msp430 || T.getArch() == llvm::Triple::mips || T.getArch() == llvm::Triple::mipsel);
}

static bool checkAVRSignalAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool isTargetavr(const TargetInfo &Target) {
  const llvm::Triple &T = Target.getTriple();
  return (T.getArch() == llvm::Triple::avr);
}

static bool isStruct(const Decl *D) {
  if (const auto *S = dyn_cast<RecordDecl>(D))
    return !S->isUnion();
  return false;
}

static bool checkAbiTagAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isStruct(D) && !isa<VarDecl>(D) && !isa<FunctionDecl>(D) && !isa<NamespaceDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedStructClassVariableFunctionOrInlineNamespace;
    return false;
  }
  return true;
}

static bool checkAcquireCapabilityAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static unsigned AcquireCapabilityAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_acquire_capability = 0,
    CXX11_clang_acquire_capability = 1,
    GNU_acquire_shared_capability = 2,
    CXX11_clang_acquire_shared_capability = 3,
    GNU_exclusive_lock_function = 4,
    GNU_shared_lock_function = 5
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_acquire_capability;
    case 1: return CXX11_clang_acquire_capability;
    case 2: return GNU_acquire_shared_capability;
    case 3: return CXX11_clang_acquire_shared_capability;
    case 4: return GNU_exclusive_lock_function;
    case 5: return GNU_shared_lock_function;
  }
}

static bool isSharedVar(const Decl *D) {
  if (const auto *S = dyn_cast<VarDecl>(D))
    return S->hasGlobalStorage() && !S->getTLSKind();
  return false;
}

static bool checkAcquiredAfterAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FieldDecl>(D) && !isSharedVar(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFieldOrGlobalVar;
    return false;
  }
  return true;
}

static bool checkAcquiredBeforeAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FieldDecl>(D) && !isSharedVar(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFieldOrGlobalVar;
    return false;
  }
  return true;
}

static bool isGlobalVar(const Decl *D) {
  if (const auto *S = dyn_cast<VarDecl>(D))
    return S->hasGlobalStorage();
  return false;
}

static bool checkAliasAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D) && !isGlobalVar(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionOrGlobalVar;
    return false;
  }
  return true;
}

static bool checkAlignValueAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D) && !isa<TypedefNameDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariableOrTypedef;
    return false;
  }
  return true;
}

static unsigned AlignedAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_aligned = 0,
    CXX11_gnu_aligned = 1,
    Declspec_align = 2,
    Keyword_alignas = 3,
    Keyword_Alignas = 4
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_aligned;
    case 1: return CXX11_gnu_aligned;
    case 2: return Declspec_align;
    case 3: return Keyword_alignas;
    case 4: return Keyword_Alignas;
  }
}

static bool isHasFunctionProto(const Decl *D) {
  if (const auto *S = dyn_cast<Decl>(D))
    return (S->getFunctionType(true) != nullptr &&
                              isa<FunctionProtoType>(S->getFunctionType())) ||
                                       isa<ObjCMethodDecl>(S) ||
                                       isa<BlockDecl>(S);
  return false;
}

static bool checkAllocAlignAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isHasFunctionProto(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionWithProtoType;
    return false;
  }
  return true;
}

static bool checkAllocSizeAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkAlwaysInlineAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static unsigned AlwaysInlineAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_always_inline = 0,
    CXX11_gnu_always_inline = 1,
    Keyword_forceinline = 2
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_always_inline;
    case 1: return CXX11_gnu_always_inline;
    case 2: return Keyword_forceinline;
  }
}

static bool checkArcWeakrefUnavailableAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCInterfaceDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedObjectiveCInterface;
    return false;
  }
  return true;
}

static unsigned ArgumentWithTypeTagAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_argument_with_type_tag = 0,
    GNU_pointer_with_type_tag = 1
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_argument_with_type_tag;
    case 1: return GNU_pointer_with_type_tag;
  }
}

static bool checkAssertCapabilityAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static unsigned AssertCapabilityAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_assert_capability = 0,
    CXX11_clang_assert_capability = 1,
    GNU_assert_shared_capability = 2,
    CXX11_clang_assert_shared_capability = 3
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_assert_capability;
    case 1: return CXX11_clang_assert_capability;
    case 2: return GNU_assert_shared_capability;
    case 3: return CXX11_clang_assert_shared_capability;
  }
}

static bool checkAssertExclusiveLockAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkAssertSharedLockAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkAssumeAlignedAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCMethodDecl>(D) && !isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionOrMethod;
    return false;
  }
  return true;
}

static bool checkCFAuditedTransferAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkCFConsumedAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ParmVarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedParameter;
    return false;
  }
  return true;
}

static bool checkCFUnknownTransferAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkCUDAConstantAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariable;
    return false;
  }
  return true;
}

static bool checkCUDALangOpts(Sema &S, const AttributeList &Attr) {
  if (S.LangOpts.CUDA)
    return true;

  S.Diag(Attr.getLoc(), diag::warn_attribute_ignored) << Attr.getName();
  return false;
}

static bool checkCUDADeviceAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D) && !isa<VarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariableOrFunction;
    return false;
  }
  return true;
}

static bool checkCUDAGlobalAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkCUDAHostAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkCUDAInvalidTargetAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool isFunctionLike(const Decl *D) {
  if (const auto *S = dyn_cast<Decl>(D))
    return S->getFunctionType(false) != nullptr;
  return false;
}

static bool checkCUDALaunchBoundsAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCMethodDecl>(D) && !isFunctionLike(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionOrMethod;
    return false;
  }
  return true;
}

static bool checkCUDASharedAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariable;
    return false;
  }
  return true;
}

static bool checkCXX11NoReturnAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkCallableWhenAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXMethodDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedMethod;
    return false;
  }
  return true;
}

static bool checkCapabilityAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<RecordDecl>(D) && !isa<TypedefNameDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedStructOrUnionOrTypedef;
    return false;
  }
  return true;
}

static unsigned CapabilityAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_capability = 0,
    CXX11_clang_capability = 1,
    GNU_shared_capability = 2,
    CXX11_clang_shared_capability = 3
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_capability;
    case 1: return CXX11_clang_capability;
    case 2: return GNU_shared_capability;
    case 3: return CXX11_clang_shared_capability;
  }
}

static bool checkCarriesDependencyAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ParmVarDecl>(D) && !isa<ObjCMethodDecl>(D) && !isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionMethodOrParameter;
    return false;
  }
  return true;
}

static bool checkCleanupAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariable;
    return false;
  }
  return true;
}

static bool checkColdAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkCommonAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariable;
    return false;
  }
  return true;
}

static bool checkConstructorAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkConsumableAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXRecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedClass;
    return false;
  }
  return true;
}

static bool checkConsumableAutoCastAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXRecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedClass;
    return false;
  }
  return true;
}

static bool checkConsumableSetOnReadAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXRecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedClass;
    return false;
  }
  return true;
}

static bool checkConvergentAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkDLLExportAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D) && !isa<VarDecl>(D) && !isa<CXXRecordDecl>(D) && !isa<ObjCInterfaceDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << (S.getLangOpts().CPlusPlus     ? ((S.getLangOpts().ObjC1 || S.getLangOpts().ObjC2)            ? ExpectedFunctionVariableClassOrObjCInterface            : ExpectedFunctionVariableOrClass)     : ((S.getLangOpts().ObjC1 || S.getLangOpts().ObjC2)            ? ExpectedFunctionVariableOrObjCInterface            : ExpectedVariableOrFunction));
    return false;
  }
  return true;
}

static bool isTargetx86x86_64armthumbWin32(const TargetInfo &Target) {
  const llvm::Triple &T = Target.getTriple();
  return (T.getArch() == llvm::Triple::x86 || T.getArch() == llvm::Triple::x86_64 || T.getArch() == llvm::Triple::arm || T.getArch() == llvm::Triple::thumb) && (T.getOS() == llvm::Triple::Win32);
}

static bool checkDLLImportAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D) && !isa<VarDecl>(D) && !isa<CXXRecordDecl>(D) && !isa<ObjCInterfaceDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << (S.getLangOpts().CPlusPlus     ? ((S.getLangOpts().ObjC1 || S.getLangOpts().ObjC2)            ? ExpectedFunctionVariableClassOrObjCInterface            : ExpectedFunctionVariableOrClass)     : ((S.getLangOpts().ObjC1 || S.getLangOpts().ObjC2)            ? ExpectedFunctionVariableOrObjCInterface            : ExpectedVariableOrFunction));
    return false;
  }
  return true;
}

static bool checkDestructorAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkDiagnoseIfAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkDisableTailCallsAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D) && !isa<ObjCMethodDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionOrMethod;
    return false;
  }
  return true;
}

static bool checkEmptyBasesAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXRecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedClass;
    return false;
  }
  return true;
}

static bool isTargetx86x86_64armthumbMicrosoft(const TargetInfo &Target) {
  const llvm::Triple &T = Target.getTriple();
  return (T.getArch() == llvm::Triple::x86 || T.getArch() == llvm::Triple::x86_64 || T.getArch() == llvm::Triple::arm || T.getArch() == llvm::Triple::thumb) && (Target.getCXXABI().getKind() == TargetCXXABI::Microsoft);
}

static bool checkEnableIfAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkEnumExtensibilityAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<EnumDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedEnum;
    return false;
  }
  return true;
}

static bool checkExclusiveTrylockFunctionAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkExtVectorTypeAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<TypedefNameDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedType;
    return false;
  }
  return true;
}

static bool checkFlagEnumAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<EnumDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedEnum;
    return false;
  }
  return true;
}

static bool checkFlattenAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkFormatAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCMethodDecl>(D) && !isa<BlockDecl>(D) && !isHasFunctionProto(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionWithProtoType;
    return false;
  }
  return true;
}

static bool checkFormatArgAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCMethodDecl>(D) && !isHasFunctionProto(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionWithProtoType;
    return false;
  }
  return true;
}

static bool checkGNUInlineAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkGuardedByAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FieldDecl>(D) && !isSharedVar(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFieldOrGlobalVar;
    return false;
  }
  return true;
}

static bool checkGuardedVarAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FieldDecl>(D) && !isSharedVar(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFieldOrGlobalVar;
    return false;
  }
  return true;
}

static bool checkHotAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool isObjCInstanceMethod(const Decl *D) {
  if (const auto *S = dyn_cast<ObjCMethodDecl>(D))
    return S->isInstanceMethod();
  return false;
}

static bool checkIBActionAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isObjCInstanceMethod(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedObjCInstanceMethod;
    return false;
  }
  return true;
}

static bool checkIFuncAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkInitPriorityAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariable;
    return false;
  }
  return true;
}

static bool checkInternalLinkageAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D) && !isa<FunctionDecl>(D) && !isa<CXXRecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << (S.getLangOpts().CPlusPlus ? ExpectedFunctionVariableOrClass : ExpectedVariableOrFunction);
    return false;
  }
  return true;
}

static bool checkLTOVisibilityPublicAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<RecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << (S.getLangOpts().CPlusPlus ? ExpectedStructOrUnionOrClass : ExpectedStructOrUnion);
    return false;
  }
  return true;
}

static bool checkLayoutVersionAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXRecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedClass;
    return false;
  }
  return true;
}

static bool checkLockReturnedAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkLockableAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<RecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << (S.getLangOpts().CPlusPlus ? ExpectedStructOrUnionOrClass : ExpectedStructOrUnion);
    return false;
  }
  return true;
}

static bool checkLocksExcludedAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static unsigned LoopHintAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    Pragma_clang_loop = 0,
    Pragma_unroll = 1,
    Pragma_nounroll = 2
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return Pragma_clang_loop;
    case 1: return Pragma_unroll;
    case 2: return Pragma_nounroll;
  }
}

static bool checkMicrosoftExtLangOpts(Sema &S, const AttributeList &Attr) {
  if (S.LangOpts.MicrosoftExt)
    return true;

  S.Diag(Attr.getLoc(), diag::warn_attribute_ignored) << Attr.getName();
  return false;
}

static unsigned MSInheritanceAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    Keyword_single_inheritance = 0,
    Keyword_multiple_inheritance = 1,
    Keyword_virtual_inheritance = 2,
    Keyword_unspecified_inheritance = 3
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return Keyword_single_inheritance;
    case 1: return Keyword_multiple_inheritance;
    case 2: return Keyword_virtual_inheritance;
    case 3: return Keyword_unspecified_inheritance;
  }
}

static bool checkMSNoVTableAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXRecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedClass;
    return false;
  }
  return true;
}

static bool checkMSStructAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<RecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << (S.getLangOpts().CPlusPlus ? ExpectedStructOrUnionOrClass : ExpectedStructOrUnion);
    return false;
  }
  return true;
}

static bool checkMinSizeAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D) && !isa<ObjCMethodDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionOrMethod;
    return false;
  }
  return true;
}

static bool checkMips16AppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool isTargetmipsmipsel(const TargetInfo &Target) {
  const llvm::Triple &T = Target.getTriple();
  return (T.getArch() == llvm::Triple::mips || T.getArch() == llvm::Triple::mipsel);
}

static bool checkModeAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D) && !isa<EnumDecl>(D) && !isa<TypedefNameDecl>(D) && !isa<FieldDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariableEnumFieldOrTypedef;
    return false;
  }
  return true;
}

static bool checkNSConsumedAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ParmVarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedParameter;
    return false;
  }
  return true;
}

static bool checkNSConsumesSelfAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCMethodDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedMethod;
    return false;
  }
  return true;
}

static bool checkNakedAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkNoAliasAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkNoCommonAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariable;
    return false;
  }
  return true;
}

static bool isNonParmVar(const Decl *D) {
  if (const auto *S = dyn_cast<VarDecl>(D))
    return S->getKind() != Decl::ParmVar;
  return false;
}

static bool checkNoDebugAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isFunctionLike(D) && !isa<ObjCMethodDecl>(D) && !isNonParmVar(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariableOrFunction;
    return false;
  }
  return true;
}

static bool checkNoDuplicateAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkNoInlineAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkNoInstrumentFunctionAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkNoMips16AppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkNoSanitizeAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D) && !isa<ObjCMethodDecl>(D) && !isGlobalVar(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionMethodOrGlobalVar;
    return false;
  }
  return true;
}

static bool checkNoSanitizeSpecificAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D) && !isGlobalVar(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionOrGlobalVar;
    return false;
  }
  return true;
}

static bool checkNoSplitStackAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkNoThreadSafetyAnalysisAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkNonNullAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCMethodDecl>(D) && !isHasFunctionProto(D) && !isa<ParmVarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionMethodOrParameter;
    return false;
  }
  return true;
}

static bool checkNotTailCalledAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkObjCBoxableAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<RecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedStructOrUnion;
    return false;
  }
  return true;
}

static bool checkObjCBridgeAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<RecordDecl>(D) && !isa<TypedefNameDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedStructOrUnionOrTypedef;
    return false;
  }
  return true;
}

static bool checkObjCBridgeMutableAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<RecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << (S.getLangOpts().CPlusPlus ? ExpectedStructOrUnionOrClass : ExpectedStructOrUnion);
    return false;
  }
  return true;
}

static bool checkObjCBridgeRelatedAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<RecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << (S.getLangOpts().CPlusPlus ? ExpectedStructOrUnionOrClass : ExpectedStructOrUnion);
    return false;
  }
  return true;
}

static bool isObjCInterfaceDeclInitMethod(const Decl *D) {
  if (const auto *S = dyn_cast<ObjCMethodDecl>(D))
    return S->getMethodFamily() == OMF_init &&
                                 (isa<ObjCInterfaceDecl>(S->getDeclContext()) ||
                                  (isa<ObjCCategoryDecl>(S->getDeclContext()) &&
            cast<ObjCCategoryDecl>(S->getDeclContext())->IsClassExtension()));
  return false;
}

static bool checkObjCDesignatedInitializerAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isObjCInterfaceDeclInitMethod(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedObjCInterfaceDeclInitMethod;
    return false;
  }
  return true;
}

static bool checkObjCExceptionAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCInterfaceDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedObjectiveCInterface;
    return false;
  }
  return true;
}

static bool checkObjCExplicitProtocolImplAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCProtocolDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedObjectiveCProtocol;
    return false;
  }
  return true;
}

static bool checkObjCMethodFamilyAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCMethodDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedMethod;
    return false;
  }
  return true;
}

static bool checkObjCPreciseLifetimeAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariable;
    return false;
  }
  return true;
}

static bool checkObjCRequiresPropertyDefsAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCInterfaceDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedObjectiveCInterface;
    return false;
  }
  return true;
}

static bool checkObjCRequiresSuperAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCMethodDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedMethod;
    return false;
  }
  return true;
}

static bool checkObjCReturnsInnerPointerAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCMethodDecl>(D) && !isa<ObjCPropertyDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedMethodOrProperty;
    return false;
  }
  return true;
}

static bool checkObjCRootClassAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCInterfaceDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedObjectiveCInterface;
    return false;
  }
  return true;
}

static bool checkObjCRuntimeNameAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCInterfaceDecl>(D) && !isa<ObjCProtocolDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedObjectiveCInterfaceOrProtocol;
    return false;
  }
  return true;
}

static bool checkObjCRuntimeVisibleAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCInterfaceDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedObjectiveCInterface;
    return false;
  }
  return true;
}

static bool checkObjCSubclassingRestrictedAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCInterfaceDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedObjectiveCInterface;
    return false;
  }
  return true;
}

static bool checkOpenCLAccessAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ParmVarDecl>(D) && !isa<TypedefNameDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedParameterOrTypedef;
    return false;
  }
  return true;
}

static unsigned OpenCLAccessAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    Keyword_read_only = 0,
    Keyword_write_only = 2,
    Keyword_read_write = 4
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return Keyword_read_only;
    case 1: return Keyword_read_only;
    case 2: return Keyword_write_only;
    case 3: return Keyword_write_only;
    case 4: return Keyword_read_write;
    case 5: return Keyword_read_write;
  }
}

static bool checkOpenCLKernelAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkOpenCLNoSVMAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariable;
    return false;
  }
  return true;
}

static bool checkOpenCLLangOpts(Sema &S, const AttributeList &Attr) {
  if (S.LangOpts.OpenCL)
    return true;

  S.Diag(Attr.getLoc(), diag::warn_attribute_ignored) << Attr.getName();
  return false;
}

static bool checkOptimizeNoneAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D) && !isa<ObjCMethodDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionOrMethod;
    return false;
  }
  return true;
}

static bool checkOverloadableAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkOwnershipAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isHasFunctionProto(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionWithProtoType;
    return false;
  }
  return true;
}

static unsigned OwnershipAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_ownership_holds = 0,
    GNU_ownership_returns = 1,
    GNU_ownership_takes = 2
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_ownership_holds;
    case 1: return GNU_ownership_returns;
    case 2: return GNU_ownership_takes;
  }
}

static bool checkParamTypestateAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ParmVarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedParameter;
    return false;
  }
  return true;
}

static bool checkPassObjectSizeAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ParmVarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedParameter;
    return false;
  }
  return true;
}

static bool checkPtGuardedByAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FieldDecl>(D) && !isSharedVar(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFieldOrGlobalVar;
    return false;
  }
  return true;
}

static bool checkPtGuardedVarAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FieldDecl>(D) && !isSharedVar(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFieldOrGlobalVar;
    return false;
  }
  return true;
}

static bool checkReleaseCapabilityAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static unsigned ReleaseCapabilityAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_release_capability = 0,
    CXX11_clang_release_capability = 1,
    GNU_release_shared_capability = 2,
    CXX11_clang_release_shared_capability = 3,
    GNU_release_generic_capability = 4,
    CXX11_clang_release_generic_capability = 5,
    GNU_unlock_function = 6
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
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

static bool checkRenderScriptKernelAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkRenderScriptLangOpts(Sema &S, const AttributeList &Attr) {
  if (S.LangOpts.RenderScript)
    return true;

  S.Diag(Attr.getLoc(), diag::warn_attribute_ignored) << Attr.getName();
  return false;
}

static bool checkReqdWorkGroupSizeAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkRequireConstantInitAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isGlobalVar(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedStaticOrTLSVar;
    return false;
  }
  return true;
}

static bool checkCPlusPlusLangOpts(Sema &S, const AttributeList &Attr) {
  if (S.LangOpts.CPlusPlus)
    return true;

  S.Diag(Attr.getLoc(), diag::warn_attribute_ignored) << Attr.getName();
  return false;
}

static bool checkRequiresCapabilityAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static unsigned RequiresCapabilityAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_requires_capability = 0,
    CXX11_clang_requires_capability = 1,
    GNU_exclusive_locks_required = 2,
    GNU_requires_shared_capability = 3,
    CXX11_clang_requires_shared_capability = 4,
    GNU_shared_locks_required = 5
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_requires_capability;
    case 1: return CXX11_clang_requires_capability;
    case 2: return GNU_exclusive_locks_required;
    case 3: return GNU_requires_shared_capability;
    case 4: return CXX11_clang_requires_shared_capability;
    case 5: return GNU_shared_locks_required;
  }
}

static bool checkRestrictAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static unsigned RestrictAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    Declspec_restrict = 0,
    GNU_malloc = 1,
    CXX11_gnu_malloc = 2
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return Declspec_restrict;
    case 1: return GNU_malloc;
    case 2: return CXX11_gnu_malloc;
  }
}

static bool checkReturnTypestateAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D) && !isa<ParmVarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionMethodOrParameter;
    return false;
  }
  return true;
}

static bool checkReturnsNonNullAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCMethodDecl>(D) && !isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionOrMethod;
    return false;
  }
  return true;
}

static bool checkReturnsTwiceAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkScopedLockableAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<RecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << (S.getLangOpts().CPlusPlus ? ExpectedStructOrUnionOrClass : ExpectedStructOrUnion);
    return false;
  }
  return true;
}

static bool checkSectionAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D) && !isGlobalVar(D) && !isa<ObjCMethodDecl>(D) && !isa<ObjCPropertyDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionGlobalVarMethodOrProperty;
    return false;
  }
  return true;
}

static unsigned SectionAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_section = 0,
    CXX11_gnu_section = 1,
    Declspec_allocate = 2
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_section;
    case 1: return CXX11_gnu_section;
    case 2: return Declspec_allocate;
  }
}

static bool checkSetTypestateAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXMethodDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedMethod;
    return false;
  }
  return true;
}

static bool checkSharedTrylockFunctionAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkSwiftContextAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ParmVarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedParameter;
    return false;
  }
  return true;
}

static bool checkSwiftErrorResultAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ParmVarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedParameter;
    return false;
  }
  return true;
}

static bool checkSwiftIndirectResultAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ParmVarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedParameter;
    return false;
  }
  return true;
}

static bool isTLSVar(const Decl *D) {
  if (const auto *S = dyn_cast<VarDecl>(D))
    return S->getTLSKind() != 0;
  return false;
}

static bool checkTLSModelAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isTLSVar(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedTLSVar;
    return false;
  }
  return true;
}

static bool checkTargetAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkTestTypestateAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXMethodDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedMethod;
    return false;
  }
  return true;
}

static bool checkThreadAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariable;
    return false;
  }
  return true;
}

static bool checkNotCPlusPlusLangOpts(Sema &S, const AttributeList &Attr) {
  if (!S.LangOpts.CPlusPlus)
    return true;

  S.Diag(Attr.getLoc(), diag::warn_attribute_ignored) << Attr.getName();
  return false;
}

static bool checkTryAcquireCapabilityAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static unsigned TryAcquireCapabilityAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_try_acquire_capability = 0,
    CXX11_clang_try_acquire_capability = 1,
    GNU_try_acquire_shared_capability = 2,
    CXX11_clang_try_acquire_shared_capability = 3
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_try_acquire_capability;
    case 1: return CXX11_clang_try_acquire_capability;
    case 2: return GNU_try_acquire_shared_capability;
    case 3: return CXX11_clang_try_acquire_shared_capability;
  }
}

static bool checkUnusedAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D) && !isa<ObjCIvarDecl>(D) && !isa<TypeDecl>(D) && !isa<EnumDecl>(D) && !isa<EnumConstantDecl>(D) && !isa<LabelDecl>(D) && !isa<FieldDecl>(D) && !isa<ObjCMethodDecl>(D) && !isFunctionLike(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedForMaybeUnused;
    return false;
  }
  return true;
}

static unsigned UnusedAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    CXX11_maybe_unused = 0,
    GNU_unused = 1,
    CXX11_gnu_unused = 2
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return CXX11_maybe_unused;
    case 1: return GNU_unused;
    case 2: return CXX11_gnu_unused;
  }
}

static bool checkUuidAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<RecordDecl>(D) && !isa<EnumDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedEnumOrClass;
    return false;
  }
  return true;
}

static bool checkMicrosoftExtBorlandLangOpts(Sema &S, const AttributeList &Attr) {
  if (S.LangOpts.MicrosoftExt || S.LangOpts.Borland)
    return true;

  S.Diag(Attr.getLoc(), diag::warn_attribute_ignored) << Attr.getName();
  return false;
}

static bool checkVecReturnAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXRecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedClass;
    return false;
  }
  return true;
}

static bool checkVecTypeHintAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool checkWarnUnusedAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<RecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << (S.getLangOpts().CPlusPlus ? ExpectedStructOrUnionOrClass : ExpectedStructOrUnion);
    return false;
  }
  return true;
}

static bool checkWarnUnusedResultAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<ObjCMethodDecl>(D) && !isa<EnumDecl>(D) && !isa<CXXRecordDecl>(D) && !isFunctionLike(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionMethodEnumOrClass;
    return false;
  }
  return true;
}

static unsigned WarnUnusedResultAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    CXX11_nodiscard = 0,
    CXX11_clang_warn_unused_result = 1,
    GNU_warn_unused_result = 2,
    CXX11_gnu_warn_unused_result = 3
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return CXX11_nodiscard;
    case 1: return CXX11_clang_warn_unused_result;
    case 2: return GNU_warn_unused_result;
    case 3: return CXX11_gnu_warn_unused_result;
  }
}

static bool checkWeakAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D) && !isa<FunctionDecl>(D) && !isa<CXXRecordDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << (S.getLangOpts().CPlusPlus ? ExpectedFunctionVariableOrClass : ExpectedVariableOrFunction);
    return false;
  }
  return true;
}

static bool checkWeakRefAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<VarDecl>(D) && !isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedVariableOrFunction;
    return false;
  }
  return true;
}

static bool checkWorkGroupSizeHintAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::err_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunction;
    return false;
  }
  return true;
}

static bool isTargetx86(const TargetInfo &Target) {
  const llvm::Triple &T = Target.getTriple();
  return (T.getArch() == llvm::Triple::x86);
}

static bool checkXRayInstrumentAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXMethodDecl>(D) && !isa<ObjCMethodDecl>(D) && !isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionOrMethod;
    return false;
  }
  return true;
}

static unsigned XRayInstrumentAttrSpellingMap(const AttributeList &Attr) {
  enum Spelling {
    GNU_xray_always_instrument = 0,
    CXX11_clang_xray_always_instrument = 1,
    GNU_xray_never_instrument = 2,
    CXX11_clang_xray_never_instrument = 3
  };

  unsigned Idx = Attr.getAttributeSpellingListIndex();
  switch (Idx) {
    default: llvm_unreachable("Unknown spelling list index");
    case 0: return GNU_xray_always_instrument;
    case 1: return CXX11_clang_xray_always_instrument;
    case 2: return GNU_xray_never_instrument;
    case 3: return CXX11_clang_xray_never_instrument;
  }
}

static bool checkXRayLogArgsAppertainsTo(Sema &S, const AttributeList &Attr, const Decl *D) {
  if (!isa<CXXMethodDecl>(D) && !isa<ObjCMethodDecl>(D) && !isa<FunctionDecl>(D)) {
    S.Diag(Attr.getLoc(), diag::warn_attribute_wrong_decl_type)
      << Attr.getName() << ExpectedFunctionOrMethod;
    return false;
  }
  return true;
}

static const ParsedAttrInfo AttrInfoMap[AttributeList::UnknownAttribute + 1] = {
  { 2, 0, 0, 0, 0, 0, 0, checkAMDGPUFlatWorkGroupSizeAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AMDGPUFlatWorkGroupSize
  { 1, 0, 0, 0, 0, 0, 0, checkAMDGPUNumSGPRAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AMDGPUNumSGPR
  { 1, 0, 0, 0, 0, 0, 0, checkAMDGPUNumVGPRAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AMDGPUNumVGPR
  { 1, 1, 0, 0, 0, 0, 0, checkAMDGPUWavesPerEUAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AMDGPUWavesPerEU
  { 0, 1, 1, 1, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, isTargetarmthumbarmebthumbebavrx86x86_64msp430mipsmipsel, defaultSpellingIndexToSemanticSpelling },  // AT_Interrupt
  { 0, 0, 0, 1, 0, 0, 0, checkAVRSignalAppertainsTo, defaultDiagnoseLangOpts, isTargetavr, defaultSpellingIndexToSemanticSpelling },  // AT_AVRSignal
  { 0, 15, 0, 0, 0, 0, 1, checkAbiTagAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AbiTag
  { 0, 15, 0, 0, 0, 0, 0, checkAcquireCapabilityAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, AcquireCapabilityAttrSpellingMap },  // AT_AcquireCapability
  { 0, 15, 0, 0, 0, 0, 0, checkAcquiredAfterAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AcquiredAfter
  { 0, 15, 0, 0, 0, 0, 0, checkAcquiredBeforeAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AcquiredBefore
  { 1, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AddressSpace
  { 1, 0, 0, 0, 0, 0, 1, checkAliasAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Alias
  { 1, 0, 0, 0, 0, 0, 0, checkAlignValueAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AlignValue
  { 0, 1, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, AlignedAttrSpellingMap },  // AT_Aligned
  { 1, 0, 0, 0, 0, 0, 1, checkAllocAlignAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AllocAlign
  { 1, 1, 0, 0, 0, 0, 1, checkAllocSizeAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AllocSize
  { 0, 0, 0, 0, 0, 0, 1, checkAlwaysInlineAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, AlwaysInlineAttrSpellingMap },  // AT_AlwaysInline
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AnalyzerNoReturn
  { 1, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Annotate
  { 0, 0, 0, 0, 0, 0, 0, checkArcWeakrefUnavailableAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ArcWeakrefUnavailable
  { 4, 0, 1, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, ArgumentWithTypeTagAttrSpellingMap },  // AT_ArgumentWithTypeTag
  { 1, 0, 0, 0, 0, 0, 0, checkAssertCapabilityAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, AssertCapabilityAttrSpellingMap },  // AT_AssertCapability
  { 0, 15, 0, 0, 0, 0, 0, checkAssertExclusiveLockAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AssertExclusiveLock
  { 0, 15, 0, 0, 0, 0, 0, checkAssertSharedLockAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AssertSharedLock
  { 1, 1, 0, 0, 0, 0, 1, checkAssumeAlignedAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_AssumeAligned
  { 8, 0, 1, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Availability
  { 1, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Blocks
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CDecl
  { 0, 0, 0, 0, 0, 0, 0, checkCFAuditedTransferAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CFAuditedTransfer
  { 0, 0, 0, 0, 0, 0, 0, checkCFConsumedAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CFConsumed
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CFReturnsNotRetained
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CFReturnsRetained
  { 0, 0, 0, 0, 0, 0, 0, checkCFUnknownTransferAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CFUnknownTransfer
  { 0, 0, 0, 0, 0, 0, 0, checkCUDAConstantAppertainsTo, checkCUDALangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CUDAConstant
  { 0, 0, 0, 0, 0, 0, 0, checkCUDADeviceAppertainsTo, checkCUDALangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CUDADevice
  { 0, 0, 0, 0, 0, 0, 0, checkCUDAGlobalAppertainsTo, checkCUDALangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CUDAGlobal
  { 0, 0, 0, 0, 0, 0, 0, checkCUDAHostAppertainsTo, checkCUDALangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CUDAHost
  { 0, 0, 0, 0, 0, 0, 0, checkCUDAInvalidTargetAppertainsTo, checkCUDALangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CUDAInvalidTarget
  { 1, 1, 0, 0, 0, 0, 0, checkCUDALaunchBoundsAppertainsTo, checkCUDALangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CUDALaunchBounds
  { 0, 0, 0, 0, 0, 0, 0, checkCUDASharedAppertainsTo, checkCUDALangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CUDAShared
  { 0, 0, 0, 0, 0, 0, 0, checkCXX11NoReturnAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CXX11NoReturn
  { 0, 15, 0, 0, 0, 0, 0, checkCallableWhenAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CallableWhen
  { 1, 0, 0, 0, 0, 0, 0, checkCapabilityAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, CapabilityAttrSpellingMap },  // AT_Capability
  { 0, 0, 0, 0, 0, 0, 0, checkCarriesDependencyAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_CarriesDependency
  { 1, 0, 0, 0, 0, 0, 1, checkCleanupAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Cleanup
  { 0, 0, 0, 0, 0, 0, 1, checkColdAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Cold
  { 0, 0, 0, 0, 0, 0, 1, checkCommonAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Common
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Const
  { 0, 1, 0, 0, 0, 0, 1, checkConstructorAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Constructor
  { 1, 0, 0, 0, 0, 0, 0, checkConsumableAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Consumable
  { 0, 0, 0, 0, 0, 0, 0, checkConsumableAutoCastAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ConsumableAutoCast
  { 0, 0, 0, 0, 0, 0, 0, checkConsumableSetOnReadAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ConsumableSetOnRead
  { 0, 0, 0, 0, 0, 0, 0, checkConvergentAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Convergent
  { 0, 0, 0, 1, 0, 0, 1, checkDLLExportAppertainsTo, defaultDiagnoseLangOpts, isTargetx86x86_64armthumbWin32, defaultSpellingIndexToSemanticSpelling },  // AT_DLLExport
  { 0, 0, 0, 1, 0, 0, 1, checkDLLImportAppertainsTo, defaultDiagnoseLangOpts, isTargetx86x86_64armthumbWin32, defaultSpellingIndexToSemanticSpelling },  // AT_DLLImport
  { 0, 2, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Deprecated
  { 0, 1, 0, 0, 0, 0, 1, checkDestructorAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Destructor
  { 3, 0, 0, 0, 0, 0, 0, checkDiagnoseIfAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_DiagnoseIf
  { 0, 0, 0, 0, 0, 0, 0, checkDisableTailCallsAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_DisableTailCalls
  { 0, 0, 0, 1, 0, 0, 0, checkEmptyBasesAppertainsTo, defaultDiagnoseLangOpts, isTargetx86x86_64armthumbMicrosoft, defaultSpellingIndexToSemanticSpelling },  // AT_EmptyBases
  { 2, 0, 0, 0, 0, 0, 0, checkEnableIfAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_EnableIf
  { 1, 0, 0, 0, 0, 0, 0, checkEnumExtensibilityAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_EnumExtensibility
  { 1, 15, 0, 0, 0, 0, 0, checkExclusiveTrylockFunctionAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ExclusiveTrylockFunction
  { 1, 0, 0, 0, 0, 0, 0, checkExtVectorTypeAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ExtVectorType
  { 0, 3, 1, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ExternalSourceSymbol
  { 0, 0, 0, 0, 0, 1, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_FallThrough
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_FastCall
  { 0, 0, 0, 0, 0, 0, 0, checkFlagEnumAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_FlagEnum
  { 0, 0, 0, 0, 0, 0, 1, checkFlattenAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Flatten
  { 3, 0, 0, 0, 0, 0, 1, checkFormatAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Format
  { 1, 0, 0, 0, 0, 0, 1, checkFormatArgAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_FormatArg
  { 0, 0, 0, 0, 0, 0, 1, checkGNUInlineAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_GNUInline
  { 1, 0, 0, 0, 0, 0, 0, checkGuardedByAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_GuardedBy
  { 0, 0, 0, 0, 0, 0, 0, checkGuardedVarAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_GuardedVar
  { 0, 0, 0, 0, 0, 0, 1, checkHotAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Hot
  { 0, 0, 0, 0, 0, 0, 0, checkIBActionAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_IBAction
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_IBOutlet
  { 0, 1, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_IBOutletCollection
  { 1, 0, 0, 0, 0, 0, 1, checkIFuncAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_IFunc
  { 1, 0, 0, 0, 0, 0, 0, checkInitPriorityAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_InitPriority
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_IntelOclBicc
  { 0, 0, 0, 0, 0, 0, 0, checkInternalLinkageAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_InternalLinkage
  { 0, 0, 0, 0, 0, 0, 0, checkLTOVisibilityPublicAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_LTOVisibilityPublic
  { 1, 0, 0, 1, 0, 0, 0, checkLayoutVersionAppertainsTo, defaultDiagnoseLangOpts, isTargetx86x86_64armthumbMicrosoft, defaultSpellingIndexToSemanticSpelling },  // AT_LayoutVersion
  { 1, 0, 0, 0, 0, 0, 0, checkLockReturnedAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_LockReturned
  { 0, 0, 0, 0, 0, 0, 0, checkLockableAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Lockable
  { 0, 15, 0, 0, 0, 0, 0, checkLocksExcludedAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_LocksExcluded
  { 3, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, LoopHintAttrSpellingMap },  // AT_LoopHint
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_MSABI
  { 0, 1, 0, 0, 0, 0, 0, defaultAppertainsTo, checkMicrosoftExtLangOpts, defaultTargetRequirements, MSInheritanceAttrSpellingMap },  // AT_MSInheritance
  { 0, 0, 0, 1, 0, 0, 0, checkMSNoVTableAppertainsTo, defaultDiagnoseLangOpts, isTargetx86x86_64armthumbMicrosoft, defaultSpellingIndexToSemanticSpelling },  // AT_MSNoVTable
  { 0, 0, 0, 0, 0, 0, 1, checkMSStructAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_MSStruct
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_MayAlias
  { 0, 0, 0, 0, 0, 0, 0, checkMinSizeAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_MinSize
  { 0, 0, 0, 1, 0, 0, 1, checkMips16AppertainsTo, defaultDiagnoseLangOpts, isTargetmipsmipsel, defaultSpellingIndexToSemanticSpelling },  // AT_Mips16
  { 1, 0, 0, 0, 0, 0, 1, checkModeAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Mode
  { 0, 0, 0, 0, 0, 0, 0, checkNSConsumedAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NSConsumed
  { 0, 0, 0, 0, 0, 0, 0, checkNSConsumesSelfAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NSConsumesSelf
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NSReturnsAutoreleased
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NSReturnsNotRetained
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NSReturnsRetained
  { 0, 0, 0, 0, 0, 0, 1, checkNakedAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Naked
  { 1, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NeonPolyVectorType
  { 1, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NeonVectorType
  { 0, 0, 0, 0, 0, 0, 0, checkNoAliasAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoAlias
  { 0, 0, 0, 0, 0, 0, 1, checkNoCommonAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoCommon
  { 0, 0, 0, 0, 0, 0, 1, checkNoDebugAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoDebug
  { 0, 0, 0, 0, 0, 0, 0, checkNoDuplicateAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoDuplicate
  { 0, 0, 0, 0, 0, 0, 1, checkNoInlineAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoInline
  { 0, 0, 0, 0, 0, 0, 1, checkNoInstrumentFunctionAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoInstrumentFunction
  { 0, 0, 0, 1, 0, 0, 1, checkNoMips16AppertainsTo, defaultDiagnoseLangOpts, isTargetmipsmipsel, defaultSpellingIndexToSemanticSpelling },  // AT_NoMips16
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoReturn
  { 0, 15, 0, 0, 0, 0, 0, checkNoSanitizeAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoSanitize
  { 0, 0, 0, 0, 0, 0, 1, checkNoSanitizeSpecificAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoSanitizeSpecific
  { 0, 0, 0, 0, 0, 0, 1, checkNoSplitStackAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoSplitStack
  { 0, 0, 0, 0, 0, 0, 0, checkNoThreadSafetyAnalysisAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoThreadSafetyAnalysis
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NoThrow
  { 0, 15, 0, 0, 0, 0, 1, checkNonNullAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NonNull
  { 0, 0, 0, 0, 0, 0, 0, checkNotTailCalledAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_NotTailCalled
  { 0, 0, 0, 0, 0, 0, 0, checkObjCBoxableAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCBoxable
  { 1, 0, 0, 0, 0, 0, 0, checkObjCBridgeAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCBridge
  { 1, 0, 0, 0, 0, 0, 0, checkObjCBridgeMutableAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCBridgeMutable
  { 1, 2, 1, 0, 0, 0, 0, checkObjCBridgeRelatedAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCBridgeRelated
  { 0, 0, 0, 0, 0, 0, 0, checkObjCDesignatedInitializerAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCDesignatedInitializer
  { 0, 0, 0, 0, 0, 0, 0, checkObjCExceptionAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCException
  { 0, 0, 0, 0, 0, 0, 0, checkObjCExplicitProtocolImplAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCExplicitProtocolImpl
  { 1, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCGC
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCIndependentClass
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCKindOf
  { 1, 0, 0, 0, 0, 0, 0, checkObjCMethodFamilyAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCMethodFamily
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCNSObject
  { 1, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCOwnership
  { 0, 0, 0, 0, 0, 0, 0, checkObjCPreciseLifetimeAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCPreciseLifetime
  { 0, 0, 0, 0, 0, 0, 0, checkObjCRequiresPropertyDefsAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCRequiresPropertyDefs
  { 0, 0, 0, 0, 0, 0, 0, checkObjCRequiresSuperAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCRequiresSuper
  { 0, 0, 0, 0, 0, 0, 0, checkObjCReturnsInnerPointerAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCReturnsInnerPointer
  { 0, 0, 0, 0, 0, 0, 0, checkObjCRootClassAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCRootClass
  { 1, 0, 0, 0, 0, 0, 0, checkObjCRuntimeNameAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCRuntimeName
  { 0, 0, 0, 0, 0, 0, 0, checkObjCRuntimeVisibleAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCRuntimeVisible
  { 0, 0, 0, 0, 0, 0, 0, checkObjCSubclassingRestrictedAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ObjCSubclassingRestricted
  { 0, 0, 0, 0, 0, 0, 0, checkOpenCLAccessAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, OpenCLAccessAttrSpellingMap },  // AT_OpenCLAccess
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_OpenCLConstantAddressSpace
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_OpenCLGenericAddressSpace
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_OpenCLGlobalAddressSpace
  { 0, 0, 0, 0, 0, 0, 0, checkOpenCLKernelAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_OpenCLKernel
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_OpenCLLocalAddressSpace
  { 0, 0, 0, 0, 0, 0, 0, checkOpenCLNoSVMAppertainsTo, checkOpenCLLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_OpenCLNoSVM
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_OpenCLPrivateAddressSpace
  { 1, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_OpenCLUnrollHint
  { 0, 0, 0, 0, 0, 0, 0, checkOptimizeNoneAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_OptimizeNone
  { 0, 0, 0, 0, 0, 0, 0, checkOverloadableAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Overloadable
  { 1, 15, 0, 0, 0, 0, 0, checkOwnershipAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, OwnershipAttrSpellingMap },  // AT_Ownership
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Packed
  { 1, 0, 0, 0, 0, 0, 0, checkParamTypestateAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ParamTypestate
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Pascal
  { 1, 0, 0, 0, 0, 0, 0, checkPassObjectSizeAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_PassObjectSize
  { 1, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Pcs
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_PreserveAll
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_PreserveMost
  { 1, 0, 0, 0, 0, 0, 0, checkPtGuardedByAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_PtGuardedBy
  { 0, 0, 0, 0, 0, 0, 0, checkPtGuardedVarAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_PtGuardedVar
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Ptr32
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Ptr64
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Pure
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_RegCall
  { 1, 0, 0, 0, 1, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Regparm
  { 0, 15, 0, 0, 0, 0, 0, checkReleaseCapabilityAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, ReleaseCapabilityAttrSpellingMap },  // AT_ReleaseCapability
  { 0, 0, 0, 0, 0, 0, 0, checkRenderScriptKernelAppertainsTo, checkRenderScriptLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_RenderScriptKernel
  { 3, 0, 0, 0, 0, 0, 0, checkReqdWorkGroupSizeAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ReqdWorkGroupSize
  { 0, 0, 0, 0, 0, 0, 0, checkRequireConstantInitAppertainsTo, checkCPlusPlusLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_RequireConstantInit
  { 0, 15, 0, 0, 0, 0, 0, checkRequiresCapabilityAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, RequiresCapabilityAttrSpellingMap },  // AT_RequiresCapability
  { 0, 0, 0, 0, 0, 0, 1, checkRestrictAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, RestrictAttrSpellingMap },  // AT_Restrict
  { 1, 0, 0, 0, 0, 0, 0, checkReturnTypestateAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ReturnTypestate
  { 0, 0, 0, 0, 0, 0, 1, checkReturnsNonNullAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ReturnsNonNull
  { 0, 0, 0, 0, 0, 0, 1, checkReturnsTwiceAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ReturnsTwice
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_SPtr
  { 0, 0, 0, 0, 0, 0, 0, checkScopedLockableAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ScopedLockable
  { 1, 0, 0, 0, 0, 0, 1, checkSectionAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, SectionAttrSpellingMap },  // AT_Section
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, checkMicrosoftExtLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_SelectAny
  { 0, 2, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Sentinel
  { 1, 0, 0, 0, 0, 0, 0, checkSetTypestateAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_SetTypestate
  { 1, 15, 0, 0, 0, 0, 0, checkSharedTrylockFunctionAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_SharedTrylockFunction
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_StdCall
  { 0, 15, 0, 0, 0, 1, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Suppress
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_SwiftCall
  { 0, 0, 0, 0, 0, 0, 1, checkSwiftContextAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_SwiftContext
  { 0, 0, 0, 0, 0, 0, 1, checkSwiftErrorResultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_SwiftErrorResult
  { 0, 0, 0, 0, 0, 0, 1, checkSwiftIndirectResultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_SwiftIndirectResult
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_SysVABI
  { 1, 0, 0, 0, 0, 0, 1, checkTLSModelAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_TLSModel
  { 1, 0, 0, 0, 0, 0, 1, checkTargetAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Target
  { 1, 0, 0, 0, 0, 0, 0, checkTestTypestateAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_TestTypestate
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_ThisCall
  { 0, 0, 0, 0, 0, 0, 0, checkThreadAppertainsTo, checkMicrosoftExtLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Thread
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, checkNotCPlusPlusLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_TransparentUnion
  { 1, 15, 0, 0, 0, 0, 0, checkTryAcquireCapabilityAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, TryAcquireCapabilityAttrSpellingMap },  // AT_TryAcquireCapability
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_TypeNonNull
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_TypeNullUnspecified
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_TypeNullable
  { 4, 0, 1, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_TypeTagForDatatype
  { 1, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_TypeVisibility
  { 0, 0, 0, 0, 1, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_UPtr
  { 0, 1, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Unavailable
  { 0, 0, 0, 0, 0, 0, 1, checkUnusedAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, UnusedAttrSpellingMap },  // AT_Unused
  { 0, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Used
  { 1, 0, 0, 0, 0, 0, 0, checkUuidAppertainsTo, checkMicrosoftExtBorlandLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Uuid
  { 0, 0, 0, 0, 0, 0, 0, checkVecReturnAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_VecReturn
  { 1, 0, 0, 0, 0, 0, 0, checkVecTypeHintAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_VecTypeHint
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_VectorCall
  { 1, 0, 0, 0, 1, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_VectorSize
  { 1, 0, 0, 0, 0, 0, 1, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Visibility
  { 0, 0, 0, 0, 0, 0, 0, checkWarnUnusedAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_WarnUnused
  { 0, 0, 0, 0, 0, 0, 1, checkWarnUnusedResultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, WarnUnusedResultAttrSpellingMap },  // AT_WarnUnusedResult
  { 0, 0, 0, 0, 0, 0, 1, checkWeakAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_Weak
  { 0, 0, 0, 0, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_WeakImport
  { 0, 1, 0, 0, 0, 0, 1, checkWeakRefAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_WeakRef
  { 3, 0, 0, 0, 0, 0, 0, checkWorkGroupSizeHintAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling },  // AT_WorkGroupSizeHint
  { 0, 0, 0, 1, 0, 0, 0, defaultAppertainsTo, defaultDiagnoseLangOpts, isTargetx86, defaultSpellingIndexToSemanticSpelling },  // AT_X86ForceAlignArgPointer
  { 0, 0, 0, 0, 0, 0, 0, checkXRayInstrumentAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, XRayInstrumentAttrSpellingMap },  // AT_XRayInstrument
  { 1, 0, 0, 0, 0, 0, 0, checkXRayLogArgsAppertainsTo, defaultDiagnoseLangOpts, defaultTargetRequirements, defaultSpellingIndexToSemanticSpelling }  // AT_XRayLogArgs
};

