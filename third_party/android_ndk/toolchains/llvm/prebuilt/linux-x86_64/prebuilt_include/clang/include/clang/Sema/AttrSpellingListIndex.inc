/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Code to translate different attribute spellings into internal identifiers  *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

  switch (AttrKind) {
  case AT_AMDGPUFlatWorkGroupSize: {
    if (Name == "amdgpu_flat_work_group_size" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_AMDGPUNumSGPR: {
    if (Name == "amdgpu_num_sgpr" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_AMDGPUNumVGPR: {
    if (Name == "amdgpu_num_vgpr" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_AMDGPUWavesPerEU: {
    if (Name == "amdgpu_waves_per_eu" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Interrupt: {
    if (Name == "interrupt" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_AVRSignal: {
    if (Name == "signal" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_AbiTag: {
    if (Name == "abi_tag" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "abi_tag" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_AcquireCapability: {
    if (Name == "acquire_capability" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "acquire_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    if (Name == "acquire_shared_capability" && SyntaxUsed == 0 && Scope == "")
        return 2;
    if (Name == "acquire_shared_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 3;
    if (Name == "exclusive_lock_function" && SyntaxUsed == 0 && Scope == "")
        return 4;
    if (Name == "shared_lock_function" && SyntaxUsed == 0 && Scope == "")
        return 5;
    break;
  }
  case AT_AcquiredAfter: {
    if (Name == "acquired_after" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_AcquiredBefore: {
    if (Name == "acquired_before" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_AddressSpace: {
    if (Name == "address_space" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Alias: {
    if (Name == "alias" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "alias" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_AlignValue: {
    if (Name == "align_value" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Aligned: {
    if (Name == "aligned" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "aligned" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "align" && SyntaxUsed == 2 && Scope == "")
        return 2;
    if (Name == "alignas" && SyntaxUsed == 4 && Scope == "")
        return 3;
    if (Name == "_Alignas" && SyntaxUsed == 4 && Scope == "")
        return 4;
    break;
  }
  case AT_AllocAlign: {
    if (Name == "alloc_align" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "alloc_align" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_AllocSize: {
    if (Name == "alloc_size" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "alloc_size" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_AlwaysInline: {
    if (Name == "always_inline" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "always_inline" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "__forceinline" && SyntaxUsed == 4 && Scope == "")
        return 2;
    break;
  }
  case AT_AnalyzerNoReturn: {
    if (Name == "analyzer_noreturn" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Annotate: {
    if (Name == "annotate" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ArcWeakrefUnavailable: {
    if (Name == "objc_arc_weak_reference_unavailable" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ArgumentWithTypeTag: {
    if (Name == "argument_with_type_tag" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "pointer_with_type_tag" && SyntaxUsed == 0 && Scope == "")
        return 1;
    break;
  }
  case AT_AssertCapability: {
    if (Name == "assert_capability" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "assert_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    if (Name == "assert_shared_capability" && SyntaxUsed == 0 && Scope == "")
        return 2;
    if (Name == "assert_shared_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 3;
    break;
  }
  case AT_AssertExclusiveLock: {
    if (Name == "assert_exclusive_lock" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_AssertSharedLock: {
    if (Name == "assert_shared_lock" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_AssumeAligned: {
    if (Name == "assume_aligned" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "assume_aligned" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_Availability: {
    if (Name == "availability" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Blocks: {
    if (Name == "blocks" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_CDecl: {
    if (Name == "cdecl" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "cdecl" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "__cdecl" && SyntaxUsed == 4 && Scope == "")
        return 2;
    if (Name == "_cdecl" && SyntaxUsed == 4 && Scope == "")
        return 3;
    break;
  }
  case AT_CFAuditedTransfer: {
    if (Name == "cf_audited_transfer" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_CFConsumed: {
    if (Name == "cf_consumed" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_CFReturnsNotRetained: {
    if (Name == "cf_returns_not_retained" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_CFReturnsRetained: {
    if (Name == "cf_returns_retained" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_CFUnknownTransfer: {
    if (Name == "cf_unknown_transfer" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_CUDAConstant: {
    if (Name == "constant" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "__constant__" && SyntaxUsed == 2 && Scope == "")
        return 1;
    break;
  }
  case AT_CUDADevice: {
    if (Name == "device" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "__device__" && SyntaxUsed == 2 && Scope == "")
        return 1;
    break;
  }
  case AT_CUDAGlobal: {
    if (Name == "global" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "__global__" && SyntaxUsed == 2 && Scope == "")
        return 1;
    break;
  }
  case AT_CUDAHost: {
    if (Name == "host" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "__host__" && SyntaxUsed == 2 && Scope == "")
        return 1;
    break;
  }
  case AT_CUDAInvalidTarget: {
    break;
  }
  case AT_CUDALaunchBounds: {
    if (Name == "launch_bounds" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "__launch_bounds__" && SyntaxUsed == 2 && Scope == "")
        return 1;
    break;
  }
  case AT_CUDAShared: {
    if (Name == "shared" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "__shared__" && SyntaxUsed == 2 && Scope == "")
        return 1;
    break;
  }
  case AT_CXX11NoReturn: {
    if (Name == "noreturn" && SyntaxUsed == 1 && Scope == "")
        return 0;
    break;
  }
  case AT_CallableWhen: {
    if (Name == "callable_when" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Capability: {
    if (Name == "capability" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "capability" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    if (Name == "shared_capability" && SyntaxUsed == 0 && Scope == "")
        return 2;
    if (Name == "shared_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 3;
    break;
  }
  case AT_CarriesDependency: {
    if (Name == "carries_dependency" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "carries_dependency" && SyntaxUsed == 1 && Scope == "")
        return 1;
    break;
  }
  case AT_Cleanup: {
    if (Name == "cleanup" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "cleanup" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_Cold: {
    if (Name == "cold" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "cold" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_Common: {
    if (Name == "common" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "common" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_Const: {
    if (Name == "const" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "const" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "__const" && SyntaxUsed == 0 && Scope == "")
        return 2;
    if (Name == "__const" && SyntaxUsed == 1 && Scope == "gnu")
        return 3;
    break;
  }
  case AT_Constructor: {
    if (Name == "constructor" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "constructor" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_Consumable: {
    if (Name == "consumable" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ConsumableAutoCast: {
    if (Name == "consumable_auto_cast_state" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ConsumableSetOnRead: {
    if (Name == "consumable_set_state_on_read" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Convergent: {
    if (Name == "convergent" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "convergent" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_DLLExport: {
    if (Name == "dllexport" && SyntaxUsed == 2 && Scope == "")
        return 0;
    if (Name == "dllexport" && SyntaxUsed == 0 && Scope == "")
        return 1;
    if (Name == "dllexport" && SyntaxUsed == 1 && Scope == "gnu")
        return 2;
    break;
  }
  case AT_DLLImport: {
    if (Name == "dllimport" && SyntaxUsed == 2 && Scope == "")
        return 0;
    if (Name == "dllimport" && SyntaxUsed == 0 && Scope == "")
        return 1;
    if (Name == "dllimport" && SyntaxUsed == 1 && Scope == "gnu")
        return 2;
    break;
  }
  case AT_Deprecated: {
    if (Name == "deprecated" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "deprecated" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "deprecated" && SyntaxUsed == 2 && Scope == "")
        return 2;
    if (Name == "deprecated" && SyntaxUsed == 1 && Scope == "")
        return 3;
    break;
  }
  case AT_Destructor: {
    if (Name == "destructor" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "destructor" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_DiagnoseIf: {
    if (Name == "diagnose_if" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_DisableTailCalls: {
    if (Name == "disable_tail_calls" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "disable_tail_calls" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_EmptyBases: {
    if (Name == "empty_bases" && SyntaxUsed == 2 && Scope == "")
        return 0;
    break;
  }
  case AT_EnableIf: {
    if (Name == "enable_if" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_EnumExtensibility: {
    if (Name == "enum_extensibility" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "enum_extensibility" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_ExclusiveTrylockFunction: {
    if (Name == "exclusive_trylock_function" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ExtVectorType: {
    if (Name == "ext_vector_type" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ExternalSourceSymbol: {
    if (Name == "external_source_symbol" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "external_source_symbol" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_FallThrough: {
    if (Name == "fallthrough" && SyntaxUsed == 1 && Scope == "")
        return 0;
    if (Name == "fallthrough" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_FastCall: {
    if (Name == "fastcall" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "fastcall" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "__fastcall" && SyntaxUsed == 4 && Scope == "")
        return 2;
    if (Name == "_fastcall" && SyntaxUsed == 4 && Scope == "")
        return 3;
    break;
  }
  case AT_FlagEnum: {
    if (Name == "flag_enum" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Flatten: {
    if (Name == "flatten" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "flatten" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_Format: {
    if (Name == "format" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "format" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_FormatArg: {
    if (Name == "format_arg" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "format_arg" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_GNUInline: {
    if (Name == "gnu_inline" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "gnu_inline" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_GuardedBy: {
    if (Name == "guarded_by" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_GuardedVar: {
    if (Name == "guarded_var" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Hot: {
    if (Name == "hot" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "hot" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_IBAction: {
    if (Name == "ibaction" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_IBOutlet: {
    if (Name == "iboutlet" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_IBOutletCollection: {
    if (Name == "iboutletcollection" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_IFunc: {
    if (Name == "ifunc" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "ifunc" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_InitPriority: {
    if (Name == "init_priority" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_IntelOclBicc: {
    if (Name == "intel_ocl_bicc" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_InternalLinkage: {
    if (Name == "internal_linkage" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "internal_linkage" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_LTOVisibilityPublic: {
    if (Name == "lto_visibility_public" && SyntaxUsed == 1 && Scope == "clang")
        return 0;
    break;
  }
  case AT_LayoutVersion: {
    if (Name == "layout_version" && SyntaxUsed == 2 && Scope == "")
        return 0;
    break;
  }
  case AT_LockReturned: {
    if (Name == "lock_returned" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Lockable: {
    if (Name == "lockable" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_LocksExcluded: {
    if (Name == "locks_excluded" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_LoopHint: {
    if (Name == "loop" && SyntaxUsed == 5 && Scope == "clang")
        return 0;
    if (Name == "unroll" && SyntaxUsed == 5 && Scope == "")
        return 1;
    if (Name == "nounroll" && SyntaxUsed == 5 && Scope == "")
        return 2;
    break;
  }
  case AT_MSABI: {
    if (Name == "ms_abi" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "ms_abi" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_MSInheritance: {
    if (Name == "__single_inheritance" && SyntaxUsed == 4 && Scope == "")
        return 0;
    if (Name == "__multiple_inheritance" && SyntaxUsed == 4 && Scope == "")
        return 1;
    if (Name == "__virtual_inheritance" && SyntaxUsed == 4 && Scope == "")
        return 2;
    if (Name == "__unspecified_inheritance" && SyntaxUsed == 4 && Scope == "")
        return 3;
    break;
  }
  case AT_MSNoVTable: {
    if (Name == "novtable" && SyntaxUsed == 2 && Scope == "")
        return 0;
    break;
  }
  case AT_MSStruct: {
    if (Name == "ms_struct" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "ms_struct" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_MayAlias: {
    if (Name == "may_alias" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "may_alias" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_MinSize: {
    if (Name == "minsize" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Mips16: {
    if (Name == "mips16" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "mips16" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_Mode: {
    if (Name == "mode" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "mode" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_NSConsumed: {
    if (Name == "ns_consumed" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_NSConsumesSelf: {
    if (Name == "ns_consumes_self" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_NSReturnsAutoreleased: {
    if (Name == "ns_returns_autoreleased" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_NSReturnsNotRetained: {
    if (Name == "ns_returns_not_retained" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_NSReturnsRetained: {
    if (Name == "ns_returns_retained" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Naked: {
    if (Name == "naked" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "naked" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "naked" && SyntaxUsed == 2 && Scope == "")
        return 2;
    break;
  }
  case AT_NeonPolyVectorType: {
    if (Name == "neon_polyvector_type" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_NeonVectorType: {
    if (Name == "neon_vector_type" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_NoAlias: {
    if (Name == "noalias" && SyntaxUsed == 2 && Scope == "")
        return 0;
    break;
  }
  case AT_NoCommon: {
    if (Name == "nocommon" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "nocommon" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_NoDebug: {
    if (Name == "nodebug" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "nodebug" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_NoDuplicate: {
    if (Name == "noduplicate" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "noduplicate" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_NoInline: {
    if (Name == "noinline" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "noinline" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "noinline" && SyntaxUsed == 2 && Scope == "")
        return 2;
    break;
  }
  case AT_NoInstrumentFunction: {
    if (Name == "no_instrument_function" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "no_instrument_function" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_NoMips16: {
    if (Name == "nomips16" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "nomips16" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_NoReturn: {
    if (Name == "noreturn" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "noreturn" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "noreturn" && SyntaxUsed == 2 && Scope == "")
        return 2;
    break;
  }
  case AT_NoSanitize: {
    if (Name == "no_sanitize" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "no_sanitize" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_NoSanitizeSpecific: {
    if (Name == "no_address_safety_analysis" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "no_address_safety_analysis" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "no_sanitize_address" && SyntaxUsed == 0 && Scope == "")
        return 2;
    if (Name == "no_sanitize_address" && SyntaxUsed == 1 && Scope == "gnu")
        return 3;
    if (Name == "no_sanitize_thread" && SyntaxUsed == 0 && Scope == "")
        return 4;
    if (Name == "no_sanitize_thread" && SyntaxUsed == 1 && Scope == "gnu")
        return 5;
    if (Name == "no_sanitize_memory" && SyntaxUsed == 0 && Scope == "")
        return 6;
    break;
  }
  case AT_NoSplitStack: {
    if (Name == "no_split_stack" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "no_split_stack" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_NoThreadSafetyAnalysis: {
    if (Name == "no_thread_safety_analysis" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_NoThrow: {
    if (Name == "nothrow" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "nothrow" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "nothrow" && SyntaxUsed == 2 && Scope == "")
        return 2;
    break;
  }
  case AT_NonNull: {
    if (Name == "nonnull" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "nonnull" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_NotTailCalled: {
    if (Name == "not_tail_called" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "not_tail_called" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_ObjCBoxable: {
    if (Name == "objc_boxable" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCBridge: {
    if (Name == "objc_bridge" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCBridgeMutable: {
    if (Name == "objc_bridge_mutable" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCBridgeRelated: {
    if (Name == "objc_bridge_related" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCDesignatedInitializer: {
    if (Name == "objc_designated_initializer" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCException: {
    if (Name == "objc_exception" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCExplicitProtocolImpl: {
    if (Name == "objc_protocol_requires_explicit_implementation" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCGC: {
    if (Name == "objc_gc" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCIndependentClass: {
    if (Name == "objc_independent_class" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCKindOf: {
    if (Name == "__kindof" && SyntaxUsed == 4 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCMethodFamily: {
    if (Name == "objc_method_family" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCNSObject: {
    if (Name == "NSObject" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCOwnership: {
    if (Name == "objc_ownership" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCPreciseLifetime: {
    if (Name == "objc_precise_lifetime" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCRequiresPropertyDefs: {
    if (Name == "objc_requires_property_definitions" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCRequiresSuper: {
    if (Name == "objc_requires_super" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCReturnsInnerPointer: {
    if (Name == "objc_returns_inner_pointer" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCRootClass: {
    if (Name == "objc_root_class" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCRuntimeName: {
    if (Name == "objc_runtime_name" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCRuntimeVisible: {
    if (Name == "objc_runtime_visible" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ObjCSubclassingRestricted: {
    if (Name == "objc_subclassing_restricted" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_OpenCLAccess: {
    if (Name == "__read_only" && SyntaxUsed == 4 && Scope == "")
        return 0;
    if (Name == "read_only" && SyntaxUsed == 4 && Scope == "")
        return 1;
    if (Name == "__write_only" && SyntaxUsed == 4 && Scope == "")
        return 2;
    if (Name == "write_only" && SyntaxUsed == 4 && Scope == "")
        return 3;
    if (Name == "__read_write" && SyntaxUsed == 4 && Scope == "")
        return 4;
    if (Name == "read_write" && SyntaxUsed == 4 && Scope == "")
        return 5;
    break;
  }
  case AT_OpenCLConstantAddressSpace: {
    if (Name == "__constant" && SyntaxUsed == 4 && Scope == "")
        return 0;
    if (Name == "constant" && SyntaxUsed == 4 && Scope == "")
        return 1;
    break;
  }
  case AT_OpenCLGenericAddressSpace: {
    if (Name == "__generic" && SyntaxUsed == 4 && Scope == "")
        return 0;
    if (Name == "generic" && SyntaxUsed == 4 && Scope == "")
        return 1;
    break;
  }
  case AT_OpenCLGlobalAddressSpace: {
    if (Name == "__global" && SyntaxUsed == 4 && Scope == "")
        return 0;
    if (Name == "global" && SyntaxUsed == 4 && Scope == "")
        return 1;
    break;
  }
  case AT_OpenCLKernel: {
    if (Name == "__kernel" && SyntaxUsed == 4 && Scope == "")
        return 0;
    if (Name == "kernel" && SyntaxUsed == 4 && Scope == "")
        return 1;
    break;
  }
  case AT_OpenCLLocalAddressSpace: {
    if (Name == "__local" && SyntaxUsed == 4 && Scope == "")
        return 0;
    if (Name == "local" && SyntaxUsed == 4 && Scope == "")
        return 1;
    break;
  }
  case AT_OpenCLNoSVM: {
    if (Name == "nosvm" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_OpenCLPrivateAddressSpace: {
    if (Name == "__private" && SyntaxUsed == 4 && Scope == "")
        return 0;
    if (Name == "private" && SyntaxUsed == 4 && Scope == "")
        return 1;
    break;
  }
  case AT_OpenCLUnrollHint: {
    if (Name == "opencl_unroll_hint" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_OptimizeNone: {
    if (Name == "optnone" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "optnone" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_Overloadable: {
    if (Name == "overloadable" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Ownership: {
    if (Name == "ownership_holds" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "ownership_returns" && SyntaxUsed == 0 && Scope == "")
        return 1;
    if (Name == "ownership_takes" && SyntaxUsed == 0 && Scope == "")
        return 2;
    break;
  }
  case AT_Packed: {
    if (Name == "packed" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "packed" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_ParamTypestate: {
    if (Name == "param_typestate" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Pascal: {
    if (Name == "pascal" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "__pascal" && SyntaxUsed == 4 && Scope == "")
        return 1;
    if (Name == "_pascal" && SyntaxUsed == 4 && Scope == "")
        return 2;
    break;
  }
  case AT_PassObjectSize: {
    if (Name == "pass_object_size" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Pcs: {
    if (Name == "pcs" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "pcs" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_PreserveAll: {
    if (Name == "preserve_all" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_PreserveMost: {
    if (Name == "preserve_most" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_PtGuardedBy: {
    if (Name == "pt_guarded_by" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_PtGuardedVar: {
    if (Name == "pt_guarded_var" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Ptr32: {
    if (Name == "__ptr32" && SyntaxUsed == 4 && Scope == "")
        return 0;
    break;
  }
  case AT_Ptr64: {
    if (Name == "__ptr64" && SyntaxUsed == 4 && Scope == "")
        return 0;
    break;
  }
  case AT_Pure: {
    if (Name == "pure" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "pure" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_RegCall: {
    if (Name == "regcall" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "regcall" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "__regcall" && SyntaxUsed == 4 && Scope == "")
        return 2;
    break;
  }
  case AT_Regparm: {
    if (Name == "regparm" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "regparm" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_ReleaseCapability: {
    if (Name == "release_capability" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "release_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    if (Name == "release_shared_capability" && SyntaxUsed == 0 && Scope == "")
        return 2;
    if (Name == "release_shared_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 3;
    if (Name == "release_generic_capability" && SyntaxUsed == 0 && Scope == "")
        return 4;
    if (Name == "release_generic_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 5;
    if (Name == "unlock_function" && SyntaxUsed == 0 && Scope == "")
        return 6;
    break;
  }
  case AT_RenderScriptKernel: {
    if (Name == "kernel" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ReqdWorkGroupSize: {
    if (Name == "reqd_work_group_size" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_RequireConstantInit: {
    if (Name == "require_constant_initialization" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "require_constant_initialization" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_RequiresCapability: {
    if (Name == "requires_capability" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "requires_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    if (Name == "exclusive_locks_required" && SyntaxUsed == 0 && Scope == "")
        return 2;
    if (Name == "requires_shared_capability" && SyntaxUsed == 0 && Scope == "")
        return 3;
    if (Name == "requires_shared_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 4;
    if (Name == "shared_locks_required" && SyntaxUsed == 0 && Scope == "")
        return 5;
    break;
  }
  case AT_Restrict: {
    if (Name == "restrict" && SyntaxUsed == 2 && Scope == "")
        return 0;
    if (Name == "malloc" && SyntaxUsed == 0 && Scope == "")
        return 1;
    if (Name == "malloc" && SyntaxUsed == 1 && Scope == "gnu")
        return 2;
    break;
  }
  case AT_ReturnTypestate: {
    if (Name == "return_typestate" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ReturnsNonNull: {
    if (Name == "returns_nonnull" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "returns_nonnull" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_ReturnsTwice: {
    if (Name == "returns_twice" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "returns_twice" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_SPtr: {
    if (Name == "__sptr" && SyntaxUsed == 4 && Scope == "")
        return 0;
    break;
  }
  case AT_ScopedLockable: {
    if (Name == "scoped_lockable" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Section: {
    if (Name == "section" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "section" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "allocate" && SyntaxUsed == 2 && Scope == "")
        return 2;
    break;
  }
  case AT_SelectAny: {
    if (Name == "selectany" && SyntaxUsed == 2 && Scope == "")
        return 0;
    break;
  }
  case AT_Sentinel: {
    if (Name == "sentinel" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "sentinel" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_SetTypestate: {
    if (Name == "set_typestate" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_SharedTrylockFunction: {
    if (Name == "shared_trylock_function" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_StdCall: {
    if (Name == "stdcall" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "stdcall" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "__stdcall" && SyntaxUsed == 4 && Scope == "")
        return 2;
    if (Name == "_stdcall" && SyntaxUsed == 4 && Scope == "")
        return 3;
    break;
  }
  case AT_Suppress: {
    if (Name == "suppress" && SyntaxUsed == 1 && Scope == "gsl")
        return 0;
    break;
  }
  case AT_SwiftCall: {
    if (Name == "swiftcall" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "swiftcall" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_SwiftContext: {
    if (Name == "swift_context" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "swift_context" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_SwiftErrorResult: {
    if (Name == "swift_error_result" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "swift_error_result" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_SwiftIndirectResult: {
    if (Name == "swift_indirect_result" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "swift_indirect_result" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_SysVABI: {
    if (Name == "sysv_abi" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "sysv_abi" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_TLSModel: {
    if (Name == "tls_model" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "tls_model" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_Target: {
    if (Name == "target" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "target" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_TestTypestate: {
    if (Name == "test_typestate" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_ThisCall: {
    if (Name == "thiscall" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "thiscall" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    if (Name == "__thiscall" && SyntaxUsed == 4 && Scope == "")
        return 2;
    if (Name == "_thiscall" && SyntaxUsed == 4 && Scope == "")
        return 3;
    break;
  }
  case AT_Thread: {
    if (Name == "thread" && SyntaxUsed == 2 && Scope == "")
        return 0;
    break;
  }
  case AT_TransparentUnion: {
    if (Name == "transparent_union" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "transparent_union" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_TryAcquireCapability: {
    if (Name == "try_acquire_capability" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "try_acquire_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    if (Name == "try_acquire_shared_capability" && SyntaxUsed == 0 && Scope == "")
        return 2;
    if (Name == "try_acquire_shared_capability" && SyntaxUsed == 1 && Scope == "clang")
        return 3;
    break;
  }
  case AT_TypeNonNull: {
    if (Name == "_Nonnull" && SyntaxUsed == 4 && Scope == "")
        return 0;
    break;
  }
  case AT_TypeNullUnspecified: {
    if (Name == "_Null_unspecified" && SyntaxUsed == 4 && Scope == "")
        return 0;
    break;
  }
  case AT_TypeNullable: {
    if (Name == "_Nullable" && SyntaxUsed == 4 && Scope == "")
        return 0;
    break;
  }
  case AT_TypeTagForDatatype: {
    if (Name == "type_tag_for_datatype" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_TypeVisibility: {
    if (Name == "type_visibility" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "type_visibility" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  case AT_UPtr: {
    if (Name == "__uptr" && SyntaxUsed == 4 && Scope == "")
        return 0;
    break;
  }
  case AT_Unavailable: {
    if (Name == "unavailable" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_Unused: {
    if (Name == "maybe_unused" && SyntaxUsed == 1 && Scope == "")
        return 0;
    if (Name == "unused" && SyntaxUsed == 0 && Scope == "")
        return 1;
    if (Name == "unused" && SyntaxUsed == 1 && Scope == "gnu")
        return 2;
    break;
  }
  case AT_Used: {
    if (Name == "used" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "used" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_Uuid: {
    if (Name == "uuid" && SyntaxUsed == 2 && Scope == "")
        return 0;
    if (Name == "uuid" && SyntaxUsed == 3 && Scope == "")
        return 1;
    break;
  }
  case AT_VecReturn: {
    if (Name == "vecreturn" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_VecTypeHint: {
    if (Name == "vec_type_hint" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_VectorCall: {
    if (Name == "vectorcall" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "__vectorcall" && SyntaxUsed == 4 && Scope == "")
        return 1;
    if (Name == "_vectorcall" && SyntaxUsed == 4 && Scope == "")
        return 2;
    break;
  }
  case AT_VectorSize: {
    if (Name == "vector_size" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "vector_size" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_Visibility: {
    if (Name == "visibility" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "visibility" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_WarnUnused: {
    if (Name == "warn_unused" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_WarnUnusedResult: {
    if (Name == "nodiscard" && SyntaxUsed == 1 && Scope == "")
        return 0;
    if (Name == "warn_unused_result" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    if (Name == "warn_unused_result" && SyntaxUsed == 0 && Scope == "")
        return 2;
    if (Name == "warn_unused_result" && SyntaxUsed == 1 && Scope == "gnu")
        return 3;
    break;
  }
  case AT_Weak: {
    if (Name == "weak" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "weak" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_WeakImport: {
    if (Name == "weak_import" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_WeakRef: {
    if (Name == "weakref" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "weakref" && SyntaxUsed == 1 && Scope == "gnu")
        return 1;
    break;
  }
  case AT_WorkGroupSizeHint: {
    if (Name == "work_group_size_hint" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_X86ForceAlignArgPointer: {
    if (Name == "force_align_arg_pointer" && SyntaxUsed == 0 && Scope == "")
        return 0;
    break;
  }
  case AT_XRayInstrument: {
    if (Name == "xray_always_instrument" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "xray_always_instrument" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    if (Name == "xray_never_instrument" && SyntaxUsed == 0 && Scope == "")
        return 2;
    if (Name == "xray_never_instrument" && SyntaxUsed == 1 && Scope == "clang")
        return 3;
    break;
  }
  case AT_XRayLogArgs: {
    if (Name == "xray_log_args" && SyntaxUsed == 0 && Scope == "")
        return 0;
    if (Name == "xray_log_args" && SyntaxUsed == 1 && Scope == "clang")
        return 1;
    break;
  }
  }
  return 0;
