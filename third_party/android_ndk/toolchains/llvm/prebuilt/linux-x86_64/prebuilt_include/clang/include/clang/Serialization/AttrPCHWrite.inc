/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Attribute serialization code                                               *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

  switch (A->getKind()) {
  case attr::AMDGPUFlatWorkGroupSize: {
    const auto *SA = cast<AMDGPUFlatWorkGroupSizeAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getMin());
    Record.push_back(SA->getMax());
    break;
  }
  case attr::AMDGPUNumSGPR: {
    const auto *SA = cast<AMDGPUNumSGPRAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getNumSGPR());
    break;
  }
  case attr::AMDGPUNumVGPR: {
    const auto *SA = cast<AMDGPUNumVGPRAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getNumVGPR());
    break;
  }
  case attr::AMDGPUWavesPerEU: {
    const auto *SA = cast<AMDGPUWavesPerEUAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getMin());
    Record.push_back(SA->getMax());
    break;
  }
  case attr::ARMInterrupt: {
    const auto *SA = cast<ARMInterruptAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getInterrupt());
    break;
  }
  case attr::AVRInterrupt: {
    const auto *SA = cast<AVRInterruptAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::AVRSignal: {
    const auto *SA = cast<AVRSignalAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::AbiTag: {
    const auto *SA = cast<AbiTagAttr>(A);
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->tags_size());
    for (auto &Val : SA->tags())
      Record.AddString(Val);
    break;
  }
  case attr::AcquireCapability: {
    const auto *SA = cast<AcquireCapabilityAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.AddStmt(Val);
    break;
  }
  case attr::AcquiredAfter: {
    const auto *SA = cast<AcquiredAfterAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.AddStmt(Val);
    break;
  }
  case attr::AcquiredBefore: {
    const auto *SA = cast<AcquiredBeforeAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.AddStmt(Val);
    break;
  }
  case attr::Alias: {
    const auto *SA = cast<AliasAttr>(A);
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getAliasee());
    break;
  }
  case attr::AlignMac68k: {
    const auto *SA = cast<AlignMac68kAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::AlignValue: {
    const auto *SA = cast<AlignValueAttr>(A);
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getAlignment());
    break;
  }
  case attr::Aligned: {
    const auto *SA = cast<AlignedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->isAlignmentExpr());
    if (SA->isAlignmentExpr())
      Record.AddStmt(SA->getAlignmentExpr());
    else
      Record.AddTypeSourceInfo(SA->getAlignmentType());
    break;
  }
  case attr::AllocAlign: {
    const auto *SA = cast<AllocAlignAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getParamIndex());
    break;
  }
  case attr::AllocSize: {
    const auto *SA = cast<AllocSizeAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getElemSizeParam());
    Record.push_back(SA->getNumElemsParam());
    break;
  }
  case attr::AlwaysInline: {
    const auto *SA = cast<AlwaysInlineAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::AnalyzerNoReturn: {
    const auto *SA = cast<AnalyzerNoReturnAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Annotate: {
    const auto *SA = cast<AnnotateAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getAnnotation());
    break;
  }
  case attr::AnyX86Interrupt: {
    const auto *SA = cast<AnyX86InterruptAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ArcWeakrefUnavailable: {
    const auto *SA = cast<ArcWeakrefUnavailableAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ArgumentWithTypeTag: {
    const auto *SA = cast<ArgumentWithTypeTagAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddIdentifierRef(SA->getArgumentKind());
    Record.push_back(SA->getArgumentIdx());
    Record.push_back(SA->getTypeTagIdx());
    Record.push_back(SA->getIsPointer());
    break;
  }
  case attr::AsmLabel: {
    const auto *SA = cast<AsmLabelAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getLabel());
    break;
  }
  case attr::AssertCapability: {
    const auto *SA = cast<AssertCapabilityAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getExpr());
    break;
  }
  case attr::AssertExclusiveLock: {
    const auto *SA = cast<AssertExclusiveLockAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.AddStmt(Val);
    break;
  }
  case attr::AssertSharedLock: {
    const auto *SA = cast<AssertSharedLockAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.AddStmt(Val);
    break;
  }
  case attr::AssumeAligned: {
    const auto *SA = cast<AssumeAlignedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getAlignment());
    Record.AddStmt(SA->getOffset());
    break;
  }
  case attr::Availability: {
    const auto *SA = cast<AvailabilityAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddIdentifierRef(SA->getPlatform());
    Record.AddVersionTuple(SA->getIntroduced());
    Record.AddVersionTuple(SA->getDeprecated());
    Record.AddVersionTuple(SA->getObsoleted());
    Record.push_back(SA->getUnavailable());
    Record.AddString(SA->getMessage());
    Record.push_back(SA->getStrict());
    Record.AddString(SA->getReplacement());
    break;
  }
  case attr::Blocks: {
    const auto *SA = cast<BlocksAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getType());
    break;
  }
  case attr::C11NoReturn: {
    const auto *SA = cast<C11NoReturnAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CDecl: {
    const auto *SA = cast<CDeclAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CFAuditedTransfer: {
    const auto *SA = cast<CFAuditedTransferAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CFConsumed: {
    const auto *SA = cast<CFConsumedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CFReturnsNotRetained: {
    const auto *SA = cast<CFReturnsNotRetainedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CFReturnsRetained: {
    const auto *SA = cast<CFReturnsRetainedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CFUnknownTransfer: {
    const auto *SA = cast<CFUnknownTransferAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CUDAConstant: {
    const auto *SA = cast<CUDAConstantAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CUDADevice: {
    const auto *SA = cast<CUDADeviceAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CUDAGlobal: {
    const auto *SA = cast<CUDAGlobalAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CUDAHost: {
    const auto *SA = cast<CUDAHostAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CUDAInvalidTarget: {
    const auto *SA = cast<CUDAInvalidTargetAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CUDALaunchBounds: {
    const auto *SA = cast<CUDALaunchBoundsAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getMaxThreads());
    Record.AddStmt(SA->getMinBlocks());
    break;
  }
  case attr::CUDAShared: {
    const auto *SA = cast<CUDASharedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CXX11NoReturn: {
    const auto *SA = cast<CXX11NoReturnAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CallableWhen: {
    const auto *SA = cast<CallableWhenAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->callableStates_size());
    for (CallableWhenAttr::callableStates_iterator i = SA->callableStates_begin(), e = SA->callableStates_end(); i != e; ++i)
      Record.push_back((*i));
    break;
  }
  case attr::Capability: {
    const auto *SA = cast<CapabilityAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getName());
    break;
  }
  case attr::CapturedRecord: {
    const auto *SA = cast<CapturedRecordAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::CarriesDependency: {
    const auto *SA = cast<CarriesDependencyAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Cleanup: {
    const auto *SA = cast<CleanupAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddDeclRef(SA->getFunctionDecl());
    break;
  }
  case attr::Cold: {
    const auto *SA = cast<ColdAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Common: {
    const auto *SA = cast<CommonAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Const: {
    const auto *SA = cast<ConstAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Constructor: {
    const auto *SA = cast<ConstructorAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getPriority());
    break;
  }
  case attr::Consumable: {
    const auto *SA = cast<ConsumableAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getDefaultState());
    break;
  }
  case attr::ConsumableAutoCast: {
    const auto *SA = cast<ConsumableAutoCastAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ConsumableSetOnRead: {
    const auto *SA = cast<ConsumableSetOnReadAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Convergent: {
    const auto *SA = cast<ConvergentAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::DLLExport: {
    const auto *SA = cast<DLLExportAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::DLLImport: {
    const auto *SA = cast<DLLImportAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Deprecated: {
    const auto *SA = cast<DeprecatedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getMessage());
    Record.AddString(SA->getReplacement());
    break;
  }
  case attr::Destructor: {
    const auto *SA = cast<DestructorAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getPriority());
    break;
  }
  case attr::DiagnoseIf: {
    const auto *SA = cast<DiagnoseIfAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getCond());
    Record.AddString(SA->getMessage());
Record.push_back(SA->getDiagnosticType());
    Record.push_back(SA->getArgDependent());
    Record.AddDeclRef(SA->getParent());
    break;
  }
  case attr::DisableTailCalls: {
    const auto *SA = cast<DisableTailCallsAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::EmptyBases: {
    const auto *SA = cast<EmptyBasesAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::EnableIf: {
    const auto *SA = cast<EnableIfAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getCond());
    Record.AddString(SA->getMessage());
    break;
  }
  case attr::EnumExtensibility: {
    const auto *SA = cast<EnumExtensibilityAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getExtensibility());
    break;
  }
  case attr::ExclusiveTrylockFunction: {
    const auto *SA = cast<ExclusiveTrylockFunctionAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getSuccessValue());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.AddStmt(Val);
    break;
  }
  case attr::ExternalSourceSymbol: {
    const auto *SA = cast<ExternalSourceSymbolAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getLanguage());
    Record.AddString(SA->getDefinedIn());
    Record.push_back(SA->getGeneratedDeclaration());
    break;
  }
  case attr::FallThrough: {
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::FastCall: {
    const auto *SA = cast<FastCallAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Final: {
    const auto *SA = cast<FinalAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::FlagEnum: {
    const auto *SA = cast<FlagEnumAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Flatten: {
    const auto *SA = cast<FlattenAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Format: {
    const auto *SA = cast<FormatAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddIdentifierRef(SA->getType());
    Record.push_back(SA->getFormatIdx());
    Record.push_back(SA->getFirstArg());
    break;
  }
  case attr::FormatArg: {
    const auto *SA = cast<FormatArgAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getFormatIdx());
    break;
  }
  case attr::GNUInline: {
    const auto *SA = cast<GNUInlineAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::GuardedBy: {
    const auto *SA = cast<GuardedByAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getArg());
    break;
  }
  case attr::GuardedVar: {
    const auto *SA = cast<GuardedVarAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Hot: {
    const auto *SA = cast<HotAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::IBAction: {
    const auto *SA = cast<IBActionAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::IBOutlet: {
    const auto *SA = cast<IBOutletAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::IBOutletCollection: {
    const auto *SA = cast<IBOutletCollectionAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddTypeSourceInfo(SA->getInterfaceLoc());
    break;
  }
  case attr::IFunc: {
    const auto *SA = cast<IFuncAttr>(A);
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getResolver());
    break;
  }
  case attr::InitPriority: {
    const auto *SA = cast<InitPriorityAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getPriority());
    break;
  }
  case attr::InitSeg: {
    const auto *SA = cast<InitSegAttr>(A);
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getSection());
    break;
  }
  case attr::IntelOclBicc: {
    const auto *SA = cast<IntelOclBiccAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::InternalLinkage: {
    const auto *SA = cast<InternalLinkageAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::LTOVisibilityPublic: {
    const auto *SA = cast<LTOVisibilityPublicAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::LayoutVersion: {
    const auto *SA = cast<LayoutVersionAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getVersion());
    break;
  }
  case attr::LockReturned: {
    const auto *SA = cast<LockReturnedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getArg());
    break;
  }
  case attr::LocksExcluded: {
    const auto *SA = cast<LocksExcludedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.AddStmt(Val);
    break;
  }
  case attr::LoopHint: {
    const auto *SA = cast<LoopHintAttr>(A);
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getOption());
Record.push_back(SA->getState());
    Record.AddStmt(SA->getValue());
    break;
  }
  case attr::MSABI: {
    const auto *SA = cast<MSABIAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::MSInheritance: {
    const auto *SA = cast<MSInheritanceAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getBestCase());
    break;
  }
  case attr::MSNoVTable: {
    const auto *SA = cast<MSNoVTableAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::MSP430Interrupt: {
    const auto *SA = cast<MSP430InterruptAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getNumber());
    break;
  }
  case attr::MSStruct: {
    const auto *SA = cast<MSStructAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::MSVtorDisp: {
    const auto *SA = cast<MSVtorDispAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getVdm());
    break;
  }
  case attr::MaxFieldAlignment: {
    const auto *SA = cast<MaxFieldAlignmentAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getAlignment());
    break;
  }
  case attr::MayAlias: {
    const auto *SA = cast<MayAliasAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::MinSize: {
    const auto *SA = cast<MinSizeAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Mips16: {
    const auto *SA = cast<Mips16Attr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::MipsInterrupt: {
    const auto *SA = cast<MipsInterruptAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getInterrupt());
    break;
  }
  case attr::Mode: {
    const auto *SA = cast<ModeAttr>(A);
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddIdentifierRef(SA->getMode());
    break;
  }
  case attr::NSConsumed: {
    const auto *SA = cast<NSConsumedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NSConsumesSelf: {
    const auto *SA = cast<NSConsumesSelfAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NSReturnsAutoreleased: {
    const auto *SA = cast<NSReturnsAutoreleasedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NSReturnsNotRetained: {
    const auto *SA = cast<NSReturnsNotRetainedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NSReturnsRetained: {
    const auto *SA = cast<NSReturnsRetainedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Naked: {
    const auto *SA = cast<NakedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NoAlias: {
    const auto *SA = cast<NoAliasAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NoCommon: {
    const auto *SA = cast<NoCommonAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NoDebug: {
    const auto *SA = cast<NoDebugAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NoDuplicate: {
    const auto *SA = cast<NoDuplicateAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NoInline: {
    const auto *SA = cast<NoInlineAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NoInstrumentFunction: {
    const auto *SA = cast<NoInstrumentFunctionAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NoMips16: {
    const auto *SA = cast<NoMips16Attr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NoReturn: {
    const auto *SA = cast<NoReturnAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NoSanitize: {
    const auto *SA = cast<NoSanitizeAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->sanitizers_size());
    for (auto &Val : SA->sanitizers())
      Record.AddString(Val);
    break;
  }
  case attr::NoSplitStack: {
    const auto *SA = cast<NoSplitStackAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NoThreadSafetyAnalysis: {
    const auto *SA = cast<NoThreadSafetyAnalysisAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NoThrow: {
    const auto *SA = cast<NoThrowAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::NonNull: {
    const auto *SA = cast<NonNullAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.push_back(Val);
    break;
  }
  case attr::NotTailCalled: {
    const auto *SA = cast<NotTailCalledAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::OMPCaptureNoInit: {
    const auto *SA = cast<OMPCaptureNoInitAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::OMPDeclareSimdDecl: {
    const auto *SA = cast<OMPDeclareSimdDeclAttr>(A);
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getBranchState());
    Record.AddStmt(SA->getSimdlen());
    Record.push_back(SA->uniforms_size());
    for (auto &Val : SA->uniforms())
      Record.AddStmt(Val);
    Record.push_back(SA->aligneds_size());
    for (auto &Val : SA->aligneds())
      Record.AddStmt(Val);
    Record.push_back(SA->alignments_size());
    for (auto &Val : SA->alignments())
      Record.AddStmt(Val);
    Record.push_back(SA->linears_size());
    for (auto &Val : SA->linears())
      Record.AddStmt(Val);
    Record.push_back(SA->modifiers_size());
    for (auto &Val : SA->modifiers())
      Record.push_back(Val);
    Record.push_back(SA->steps_size());
    for (auto &Val : SA->steps())
      Record.AddStmt(Val);
    break;
  }
  case attr::OMPDeclareTargetDecl: {
    const auto *SA = cast<OMPDeclareTargetDeclAttr>(A);
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getMapType());
    break;
  }
  case attr::OMPThreadPrivateDecl: {
    const auto *SA = cast<OMPThreadPrivateDeclAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCBoxable: {
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCBridge: {
    const auto *SA = cast<ObjCBridgeAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddIdentifierRef(SA->getBridgedType());
    break;
  }
  case attr::ObjCBridgeMutable: {
    const auto *SA = cast<ObjCBridgeMutableAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddIdentifierRef(SA->getBridgedType());
    break;
  }
  case attr::ObjCBridgeRelated: {
    const auto *SA = cast<ObjCBridgeRelatedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddIdentifierRef(SA->getRelatedClass());
    Record.AddIdentifierRef(SA->getClassMethod());
    Record.AddIdentifierRef(SA->getInstanceMethod());
    break;
  }
  case attr::ObjCDesignatedInitializer: {
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCException: {
    const auto *SA = cast<ObjCExceptionAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCExplicitProtocolImpl: {
    const auto *SA = cast<ObjCExplicitProtocolImplAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCIndependentClass: {
    const auto *SA = cast<ObjCIndependentClassAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCMethodFamily: {
    const auto *SA = cast<ObjCMethodFamilyAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getFamily());
    break;
  }
  case attr::ObjCNSObject: {
    const auto *SA = cast<ObjCNSObjectAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCPreciseLifetime: {
    const auto *SA = cast<ObjCPreciseLifetimeAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCRequiresPropertyDefs: {
    const auto *SA = cast<ObjCRequiresPropertyDefsAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCRequiresSuper: {
    const auto *SA = cast<ObjCRequiresSuperAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCReturnsInnerPointer: {
    const auto *SA = cast<ObjCReturnsInnerPointerAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCRootClass: {
    const auto *SA = cast<ObjCRootClassAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCRuntimeName: {
    const auto *SA = cast<ObjCRuntimeNameAttr>(A);
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getMetadataName());
    break;
  }
  case attr::ObjCRuntimeVisible: {
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ObjCSubclassingRestricted: {
    const auto *SA = cast<ObjCSubclassingRestrictedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::OpenCLAccess: {
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::OpenCLKernel: {
    const auto *SA = cast<OpenCLKernelAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::OpenCLUnrollHint: {
    const auto *SA = cast<OpenCLUnrollHintAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getUnrollHint());
    break;
  }
  case attr::OptimizeNone: {
    const auto *SA = cast<OptimizeNoneAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Overloadable: {
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Override: {
    const auto *SA = cast<OverrideAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Ownership: {
    const auto *SA = cast<OwnershipAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddIdentifierRef(SA->getModule());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.push_back(Val);
    break;
  }
  case attr::Packed: {
    const auto *SA = cast<PackedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ParamTypestate: {
    const auto *SA = cast<ParamTypestateAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getParamState());
    break;
  }
  case attr::Pascal: {
    const auto *SA = cast<PascalAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::PassObjectSize: {
    const auto *SA = cast<PassObjectSizeAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getType());
    break;
  }
  case attr::Pcs: {
    const auto *SA = cast<PcsAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getPCS());
    break;
  }
  case attr::PreserveAll: {
    const auto *SA = cast<PreserveAllAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::PreserveMost: {
    const auto *SA = cast<PreserveMostAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::PtGuardedBy: {
    const auto *SA = cast<PtGuardedByAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getArg());
    break;
  }
  case attr::PtGuardedVar: {
    const auto *SA = cast<PtGuardedVarAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Pure: {
    const auto *SA = cast<PureAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::RegCall: {
    const auto *SA = cast<RegCallAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ReleaseCapability: {
    const auto *SA = cast<ReleaseCapabilityAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.AddStmt(Val);
    break;
  }
  case attr::RenderScriptKernel: {
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ReqdWorkGroupSize: {
    const auto *SA = cast<ReqdWorkGroupSizeAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getXDim());
    Record.push_back(SA->getYDim());
    Record.push_back(SA->getZDim());
    break;
  }
  case attr::RequireConstantInit: {
    const auto *SA = cast<RequireConstantInitAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::RequiresCapability: {
    const auto *SA = cast<RequiresCapabilityAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.AddStmt(Val);
    break;
  }
  case attr::Restrict: {
    const auto *SA = cast<RestrictAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ReturnTypestate: {
    const auto *SA = cast<ReturnTypestateAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getState());
    break;
  }
  case attr::ReturnsNonNull: {
    const auto *SA = cast<ReturnsNonNullAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ReturnsTwice: {
    const auto *SA = cast<ReturnsTwiceAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::ScopedLockable: {
    const auto *SA = cast<ScopedLockableAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Section: {
    const auto *SA = cast<SectionAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getName());
    break;
  }
  case attr::SelectAny: {
    const auto *SA = cast<SelectAnyAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Sentinel: {
    const auto *SA = cast<SentinelAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getSentinel());
    Record.push_back(SA->getNullPos());
    break;
  }
  case attr::SetTypestate: {
    const auto *SA = cast<SetTypestateAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getNewState());
    break;
  }
  case attr::SharedTrylockFunction: {
    const auto *SA = cast<SharedTrylockFunctionAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getSuccessValue());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.AddStmt(Val);
    break;
  }
  case attr::StdCall: {
    const auto *SA = cast<StdCallAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Suppress: {
    const auto *SA = cast<SuppressAttr>(A);
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->diagnosticIdentifiers_size());
    for (auto &Val : SA->diagnosticIdentifiers())
      Record.AddString(Val);
    break;
  }
  case attr::SwiftCall: {
    const auto *SA = cast<SwiftCallAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::SwiftContext: {
    const auto *SA = cast<SwiftContextAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::SwiftErrorResult: {
    const auto *SA = cast<SwiftErrorResultAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::SwiftIndirectResult: {
    const auto *SA = cast<SwiftIndirectResultAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::SysVABI: {
    const auto *SA = cast<SysVABIAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::TLSModel: {
    const auto *SA = cast<TLSModelAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getModel());
    break;
  }
  case attr::Target: {
    const auto *SA = cast<TargetAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getFeaturesStr());
    break;
  }
  case attr::TestTypestate: {
    const auto *SA = cast<TestTypestateAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getTestState());
    break;
  }
  case attr::ThisCall: {
    const auto *SA = cast<ThisCallAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Thread: {
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::TransparentUnion: {
    const auto *SA = cast<TransparentUnionAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::TryAcquireCapability: {
    const auto *SA = cast<TryAcquireCapabilityAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddStmt(SA->getSuccessValue());
    Record.push_back(SA->args_size());
    for (auto &Val : SA->args())
      Record.AddStmt(Val);
    break;
  }
  case attr::TypeTagForDatatype: {
    const auto *SA = cast<TypeTagForDatatypeAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddIdentifierRef(SA->getArgumentKind());
    Record.AddTypeSourceInfo(SA->getMatchingCTypeLoc());
    Record.push_back(SA->getLayoutCompatible());
    Record.push_back(SA->getMustBeNull());
    break;
  }
  case attr::TypeVisibility: {
    const auto *SA = cast<TypeVisibilityAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getVisibility());
    break;
  }
  case attr::Unavailable: {
    const auto *SA = cast<UnavailableAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getMessage());
Record.push_back(SA->getImplicitReason());
    break;
  }
  case attr::Unused: {
    const auto *SA = cast<UnusedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Used: {
    const auto *SA = cast<UsedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Uuid: {
    const auto *SA = cast<UuidAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getGuid());
    break;
  }
  case attr::VecReturn: {
    const auto *SA = cast<VecReturnAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::VecTypeHint: {
    const auto *SA = cast<VecTypeHintAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddTypeSourceInfo(SA->getTypeHintLoc());
    break;
  }
  case attr::VectorCall: {
    const auto *SA = cast<VectorCallAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Visibility: {
    const auto *SA = cast<VisibilityAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
Record.push_back(SA->getVisibility());
    break;
  }
  case attr::WarnUnused: {
    const auto *SA = cast<WarnUnusedAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::WarnUnusedResult: {
    const auto *SA = cast<WarnUnusedResultAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::Weak: {
    const auto *SA = cast<WeakAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::WeakImport: {
    const auto *SA = cast<WeakImportAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::WeakRef: {
    const auto *SA = cast<WeakRefAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.AddString(SA->getAliasee());
    break;
  }
  case attr::WorkGroupSizeHint: {
    const auto *SA = cast<WorkGroupSizeHintAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getXDim());
    Record.push_back(SA->getYDim());
    Record.push_back(SA->getZDim());
    break;
  }
  case attr::X86ForceAlignArgPointer: {
    const auto *SA = cast<X86ForceAlignArgPointerAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::XRayInstrument: {
    const auto *SA = cast<XRayInstrumentAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    break;
  }
  case attr::XRayLogArgs: {
    const auto *SA = cast<XRayLogArgsAttr>(A);
    Record.push_back(SA->isInherited());
    Record.push_back(A->isImplicit());
    Record.push_back(A->getSpellingListIndex());
    Record.push_back(SA->getArgumentCount());
    break;
  }
  }
