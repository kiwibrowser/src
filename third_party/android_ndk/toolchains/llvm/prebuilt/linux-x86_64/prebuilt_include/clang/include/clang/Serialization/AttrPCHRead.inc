/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Attribute deserialization code                                             *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

  switch (Kind) {
  case attr::AMDGPUFlatWorkGroupSize: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned min = Record.readInt();
    unsigned max = Record.readInt();
    New = new (Context) AMDGPUFlatWorkGroupSizeAttr(Range, Context, min, max, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AMDGPUNumSGPR: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned numSGPR = Record.readInt();
    New = new (Context) AMDGPUNumSGPRAttr(Range, Context, numSGPR, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AMDGPUNumVGPR: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned numVGPR = Record.readInt();
    New = new (Context) AMDGPUNumVGPRAttr(Range, Context, numVGPR, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AMDGPUWavesPerEU: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned min = Record.readInt();
    unsigned max = Record.readInt();
    New = new (Context) AMDGPUWavesPerEUAttr(Range, Context, min, max, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ARMInterrupt: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    ARMInterruptAttr::InterruptType interrupt(static_cast<ARMInterruptAttr::InterruptType>(Record.readInt()));
    New = new (Context) ARMInterruptAttr(Range, Context, interrupt, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AVRInterrupt: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) AVRInterruptAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AVRSignal: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) AVRSignalAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AbiTag: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned tagsSize = Record.readInt();
    SmallVector<StringRef, 4> tags;
    tags.reserve(tagsSize);
    SmallVector<std::string, 4> tagsStorage;
    tagsStorage.reserve(tagsSize);
    for (unsigned i = 0; i != tagsSize; ++i)
      tagsStorage.push_back(Record.readString());
    for (unsigned i = 0; i != tagsSize; ++i)
      tags.push_back(tagsStorage[i]);
    New = new (Context) AbiTagAttr(Range, Context, tags.data(), tagsSize, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AcquireCapability: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned argsSize = Record.readInt();
    SmallVector<Expr *, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readExpr());
    New = new (Context) AcquireCapabilityAttr(Range, Context, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AcquiredAfter: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned argsSize = Record.readInt();
    SmallVector<Expr *, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readExpr());
    New = new (Context) AcquiredAfterAttr(Range, Context, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AcquiredBefore: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned argsSize = Record.readInt();
    SmallVector<Expr *, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readExpr());
    New = new (Context) AcquiredBeforeAttr(Range, Context, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Alias: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string aliasee= Record.readString();
    New = new (Context) AliasAttr(Range, Context, aliasee, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AlignMac68k: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) AlignMac68kAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AlignValue: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * alignment = Record.readExpr();
    New = new (Context) AlignValueAttr(Range, Context, alignment, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Aligned: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    bool isalignmentExpr = Record.readInt();
    void *alignmentPtr;
    if (isalignmentExpr)
      alignmentPtr = Record.readExpr();
    else
      alignmentPtr = Record.getTypeSourceInfo();
    New = new (Context) AlignedAttr(Range, Context, isalignmentExpr, alignmentPtr, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AllocAlign: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    int paramIndex = Record.readInt();
    New = new (Context) AllocAlignAttr(Range, Context, paramIndex, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AllocSize: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    int elemSizeParam = Record.readInt();
    int numElemsParam = Record.readInt();
    New = new (Context) AllocSizeAttr(Range, Context, elemSizeParam, numElemsParam, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AlwaysInline: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) AlwaysInlineAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AnalyzerNoReturn: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) AnalyzerNoReturnAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Annotate: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string annotation= Record.readString();
    New = new (Context) AnnotateAttr(Range, Context, annotation, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AnyX86Interrupt: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) AnyX86InterruptAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ArcWeakrefUnavailable: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ArcWeakrefUnavailableAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ArgumentWithTypeTag: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    IdentifierInfo * argumentKind = Record.getIdentifierInfo();
    unsigned argumentIdx = Record.readInt();
    unsigned typeTagIdx = Record.readInt();
    bool isPointer = Record.readInt();
    New = new (Context) ArgumentWithTypeTagAttr(Range, Context, argumentKind, argumentIdx, typeTagIdx, isPointer, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AsmLabel: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string label= Record.readString();
    New = new (Context) AsmLabelAttr(Range, Context, label, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AssertCapability: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * expr = Record.readExpr();
    New = new (Context) AssertCapabilityAttr(Range, Context, expr, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AssertExclusiveLock: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned argsSize = Record.readInt();
    SmallVector<Expr *, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readExpr());
    New = new (Context) AssertExclusiveLockAttr(Range, Context, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AssertSharedLock: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned argsSize = Record.readInt();
    SmallVector<Expr *, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readExpr());
    New = new (Context) AssertSharedLockAttr(Range, Context, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::AssumeAligned: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * alignment = Record.readExpr();
    Expr * offset = Record.readExpr();
    New = new (Context) AssumeAlignedAttr(Range, Context, alignment, offset, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Availability: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    IdentifierInfo * platform = Record.getIdentifierInfo();
    VersionTuple introduced= Record.readVersionTuple();
    VersionTuple deprecated= Record.readVersionTuple();
    VersionTuple obsoleted= Record.readVersionTuple();
    bool unavailable = Record.readInt();
    std::string message= Record.readString();
    bool strict = Record.readInt();
    std::string replacement= Record.readString();
    New = new (Context) AvailabilityAttr(Range, Context, platform, introduced, deprecated, obsoleted, unavailable, message, strict, replacement, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Blocks: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    BlocksAttr::BlockType type(static_cast<BlocksAttr::BlockType>(Record.readInt()));
    New = new (Context) BlocksAttr(Range, Context, type, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::C11NoReturn: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) C11NoReturnAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CDecl: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CDeclAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CFAuditedTransfer: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CFAuditedTransferAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CFConsumed: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CFConsumedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CFReturnsNotRetained: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CFReturnsNotRetainedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CFReturnsRetained: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CFReturnsRetainedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CFUnknownTransfer: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CFUnknownTransferAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CUDAConstant: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CUDAConstantAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CUDADevice: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CUDADeviceAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CUDAGlobal: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CUDAGlobalAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CUDAHost: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CUDAHostAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CUDAInvalidTarget: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CUDAInvalidTargetAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CUDALaunchBounds: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * maxThreads = Record.readExpr();
    Expr * minBlocks = Record.readExpr();
    New = new (Context) CUDALaunchBoundsAttr(Range, Context, maxThreads, minBlocks, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CUDAShared: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CUDASharedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CXX11NoReturn: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CXX11NoReturnAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CallableWhen: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned callableStatesSize = Record.readInt();
    SmallVector<CallableWhenAttr::ConsumedState, 4> callableStates;
    callableStates.reserve(callableStatesSize);
    for (unsigned i = callableStatesSize; i; --i)
      callableStates.push_back(static_cast<CallableWhenAttr::ConsumedState>(Record.readInt()));
    New = new (Context) CallableWhenAttr(Range, Context, callableStates.data(), callableStatesSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Capability: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string name= Record.readString();
    New = new (Context) CapabilityAttr(Range, Context, name, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CapturedRecord: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CapturedRecordAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::CarriesDependency: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CarriesDependencyAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Cleanup: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    FunctionDecl * functionDecl = Record.GetLocalDeclAs<FunctionDecl >(Record.readInt());
    New = new (Context) CleanupAttr(Range, Context, functionDecl, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Cold: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ColdAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Common: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) CommonAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Const: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ConstAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Constructor: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    int priority = Record.readInt();
    New = new (Context) ConstructorAttr(Range, Context, priority, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Consumable: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    ConsumableAttr::ConsumedState defaultState(static_cast<ConsumableAttr::ConsumedState>(Record.readInt()));
    New = new (Context) ConsumableAttr(Range, Context, defaultState, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ConsumableAutoCast: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ConsumableAutoCastAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ConsumableSetOnRead: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ConsumableSetOnReadAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Convergent: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ConvergentAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::DLLExport: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) DLLExportAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::DLLImport: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) DLLImportAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Deprecated: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string message= Record.readString();
    std::string replacement= Record.readString();
    New = new (Context) DeprecatedAttr(Range, Context, message, replacement, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Destructor: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    int priority = Record.readInt();
    New = new (Context) DestructorAttr(Range, Context, priority, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::DiagnoseIf: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * cond = Record.readExpr();
    std::string message= Record.readString();
    DiagnoseIfAttr::DiagnosticType diagnosticType(static_cast<DiagnoseIfAttr::DiagnosticType>(Record.readInt()));
    bool argDependent = Record.readInt();
    FunctionDecl * parent = Record.GetLocalDeclAs<FunctionDecl >(Record.readInt());
    New = new (Context) DiagnoseIfAttr(Range, Context, cond, message, diagnosticType, argDependent, parent, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::DisableTailCalls: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) DisableTailCallsAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::EmptyBases: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) EmptyBasesAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::EnableIf: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * cond = Record.readExpr();
    std::string message= Record.readString();
    New = new (Context) EnableIfAttr(Range, Context, cond, message, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::EnumExtensibility: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    EnumExtensibilityAttr::Kind extensibility(static_cast<EnumExtensibilityAttr::Kind>(Record.readInt()));
    New = new (Context) EnumExtensibilityAttr(Range, Context, extensibility, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ExclusiveTrylockFunction: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * successValue = Record.readExpr();
    unsigned argsSize = Record.readInt();
    SmallVector<Expr *, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readExpr());
    New = new (Context) ExclusiveTrylockFunctionAttr(Range, Context, successValue, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ExternalSourceSymbol: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string language= Record.readString();
    std::string definedIn= Record.readString();
    bool generatedDeclaration = Record.readInt();
    New = new (Context) ExternalSourceSymbolAttr(Range, Context, language, definedIn, generatedDeclaration, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::FallThrough: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) FallThroughAttr(Range, Context, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::FastCall: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) FastCallAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Final: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) FinalAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::FlagEnum: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) FlagEnumAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Flatten: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) FlattenAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Format: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    IdentifierInfo * type = Record.getIdentifierInfo();
    int formatIdx = Record.readInt();
    int firstArg = Record.readInt();
    New = new (Context) FormatAttr(Range, Context, type, formatIdx, firstArg, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::FormatArg: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    int formatIdx = Record.readInt();
    New = new (Context) FormatArgAttr(Range, Context, formatIdx, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::GNUInline: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) GNUInlineAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::GuardedBy: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * arg = Record.readExpr();
    New = new (Context) GuardedByAttr(Range, Context, arg, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::GuardedVar: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) GuardedVarAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Hot: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) HotAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::IBAction: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) IBActionAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::IBOutlet: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) IBOutletAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::IBOutletCollection: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    TypeSourceInfo * interface_ = Record.getTypeSourceInfo();
    New = new (Context) IBOutletCollectionAttr(Range, Context, interface_, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::IFunc: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string resolver= Record.readString();
    New = new (Context) IFuncAttr(Range, Context, resolver, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::InitPriority: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned priority = Record.readInt();
    New = new (Context) InitPriorityAttr(Range, Context, priority, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::InitSeg: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string section= Record.readString();
    New = new (Context) InitSegAttr(Range, Context, section, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::IntelOclBicc: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) IntelOclBiccAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::InternalLinkage: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) InternalLinkageAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::LTOVisibilityPublic: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) LTOVisibilityPublicAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::LayoutVersion: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned version = Record.readInt();
    New = new (Context) LayoutVersionAttr(Range, Context, version, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::LockReturned: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * arg = Record.readExpr();
    New = new (Context) LockReturnedAttr(Range, Context, arg, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::LocksExcluded: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned argsSize = Record.readInt();
    SmallVector<Expr *, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readExpr());
    New = new (Context) LocksExcludedAttr(Range, Context, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::LoopHint: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    LoopHintAttr::OptionType option(static_cast<LoopHintAttr::OptionType>(Record.readInt()));
    LoopHintAttr::LoopHintState state(static_cast<LoopHintAttr::LoopHintState>(Record.readInt()));
    Expr * value = Record.readExpr();
    New = new (Context) LoopHintAttr(Range, Context, option, state, value, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::MSABI: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) MSABIAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::MSInheritance: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    bool bestCase = Record.readInt();
    New = new (Context) MSInheritanceAttr(Range, Context, bestCase, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::MSNoVTable: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) MSNoVTableAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::MSP430Interrupt: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned number = Record.readInt();
    New = new (Context) MSP430InterruptAttr(Range, Context, number, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::MSStruct: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) MSStructAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::MSVtorDisp: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned vdm = Record.readInt();
    New = new (Context) MSVtorDispAttr(Range, Context, vdm, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::MaxFieldAlignment: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned alignment = Record.readInt();
    New = new (Context) MaxFieldAlignmentAttr(Range, Context, alignment, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::MayAlias: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) MayAliasAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::MinSize: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) MinSizeAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Mips16: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) Mips16Attr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::MipsInterrupt: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    MipsInterruptAttr::InterruptType interrupt(static_cast<MipsInterruptAttr::InterruptType>(Record.readInt()));
    New = new (Context) MipsInterruptAttr(Range, Context, interrupt, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Mode: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    IdentifierInfo * mode = Record.getIdentifierInfo();
    New = new (Context) ModeAttr(Range, Context, mode, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NSConsumed: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NSConsumedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NSConsumesSelf: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NSConsumesSelfAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NSReturnsAutoreleased: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NSReturnsAutoreleasedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NSReturnsNotRetained: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NSReturnsNotRetainedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NSReturnsRetained: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NSReturnsRetainedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Naked: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NakedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoAlias: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NoAliasAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoCommon: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NoCommonAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoDebug: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NoDebugAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoDuplicate: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NoDuplicateAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoInline: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NoInlineAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoInstrumentFunction: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NoInstrumentFunctionAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoMips16: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NoMips16Attr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoReturn: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NoReturnAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoSanitize: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned sanitizersSize = Record.readInt();
    SmallVector<StringRef, 4> sanitizers;
    sanitizers.reserve(sanitizersSize);
    SmallVector<std::string, 4> sanitizersStorage;
    sanitizersStorage.reserve(sanitizersSize);
    for (unsigned i = 0; i != sanitizersSize; ++i)
      sanitizersStorage.push_back(Record.readString());
    for (unsigned i = 0; i != sanitizersSize; ++i)
      sanitizers.push_back(sanitizersStorage[i]);
    New = new (Context) NoSanitizeAttr(Range, Context, sanitizers.data(), sanitizersSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoSplitStack: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NoSplitStackAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoThreadSafetyAnalysis: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NoThreadSafetyAnalysisAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NoThrow: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NoThrowAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NonNull: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned argsSize = Record.readInt();
    SmallVector<unsigned, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readInt());
    New = new (Context) NonNullAttr(Range, Context, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::NotTailCalled: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) NotTailCalledAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::OMPCaptureNoInit: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) OMPCaptureNoInitAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::OMPDeclareSimdDecl: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    OMPDeclareSimdDeclAttr::BranchStateTy branchState(static_cast<OMPDeclareSimdDeclAttr::BranchStateTy>(Record.readInt()));
    Expr * simdlen = Record.readExpr();
    unsigned uniformsSize = Record.readInt();
    SmallVector<Expr *, 4> uniforms;
    uniforms.reserve(uniformsSize);
    for (unsigned i = 0; i != uniformsSize; ++i)
      uniforms.push_back(Record.readExpr());
    unsigned alignedsSize = Record.readInt();
    SmallVector<Expr *, 4> aligneds;
    aligneds.reserve(alignedsSize);
    for (unsigned i = 0; i != alignedsSize; ++i)
      aligneds.push_back(Record.readExpr());
    unsigned alignmentsSize = Record.readInt();
    SmallVector<Expr *, 4> alignments;
    alignments.reserve(alignmentsSize);
    for (unsigned i = 0; i != alignmentsSize; ++i)
      alignments.push_back(Record.readExpr());
    unsigned linearsSize = Record.readInt();
    SmallVector<Expr *, 4> linears;
    linears.reserve(linearsSize);
    for (unsigned i = 0; i != linearsSize; ++i)
      linears.push_back(Record.readExpr());
    unsigned modifiersSize = Record.readInt();
    SmallVector<unsigned, 4> modifiers;
    modifiers.reserve(modifiersSize);
    for (unsigned i = 0; i != modifiersSize; ++i)
      modifiers.push_back(Record.readInt());
    unsigned stepsSize = Record.readInt();
    SmallVector<Expr *, 4> steps;
    steps.reserve(stepsSize);
    for (unsigned i = 0; i != stepsSize; ++i)
      steps.push_back(Record.readExpr());
    New = new (Context) OMPDeclareSimdDeclAttr(Range, Context, branchState, simdlen, uniforms.data(), uniformsSize, aligneds.data(), alignedsSize, alignments.data(), alignmentsSize, linears.data(), linearsSize, modifiers.data(), modifiersSize, steps.data(), stepsSize, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::OMPDeclareTargetDecl: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    OMPDeclareTargetDeclAttr::MapTypeTy mapType(static_cast<OMPDeclareTargetDeclAttr::MapTypeTy>(Record.readInt()));
    New = new (Context) OMPDeclareTargetDeclAttr(Range, Context, mapType, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::OMPThreadPrivateDecl: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) OMPThreadPrivateDeclAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCBoxable: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCBoxableAttr(Range, Context, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCBridge: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    IdentifierInfo * bridgedType = Record.getIdentifierInfo();
    New = new (Context) ObjCBridgeAttr(Range, Context, bridgedType, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCBridgeMutable: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    IdentifierInfo * bridgedType = Record.getIdentifierInfo();
    New = new (Context) ObjCBridgeMutableAttr(Range, Context, bridgedType, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCBridgeRelated: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    IdentifierInfo * relatedClass = Record.getIdentifierInfo();
    IdentifierInfo * classMethod = Record.getIdentifierInfo();
    IdentifierInfo * instanceMethod = Record.getIdentifierInfo();
    New = new (Context) ObjCBridgeRelatedAttr(Range, Context, relatedClass, classMethod, instanceMethod, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCDesignatedInitializer: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCDesignatedInitializerAttr(Range, Context, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCException: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCExceptionAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCExplicitProtocolImpl: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCExplicitProtocolImplAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCIndependentClass: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCIndependentClassAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCMethodFamily: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    ObjCMethodFamilyAttr::FamilyKind family(static_cast<ObjCMethodFamilyAttr::FamilyKind>(Record.readInt()));
    New = new (Context) ObjCMethodFamilyAttr(Range, Context, family, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCNSObject: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCNSObjectAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCPreciseLifetime: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCPreciseLifetimeAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCRequiresPropertyDefs: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCRequiresPropertyDefsAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCRequiresSuper: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCRequiresSuperAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCReturnsInnerPointer: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCReturnsInnerPointerAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCRootClass: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCRootClassAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCRuntimeName: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string metadataName= Record.readString();
    New = new (Context) ObjCRuntimeNameAttr(Range, Context, metadataName, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCRuntimeVisible: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCRuntimeVisibleAttr(Range, Context, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ObjCSubclassingRestricted: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ObjCSubclassingRestrictedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::OpenCLAccess: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) OpenCLAccessAttr(Range, Context, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::OpenCLKernel: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) OpenCLKernelAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::OpenCLUnrollHint: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned unrollHint = Record.readInt();
    New = new (Context) OpenCLUnrollHintAttr(Range, Context, unrollHint, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::OptimizeNone: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) OptimizeNoneAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Overloadable: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) OverloadableAttr(Range, Context, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Override: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) OverrideAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Ownership: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    IdentifierInfo * module = Record.getIdentifierInfo();
    unsigned argsSize = Record.readInt();
    SmallVector<unsigned, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readInt());
    New = new (Context) OwnershipAttr(Range, Context, module, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Packed: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) PackedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ParamTypestate: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    ParamTypestateAttr::ConsumedState paramState(static_cast<ParamTypestateAttr::ConsumedState>(Record.readInt()));
    New = new (Context) ParamTypestateAttr(Range, Context, paramState, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Pascal: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) PascalAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::PassObjectSize: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    int type = Record.readInt();
    New = new (Context) PassObjectSizeAttr(Range, Context, type, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Pcs: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    PcsAttr::PCSType pCS(static_cast<PcsAttr::PCSType>(Record.readInt()));
    New = new (Context) PcsAttr(Range, Context, pCS, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::PreserveAll: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) PreserveAllAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::PreserveMost: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) PreserveMostAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::PtGuardedBy: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * arg = Record.readExpr();
    New = new (Context) PtGuardedByAttr(Range, Context, arg, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::PtGuardedVar: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) PtGuardedVarAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Pure: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) PureAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::RegCall: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) RegCallAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ReleaseCapability: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned argsSize = Record.readInt();
    SmallVector<Expr *, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readExpr());
    New = new (Context) ReleaseCapabilityAttr(Range, Context, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::RenderScriptKernel: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) RenderScriptKernelAttr(Range, Context, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ReqdWorkGroupSize: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned xDim = Record.readInt();
    unsigned yDim = Record.readInt();
    unsigned zDim = Record.readInt();
    New = new (Context) ReqdWorkGroupSizeAttr(Range, Context, xDim, yDim, zDim, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::RequireConstantInit: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) RequireConstantInitAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::RequiresCapability: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned argsSize = Record.readInt();
    SmallVector<Expr *, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readExpr());
    New = new (Context) RequiresCapabilityAttr(Range, Context, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Restrict: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) RestrictAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ReturnTypestate: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    ReturnTypestateAttr::ConsumedState state(static_cast<ReturnTypestateAttr::ConsumedState>(Record.readInt()));
    New = new (Context) ReturnTypestateAttr(Range, Context, state, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ReturnsNonNull: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ReturnsNonNullAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ReturnsTwice: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ReturnsTwiceAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ScopedLockable: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ScopedLockableAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Section: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string name= Record.readString();
    New = new (Context) SectionAttr(Range, Context, name, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::SelectAny: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) SelectAnyAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Sentinel: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    int sentinel = Record.readInt();
    int nullPos = Record.readInt();
    New = new (Context) SentinelAttr(Range, Context, sentinel, nullPos, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::SetTypestate: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    SetTypestateAttr::ConsumedState newState(static_cast<SetTypestateAttr::ConsumedState>(Record.readInt()));
    New = new (Context) SetTypestateAttr(Range, Context, newState, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::SharedTrylockFunction: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * successValue = Record.readExpr();
    unsigned argsSize = Record.readInt();
    SmallVector<Expr *, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readExpr());
    New = new (Context) SharedTrylockFunctionAttr(Range, Context, successValue, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::StdCall: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) StdCallAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Suppress: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned diagnosticIdentifiersSize = Record.readInt();
    SmallVector<StringRef, 4> diagnosticIdentifiers;
    diagnosticIdentifiers.reserve(diagnosticIdentifiersSize);
    SmallVector<std::string, 4> diagnosticIdentifiersStorage;
    diagnosticIdentifiersStorage.reserve(diagnosticIdentifiersSize);
    for (unsigned i = 0; i != diagnosticIdentifiersSize; ++i)
      diagnosticIdentifiersStorage.push_back(Record.readString());
    for (unsigned i = 0; i != diagnosticIdentifiersSize; ++i)
      diagnosticIdentifiers.push_back(diagnosticIdentifiersStorage[i]);
    New = new (Context) SuppressAttr(Range, Context, diagnosticIdentifiers.data(), diagnosticIdentifiersSize, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::SwiftCall: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) SwiftCallAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::SwiftContext: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) SwiftContextAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::SwiftErrorResult: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) SwiftErrorResultAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::SwiftIndirectResult: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) SwiftIndirectResultAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::SysVABI: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) SysVABIAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::TLSModel: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string model= Record.readString();
    New = new (Context) TLSModelAttr(Range, Context, model, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Target: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string featuresStr= Record.readString();
    New = new (Context) TargetAttr(Range, Context, featuresStr, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::TestTypestate: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    TestTypestateAttr::ConsumedState testState(static_cast<TestTypestateAttr::ConsumedState>(Record.readInt()));
    New = new (Context) TestTypestateAttr(Range, Context, testState, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::ThisCall: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ThisCallAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Thread: {
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) ThreadAttr(Range, Context, Spelling);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::TransparentUnion: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) TransparentUnionAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::TryAcquireCapability: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    Expr * successValue = Record.readExpr();
    unsigned argsSize = Record.readInt();
    SmallVector<Expr *, 4> args;
    args.reserve(argsSize);
    for (unsigned i = 0; i != argsSize; ++i)
      args.push_back(Record.readExpr());
    New = new (Context) TryAcquireCapabilityAttr(Range, Context, successValue, args.data(), argsSize, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::TypeTagForDatatype: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    IdentifierInfo * argumentKind = Record.getIdentifierInfo();
    TypeSourceInfo * matchingCType = Record.getTypeSourceInfo();
    bool layoutCompatible = Record.readInt();
    bool mustBeNull = Record.readInt();
    New = new (Context) TypeTagForDatatypeAttr(Range, Context, argumentKind, matchingCType, layoutCompatible, mustBeNull, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::TypeVisibility: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    TypeVisibilityAttr::VisibilityType visibility(static_cast<TypeVisibilityAttr::VisibilityType>(Record.readInt()));
    New = new (Context) TypeVisibilityAttr(Range, Context, visibility, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Unavailable: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string message= Record.readString();
    UnavailableAttr::ImplicitReason implicitReason(static_cast<UnavailableAttr::ImplicitReason>(Record.readInt()));
    New = new (Context) UnavailableAttr(Range, Context, message, implicitReason, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Unused: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) UnusedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Used: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) UsedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Uuid: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string guid= Record.readString();
    New = new (Context) UuidAttr(Range, Context, guid, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::VecReturn: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) VecReturnAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::VecTypeHint: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    TypeSourceInfo * typeHint = Record.getTypeSourceInfo();
    New = new (Context) VecTypeHintAttr(Range, Context, typeHint, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::VectorCall: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) VectorCallAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Visibility: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    VisibilityAttr::VisibilityType visibility(static_cast<VisibilityAttr::VisibilityType>(Record.readInt()));
    New = new (Context) VisibilityAttr(Range, Context, visibility, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::WarnUnused: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) WarnUnusedAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::WarnUnusedResult: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) WarnUnusedResultAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::Weak: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) WeakAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::WeakImport: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) WeakImportAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::WeakRef: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    std::string aliasee= Record.readString();
    New = new (Context) WeakRefAttr(Range, Context, aliasee, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::WorkGroupSizeHint: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned xDim = Record.readInt();
    unsigned yDim = Record.readInt();
    unsigned zDim = Record.readInt();
    New = new (Context) WorkGroupSizeHintAttr(Range, Context, xDim, yDim, zDim, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::X86ForceAlignArgPointer: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) X86ForceAlignArgPointerAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::XRayInstrument: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    New = new (Context) XRayInstrumentAttr(Range, Context, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  case attr::XRayLogArgs: {
    bool isInherited = Record.readInt();
    bool isImplicit = Record.readInt();
    unsigned Spelling = Record.readInt();
    unsigned argumentCount = Record.readInt();
    New = new (Context) XRayLogArgsAttr(Range, Context, argumentCount, Spelling);
    cast<InheritableAttr>(New)->setInherited(isInherited);
    New->setImplicit(isImplicit);
    break;
  }
  }
