/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Attribute dumper                                                           *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

  switch (A->getKind()) {
  case attr::AMDGPUFlatWorkGroupSize: {
    const auto *SA = cast<AMDGPUFlatWorkGroupSizeAttr>(A);
    OS << " " << SA->getMin();
    OS << " " << SA->getMax();
    break;
  }
  case attr::AMDGPUNumSGPR: {
    const auto *SA = cast<AMDGPUNumSGPRAttr>(A);
    OS << " " << SA->getNumSGPR();
    break;
  }
  case attr::AMDGPUNumVGPR: {
    const auto *SA = cast<AMDGPUNumVGPRAttr>(A);
    OS << " " << SA->getNumVGPR();
    break;
  }
  case attr::AMDGPUWavesPerEU: {
    const auto *SA = cast<AMDGPUWavesPerEUAttr>(A);
    OS << " " << SA->getMin();
    OS << " " << SA->getMax();
    break;
  }
  case attr::ARMInterrupt: {
    const auto *SA = cast<ARMInterruptAttr>(A);
    switch(SA->getInterrupt()) {
    case ARMInterruptAttr::IRQ:
      OS << " IRQ";
      break;
    case ARMInterruptAttr::FIQ:
      OS << " FIQ";
      break;
    case ARMInterruptAttr::SWI:
      OS << " SWI";
      break;
    case ARMInterruptAttr::ABORT:
      OS << " ABORT";
      break;
    case ARMInterruptAttr::UNDEF:
      OS << " UNDEF";
      break;
    case ARMInterruptAttr::Generic:
      OS << " Generic";
      break;
    }
    break;
  }
  case attr::AVRInterrupt: {
    break;
  }
  case attr::AVRSignal: {
    break;
  }
  case attr::AbiTag: {
    const auto *SA = cast<AbiTagAttr>(A);
    for (const auto &Val : SA->tags())
      OS << " " << Val;
    break;
  }
  case attr::AcquireCapability: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<AcquireCapabilityAttr>(A);
    for (AcquireCapabilityAttr::args_iterator I = SA->args_begin(), E = SA->args_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::AcquiredAfter: {
    const auto *SA = cast<AcquiredAfterAttr>(A);
    for (AcquiredAfterAttr::args_iterator I = SA->args_begin(), E = SA->args_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::AcquiredBefore: {
    const auto *SA = cast<AcquiredBeforeAttr>(A);
    for (AcquiredBeforeAttr::args_iterator I = SA->args_begin(), E = SA->args_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::Alias: {
    const auto *SA = cast<AliasAttr>(A);
    OS << " \"" << SA->getAliasee() << "\"";
    break;
  }
  case attr::AlignMac68k: {
    break;
  }
  case attr::AlignValue: {
    const auto *SA = cast<AlignValueAttr>(A);
    dumpStmt(SA->getAlignment());
    break;
  }
  case attr::Aligned: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<AlignedAttr>(A);
    if (SA->isAlignmentExpr())
      dumpStmt(SA->getAlignmentExpr());
    else
      dumpType(SA->getAlignmentType()->getType());
    break;
  }
  case attr::AllocAlign: {
    const auto *SA = cast<AllocAlignAttr>(A);
    OS << " " << SA->getParamIndex();
    break;
  }
  case attr::AllocSize: {
    const auto *SA = cast<AllocSizeAttr>(A);
    OS << " " << SA->getElemSizeParam();
    OS << " " << SA->getNumElemsParam();
    break;
  }
  case attr::AlwaysInline: {
    OS << " " << A->getSpelling();
    break;
  }
  case attr::AnalyzerNoReturn: {
    break;
  }
  case attr::Annotate: {
    const auto *SA = cast<AnnotateAttr>(A);
    OS << " \"" << SA->getAnnotation() << "\"";
    break;
  }
  case attr::AnyX86Interrupt: {
    break;
  }
  case attr::ArcWeakrefUnavailable: {
    break;
  }
  case attr::ArgumentWithTypeTag: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<ArgumentWithTypeTagAttr>(A);
    OS << " " << SA->getArgumentKind()->getName();
    OS << " " << SA->getArgumentIdx();
    OS << " " << SA->getTypeTagIdx();
    if (SA->getIsPointer()) OS << " IsPointer";
    break;
  }
  case attr::AsmLabel: {
    const auto *SA = cast<AsmLabelAttr>(A);
    OS << " \"" << SA->getLabel() << "\"";
    break;
  }
  case attr::AssertCapability: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<AssertCapabilityAttr>(A);
    dumpStmt(SA->getExpr());
    break;
  }
  case attr::AssertExclusiveLock: {
    const auto *SA = cast<AssertExclusiveLockAttr>(A);
    for (AssertExclusiveLockAttr::args_iterator I = SA->args_begin(), E = SA->args_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::AssertSharedLock: {
    const auto *SA = cast<AssertSharedLockAttr>(A);
    for (AssertSharedLockAttr::args_iterator I = SA->args_begin(), E = SA->args_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::AssumeAligned: {
    const auto *SA = cast<AssumeAlignedAttr>(A);
    dumpStmt(SA->getAlignment());
    dumpStmt(SA->getOffset());
    break;
  }
  case attr::Availability: {
    const auto *SA = cast<AvailabilityAttr>(A);
    OS << " " << SA->getPlatform()->getName();
    OS << " " << SA->getIntroduced();
    OS << " " << SA->getDeprecated();
    OS << " " << SA->getObsoleted();
    if (SA->getUnavailable()) OS << " Unavailable";
    OS << " \"" << SA->getMessage() << "\"";
    if (SA->getStrict()) OS << " Strict";
    OS << " \"" << SA->getReplacement() << "\"";
    break;
  }
  case attr::Blocks: {
    const auto *SA = cast<BlocksAttr>(A);
    switch(SA->getType()) {
    case BlocksAttr::ByRef:
      OS << " ByRef";
      break;
    }
    break;
  }
  case attr::C11NoReturn: {
    break;
  }
  case attr::CDecl: {
    break;
  }
  case attr::CFAuditedTransfer: {
    break;
  }
  case attr::CFConsumed: {
    break;
  }
  case attr::CFReturnsNotRetained: {
    break;
  }
  case attr::CFReturnsRetained: {
    break;
  }
  case attr::CFUnknownTransfer: {
    break;
  }
  case attr::CUDAConstant: {
    break;
  }
  case attr::CUDADevice: {
    break;
  }
  case attr::CUDAGlobal: {
    break;
  }
  case attr::CUDAHost: {
    break;
  }
  case attr::CUDAInvalidTarget: {
    break;
  }
  case attr::CUDALaunchBounds: {
    const auto *SA = cast<CUDALaunchBoundsAttr>(A);
    dumpStmt(SA->getMaxThreads());
    dumpStmt(SA->getMinBlocks());
    break;
  }
  case attr::CUDAShared: {
    break;
  }
  case attr::CXX11NoReturn: {
    break;
  }
  case attr::CallableWhen: {
    const auto *SA = cast<CallableWhenAttr>(A);
    for (CallableWhenAttr::callableStates_iterator I = SA->callableStates_begin(), E = SA->callableStates_end(); I != E; ++I) {
      switch(*I) {
    case CallableWhenAttr::Unknown:
      OS << " Unknown";
      break;
    case CallableWhenAttr::Consumed:
      OS << " Consumed";
      break;
    case CallableWhenAttr::Unconsumed:
      OS << " Unconsumed";
      break;
      }
    }
    break;
  }
  case attr::Capability: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<CapabilityAttr>(A);
    OS << " \"" << SA->getName() << "\"";
    break;
  }
  case attr::CapturedRecord: {
    break;
  }
  case attr::CarriesDependency: {
    break;
  }
  case attr::Cleanup: {
    const auto *SA = cast<CleanupAttr>(A);
    OS << " ";
    dumpBareDeclRef(SA->getFunctionDecl());
    break;
  }
  case attr::Cold: {
    break;
  }
  case attr::Common: {
    break;
  }
  case attr::Const: {
    break;
  }
  case attr::Constructor: {
    const auto *SA = cast<ConstructorAttr>(A);
    OS << " " << SA->getPriority();
    break;
  }
  case attr::Consumable: {
    const auto *SA = cast<ConsumableAttr>(A);
    switch(SA->getDefaultState()) {
    case ConsumableAttr::Unknown:
      OS << " Unknown";
      break;
    case ConsumableAttr::Consumed:
      OS << " Consumed";
      break;
    case ConsumableAttr::Unconsumed:
      OS << " Unconsumed";
      break;
    }
    break;
  }
  case attr::ConsumableAutoCast: {
    break;
  }
  case attr::ConsumableSetOnRead: {
    break;
  }
  case attr::Convergent: {
    break;
  }
  case attr::DLLExport: {
    break;
  }
  case attr::DLLImport: {
    break;
  }
  case attr::Deprecated: {
    const auto *SA = cast<DeprecatedAttr>(A);
    OS << " \"" << SA->getMessage() << "\"";
    OS << " \"" << SA->getReplacement() << "\"";
    break;
  }
  case attr::Destructor: {
    const auto *SA = cast<DestructorAttr>(A);
    OS << " " << SA->getPriority();
    break;
  }
  case attr::DiagnoseIf: {
    const auto *SA = cast<DiagnoseIfAttr>(A);
    OS << " \"" << SA->getMessage() << "\"";
    switch(SA->getDiagnosticType()) {
    case DiagnoseIfAttr::DT_Error:
      OS << " DT_Error";
      break;
    case DiagnoseIfAttr::DT_Warning:
      OS << " DT_Warning";
      break;
    }
    if (SA->getArgDependent()) OS << " ArgDependent";
    OS << " ";
    dumpBareDeclRef(SA->getParent());
    dumpStmt(SA->getCond());
    break;
  }
  case attr::DisableTailCalls: {
    break;
  }
  case attr::EmptyBases: {
    break;
  }
  case attr::EnableIf: {
    const auto *SA = cast<EnableIfAttr>(A);
    OS << " \"" << SA->getMessage() << "\"";
    dumpStmt(SA->getCond());
    break;
  }
  case attr::EnumExtensibility: {
    const auto *SA = cast<EnumExtensibilityAttr>(A);
    switch(SA->getExtensibility()) {
    case EnumExtensibilityAttr::Closed:
      OS << " Closed";
      break;
    case EnumExtensibilityAttr::Open:
      OS << " Open";
      break;
    }
    break;
  }
  case attr::ExclusiveTrylockFunction: {
    const auto *SA = cast<ExclusiveTrylockFunctionAttr>(A);
    dumpStmt(SA->getSuccessValue());
    for (ExclusiveTrylockFunctionAttr::args_iterator I = SA->args_begin(), E = SA->args_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::ExternalSourceSymbol: {
    const auto *SA = cast<ExternalSourceSymbolAttr>(A);
    OS << " \"" << SA->getLanguage() << "\"";
    OS << " \"" << SA->getDefinedIn() << "\"";
    if (SA->getGeneratedDeclaration()) OS << " GeneratedDeclaration";
    break;
  }
  case attr::FallThrough: {
    break;
  }
  case attr::FastCall: {
    break;
  }
  case attr::Final: {
    OS << " " << A->getSpelling();
    break;
  }
  case attr::FlagEnum: {
    break;
  }
  case attr::Flatten: {
    break;
  }
  case attr::Format: {
    const auto *SA = cast<FormatAttr>(A);
    OS << " " << SA->getType()->getName();
    OS << " " << SA->getFormatIdx();
    OS << " " << SA->getFirstArg();
    break;
  }
  case attr::FormatArg: {
    const auto *SA = cast<FormatArgAttr>(A);
    OS << " " << SA->getFormatIdx();
    break;
  }
  case attr::GNUInline: {
    break;
  }
  case attr::GuardedBy: {
    const auto *SA = cast<GuardedByAttr>(A);
    dumpStmt(SA->getArg());
    break;
  }
  case attr::GuardedVar: {
    break;
  }
  case attr::Hot: {
    break;
  }
  case attr::IBAction: {
    break;
  }
  case attr::IBOutlet: {
    break;
  }
  case attr::IBOutletCollection: {
    const auto *SA = cast<IBOutletCollectionAttr>(A);
    OS << " " << SA->getInterface().getAsString();
    break;
  }
  case attr::IFunc: {
    const auto *SA = cast<IFuncAttr>(A);
    OS << " \"" << SA->getResolver() << "\"";
    break;
  }
  case attr::InitPriority: {
    const auto *SA = cast<InitPriorityAttr>(A);
    OS << " " << SA->getPriority();
    break;
  }
  case attr::InitSeg: {
    const auto *SA = cast<InitSegAttr>(A);
    OS << " \"" << SA->getSection() << "\"";
    break;
  }
  case attr::IntelOclBicc: {
    break;
  }
  case attr::InternalLinkage: {
    break;
  }
  case attr::LTOVisibilityPublic: {
    break;
  }
  case attr::LayoutVersion: {
    const auto *SA = cast<LayoutVersionAttr>(A);
    OS << " " << SA->getVersion();
    break;
  }
  case attr::LockReturned: {
    const auto *SA = cast<LockReturnedAttr>(A);
    dumpStmt(SA->getArg());
    break;
  }
  case attr::LocksExcluded: {
    const auto *SA = cast<LocksExcludedAttr>(A);
    for (LocksExcludedAttr::args_iterator I = SA->args_begin(), E = SA->args_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::LoopHint: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<LoopHintAttr>(A);
    switch(SA->getOption()) {
    case LoopHintAttr::Vectorize:
      OS << " Vectorize";
      break;
    case LoopHintAttr::VectorizeWidth:
      OS << " VectorizeWidth";
      break;
    case LoopHintAttr::Interleave:
      OS << " Interleave";
      break;
    case LoopHintAttr::InterleaveCount:
      OS << " InterleaveCount";
      break;
    case LoopHintAttr::Unroll:
      OS << " Unroll";
      break;
    case LoopHintAttr::UnrollCount:
      OS << " UnrollCount";
      break;
    case LoopHintAttr::Distribute:
      OS << " Distribute";
      break;
    }
    switch(SA->getState()) {
    case LoopHintAttr::Enable:
      OS << " Enable";
      break;
    case LoopHintAttr::Disable:
      OS << " Disable";
      break;
    case LoopHintAttr::Numeric:
      OS << " Numeric";
      break;
    case LoopHintAttr::AssumeSafety:
      OS << " AssumeSafety";
      break;
    case LoopHintAttr::Full:
      OS << " Full";
      break;
    }
    dumpStmt(SA->getValue());
    break;
  }
  case attr::MSABI: {
    break;
  }
  case attr::MSInheritance: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<MSInheritanceAttr>(A);
    if (SA->getBestCase()) OS << " BestCase";
    break;
  }
  case attr::MSNoVTable: {
    break;
  }
  case attr::MSP430Interrupt: {
    const auto *SA = cast<MSP430InterruptAttr>(A);
    OS << " " << SA->getNumber();
    break;
  }
  case attr::MSStruct: {
    break;
  }
  case attr::MSVtorDisp: {
    const auto *SA = cast<MSVtorDispAttr>(A);
    OS << " " << SA->getVdm();
    break;
  }
  case attr::MaxFieldAlignment: {
    const auto *SA = cast<MaxFieldAlignmentAttr>(A);
    OS << " " << SA->getAlignment();
    break;
  }
  case attr::MayAlias: {
    break;
  }
  case attr::MinSize: {
    break;
  }
  case attr::Mips16: {
    break;
  }
  case attr::MipsInterrupt: {
    const auto *SA = cast<MipsInterruptAttr>(A);
    switch(SA->getInterrupt()) {
    case MipsInterruptAttr::sw0:
      OS << " sw0";
      break;
    case MipsInterruptAttr::sw1:
      OS << " sw1";
      break;
    case MipsInterruptAttr::hw0:
      OS << " hw0";
      break;
    case MipsInterruptAttr::hw1:
      OS << " hw1";
      break;
    case MipsInterruptAttr::hw2:
      OS << " hw2";
      break;
    case MipsInterruptAttr::hw3:
      OS << " hw3";
      break;
    case MipsInterruptAttr::hw4:
      OS << " hw4";
      break;
    case MipsInterruptAttr::hw5:
      OS << " hw5";
      break;
    case MipsInterruptAttr::eic:
      OS << " eic";
      break;
    }
    break;
  }
  case attr::Mode: {
    const auto *SA = cast<ModeAttr>(A);
    OS << " " << SA->getMode()->getName();
    break;
  }
  case attr::NSConsumed: {
    break;
  }
  case attr::NSConsumesSelf: {
    break;
  }
  case attr::NSReturnsAutoreleased: {
    break;
  }
  case attr::NSReturnsNotRetained: {
    break;
  }
  case attr::NSReturnsRetained: {
    break;
  }
  case attr::Naked: {
    break;
  }
  case attr::NoAlias: {
    break;
  }
  case attr::NoCommon: {
    break;
  }
  case attr::NoDebug: {
    break;
  }
  case attr::NoDuplicate: {
    break;
  }
  case attr::NoInline: {
    break;
  }
  case attr::NoInstrumentFunction: {
    break;
  }
  case attr::NoMips16: {
    break;
  }
  case attr::NoReturn: {
    break;
  }
  case attr::NoSanitize: {
    const auto *SA = cast<NoSanitizeAttr>(A);
    for (const auto &Val : SA->sanitizers())
      OS << " " << Val;
    break;
  }
  case attr::NoSplitStack: {
    break;
  }
  case attr::NoThreadSafetyAnalysis: {
    break;
  }
  case attr::NoThrow: {
    break;
  }
  case attr::NonNull: {
    const auto *SA = cast<NonNullAttr>(A);
    for (const auto &Val : SA->args())
      OS << " " << Val;
    break;
  }
  case attr::NotTailCalled: {
    break;
  }
  case attr::OMPCaptureNoInit: {
    break;
  }
  case attr::OMPDeclareSimdDecl: {
    const auto *SA = cast<OMPDeclareSimdDeclAttr>(A);
    switch(SA->getBranchState()) {
    case OMPDeclareSimdDeclAttr::BS_Undefined:
      OS << " BS_Undefined";
      break;
    case OMPDeclareSimdDeclAttr::BS_Inbranch:
      OS << " BS_Inbranch";
      break;
    case OMPDeclareSimdDeclAttr::BS_Notinbranch:
      OS << " BS_Notinbranch";
      break;
    }
    for (const auto &Val : SA->modifiers())
      OS << " " << Val;
    dumpStmt(SA->getSimdlen());
    for (OMPDeclareSimdDeclAttr::uniforms_iterator I = SA->uniforms_begin(), E = SA->uniforms_end(); I != E; ++I)
      dumpStmt(*I);
    for (OMPDeclareSimdDeclAttr::aligneds_iterator I = SA->aligneds_begin(), E = SA->aligneds_end(); I != E; ++I)
      dumpStmt(*I);
    for (OMPDeclareSimdDeclAttr::alignments_iterator I = SA->alignments_begin(), E = SA->alignments_end(); I != E; ++I)
      dumpStmt(*I);
    for (OMPDeclareSimdDeclAttr::linears_iterator I = SA->linears_begin(), E = SA->linears_end(); I != E; ++I)
      dumpStmt(*I);
    for (OMPDeclareSimdDeclAttr::steps_iterator I = SA->steps_begin(), E = SA->steps_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::OMPDeclareTargetDecl: {
    const auto *SA = cast<OMPDeclareTargetDeclAttr>(A);
    switch(SA->getMapType()) {
    case OMPDeclareTargetDeclAttr::MT_To:
      OS << " MT_To";
      break;
    case OMPDeclareTargetDeclAttr::MT_Link:
      OS << " MT_Link";
      break;
    }
    break;
  }
  case attr::OMPThreadPrivateDecl: {
    break;
  }
  case attr::ObjCBoxable: {
    break;
  }
  case attr::ObjCBridge: {
    const auto *SA = cast<ObjCBridgeAttr>(A);
    OS << " " << SA->getBridgedType()->getName();
    break;
  }
  case attr::ObjCBridgeMutable: {
    const auto *SA = cast<ObjCBridgeMutableAttr>(A);
    OS << " " << SA->getBridgedType()->getName();
    break;
  }
  case attr::ObjCBridgeRelated: {
    const auto *SA = cast<ObjCBridgeRelatedAttr>(A);
    OS << " " << SA->getRelatedClass()->getName();
    if (SA->getClassMethod())
      OS << " " << SA->getClassMethod()->getName();
    if (SA->getInstanceMethod())
      OS << " " << SA->getInstanceMethod()->getName();
    break;
  }
  case attr::ObjCDesignatedInitializer: {
    break;
  }
  case attr::ObjCException: {
    break;
  }
  case attr::ObjCExplicitProtocolImpl: {
    break;
  }
  case attr::ObjCIndependentClass: {
    break;
  }
  case attr::ObjCMethodFamily: {
    const auto *SA = cast<ObjCMethodFamilyAttr>(A);
    switch(SA->getFamily()) {
    case ObjCMethodFamilyAttr::OMF_None:
      OS << " OMF_None";
      break;
    case ObjCMethodFamilyAttr::OMF_alloc:
      OS << " OMF_alloc";
      break;
    case ObjCMethodFamilyAttr::OMF_copy:
      OS << " OMF_copy";
      break;
    case ObjCMethodFamilyAttr::OMF_init:
      OS << " OMF_init";
      break;
    case ObjCMethodFamilyAttr::OMF_mutableCopy:
      OS << " OMF_mutableCopy";
      break;
    case ObjCMethodFamilyAttr::OMF_new:
      OS << " OMF_new";
      break;
    }
    break;
  }
  case attr::ObjCNSObject: {
    break;
  }
  case attr::ObjCPreciseLifetime: {
    break;
  }
  case attr::ObjCRequiresPropertyDefs: {
    break;
  }
  case attr::ObjCRequiresSuper: {
    break;
  }
  case attr::ObjCReturnsInnerPointer: {
    break;
  }
  case attr::ObjCRootClass: {
    break;
  }
  case attr::ObjCRuntimeName: {
    const auto *SA = cast<ObjCRuntimeNameAttr>(A);
    OS << " \"" << SA->getMetadataName() << "\"";
    break;
  }
  case attr::ObjCRuntimeVisible: {
    break;
  }
  case attr::ObjCSubclassingRestricted: {
    break;
  }
  case attr::OpenCLAccess: {
    OS << " " << A->getSpelling();
    break;
  }
  case attr::OpenCLKernel: {
    break;
  }
  case attr::OpenCLUnrollHint: {
    const auto *SA = cast<OpenCLUnrollHintAttr>(A);
    OS << " " << SA->getUnrollHint();
    break;
  }
  case attr::OptimizeNone: {
    break;
  }
  case attr::Overloadable: {
    break;
  }
  case attr::Override: {
    break;
  }
  case attr::Ownership: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<OwnershipAttr>(A);
    OS << " " << SA->getModule()->getName();
    for (const auto &Val : SA->args())
      OS << " " << Val;
    break;
  }
  case attr::Packed: {
    break;
  }
  case attr::ParamTypestate: {
    const auto *SA = cast<ParamTypestateAttr>(A);
    switch(SA->getParamState()) {
    case ParamTypestateAttr::Unknown:
      OS << " Unknown";
      break;
    case ParamTypestateAttr::Consumed:
      OS << " Consumed";
      break;
    case ParamTypestateAttr::Unconsumed:
      OS << " Unconsumed";
      break;
    }
    break;
  }
  case attr::Pascal: {
    break;
  }
  case attr::PassObjectSize: {
    const auto *SA = cast<PassObjectSizeAttr>(A);
    OS << " " << SA->getType();
    break;
  }
  case attr::Pcs: {
    const auto *SA = cast<PcsAttr>(A);
    switch(SA->getPCS()) {
    case PcsAttr::AAPCS:
      OS << " AAPCS";
      break;
    case PcsAttr::AAPCS_VFP:
      OS << " AAPCS_VFP";
      break;
    }
    break;
  }
  case attr::PreserveAll: {
    break;
  }
  case attr::PreserveMost: {
    break;
  }
  case attr::PtGuardedBy: {
    const auto *SA = cast<PtGuardedByAttr>(A);
    dumpStmt(SA->getArg());
    break;
  }
  case attr::PtGuardedVar: {
    break;
  }
  case attr::Pure: {
    break;
  }
  case attr::RegCall: {
    break;
  }
  case attr::ReleaseCapability: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<ReleaseCapabilityAttr>(A);
    for (ReleaseCapabilityAttr::args_iterator I = SA->args_begin(), E = SA->args_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::RenderScriptKernel: {
    break;
  }
  case attr::ReqdWorkGroupSize: {
    const auto *SA = cast<ReqdWorkGroupSizeAttr>(A);
    OS << " " << SA->getXDim();
    OS << " " << SA->getYDim();
    OS << " " << SA->getZDim();
    break;
  }
  case attr::RequireConstantInit: {
    break;
  }
  case attr::RequiresCapability: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<RequiresCapabilityAttr>(A);
    for (RequiresCapabilityAttr::args_iterator I = SA->args_begin(), E = SA->args_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::Restrict: {
    OS << " " << A->getSpelling();
    break;
  }
  case attr::ReturnTypestate: {
    const auto *SA = cast<ReturnTypestateAttr>(A);
    switch(SA->getState()) {
    case ReturnTypestateAttr::Unknown:
      OS << " Unknown";
      break;
    case ReturnTypestateAttr::Consumed:
      OS << " Consumed";
      break;
    case ReturnTypestateAttr::Unconsumed:
      OS << " Unconsumed";
      break;
    }
    break;
  }
  case attr::ReturnsNonNull: {
    break;
  }
  case attr::ReturnsTwice: {
    break;
  }
  case attr::ScopedLockable: {
    break;
  }
  case attr::Section: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<SectionAttr>(A);
    OS << " \"" << SA->getName() << "\"";
    break;
  }
  case attr::SelectAny: {
    break;
  }
  case attr::Sentinel: {
    const auto *SA = cast<SentinelAttr>(A);
    OS << " " << SA->getSentinel();
    OS << " " << SA->getNullPos();
    break;
  }
  case attr::SetTypestate: {
    const auto *SA = cast<SetTypestateAttr>(A);
    switch(SA->getNewState()) {
    case SetTypestateAttr::Unknown:
      OS << " Unknown";
      break;
    case SetTypestateAttr::Consumed:
      OS << " Consumed";
      break;
    case SetTypestateAttr::Unconsumed:
      OS << " Unconsumed";
      break;
    }
    break;
  }
  case attr::SharedTrylockFunction: {
    const auto *SA = cast<SharedTrylockFunctionAttr>(A);
    dumpStmt(SA->getSuccessValue());
    for (SharedTrylockFunctionAttr::args_iterator I = SA->args_begin(), E = SA->args_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::StdCall: {
    break;
  }
  case attr::Suppress: {
    const auto *SA = cast<SuppressAttr>(A);
    for (const auto &Val : SA->diagnosticIdentifiers())
      OS << " " << Val;
    break;
  }
  case attr::SwiftCall: {
    break;
  }
  case attr::SwiftContext: {
    break;
  }
  case attr::SwiftErrorResult: {
    break;
  }
  case attr::SwiftIndirectResult: {
    break;
  }
  case attr::SysVABI: {
    break;
  }
  case attr::TLSModel: {
    const auto *SA = cast<TLSModelAttr>(A);
    OS << " \"" << SA->getModel() << "\"";
    break;
  }
  case attr::Target: {
    const auto *SA = cast<TargetAttr>(A);
    OS << " \"" << SA->getFeaturesStr() << "\"";
    break;
  }
  case attr::TestTypestate: {
    const auto *SA = cast<TestTypestateAttr>(A);
    switch(SA->getTestState()) {
    case TestTypestateAttr::Consumed:
      OS << " Consumed";
      break;
    case TestTypestateAttr::Unconsumed:
      OS << " Unconsumed";
      break;
    }
    break;
  }
  case attr::ThisCall: {
    break;
  }
  case attr::Thread: {
    break;
  }
  case attr::TransparentUnion: {
    break;
  }
  case attr::TryAcquireCapability: {
    OS << " " << A->getSpelling();
    const auto *SA = cast<TryAcquireCapabilityAttr>(A);
    dumpStmt(SA->getSuccessValue());
    for (TryAcquireCapabilityAttr::args_iterator I = SA->args_begin(), E = SA->args_end(); I != E; ++I)
      dumpStmt(*I);
    break;
  }
  case attr::TypeTagForDatatype: {
    const auto *SA = cast<TypeTagForDatatypeAttr>(A);
    OS << " " << SA->getArgumentKind()->getName();
    OS << " " << SA->getMatchingCType().getAsString();
    if (SA->getLayoutCompatible()) OS << " LayoutCompatible";
    if (SA->getMustBeNull()) OS << " MustBeNull";
    break;
  }
  case attr::TypeVisibility: {
    const auto *SA = cast<TypeVisibilityAttr>(A);
    switch(SA->getVisibility()) {
    case TypeVisibilityAttr::Default:
      OS << " Default";
      break;
    case TypeVisibilityAttr::Hidden:
      OS << " Hidden";
      break;
    case TypeVisibilityAttr::Protected:
      OS << " Protected";
      break;
    }
    break;
  }
  case attr::Unavailable: {
    const auto *SA = cast<UnavailableAttr>(A);
    OS << " \"" << SA->getMessage() << "\"";
    switch(SA->getImplicitReason()) {
    case UnavailableAttr::IR_None:
      OS << " IR_None";
      break;
    case UnavailableAttr::IR_ARCForbiddenType:
      OS << " IR_ARCForbiddenType";
      break;
    case UnavailableAttr::IR_ForbiddenWeak:
      OS << " IR_ForbiddenWeak";
      break;
    case UnavailableAttr::IR_ARCForbiddenConversion:
      OS << " IR_ARCForbiddenConversion";
      break;
    case UnavailableAttr::IR_ARCInitReturnsUnrelated:
      OS << " IR_ARCInitReturnsUnrelated";
      break;
    case UnavailableAttr::IR_ARCFieldWithOwnership:
      OS << " IR_ARCFieldWithOwnership";
      break;
    }
    break;
  }
  case attr::Unused: {
    OS << " " << A->getSpelling();
    break;
  }
  case attr::Used: {
    break;
  }
  case attr::Uuid: {
    const auto *SA = cast<UuidAttr>(A);
    OS << " \"" << SA->getGuid() << "\"";
    break;
  }
  case attr::VecReturn: {
    break;
  }
  case attr::VecTypeHint: {
    const auto *SA = cast<VecTypeHintAttr>(A);
    OS << " " << SA->getTypeHint().getAsString();
    break;
  }
  case attr::VectorCall: {
    break;
  }
  case attr::Visibility: {
    const auto *SA = cast<VisibilityAttr>(A);
    switch(SA->getVisibility()) {
    case VisibilityAttr::Default:
      OS << " Default";
      break;
    case VisibilityAttr::Hidden:
      OS << " Hidden";
      break;
    case VisibilityAttr::Protected:
      OS << " Protected";
      break;
    }
    break;
  }
  case attr::WarnUnused: {
    break;
  }
  case attr::WarnUnusedResult: {
    OS << " " << A->getSpelling();
    break;
  }
  case attr::Weak: {
    break;
  }
  case attr::WeakImport: {
    break;
  }
  case attr::WeakRef: {
    const auto *SA = cast<WeakRefAttr>(A);
    OS << " \"" << SA->getAliasee() << "\"";
    break;
  }
  case attr::WorkGroupSizeHint: {
    const auto *SA = cast<WorkGroupSizeHintAttr>(A);
    OS << " " << SA->getXDim();
    OS << " " << SA->getYDim();
    OS << " " << SA->getZDim();
    break;
  }
  case attr::X86ForceAlignArgPointer: {
    break;
  }
  case attr::XRayInstrument: {
    OS << " " << A->getSpelling();
    break;
  }
  case attr::XRayLogArgs: {
    const auto *SA = cast<XRayLogArgsAttr>(A);
    OS << " " << SA->getArgumentCount();
    break;
  }
  }
