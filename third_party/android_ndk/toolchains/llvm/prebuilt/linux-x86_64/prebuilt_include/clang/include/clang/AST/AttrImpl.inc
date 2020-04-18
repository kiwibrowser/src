/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Attribute classes' member function definitions                             *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

AMDGPUFlatWorkGroupSizeAttr *AMDGPUFlatWorkGroupSizeAttr::clone(ASTContext &C) const {
  auto *A = new (C) AMDGPUFlatWorkGroupSizeAttr(getLocation(), C, min, max, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AMDGPUFlatWorkGroupSizeAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((amdgpu_flat_work_group_size(" << getMin() << ", " << getMax() << ")))";
    break;
  }
}
}

const char *AMDGPUFlatWorkGroupSizeAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "amdgpu_flat_work_group_size";
  }
}

AMDGPUNumSGPRAttr *AMDGPUNumSGPRAttr::clone(ASTContext &C) const {
  auto *A = new (C) AMDGPUNumSGPRAttr(getLocation(), C, numSGPR, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AMDGPUNumSGPRAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((amdgpu_num_sgpr(" << getNumSGPR() << ")))";
    break;
  }
}
}

const char *AMDGPUNumSGPRAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "amdgpu_num_sgpr";
  }
}

AMDGPUNumVGPRAttr *AMDGPUNumVGPRAttr::clone(ASTContext &C) const {
  auto *A = new (C) AMDGPUNumVGPRAttr(getLocation(), C, numVGPR, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AMDGPUNumVGPRAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((amdgpu_num_vgpr(" << getNumVGPR() << ")))";
    break;
  }
}
}

const char *AMDGPUNumVGPRAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "amdgpu_num_vgpr";
  }
}

AMDGPUWavesPerEUAttr *AMDGPUWavesPerEUAttr::clone(ASTContext &C) const {
  auto *A = new (C) AMDGPUWavesPerEUAttr(getLocation(), C, min, max, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AMDGPUWavesPerEUAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((amdgpu_waves_per_eu(" << getMin() << ", " << getMax() << ")))";
    break;
  }
}
}

const char *AMDGPUWavesPerEUAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "amdgpu_waves_per_eu";
  }
}

ARMInterruptAttr *ARMInterruptAttr::clone(ASTContext &C) const {
  auto *A = new (C) ARMInterruptAttr(getLocation(), C, interrupt, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ARMInterruptAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((interrupt(\"" << ARMInterruptAttr::ConvertInterruptTypeToStr(getInterrupt()) << "\")))";
    break;
  }
}
}

const char *ARMInterruptAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "interrupt";
  }
}

AVRInterruptAttr *AVRInterruptAttr::clone(ASTContext &C) const {
  auto *A = new (C) AVRInterruptAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AVRInterruptAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((interrupt))";
    break;
  }
}
}

const char *AVRInterruptAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "interrupt";
  }
}

AVRSignalAttr *AVRSignalAttr::clone(ASTContext &C) const {
  auto *A = new (C) AVRSignalAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AVRSignalAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((signal))";
    break;
  }
}
}

const char *AVRSignalAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "signal";
  }
}

AbiTagAttr *AbiTagAttr::clone(ASTContext &C) const {
  auto *A = new (C) AbiTagAttr(getLocation(), C, tags_, tags_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AbiTagAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((abi_tag(";
  bool isFirst = true;
  for (const auto &Val : tags()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << "\"" << Val << "\"";
  }
  OS << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::abi_tag(";
  bool isFirst = true;
  for (const auto &Val : tags()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << "\"" << Val << "\"";
  }
  OS << ")]]";
    break;
  }
}
}

const char *AbiTagAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "abi_tag";
  case 1:
    return "abi_tag";
  }
}

AcquireCapabilityAttr *AcquireCapabilityAttr::clone(ASTContext &C) const {
  auto *A = new (C) AcquireCapabilityAttr(getLocation(), C, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AcquireCapabilityAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((acquire_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 1 : {
    OS << " [[clang::acquire_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")]]";
    break;
  }
  case 2 : {
    OS << " __attribute__((acquire_shared_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 3 : {
    OS << " [[clang::acquire_shared_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")]]";
    break;
  }
  case 4 : {
    OS << " __attribute__((exclusive_lock_function(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 5 : {
    OS << " __attribute__((shared_lock_function(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
}
}

const char *AcquireCapabilityAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "acquire_capability";
  case 1:
    return "acquire_capability";
  case 2:
    return "acquire_shared_capability";
  case 3:
    return "acquire_shared_capability";
  case 4:
    return "exclusive_lock_function";
  case 5:
    return "shared_lock_function";
  }
}

AcquiredAfterAttr *AcquiredAfterAttr::clone(ASTContext &C) const {
  auto *A = new (C) AcquiredAfterAttr(getLocation(), C, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AcquiredAfterAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((acquired_after(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
}
}

const char *AcquiredAfterAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "acquired_after";
  }
}

AcquiredBeforeAttr *AcquiredBeforeAttr::clone(ASTContext &C) const {
  auto *A = new (C) AcquiredBeforeAttr(getLocation(), C, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AcquiredBeforeAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((acquired_before(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
}
}

const char *AcquiredBeforeAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "acquired_before";
  }
}

AliasAttr *AliasAttr::clone(ASTContext &C) const {
  auto *A = new (C) AliasAttr(getLocation(), C, getAliasee(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AliasAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((alias(\"" << getAliasee() << "\")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::alias(\"" << getAliasee() << "\")]]";
    break;
  }
}
}

const char *AliasAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "alias";
  case 1:
    return "alias";
  }
}

AlignMac68kAttr *AlignMac68kAttr::clone(ASTContext &C) const {
  auto *A = new (C) AlignMac68kAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AlignMac68kAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
}

const char *AlignMac68kAttr::getSpelling() const {
  return "(No spelling)";
}

AlignValueAttr *AlignValueAttr::clone(ASTContext &C) const {
  auto *A = new (C) AlignValueAttr(getLocation(), C, alignment, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AlignValueAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((align_value(" << getAlignment() << ")))";
    break;
  }
}
}

const char *AlignValueAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "align_value";
  }
}

bool AlignedAttr::isAlignmentDependent() const {
  if (isalignmentExpr)
    return alignmentExpr && (alignmentExpr->isValueDependent() || alignmentExpr->isTypeDependent());
  else
    return alignmentType->getType()->isDependentType();
}
unsigned AlignedAttr::getAlignment(ASTContext &Ctx) const {
  assert(!isAlignmentDependent());
  if (isalignmentExpr)
    return alignmentExpr ? alignmentExpr->EvaluateKnownConstInt(Ctx).getZExtValue() * Ctx.getCharWidth() : Ctx.getTargetDefaultAlignForAttributeAligned();
  else
    return 0; // FIXME
}
AlignedAttr *AlignedAttr::clone(ASTContext &C) const {
  auto *A = new (C) AlignedAttr(getLocation(), C, isalignmentExpr, isalignmentExpr ? static_cast<void*>(alignmentExpr) : alignmentType, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AlignedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((aligned(";
    if (isalignmentExpr && alignmentExpr)
      alignmentExpr->printPretty(OS, nullptr, Policy);
    OS << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::aligned(";
    if (isalignmentExpr && alignmentExpr)
      alignmentExpr->printPretty(OS, nullptr, Policy);
    OS << ")]]";
    break;
  }
  case 2 : {
    OS << " __declspec(align(";
    if (isalignmentExpr && alignmentExpr)
      alignmentExpr->printPretty(OS, nullptr, Policy);
    OS << "))";
    break;
  }
  case 3 : {
    OS << " alignas(";
    if (isalignmentExpr && alignmentExpr)
      alignmentExpr->printPretty(OS, nullptr, Policy);
    OS << ")";
    break;
  }
  case 4 : {
    OS << " _Alignas(";
    if (isalignmentExpr && alignmentExpr)
      alignmentExpr->printPretty(OS, nullptr, Policy);
    OS << ")";
    break;
  }
}
}

const char *AlignedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "aligned";
  case 1:
    return "aligned";
  case 2:
    return "align";
  case 3:
    return "alignas";
  case 4:
    return "_Alignas";
  }
}

AllocAlignAttr *AllocAlignAttr::clone(ASTContext &C) const {
  auto *A = new (C) AllocAlignAttr(getLocation(), C, paramIndex, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AllocAlignAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((alloc_align(" << getParamIndex() << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::alloc_align(" << getParamIndex() << ")]]";
    break;
  }
}
}

const char *AllocAlignAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "alloc_align";
  case 1:
    return "alloc_align";
  }
}

AllocSizeAttr *AllocSizeAttr::clone(ASTContext &C) const {
  auto *A = new (C) AllocSizeAttr(getLocation(), C, elemSizeParam, numElemsParam, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AllocSizeAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((alloc_size(" << getElemSizeParam() << ", " << getNumElemsParam() << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::alloc_size(" << getElemSizeParam() << ", " << getNumElemsParam() << ")]]";
    break;
  }
}
}

const char *AllocSizeAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "alloc_size";
  case 1:
    return "alloc_size";
  }
}

AlwaysInlineAttr *AlwaysInlineAttr::clone(ASTContext &C) const {
  auto *A = new (C) AlwaysInlineAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AlwaysInlineAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((always_inline))";
    break;
  }
  case 1 : {
    OS << " [[gnu::always_inline]]";
    break;
  }
  case 2 : {
    OS << " __forceinline";
    break;
  }
}
}

const char *AlwaysInlineAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "always_inline";
  case 1:
    return "always_inline";
  case 2:
    return "__forceinline";
  }
}

AnalyzerNoReturnAttr *AnalyzerNoReturnAttr::clone(ASTContext &C) const {
  auto *A = new (C) AnalyzerNoReturnAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AnalyzerNoReturnAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((analyzer_noreturn))";
    break;
  }
}
}

const char *AnalyzerNoReturnAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "analyzer_noreturn";
  }
}

AnnotateAttr *AnnotateAttr::clone(ASTContext &C) const {
  auto *A = new (C) AnnotateAttr(getLocation(), C, getAnnotation(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AnnotateAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((annotate(\"" << getAnnotation() << "\")))";
    break;
  }
}
}

const char *AnnotateAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "annotate";
  }
}

AnyX86InterruptAttr *AnyX86InterruptAttr::clone(ASTContext &C) const {
  auto *A = new (C) AnyX86InterruptAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AnyX86InterruptAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((interrupt))";
    break;
  }
}
}

const char *AnyX86InterruptAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "interrupt";
  }
}

ArcWeakrefUnavailableAttr *ArcWeakrefUnavailableAttr::clone(ASTContext &C) const {
  auto *A = new (C) ArcWeakrefUnavailableAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ArcWeakrefUnavailableAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_arc_weak_reference_unavailable))";
    break;
  }
}
}

const char *ArcWeakrefUnavailableAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_arc_weak_reference_unavailable";
  }
}

ArgumentWithTypeTagAttr *ArgumentWithTypeTagAttr::clone(ASTContext &C) const {
  auto *A = new (C) ArgumentWithTypeTagAttr(getLocation(), C, argumentKind, argumentIdx, typeTagIdx, isPointer, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ArgumentWithTypeTagAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((argument_with_type_tag(";
    OS << getArgumentKind()->getName();
    OS << ", " << getArgumentIdx() << ", " << getTypeTagIdx() << ", " << getIsPointer() << ")))";
    break;
  }
  case 1 : {
    OS << " __attribute__((pointer_with_type_tag(";
    OS << getArgumentKind()->getName();
    OS << ", " << getArgumentIdx() << ", " << getTypeTagIdx() << ", " << getIsPointer() << ")))";
    break;
  }
}
}

const char *ArgumentWithTypeTagAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "argument_with_type_tag";
  case 1:
    return "pointer_with_type_tag";
  }
}

AsmLabelAttr *AsmLabelAttr::clone(ASTContext &C) const {
  auto *A = new (C) AsmLabelAttr(getLocation(), C, getLabel(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AsmLabelAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " asm(\"" << getLabel() << "\")";
    break;
  }
  case 1 : {
    OS << " __asm__(\"" << getLabel() << "\")";
    break;
  }
}
}

const char *AsmLabelAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "asm";
  case 1:
    return "__asm__";
  }
}

AssertCapabilityAttr *AssertCapabilityAttr::clone(ASTContext &C) const {
  auto *A = new (C) AssertCapabilityAttr(getLocation(), C, expr, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AssertCapabilityAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((assert_capability(" << getExpr() << ")))";
    break;
  }
  case 1 : {
    OS << " [[clang::assert_capability(" << getExpr() << ")]]";
    break;
  }
  case 2 : {
    OS << " __attribute__((assert_shared_capability(" << getExpr() << ")))";
    break;
  }
  case 3 : {
    OS << " [[clang::assert_shared_capability(" << getExpr() << ")]]";
    break;
  }
}
}

const char *AssertCapabilityAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "assert_capability";
  case 1:
    return "assert_capability";
  case 2:
    return "assert_shared_capability";
  case 3:
    return "assert_shared_capability";
  }
}

AssertExclusiveLockAttr *AssertExclusiveLockAttr::clone(ASTContext &C) const {
  auto *A = new (C) AssertExclusiveLockAttr(getLocation(), C, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AssertExclusiveLockAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((assert_exclusive_lock(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
}
}

const char *AssertExclusiveLockAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "assert_exclusive_lock";
  }
}

AssertSharedLockAttr *AssertSharedLockAttr::clone(ASTContext &C) const {
  auto *A = new (C) AssertSharedLockAttr(getLocation(), C, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AssertSharedLockAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((assert_shared_lock(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
}
}

const char *AssertSharedLockAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "assert_shared_lock";
  }
}

AssumeAlignedAttr *AssumeAlignedAttr::clone(ASTContext &C) const {
  auto *A = new (C) AssumeAlignedAttr(getLocation(), C, alignment, offset, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AssumeAlignedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((assume_aligned(" << getAlignment() << ", " << getOffset() << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::assume_aligned(" << getAlignment() << ", " << getOffset() << ")]]";
    break;
  }
}
}

const char *AssumeAlignedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "assume_aligned";
  case 1:
    return "assume_aligned";
  }
}

AvailabilityAttr *AvailabilityAttr::clone(ASTContext &C) const {
  auto *A = new (C) AvailabilityAttr(getLocation(), C, platform, getIntroduced(), getDeprecated(), getObsoleted(), unavailable, getMessage(), strict, getReplacement(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void AvailabilityAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((availability(" << getPlatform()->getName();
  if (getStrict()) OS << ", strict";
  if (!getIntroduced().empty()) OS << ", introduced=" << getIntroduced();
  if (!getDeprecated().empty()) OS << ", deprecated=" << getDeprecated();
  if (!getObsoleted().empty()) OS << ", obsoleted=" << getObsoleted();
  if (getUnavailable()) OS << ", unavailable";
  OS << ")))";
    break;
  }
}
}

const char *AvailabilityAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "availability";
  }
}

BlocksAttr *BlocksAttr::clone(ASTContext &C) const {
  auto *A = new (C) BlocksAttr(getLocation(), C, type, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void BlocksAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((blocks(\"" << BlocksAttr::ConvertBlockTypeToStr(getType()) << "\")))";
    break;
  }
}
}

const char *BlocksAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "blocks";
  }
}

C11NoReturnAttr *C11NoReturnAttr::clone(ASTContext &C) const {
  auto *A = new (C) C11NoReturnAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void C11NoReturnAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " _Noreturn";
    break;
  }
}
}

const char *C11NoReturnAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "_Noreturn";
  }
}

CDeclAttr *CDeclAttr::clone(ASTContext &C) const {
  auto *A = new (C) CDeclAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CDeclAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((cdecl))";
    break;
  }
  case 1 : {
    OS << " [[gnu::cdecl]]";
    break;
  }
  case 2 : {
    OS << " __cdecl";
    break;
  }
  case 3 : {
    OS << " _cdecl";
    break;
  }
}
}

const char *CDeclAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "cdecl";
  case 1:
    return "cdecl";
  case 2:
    return "__cdecl";
  case 3:
    return "_cdecl";
  }
}

CFAuditedTransferAttr *CFAuditedTransferAttr::clone(ASTContext &C) const {
  auto *A = new (C) CFAuditedTransferAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CFAuditedTransferAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((cf_audited_transfer))";
    break;
  }
}
}

const char *CFAuditedTransferAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "cf_audited_transfer";
  }
}

CFConsumedAttr *CFConsumedAttr::clone(ASTContext &C) const {
  auto *A = new (C) CFConsumedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CFConsumedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((cf_consumed))";
    break;
  }
}
}

const char *CFConsumedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "cf_consumed";
  }
}

CFReturnsNotRetainedAttr *CFReturnsNotRetainedAttr::clone(ASTContext &C) const {
  auto *A = new (C) CFReturnsNotRetainedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CFReturnsNotRetainedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((cf_returns_not_retained))";
    break;
  }
}
}

const char *CFReturnsNotRetainedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "cf_returns_not_retained";
  }
}

CFReturnsRetainedAttr *CFReturnsRetainedAttr::clone(ASTContext &C) const {
  auto *A = new (C) CFReturnsRetainedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CFReturnsRetainedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((cf_returns_retained))";
    break;
  }
}
}

const char *CFReturnsRetainedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "cf_returns_retained";
  }
}

CFUnknownTransferAttr *CFUnknownTransferAttr::clone(ASTContext &C) const {
  auto *A = new (C) CFUnknownTransferAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CFUnknownTransferAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((cf_unknown_transfer))";
    break;
  }
}
}

const char *CFUnknownTransferAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "cf_unknown_transfer";
  }
}

CUDAConstantAttr *CUDAConstantAttr::clone(ASTContext &C) const {
  auto *A = new (C) CUDAConstantAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CUDAConstantAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((constant))";
    break;
  }
  case 1 : {
    OS << " __declspec(__constant__)";
    break;
  }
}
}

const char *CUDAConstantAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "constant";
  case 1:
    return "__constant__";
  }
}

CUDADeviceAttr *CUDADeviceAttr::clone(ASTContext &C) const {
  auto *A = new (C) CUDADeviceAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CUDADeviceAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((device))";
    break;
  }
  case 1 : {
    OS << " __declspec(__device__)";
    break;
  }
}
}

const char *CUDADeviceAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "device";
  case 1:
    return "__device__";
  }
}

CUDAGlobalAttr *CUDAGlobalAttr::clone(ASTContext &C) const {
  auto *A = new (C) CUDAGlobalAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CUDAGlobalAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((global))";
    break;
  }
  case 1 : {
    OS << " __declspec(__global__)";
    break;
  }
}
}

const char *CUDAGlobalAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "global";
  case 1:
    return "__global__";
  }
}

CUDAHostAttr *CUDAHostAttr::clone(ASTContext &C) const {
  auto *A = new (C) CUDAHostAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CUDAHostAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((host))";
    break;
  }
  case 1 : {
    OS << " __declspec(__host__)";
    break;
  }
}
}

const char *CUDAHostAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "host";
  case 1:
    return "__host__";
  }
}

CUDAInvalidTargetAttr *CUDAInvalidTargetAttr::clone(ASTContext &C) const {
  auto *A = new (C) CUDAInvalidTargetAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CUDAInvalidTargetAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
}

const char *CUDAInvalidTargetAttr::getSpelling() const {
  return "(No spelling)";
}

CUDALaunchBoundsAttr *CUDALaunchBoundsAttr::clone(ASTContext &C) const {
  auto *A = new (C) CUDALaunchBoundsAttr(getLocation(), C, maxThreads, minBlocks, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CUDALaunchBoundsAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((launch_bounds(" << getMaxThreads() << ", " << getMinBlocks() << ")))";
    break;
  }
  case 1 : {
    OS << " __declspec(__launch_bounds__(" << getMaxThreads() << ", " << getMinBlocks() << "))";
    break;
  }
}
}

const char *CUDALaunchBoundsAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "launch_bounds";
  case 1:
    return "__launch_bounds__";
  }
}

CUDASharedAttr *CUDASharedAttr::clone(ASTContext &C) const {
  auto *A = new (C) CUDASharedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CUDASharedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((shared))";
    break;
  }
  case 1 : {
    OS << " __declspec(__shared__)";
    break;
  }
}
}

const char *CUDASharedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "shared";
  case 1:
    return "__shared__";
  }
}

CXX11NoReturnAttr *CXX11NoReturnAttr::clone(ASTContext &C) const {
  auto *A = new (C) CXX11NoReturnAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CXX11NoReturnAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " [[noreturn]]";
    break;
  }
}
}

const char *CXX11NoReturnAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "noreturn";
  }
}

CallableWhenAttr *CallableWhenAttr::clone(ASTContext &C) const {
  auto *A = new (C) CallableWhenAttr(getLocation(), C, callableStates_, callableStates_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CallableWhenAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((callable_when(";
  bool isFirst = true;
  for (const auto &Val : callableStates()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << "\"" << CallableWhenAttr::ConvertConsumedStateToStr(Val)<< "\"";
  }
  OS << ")))";
    break;
  }
}
}

const char *CallableWhenAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "callable_when";
  }
}

CapabilityAttr *CapabilityAttr::clone(ASTContext &C) const {
  auto *A = new (C) CapabilityAttr(getLocation(), C, getName(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CapabilityAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((capability(\"" << getName() << "\")))";
    break;
  }
  case 1 : {
    OS << " [[clang::capability(\"" << getName() << "\")]]";
    break;
  }
  case 2 : {
    OS << " __attribute__((shared_capability(\"" << getName() << "\")))";
    break;
  }
  case 3 : {
    OS << " [[clang::shared_capability(\"" << getName() << "\")]]";
    break;
  }
}
}

const char *CapabilityAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "capability";
  case 1:
    return "capability";
  case 2:
    return "shared_capability";
  case 3:
    return "shared_capability";
  }
}

CapturedRecordAttr *CapturedRecordAttr::clone(ASTContext &C) const {
  auto *A = new (C) CapturedRecordAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CapturedRecordAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
}

const char *CapturedRecordAttr::getSpelling() const {
  return "(No spelling)";
}

CarriesDependencyAttr *CarriesDependencyAttr::clone(ASTContext &C) const {
  auto *A = new (C) CarriesDependencyAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CarriesDependencyAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((carries_dependency))";
    break;
  }
  case 1 : {
    OS << " [[carries_dependency]]";
    break;
  }
}
}

const char *CarriesDependencyAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "carries_dependency";
  case 1:
    return "carries_dependency";
  }
}

CleanupAttr *CleanupAttr::clone(ASTContext &C) const {
  auto *A = new (C) CleanupAttr(getLocation(), C, functionDecl, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CleanupAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((cleanup(" << getFunctionDecl()->getNameInfo().getAsString() << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::cleanup(" << getFunctionDecl()->getNameInfo().getAsString() << ")]]";
    break;
  }
}
}

const char *CleanupAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "cleanup";
  case 1:
    return "cleanup";
  }
}

ColdAttr *ColdAttr::clone(ASTContext &C) const {
  auto *A = new (C) ColdAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ColdAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((cold))";
    break;
  }
  case 1 : {
    OS << " [[gnu::cold]]";
    break;
  }
}
}

const char *ColdAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "cold";
  case 1:
    return "cold";
  }
}

CommonAttr *CommonAttr::clone(ASTContext &C) const {
  auto *A = new (C) CommonAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void CommonAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((common))";
    break;
  }
  case 1 : {
    OS << " [[gnu::common]]";
    break;
  }
}
}

const char *CommonAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "common";
  case 1:
    return "common";
  }
}

ConstAttr *ConstAttr::clone(ASTContext &C) const {
  auto *A = new (C) ConstAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ConstAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((const))";
    break;
  }
  case 1 : {
    OS << " [[gnu::const]]";
    break;
  }
  case 2 : {
    OS << " __attribute__((__const))";
    break;
  }
  case 3 : {
    OS << " [[gnu::__const]]";
    break;
  }
}
}

const char *ConstAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "const";
  case 1:
    return "const";
  case 2:
    return "__const";
  case 3:
    return "__const";
  }
}

ConstructorAttr *ConstructorAttr::clone(ASTContext &C) const {
  auto *A = new (C) ConstructorAttr(getLocation(), C, priority, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ConstructorAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((constructor(" << getPriority() << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::constructor(" << getPriority() << ")]]";
    break;
  }
}
}

const char *ConstructorAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "constructor";
  case 1:
    return "constructor";
  }
}

ConsumableAttr *ConsumableAttr::clone(ASTContext &C) const {
  auto *A = new (C) ConsumableAttr(getLocation(), C, defaultState, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ConsumableAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((consumable(\"" << ConsumableAttr::ConvertConsumedStateToStr(getDefaultState()) << "\")))";
    break;
  }
}
}

const char *ConsumableAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "consumable";
  }
}

ConsumableAutoCastAttr *ConsumableAutoCastAttr::clone(ASTContext &C) const {
  auto *A = new (C) ConsumableAutoCastAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ConsumableAutoCastAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((consumable_auto_cast_state))";
    break;
  }
}
}

const char *ConsumableAutoCastAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "consumable_auto_cast_state";
  }
}

ConsumableSetOnReadAttr *ConsumableSetOnReadAttr::clone(ASTContext &C) const {
  auto *A = new (C) ConsumableSetOnReadAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ConsumableSetOnReadAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((consumable_set_state_on_read))";
    break;
  }
}
}

const char *ConsumableSetOnReadAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "consumable_set_state_on_read";
  }
}

ConvergentAttr *ConvergentAttr::clone(ASTContext &C) const {
  auto *A = new (C) ConvergentAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ConvergentAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((convergent))";
    break;
  }
  case 1 : {
    OS << " [[clang::convergent]]";
    break;
  }
}
}

const char *ConvergentAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "convergent";
  case 1:
    return "convergent";
  }
}

DLLExportAttr *DLLExportAttr::clone(ASTContext &C) const {
  auto *A = new (C) DLLExportAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void DLLExportAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __declspec(dllexport)";
    break;
  }
  case 1 : {
    OS << " __attribute__((dllexport))";
    break;
  }
  case 2 : {
    OS << " [[gnu::dllexport]]";
    break;
  }
}
}

const char *DLLExportAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "dllexport";
  case 1:
    return "dllexport";
  case 2:
    return "dllexport";
  }
}

DLLImportAttr *DLLImportAttr::clone(ASTContext &C) const {
  auto *A = new (C) DLLImportAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void DLLImportAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __declspec(dllimport)";
    break;
  }
  case 1 : {
    OS << " __attribute__((dllimport))";
    break;
  }
  case 2 : {
    OS << " [[gnu::dllimport]]";
    break;
  }
}
}

const char *DLLImportAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "dllimport";
  case 1:
    return "dllimport";
  case 2:
    return "dllimport";
  }
}

DeprecatedAttr *DeprecatedAttr::clone(ASTContext &C) const {
  auto *A = new (C) DeprecatedAttr(getLocation(), C, getMessage(), getReplacement(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void DeprecatedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((deprecated(\"" << getMessage() << "\"";
    if (!getReplacement().empty()) OS << ", \"" << getReplacement() << "\"";
    OS << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::deprecated(\"" << getMessage() << "\"";
    OS << ")]]";
    break;
  }
  case 2 : {
    OS << " __declspec(deprecated(\"" << getMessage() << "\"";
    OS << "))";
    break;
  }
  case 3 : {
    OS << " [[deprecated(\"" << getMessage() << "\"";
    OS << ")]]";
    break;
  }
}
}

const char *DeprecatedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "deprecated";
  case 1:
    return "deprecated";
  case 2:
    return "deprecated";
  case 3:
    return "deprecated";
  }
}

DestructorAttr *DestructorAttr::clone(ASTContext &C) const {
  auto *A = new (C) DestructorAttr(getLocation(), C, priority, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void DestructorAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((destructor(" << getPriority() << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::destructor(" << getPriority() << ")]]";
    break;
  }
}
}

const char *DestructorAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "destructor";
  case 1:
    return "destructor";
  }
}

DiagnoseIfAttr *DiagnoseIfAttr::clone(ASTContext &C) const {
  auto *A = new (C) DiagnoseIfAttr(getLocation(), C, cond, getMessage(), diagnosticType, argDependent, parent, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void DiagnoseIfAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((diagnose_if(" << getCond() << ", \"" << getMessage() << "\", \"" << DiagnoseIfAttr::ConvertDiagnosticTypeToStr(getDiagnosticType()) << "\")))";
    break;
  }
}
}

const char *DiagnoseIfAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "diagnose_if";
  }
}

DisableTailCallsAttr *DisableTailCallsAttr::clone(ASTContext &C) const {
  auto *A = new (C) DisableTailCallsAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void DisableTailCallsAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((disable_tail_calls))";
    break;
  }
  case 1 : {
    OS << " [[clang::disable_tail_calls]]";
    break;
  }
}
}

const char *DisableTailCallsAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "disable_tail_calls";
  case 1:
    return "disable_tail_calls";
  }
}

EmptyBasesAttr *EmptyBasesAttr::clone(ASTContext &C) const {
  auto *A = new (C) EmptyBasesAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void EmptyBasesAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __declspec(empty_bases)";
    break;
  }
}
}

const char *EmptyBasesAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "empty_bases";
  }
}

EnableIfAttr *EnableIfAttr::clone(ASTContext &C) const {
  auto *A = new (C) EnableIfAttr(getLocation(), C, cond, getMessage(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void EnableIfAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((enable_if(" << getCond() << ", \"" << getMessage() << "\")))";
    break;
  }
}
}

const char *EnableIfAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "enable_if";
  }
}

EnumExtensibilityAttr *EnumExtensibilityAttr::clone(ASTContext &C) const {
  auto *A = new (C) EnumExtensibilityAttr(getLocation(), C, extensibility, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void EnumExtensibilityAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((enum_extensibility(\"" << EnumExtensibilityAttr::ConvertKindToStr(getExtensibility()) << "\")))";
    break;
  }
  case 1 : {
    OS << " [[clang::enum_extensibility(\"" << EnumExtensibilityAttr::ConvertKindToStr(getExtensibility()) << "\")]]";
    break;
  }
}
}

const char *EnumExtensibilityAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "enum_extensibility";
  case 1:
    return "enum_extensibility";
  }
}

ExclusiveTrylockFunctionAttr *ExclusiveTrylockFunctionAttr::clone(ASTContext &C) const {
  auto *A = new (C) ExclusiveTrylockFunctionAttr(getLocation(), C, successValue, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ExclusiveTrylockFunctionAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((exclusive_trylock_function(" << getSuccessValue() << ", ";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
}
}

const char *ExclusiveTrylockFunctionAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "exclusive_trylock_function";
  }
}

ExternalSourceSymbolAttr *ExternalSourceSymbolAttr::clone(ASTContext &C) const {
  auto *A = new (C) ExternalSourceSymbolAttr(getLocation(), C, getLanguage(), getDefinedIn(), generatedDeclaration, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ExternalSourceSymbolAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((external_source_symbol(\"" << getLanguage() << "\", \"" << getDefinedIn() << "\", " << getGeneratedDeclaration() << ")))";
    break;
  }
  case 1 : {
    OS << " [[clang::external_source_symbol(\"" << getLanguage() << "\", \"" << getDefinedIn() << "\", " << getGeneratedDeclaration() << ")]]";
    break;
  }
}
}

const char *ExternalSourceSymbolAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "external_source_symbol";
  case 1:
    return "external_source_symbol";
  }
}

FallThroughAttr *FallThroughAttr::clone(ASTContext &C) const {
  auto *A = new (C) FallThroughAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void FallThroughAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " [[fallthrough]]";
    break;
  }
  case 1 : {
    OS << " [[clang::fallthrough]]";
    break;
  }
}
}

const char *FallThroughAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "fallthrough";
  case 1:
    return "fallthrough";
  }
}

FastCallAttr *FastCallAttr::clone(ASTContext &C) const {
  auto *A = new (C) FastCallAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void FastCallAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((fastcall))";
    break;
  }
  case 1 : {
    OS << " [[gnu::fastcall]]";
    break;
  }
  case 2 : {
    OS << " __fastcall";
    break;
  }
  case 3 : {
    OS << " _fastcall";
    break;
  }
}
}

const char *FastCallAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "fastcall";
  case 1:
    return "fastcall";
  case 2:
    return "__fastcall";
  case 3:
    return "_fastcall";
  }
}

FinalAttr *FinalAttr::clone(ASTContext &C) const {
  auto *A = new (C) FinalAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void FinalAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " final";
    break;
  }
  case 1 : {
    OS << " sealed";
    break;
  }
}
}

const char *FinalAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "final";
  case 1:
    return "sealed";
  }
}

FlagEnumAttr *FlagEnumAttr::clone(ASTContext &C) const {
  auto *A = new (C) FlagEnumAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void FlagEnumAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((flag_enum))";
    break;
  }
}
}

const char *FlagEnumAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "flag_enum";
  }
}

FlattenAttr *FlattenAttr::clone(ASTContext &C) const {
  auto *A = new (C) FlattenAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void FlattenAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((flatten))";
    break;
  }
  case 1 : {
    OS << " [[gnu::flatten]]";
    break;
  }
}
}

const char *FlattenAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "flatten";
  case 1:
    return "flatten";
  }
}

FormatAttr *FormatAttr::clone(ASTContext &C) const {
  auto *A = new (C) FormatAttr(getLocation(), C, type, formatIdx, firstArg, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void FormatAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((format(";
    OS << getType()->getName();
    OS << ", " << getFormatIdx() << ", " << getFirstArg() << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::format(";
    OS << getType()->getName();
    OS << ", " << getFormatIdx() << ", " << getFirstArg() << ")]]";
    break;
  }
}
}

const char *FormatAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "format";
  case 1:
    return "format";
  }
}

FormatArgAttr *FormatArgAttr::clone(ASTContext &C) const {
  auto *A = new (C) FormatArgAttr(getLocation(), C, formatIdx, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void FormatArgAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((format_arg(" << getFormatIdx() << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::format_arg(" << getFormatIdx() << ")]]";
    break;
  }
}
}

const char *FormatArgAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "format_arg";
  case 1:
    return "format_arg";
  }
}

GNUInlineAttr *GNUInlineAttr::clone(ASTContext &C) const {
  auto *A = new (C) GNUInlineAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void GNUInlineAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((gnu_inline))";
    break;
  }
  case 1 : {
    OS << " [[gnu::gnu_inline]]";
    break;
  }
}
}

const char *GNUInlineAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "gnu_inline";
  case 1:
    return "gnu_inline";
  }
}

GuardedByAttr *GuardedByAttr::clone(ASTContext &C) const {
  auto *A = new (C) GuardedByAttr(getLocation(), C, arg, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void GuardedByAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((guarded_by(" << getArg() << ")))";
    break;
  }
}
}

const char *GuardedByAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "guarded_by";
  }
}

GuardedVarAttr *GuardedVarAttr::clone(ASTContext &C) const {
  auto *A = new (C) GuardedVarAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void GuardedVarAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((guarded_var))";
    break;
  }
}
}

const char *GuardedVarAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "guarded_var";
  }
}

HotAttr *HotAttr::clone(ASTContext &C) const {
  auto *A = new (C) HotAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void HotAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((hot))";
    break;
  }
  case 1 : {
    OS << " [[gnu::hot]]";
    break;
  }
}
}

const char *HotAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "hot";
  case 1:
    return "hot";
  }
}

IBActionAttr *IBActionAttr::clone(ASTContext &C) const {
  auto *A = new (C) IBActionAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void IBActionAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((ibaction))";
    break;
  }
}
}

const char *IBActionAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "ibaction";
  }
}

IBOutletAttr *IBOutletAttr::clone(ASTContext &C) const {
  auto *A = new (C) IBOutletAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void IBOutletAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((iboutlet))";
    break;
  }
}
}

const char *IBOutletAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "iboutlet";
  }
}

IBOutletCollectionAttr *IBOutletCollectionAttr::clone(ASTContext &C) const {
  auto *A = new (C) IBOutletCollectionAttr(getLocation(), C, interface_, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void IBOutletCollectionAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((iboutletcollection(" << getInterface().getAsString() << ")))";
    break;
  }
}
}

const char *IBOutletCollectionAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "iboutletcollection";
  }
}

IFuncAttr *IFuncAttr::clone(ASTContext &C) const {
  auto *A = new (C) IFuncAttr(getLocation(), C, getResolver(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void IFuncAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((ifunc(\"" << getResolver() << "\")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::ifunc(\"" << getResolver() << "\")]]";
    break;
  }
}
}

const char *IFuncAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "ifunc";
  case 1:
    return "ifunc";
  }
}

InitPriorityAttr *InitPriorityAttr::clone(ASTContext &C) const {
  auto *A = new (C) InitPriorityAttr(getLocation(), C, priority, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void InitPriorityAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((init_priority(" << getPriority() << ")))";
    break;
  }
}
}

const char *InitPriorityAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "init_priority";
  }
}

InitSegAttr *InitSegAttr::clone(ASTContext &C) const {
  auto *A = new (C) InitSegAttr(getLocation(), C, getSection(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void InitSegAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << "#pragma init_seg ";
    printPrettyPragma(OS, Policy);
    OS << "\n";    break;
  }
}
}

const char *InitSegAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "init_seg";
  }
}

IntelOclBiccAttr *IntelOclBiccAttr::clone(ASTContext &C) const {
  auto *A = new (C) IntelOclBiccAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void IntelOclBiccAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((intel_ocl_bicc))";
    break;
  }
}
}

const char *IntelOclBiccAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "intel_ocl_bicc";
  }
}

InternalLinkageAttr *InternalLinkageAttr::clone(ASTContext &C) const {
  auto *A = new (C) InternalLinkageAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void InternalLinkageAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((internal_linkage))";
    break;
  }
  case 1 : {
    OS << " [[clang::internal_linkage]]";
    break;
  }
}
}

const char *InternalLinkageAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "internal_linkage";
  case 1:
    return "internal_linkage";
  }
}

LTOVisibilityPublicAttr *LTOVisibilityPublicAttr::clone(ASTContext &C) const {
  auto *A = new (C) LTOVisibilityPublicAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void LTOVisibilityPublicAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " [[clang::lto_visibility_public]]";
    break;
  }
}
}

const char *LTOVisibilityPublicAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "lto_visibility_public";
  }
}

LayoutVersionAttr *LayoutVersionAttr::clone(ASTContext &C) const {
  auto *A = new (C) LayoutVersionAttr(getLocation(), C, version, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void LayoutVersionAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __declspec(layout_version(" << getVersion() << "))";
    break;
  }
}
}

const char *LayoutVersionAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "layout_version";
  }
}

LockReturnedAttr *LockReturnedAttr::clone(ASTContext &C) const {
  auto *A = new (C) LockReturnedAttr(getLocation(), C, arg, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void LockReturnedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((lock_returned(" << getArg() << ")))";
    break;
  }
}
}

const char *LockReturnedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "lock_returned";
  }
}

LocksExcludedAttr *LocksExcludedAttr::clone(ASTContext &C) const {
  auto *A = new (C) LocksExcludedAttr(getLocation(), C, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void LocksExcludedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((locks_excluded(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
}
}

const char *LocksExcludedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "locks_excluded";
  }
}

LoopHintAttr *LoopHintAttr::clone(ASTContext &C) const {
  auto *A = new (C) LoopHintAttr(getLocation(), C, option, state, value, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void LoopHintAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << "#pragma clang loop ";
    printPrettyPragma(OS, Policy);
    OS << "\n";    break;
  }
  case 1 : {
    OS << "#pragma unroll ";
    printPrettyPragma(OS, Policy);
    OS << "\n";    break;
  }
  case 2 : {
    OS << "#pragma nounroll ";
    printPrettyPragma(OS, Policy);
    OS << "\n";    break;
  }
}
}

const char *LoopHintAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "loop";
  case 1:
    return "unroll";
  case 2:
    return "nounroll";
  }
}

MSABIAttr *MSABIAttr::clone(ASTContext &C) const {
  auto *A = new (C) MSABIAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void MSABIAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((ms_abi))";
    break;
  }
  case 1 : {
    OS << " [[gnu::ms_abi]]";
    break;
  }
}
}

const char *MSABIAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "ms_abi";
  case 1:
    return "ms_abi";
  }
}

MSInheritanceAttr *MSInheritanceAttr::clone(ASTContext &C) const {
  auto *A = new (C) MSInheritanceAttr(getLocation(), C, bestCase, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void MSInheritanceAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __single_inheritance(" << getBestCase() << ")";
    break;
  }
  case 1 : {
    OS << " __multiple_inheritance(" << getBestCase() << ")";
    break;
  }
  case 2 : {
    OS << " __virtual_inheritance(" << getBestCase() << ")";
    break;
  }
  case 3 : {
    OS << " __unspecified_inheritance(" << getBestCase() << ")";
    break;
  }
}
}

const char *MSInheritanceAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "__single_inheritance";
  case 1:
    return "__multiple_inheritance";
  case 2:
    return "__virtual_inheritance";
  case 3:
    return "__unspecified_inheritance";
  }
}

MSNoVTableAttr *MSNoVTableAttr::clone(ASTContext &C) const {
  auto *A = new (C) MSNoVTableAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void MSNoVTableAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __declspec(novtable)";
    break;
  }
}
}

const char *MSNoVTableAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "novtable";
  }
}

MSP430InterruptAttr *MSP430InterruptAttr::clone(ASTContext &C) const {
  auto *A = new (C) MSP430InterruptAttr(getLocation(), C, number, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void MSP430InterruptAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((interrupt(" << getNumber() << ")))";
    break;
  }
}
}

const char *MSP430InterruptAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "interrupt";
  }
}

MSStructAttr *MSStructAttr::clone(ASTContext &C) const {
  auto *A = new (C) MSStructAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void MSStructAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((ms_struct))";
    break;
  }
  case 1 : {
    OS << " [[gnu::ms_struct]]";
    break;
  }
}
}

const char *MSStructAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "ms_struct";
  case 1:
    return "ms_struct";
  }
}

MSVtorDispAttr *MSVtorDispAttr::clone(ASTContext &C) const {
  auto *A = new (C) MSVtorDispAttr(getLocation(), C, vdm, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void MSVtorDispAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
}

const char *MSVtorDispAttr::getSpelling() const {
  return "(No spelling)";
}

MaxFieldAlignmentAttr *MaxFieldAlignmentAttr::clone(ASTContext &C) const {
  auto *A = new (C) MaxFieldAlignmentAttr(getLocation(), C, alignment, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void MaxFieldAlignmentAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
}

const char *MaxFieldAlignmentAttr::getSpelling() const {
  return "(No spelling)";
}

MayAliasAttr *MayAliasAttr::clone(ASTContext &C) const {
  auto *A = new (C) MayAliasAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void MayAliasAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((may_alias))";
    break;
  }
  case 1 : {
    OS << " [[gnu::may_alias]]";
    break;
  }
}
}

const char *MayAliasAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "may_alias";
  case 1:
    return "may_alias";
  }
}

MinSizeAttr *MinSizeAttr::clone(ASTContext &C) const {
  auto *A = new (C) MinSizeAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void MinSizeAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((minsize))";
    break;
  }
}
}

const char *MinSizeAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "minsize";
  }
}

Mips16Attr *Mips16Attr::clone(ASTContext &C) const {
  auto *A = new (C) Mips16Attr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void Mips16Attr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((mips16))";
    break;
  }
  case 1 : {
    OS << " [[gnu::mips16]]";
    break;
  }
}
}

const char *Mips16Attr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "mips16";
  case 1:
    return "mips16";
  }
}

MipsInterruptAttr *MipsInterruptAttr::clone(ASTContext &C) const {
  auto *A = new (C) MipsInterruptAttr(getLocation(), C, interrupt, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void MipsInterruptAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((interrupt(\"" << MipsInterruptAttr::ConvertInterruptTypeToStr(getInterrupt()) << "\")))";
    break;
  }
}
}

const char *MipsInterruptAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "interrupt";
  }
}

ModeAttr *ModeAttr::clone(ASTContext &C) const {
  auto *A = new (C) ModeAttr(getLocation(), C, mode, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ModeAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((mode(";
    OS << getMode()->getName();
    OS << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::mode(";
    OS << getMode()->getName();
    OS << ")]]";
    break;
  }
}
}

const char *ModeAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "mode";
  case 1:
    return "mode";
  }
}

NSConsumedAttr *NSConsumedAttr::clone(ASTContext &C) const {
  auto *A = new (C) NSConsumedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NSConsumedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((ns_consumed))";
    break;
  }
}
}

const char *NSConsumedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "ns_consumed";
  }
}

NSConsumesSelfAttr *NSConsumesSelfAttr::clone(ASTContext &C) const {
  auto *A = new (C) NSConsumesSelfAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NSConsumesSelfAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((ns_consumes_self))";
    break;
  }
}
}

const char *NSConsumesSelfAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "ns_consumes_self";
  }
}

NSReturnsAutoreleasedAttr *NSReturnsAutoreleasedAttr::clone(ASTContext &C) const {
  auto *A = new (C) NSReturnsAutoreleasedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NSReturnsAutoreleasedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((ns_returns_autoreleased))";
    break;
  }
}
}

const char *NSReturnsAutoreleasedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "ns_returns_autoreleased";
  }
}

NSReturnsNotRetainedAttr *NSReturnsNotRetainedAttr::clone(ASTContext &C) const {
  auto *A = new (C) NSReturnsNotRetainedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NSReturnsNotRetainedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((ns_returns_not_retained))";
    break;
  }
}
}

const char *NSReturnsNotRetainedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "ns_returns_not_retained";
  }
}

NSReturnsRetainedAttr *NSReturnsRetainedAttr::clone(ASTContext &C) const {
  auto *A = new (C) NSReturnsRetainedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NSReturnsRetainedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((ns_returns_retained))";
    break;
  }
}
}

const char *NSReturnsRetainedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "ns_returns_retained";
  }
}

NakedAttr *NakedAttr::clone(ASTContext &C) const {
  auto *A = new (C) NakedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NakedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((naked))";
    break;
  }
  case 1 : {
    OS << " [[gnu::naked]]";
    break;
  }
  case 2 : {
    OS << " __declspec(naked)";
    break;
  }
}
}

const char *NakedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "naked";
  case 1:
    return "naked";
  case 2:
    return "naked";
  }
}

NoAliasAttr *NoAliasAttr::clone(ASTContext &C) const {
  auto *A = new (C) NoAliasAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoAliasAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __declspec(noalias)";
    break;
  }
}
}

const char *NoAliasAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "noalias";
  }
}

NoCommonAttr *NoCommonAttr::clone(ASTContext &C) const {
  auto *A = new (C) NoCommonAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoCommonAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((nocommon))";
    break;
  }
  case 1 : {
    OS << " [[gnu::nocommon]]";
    break;
  }
}
}

const char *NoCommonAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "nocommon";
  case 1:
    return "nocommon";
  }
}

NoDebugAttr *NoDebugAttr::clone(ASTContext &C) const {
  auto *A = new (C) NoDebugAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoDebugAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((nodebug))";
    break;
  }
  case 1 : {
    OS << " [[gnu::nodebug]]";
    break;
  }
}
}

const char *NoDebugAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "nodebug";
  case 1:
    return "nodebug";
  }
}

NoDuplicateAttr *NoDuplicateAttr::clone(ASTContext &C) const {
  auto *A = new (C) NoDuplicateAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoDuplicateAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((noduplicate))";
    break;
  }
  case 1 : {
    OS << " [[clang::noduplicate]]";
    break;
  }
}
}

const char *NoDuplicateAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "noduplicate";
  case 1:
    return "noduplicate";
  }
}

NoInlineAttr *NoInlineAttr::clone(ASTContext &C) const {
  auto *A = new (C) NoInlineAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoInlineAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((noinline))";
    break;
  }
  case 1 : {
    OS << " [[gnu::noinline]]";
    break;
  }
  case 2 : {
    OS << " __declspec(noinline)";
    break;
  }
}
}

const char *NoInlineAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "noinline";
  case 1:
    return "noinline";
  case 2:
    return "noinline";
  }
}

NoInstrumentFunctionAttr *NoInstrumentFunctionAttr::clone(ASTContext &C) const {
  auto *A = new (C) NoInstrumentFunctionAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoInstrumentFunctionAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((no_instrument_function))";
    break;
  }
  case 1 : {
    OS << " [[gnu::no_instrument_function]]";
    break;
  }
}
}

const char *NoInstrumentFunctionAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "no_instrument_function";
  case 1:
    return "no_instrument_function";
  }
}

NoMips16Attr *NoMips16Attr::clone(ASTContext &C) const {
  auto *A = new (C) NoMips16Attr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoMips16Attr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((nomips16))";
    break;
  }
  case 1 : {
    OS << " [[gnu::nomips16]]";
    break;
  }
}
}

const char *NoMips16Attr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "nomips16";
  case 1:
    return "nomips16";
  }
}

NoReturnAttr *NoReturnAttr::clone(ASTContext &C) const {
  auto *A = new (C) NoReturnAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoReturnAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((noreturn))";
    break;
  }
  case 1 : {
    OS << " [[gnu::noreturn]]";
    break;
  }
  case 2 : {
    OS << " __declspec(noreturn)";
    break;
  }
}
}

const char *NoReturnAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "noreturn";
  case 1:
    return "noreturn";
  case 2:
    return "noreturn";
  }
}

NoSanitizeAttr *NoSanitizeAttr::clone(ASTContext &C) const {
  auto *A = new (C) NoSanitizeAttr(getLocation(), C, sanitizers_, sanitizers_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoSanitizeAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((no_sanitize(";
  bool isFirst = true;
  for (const auto &Val : sanitizers()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << "\"" << Val << "\"";
  }
  OS << ")))";
    break;
  }
  case 1 : {
    OS << " [[clang::no_sanitize(";
  bool isFirst = true;
  for (const auto &Val : sanitizers()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << "\"" << Val << "\"";
  }
  OS << ")]]";
    break;
  }
}
}

const char *NoSanitizeAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "no_sanitize";
  case 1:
    return "no_sanitize";
  }
}

NoSplitStackAttr *NoSplitStackAttr::clone(ASTContext &C) const {
  auto *A = new (C) NoSplitStackAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoSplitStackAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((no_split_stack))";
    break;
  }
  case 1 : {
    OS << " [[gnu::no_split_stack]]";
    break;
  }
}
}

const char *NoSplitStackAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "no_split_stack";
  case 1:
    return "no_split_stack";
  }
}

NoThreadSafetyAnalysisAttr *NoThreadSafetyAnalysisAttr::clone(ASTContext &C) const {
  auto *A = new (C) NoThreadSafetyAnalysisAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoThreadSafetyAnalysisAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((no_thread_safety_analysis))";
    break;
  }
}
}

const char *NoThreadSafetyAnalysisAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "no_thread_safety_analysis";
  }
}

NoThrowAttr *NoThrowAttr::clone(ASTContext &C) const {
  auto *A = new (C) NoThrowAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NoThrowAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((nothrow))";
    break;
  }
  case 1 : {
    OS << " [[gnu::nothrow]]";
    break;
  }
  case 2 : {
    OS << " __declspec(nothrow)";
    break;
  }
}
}

const char *NoThrowAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "nothrow";
  case 1:
    return "nothrow";
  case 2:
    return "nothrow";
  }
}

NonNullAttr *NonNullAttr::clone(ASTContext &C) const {
  auto *A = new (C) NonNullAttr(getLocation(), C, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NonNullAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((nonnull(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::nonnull(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")]]";
    break;
  }
}
}

const char *NonNullAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "nonnull";
  case 1:
    return "nonnull";
  }
}

NotTailCalledAttr *NotTailCalledAttr::clone(ASTContext &C) const {
  auto *A = new (C) NotTailCalledAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void NotTailCalledAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((not_tail_called))";
    break;
  }
  case 1 : {
    OS << " [[clang::not_tail_called]]";
    break;
  }
}
}

const char *NotTailCalledAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "not_tail_called";
  case 1:
    return "not_tail_called";
  }
}

OMPCaptureNoInitAttr *OMPCaptureNoInitAttr::clone(ASTContext &C) const {
  auto *A = new (C) OMPCaptureNoInitAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void OMPCaptureNoInitAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
}

const char *OMPCaptureNoInitAttr::getSpelling() const {
  return "(No spelling)";
}

OMPDeclareSimdDeclAttr *OMPDeclareSimdDeclAttr::clone(ASTContext &C) const {
  auto *A = new (C) OMPDeclareSimdDeclAttr(getLocation(), C, branchState, simdlen, uniforms_, uniforms_Size, aligneds_, aligneds_Size, alignments_, alignments_Size, linears_, linears_Size, modifiers_, modifiers_Size, steps_, steps_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void OMPDeclareSimdDeclAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << "#pragma omp declare simd ";
    printPrettyPragma(OS, Policy);
    OS << "\n";    break;
  }
}
}

const char *OMPDeclareSimdDeclAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "declare simd";
  }
}

OMPDeclareTargetDeclAttr *OMPDeclareTargetDeclAttr::clone(ASTContext &C) const {
  auto *A = new (C) OMPDeclareTargetDeclAttr(getLocation(), C, mapType, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void OMPDeclareTargetDeclAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << "#pragma omp declare target ";
    printPrettyPragma(OS, Policy);
    OS << "\n";    break;
  }
}
}

const char *OMPDeclareTargetDeclAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "declare target";
  }
}

OMPThreadPrivateDeclAttr *OMPThreadPrivateDeclAttr::clone(ASTContext &C) const {
  auto *A = new (C) OMPThreadPrivateDeclAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void OMPThreadPrivateDeclAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
}

const char *OMPThreadPrivateDeclAttr::getSpelling() const {
  return "(No spelling)";
}

ObjCBoxableAttr *ObjCBoxableAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCBoxableAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCBoxableAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_boxable))";
    break;
  }
}
}

const char *ObjCBoxableAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_boxable";
  }
}

ObjCBridgeAttr *ObjCBridgeAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCBridgeAttr(getLocation(), C, bridgedType, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCBridgeAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_bridge(";
    OS << getBridgedType()->getName();
    OS << ")))";
    break;
  }
}
}

const char *ObjCBridgeAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_bridge";
  }
}

ObjCBridgeMutableAttr *ObjCBridgeMutableAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCBridgeMutableAttr(getLocation(), C, bridgedType, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCBridgeMutableAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_bridge_mutable(";
    OS << getBridgedType()->getName();
    OS << ")))";
    break;
  }
}
}

const char *ObjCBridgeMutableAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_bridge_mutable";
  }
}

ObjCBridgeRelatedAttr *ObjCBridgeRelatedAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCBridgeRelatedAttr(getLocation(), C, relatedClass, classMethod, instanceMethod, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCBridgeRelatedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_bridge_related(";
    OS << getRelatedClass()->getName();
    OS << ", ";
    if (getClassMethod()) OS << getClassMethod()->getName();
    OS << ", ";
    if (getInstanceMethod()) OS << getInstanceMethod()->getName();
    OS << ")))";
    break;
  }
}
}

const char *ObjCBridgeRelatedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_bridge_related";
  }
}

ObjCDesignatedInitializerAttr *ObjCDesignatedInitializerAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCDesignatedInitializerAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCDesignatedInitializerAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_designated_initializer))";
    break;
  }
}
}

const char *ObjCDesignatedInitializerAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_designated_initializer";
  }
}

ObjCExceptionAttr *ObjCExceptionAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCExceptionAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCExceptionAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_exception))";
    break;
  }
}
}

const char *ObjCExceptionAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_exception";
  }
}

ObjCExplicitProtocolImplAttr *ObjCExplicitProtocolImplAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCExplicitProtocolImplAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCExplicitProtocolImplAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_protocol_requires_explicit_implementation))";
    break;
  }
}
}

const char *ObjCExplicitProtocolImplAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_protocol_requires_explicit_implementation";
  }
}

ObjCIndependentClassAttr *ObjCIndependentClassAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCIndependentClassAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCIndependentClassAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_independent_class))";
    break;
  }
}
}

const char *ObjCIndependentClassAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_independent_class";
  }
}

ObjCMethodFamilyAttr *ObjCMethodFamilyAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCMethodFamilyAttr(getLocation(), C, family, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCMethodFamilyAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_method_family(\"" << ObjCMethodFamilyAttr::ConvertFamilyKindToStr(getFamily()) << "\")))";
    break;
  }
}
}

const char *ObjCMethodFamilyAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_method_family";
  }
}

ObjCNSObjectAttr *ObjCNSObjectAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCNSObjectAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCNSObjectAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((NSObject))";
    break;
  }
}
}

const char *ObjCNSObjectAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "NSObject";
  }
}

ObjCPreciseLifetimeAttr *ObjCPreciseLifetimeAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCPreciseLifetimeAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCPreciseLifetimeAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_precise_lifetime))";
    break;
  }
}
}

const char *ObjCPreciseLifetimeAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_precise_lifetime";
  }
}

ObjCRequiresPropertyDefsAttr *ObjCRequiresPropertyDefsAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCRequiresPropertyDefsAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCRequiresPropertyDefsAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_requires_property_definitions))";
    break;
  }
}
}

const char *ObjCRequiresPropertyDefsAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_requires_property_definitions";
  }
}

ObjCRequiresSuperAttr *ObjCRequiresSuperAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCRequiresSuperAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCRequiresSuperAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_requires_super))";
    break;
  }
}
}

const char *ObjCRequiresSuperAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_requires_super";
  }
}

ObjCReturnsInnerPointerAttr *ObjCReturnsInnerPointerAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCReturnsInnerPointerAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCReturnsInnerPointerAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_returns_inner_pointer))";
    break;
  }
}
}

const char *ObjCReturnsInnerPointerAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_returns_inner_pointer";
  }
}

ObjCRootClassAttr *ObjCRootClassAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCRootClassAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCRootClassAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_root_class))";
    break;
  }
}
}

const char *ObjCRootClassAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_root_class";
  }
}

ObjCRuntimeNameAttr *ObjCRuntimeNameAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCRuntimeNameAttr(getLocation(), C, getMetadataName(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCRuntimeNameAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_runtime_name(\"" << getMetadataName() << "\")))";
    break;
  }
}
}

const char *ObjCRuntimeNameAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_runtime_name";
  }
}

ObjCRuntimeVisibleAttr *ObjCRuntimeVisibleAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCRuntimeVisibleAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCRuntimeVisibleAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_runtime_visible))";
    break;
  }
}
}

const char *ObjCRuntimeVisibleAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_runtime_visible";
  }
}

ObjCSubclassingRestrictedAttr *ObjCSubclassingRestrictedAttr::clone(ASTContext &C) const {
  auto *A = new (C) ObjCSubclassingRestrictedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ObjCSubclassingRestrictedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((objc_subclassing_restricted))";
    break;
  }
}
}

const char *ObjCSubclassingRestrictedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "objc_subclassing_restricted";
  }
}

OpenCLAccessAttr *OpenCLAccessAttr::clone(ASTContext &C) const {
  auto *A = new (C) OpenCLAccessAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void OpenCLAccessAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __read_only";
    break;
  }
  case 1 : {
    OS << " read_only";
    break;
  }
  case 2 : {
    OS << " __write_only";
    break;
  }
  case 3 : {
    OS << " write_only";
    break;
  }
  case 4 : {
    OS << " __read_write";
    break;
  }
  case 5 : {
    OS << " read_write";
    break;
  }
}
}

const char *OpenCLAccessAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "__read_only";
  case 1:
    return "read_only";
  case 2:
    return "__write_only";
  case 3:
    return "write_only";
  case 4:
    return "__read_write";
  case 5:
    return "read_write";
  }
}

OpenCLKernelAttr *OpenCLKernelAttr::clone(ASTContext &C) const {
  auto *A = new (C) OpenCLKernelAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void OpenCLKernelAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __kernel";
    break;
  }
  case 1 : {
    OS << " kernel";
    break;
  }
}
}

const char *OpenCLKernelAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "__kernel";
  case 1:
    return "kernel";
  }
}

OpenCLUnrollHintAttr *OpenCLUnrollHintAttr::clone(ASTContext &C) const {
  auto *A = new (C) OpenCLUnrollHintAttr(getLocation(), C, unrollHint, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void OpenCLUnrollHintAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((opencl_unroll_hint(" << getUnrollHint() << ")))";
    break;
  }
}
}

const char *OpenCLUnrollHintAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "opencl_unroll_hint";
  }
}

OptimizeNoneAttr *OptimizeNoneAttr::clone(ASTContext &C) const {
  auto *A = new (C) OptimizeNoneAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void OptimizeNoneAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((optnone))";
    break;
  }
  case 1 : {
    OS << " [[clang::optnone]]";
    break;
  }
}
}

const char *OptimizeNoneAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "optnone";
  case 1:
    return "optnone";
  }
}

OverloadableAttr *OverloadableAttr::clone(ASTContext &C) const {
  auto *A = new (C) OverloadableAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void OverloadableAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((overloadable))";
    break;
  }
}
}

const char *OverloadableAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "overloadable";
  }
}

OverrideAttr *OverrideAttr::clone(ASTContext &C) const {
  auto *A = new (C) OverrideAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void OverrideAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " override";
    break;
  }
}
}

const char *OverrideAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "override";
  }
}

OwnershipAttr *OwnershipAttr::clone(ASTContext &C) const {
  auto *A = new (C) OwnershipAttr(getLocation(), C, module, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void OwnershipAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((ownership_holds(";
    OS << getModule()->getName();
    OS << ", ";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 1 : {
    OS << " __attribute__((ownership_returns(";
    OS << getModule()->getName();
    OS << ", ";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 2 : {
    OS << " __attribute__((ownership_takes(";
    OS << getModule()->getName();
    OS << ", ";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
}
}

const char *OwnershipAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "ownership_holds";
  case 1:
    return "ownership_returns";
  case 2:
    return "ownership_takes";
  }
}

PackedAttr *PackedAttr::clone(ASTContext &C) const {
  auto *A = new (C) PackedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void PackedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((packed))";
    break;
  }
  case 1 : {
    OS << " [[gnu::packed]]";
    break;
  }
}
}

const char *PackedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "packed";
  case 1:
    return "packed";
  }
}

ParamTypestateAttr *ParamTypestateAttr::clone(ASTContext &C) const {
  auto *A = new (C) ParamTypestateAttr(getLocation(), C, paramState, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ParamTypestateAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((param_typestate(\"" << ParamTypestateAttr::ConvertConsumedStateToStr(getParamState()) << "\")))";
    break;
  }
}
}

const char *ParamTypestateAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "param_typestate";
  }
}

PascalAttr *PascalAttr::clone(ASTContext &C) const {
  auto *A = new (C) PascalAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void PascalAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((pascal))";
    break;
  }
  case 1 : {
    OS << " __pascal";
    break;
  }
  case 2 : {
    OS << " _pascal";
    break;
  }
}
}

const char *PascalAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "pascal";
  case 1:
    return "__pascal";
  case 2:
    return "_pascal";
  }
}

PassObjectSizeAttr *PassObjectSizeAttr::clone(ASTContext &C) const {
  auto *A = new (C) PassObjectSizeAttr(getLocation(), C, type, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void PassObjectSizeAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((pass_object_size(" << getType() << ")))";
    break;
  }
}
}

const char *PassObjectSizeAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "pass_object_size";
  }
}

PcsAttr *PcsAttr::clone(ASTContext &C) const {
  auto *A = new (C) PcsAttr(getLocation(), C, pCS, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void PcsAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((pcs(\"" << PcsAttr::ConvertPCSTypeToStr(getPCS()) << "\")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::pcs(\"" << PcsAttr::ConvertPCSTypeToStr(getPCS()) << "\")]]";
    break;
  }
}
}

const char *PcsAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "pcs";
  case 1:
    return "pcs";
  }
}

PreserveAllAttr *PreserveAllAttr::clone(ASTContext &C) const {
  auto *A = new (C) PreserveAllAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void PreserveAllAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((preserve_all))";
    break;
  }
}
}

const char *PreserveAllAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "preserve_all";
  }
}

PreserveMostAttr *PreserveMostAttr::clone(ASTContext &C) const {
  auto *A = new (C) PreserveMostAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void PreserveMostAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((preserve_most))";
    break;
  }
}
}

const char *PreserveMostAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "preserve_most";
  }
}

PtGuardedByAttr *PtGuardedByAttr::clone(ASTContext &C) const {
  auto *A = new (C) PtGuardedByAttr(getLocation(), C, arg, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void PtGuardedByAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((pt_guarded_by(" << getArg() << ")))";
    break;
  }
}
}

const char *PtGuardedByAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "pt_guarded_by";
  }
}

PtGuardedVarAttr *PtGuardedVarAttr::clone(ASTContext &C) const {
  auto *A = new (C) PtGuardedVarAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void PtGuardedVarAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((pt_guarded_var))";
    break;
  }
}
}

const char *PtGuardedVarAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "pt_guarded_var";
  }
}

PureAttr *PureAttr::clone(ASTContext &C) const {
  auto *A = new (C) PureAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void PureAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((pure))";
    break;
  }
  case 1 : {
    OS << " [[gnu::pure]]";
    break;
  }
}
}

const char *PureAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "pure";
  case 1:
    return "pure";
  }
}

RegCallAttr *RegCallAttr::clone(ASTContext &C) const {
  auto *A = new (C) RegCallAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void RegCallAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((regcall))";
    break;
  }
  case 1 : {
    OS << " [[gnu::regcall]]";
    break;
  }
  case 2 : {
    OS << " __regcall";
    break;
  }
}
}

const char *RegCallAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "regcall";
  case 1:
    return "regcall";
  case 2:
    return "__regcall";
  }
}

ReleaseCapabilityAttr *ReleaseCapabilityAttr::clone(ASTContext &C) const {
  auto *A = new (C) ReleaseCapabilityAttr(getLocation(), C, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ReleaseCapabilityAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((release_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 1 : {
    OS << " [[clang::release_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")]]";
    break;
  }
  case 2 : {
    OS << " __attribute__((release_shared_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 3 : {
    OS << " [[clang::release_shared_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")]]";
    break;
  }
  case 4 : {
    OS << " __attribute__((release_generic_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 5 : {
    OS << " [[clang::release_generic_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")]]";
    break;
  }
  case 6 : {
    OS << " __attribute__((unlock_function(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
}
}

const char *ReleaseCapabilityAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "release_capability";
  case 1:
    return "release_capability";
  case 2:
    return "release_shared_capability";
  case 3:
    return "release_shared_capability";
  case 4:
    return "release_generic_capability";
  case 5:
    return "release_generic_capability";
  case 6:
    return "unlock_function";
  }
}

RenderScriptKernelAttr *RenderScriptKernelAttr::clone(ASTContext &C) const {
  auto *A = new (C) RenderScriptKernelAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void RenderScriptKernelAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((kernel))";
    break;
  }
}
}

const char *RenderScriptKernelAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "kernel";
  }
}

ReqdWorkGroupSizeAttr *ReqdWorkGroupSizeAttr::clone(ASTContext &C) const {
  auto *A = new (C) ReqdWorkGroupSizeAttr(getLocation(), C, xDim, yDim, zDim, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ReqdWorkGroupSizeAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((reqd_work_group_size(" << getXDim() << ", " << getYDim() << ", " << getZDim() << ")))";
    break;
  }
}
}

const char *ReqdWorkGroupSizeAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "reqd_work_group_size";
  }
}

RequireConstantInitAttr *RequireConstantInitAttr::clone(ASTContext &C) const {
  auto *A = new (C) RequireConstantInitAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void RequireConstantInitAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((require_constant_initialization))";
    break;
  }
  case 1 : {
    OS << " [[clang::require_constant_initialization]]";
    break;
  }
}
}

const char *RequireConstantInitAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "require_constant_initialization";
  case 1:
    return "require_constant_initialization";
  }
}

RequiresCapabilityAttr *RequiresCapabilityAttr::clone(ASTContext &C) const {
  auto *A = new (C) RequiresCapabilityAttr(getLocation(), C, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void RequiresCapabilityAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((requires_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 1 : {
    OS << " [[clang::requires_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")]]";
    break;
  }
  case 2 : {
    OS << " __attribute__((exclusive_locks_required(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 3 : {
    OS << " __attribute__((requires_shared_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 4 : {
    OS << " [[clang::requires_shared_capability(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")]]";
    break;
  }
  case 5 : {
    OS << " __attribute__((shared_locks_required(";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
}
}

const char *RequiresCapabilityAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "requires_capability";
  case 1:
    return "requires_capability";
  case 2:
    return "exclusive_locks_required";
  case 3:
    return "requires_shared_capability";
  case 4:
    return "requires_shared_capability";
  case 5:
    return "shared_locks_required";
  }
}

RestrictAttr *RestrictAttr::clone(ASTContext &C) const {
  auto *A = new (C) RestrictAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void RestrictAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __declspec(restrict)";
    break;
  }
  case 1 : {
    OS << " __attribute__((malloc))";
    break;
  }
  case 2 : {
    OS << " [[gnu::malloc]]";
    break;
  }
}
}

const char *RestrictAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "restrict";
  case 1:
    return "malloc";
  case 2:
    return "malloc";
  }
}

ReturnTypestateAttr *ReturnTypestateAttr::clone(ASTContext &C) const {
  auto *A = new (C) ReturnTypestateAttr(getLocation(), C, state, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ReturnTypestateAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((return_typestate(\"" << ReturnTypestateAttr::ConvertConsumedStateToStr(getState()) << "\")))";
    break;
  }
}
}

const char *ReturnTypestateAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "return_typestate";
  }
}

ReturnsNonNullAttr *ReturnsNonNullAttr::clone(ASTContext &C) const {
  auto *A = new (C) ReturnsNonNullAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ReturnsNonNullAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((returns_nonnull))";
    break;
  }
  case 1 : {
    OS << " [[gnu::returns_nonnull]]";
    break;
  }
}
}

const char *ReturnsNonNullAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "returns_nonnull";
  case 1:
    return "returns_nonnull";
  }
}

ReturnsTwiceAttr *ReturnsTwiceAttr::clone(ASTContext &C) const {
  auto *A = new (C) ReturnsTwiceAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ReturnsTwiceAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((returns_twice))";
    break;
  }
  case 1 : {
    OS << " [[gnu::returns_twice]]";
    break;
  }
}
}

const char *ReturnsTwiceAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "returns_twice";
  case 1:
    return "returns_twice";
  }
}

ScopedLockableAttr *ScopedLockableAttr::clone(ASTContext &C) const {
  auto *A = new (C) ScopedLockableAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ScopedLockableAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((scoped_lockable))";
    break;
  }
}
}

const char *ScopedLockableAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "scoped_lockable";
  }
}

SectionAttr *SectionAttr::clone(ASTContext &C) const {
  auto *A = new (C) SectionAttr(getLocation(), C, getName(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void SectionAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((section(\"" << getName() << "\")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::section(\"" << getName() << "\")]]";
    break;
  }
  case 2 : {
    OS << " __declspec(allocate(\"" << getName() << "\"))";
    break;
  }
}
}

const char *SectionAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "section";
  case 1:
    return "section";
  case 2:
    return "allocate";
  }
}

SelectAnyAttr *SelectAnyAttr::clone(ASTContext &C) const {
  auto *A = new (C) SelectAnyAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void SelectAnyAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __declspec(selectany)";
    break;
  }
}
}

const char *SelectAnyAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "selectany";
  }
}

SentinelAttr *SentinelAttr::clone(ASTContext &C) const {
  auto *A = new (C) SentinelAttr(getLocation(), C, sentinel, nullPos, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void SentinelAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((sentinel(" << getSentinel() << ", " << getNullPos() << ")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::sentinel(" << getSentinel() << ", " << getNullPos() << ")]]";
    break;
  }
}
}

const char *SentinelAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "sentinel";
  case 1:
    return "sentinel";
  }
}

SetTypestateAttr *SetTypestateAttr::clone(ASTContext &C) const {
  auto *A = new (C) SetTypestateAttr(getLocation(), C, newState, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void SetTypestateAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((set_typestate(\"" << SetTypestateAttr::ConvertConsumedStateToStr(getNewState()) << "\")))";
    break;
  }
}
}

const char *SetTypestateAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "set_typestate";
  }
}

SharedTrylockFunctionAttr *SharedTrylockFunctionAttr::clone(ASTContext &C) const {
  auto *A = new (C) SharedTrylockFunctionAttr(getLocation(), C, successValue, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void SharedTrylockFunctionAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((shared_trylock_function(" << getSuccessValue() << ", ";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
}
}

const char *SharedTrylockFunctionAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "shared_trylock_function";
  }
}

StdCallAttr *StdCallAttr::clone(ASTContext &C) const {
  auto *A = new (C) StdCallAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void StdCallAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((stdcall))";
    break;
  }
  case 1 : {
    OS << " [[gnu::stdcall]]";
    break;
  }
  case 2 : {
    OS << " __stdcall";
    break;
  }
  case 3 : {
    OS << " _stdcall";
    break;
  }
}
}

const char *StdCallAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "stdcall";
  case 1:
    return "stdcall";
  case 2:
    return "__stdcall";
  case 3:
    return "_stdcall";
  }
}

SuppressAttr *SuppressAttr::clone(ASTContext &C) const {
  auto *A = new (C) SuppressAttr(getLocation(), C, diagnosticIdentifiers_, diagnosticIdentifiers_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void SuppressAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " [[gsl::suppress(";
  bool isFirst = true;
  for (const auto &Val : diagnosticIdentifiers()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << "\"" << Val << "\"";
  }
  OS << ")]]";
    break;
  }
}
}

const char *SuppressAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "suppress";
  }
}

SwiftCallAttr *SwiftCallAttr::clone(ASTContext &C) const {
  auto *A = new (C) SwiftCallAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void SwiftCallAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((swiftcall))";
    break;
  }
  case 1 : {
    OS << " [[gnu::swiftcall]]";
    break;
  }
}
}

const char *SwiftCallAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "swiftcall";
  case 1:
    return "swiftcall";
  }
}

SwiftContextAttr *SwiftContextAttr::clone(ASTContext &C) const {
  auto *A = new (C) SwiftContextAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void SwiftContextAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((swift_context))";
    break;
  }
  case 1 : {
    OS << " [[gnu::swift_context]]";
    break;
  }
}
}

const char *SwiftContextAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "swift_context";
  case 1:
    return "swift_context";
  }
}

SwiftErrorResultAttr *SwiftErrorResultAttr::clone(ASTContext &C) const {
  auto *A = new (C) SwiftErrorResultAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void SwiftErrorResultAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((swift_error_result))";
    break;
  }
  case 1 : {
    OS << " [[gnu::swift_error_result]]";
    break;
  }
}
}

const char *SwiftErrorResultAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "swift_error_result";
  case 1:
    return "swift_error_result";
  }
}

SwiftIndirectResultAttr *SwiftIndirectResultAttr::clone(ASTContext &C) const {
  auto *A = new (C) SwiftIndirectResultAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void SwiftIndirectResultAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((swift_indirect_result))";
    break;
  }
  case 1 : {
    OS << " [[gnu::swift_indirect_result]]";
    break;
  }
}
}

const char *SwiftIndirectResultAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "swift_indirect_result";
  case 1:
    return "swift_indirect_result";
  }
}

SysVABIAttr *SysVABIAttr::clone(ASTContext &C) const {
  auto *A = new (C) SysVABIAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void SysVABIAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((sysv_abi))";
    break;
  }
  case 1 : {
    OS << " [[gnu::sysv_abi]]";
    break;
  }
}
}

const char *SysVABIAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "sysv_abi";
  case 1:
    return "sysv_abi";
  }
}

TLSModelAttr *TLSModelAttr::clone(ASTContext &C) const {
  auto *A = new (C) TLSModelAttr(getLocation(), C, getModel(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void TLSModelAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((tls_model(\"" << getModel() << "\")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::tls_model(\"" << getModel() << "\")]]";
    break;
  }
}
}

const char *TLSModelAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "tls_model";
  case 1:
    return "tls_model";
  }
}

TargetAttr *TargetAttr::clone(ASTContext &C) const {
  auto *A = new (C) TargetAttr(getLocation(), C, getFeaturesStr(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void TargetAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((target(\"" << getFeaturesStr() << "\")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::target(\"" << getFeaturesStr() << "\")]]";
    break;
  }
}
}

const char *TargetAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "target";
  case 1:
    return "target";
  }
}

TestTypestateAttr *TestTypestateAttr::clone(ASTContext &C) const {
  auto *A = new (C) TestTypestateAttr(getLocation(), C, testState, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void TestTypestateAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((test_typestate(\"" << TestTypestateAttr::ConvertConsumedStateToStr(getTestState()) << "\")))";
    break;
  }
}
}

const char *TestTypestateAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "test_typestate";
  }
}

ThisCallAttr *ThisCallAttr::clone(ASTContext &C) const {
  auto *A = new (C) ThisCallAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ThisCallAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((thiscall))";
    break;
  }
  case 1 : {
    OS << " [[gnu::thiscall]]";
    break;
  }
  case 2 : {
    OS << " __thiscall";
    break;
  }
  case 3 : {
    OS << " _thiscall";
    break;
  }
}
}

const char *ThisCallAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "thiscall";
  case 1:
    return "thiscall";
  case 2:
    return "__thiscall";
  case 3:
    return "_thiscall";
  }
}

ThreadAttr *ThreadAttr::clone(ASTContext &C) const {
  auto *A = new (C) ThreadAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void ThreadAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __declspec(thread)";
    break;
  }
}
}

const char *ThreadAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "thread";
  }
}

TransparentUnionAttr *TransparentUnionAttr::clone(ASTContext &C) const {
  auto *A = new (C) TransparentUnionAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void TransparentUnionAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((transparent_union))";
    break;
  }
  case 1 : {
    OS << " [[gnu::transparent_union]]";
    break;
  }
}
}

const char *TransparentUnionAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "transparent_union";
  case 1:
    return "transparent_union";
  }
}

TryAcquireCapabilityAttr *TryAcquireCapabilityAttr::clone(ASTContext &C) const {
  auto *A = new (C) TryAcquireCapabilityAttr(getLocation(), C, successValue, args_, args_Size, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void TryAcquireCapabilityAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((try_acquire_capability(" << getSuccessValue() << ", ";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 1 : {
    OS << " [[clang::try_acquire_capability(" << getSuccessValue() << ", ";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")]]";
    break;
  }
  case 2 : {
    OS << " __attribute__((try_acquire_shared_capability(" << getSuccessValue() << ", ";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")))";
    break;
  }
  case 3 : {
    OS << " [[clang::try_acquire_shared_capability(" << getSuccessValue() << ", ";
  bool isFirst = true;
  for (const auto &Val : args()) {
    if (isFirst) isFirst = false;
    else OS << ", ";
    OS << Val;
  }
  OS << ")]]";
    break;
  }
}
}

const char *TryAcquireCapabilityAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "try_acquire_capability";
  case 1:
    return "try_acquire_capability";
  case 2:
    return "try_acquire_shared_capability";
  case 3:
    return "try_acquire_shared_capability";
  }
}

TypeTagForDatatypeAttr *TypeTagForDatatypeAttr::clone(ASTContext &C) const {
  auto *A = new (C) TypeTagForDatatypeAttr(getLocation(), C, argumentKind, matchingCType, layoutCompatible, mustBeNull, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void TypeTagForDatatypeAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((type_tag_for_datatype(";
    OS << getArgumentKind()->getName();
    OS << ", " << getMatchingCType().getAsString() << ", " << getLayoutCompatible() << ", " << getMustBeNull() << ")))";
    break;
  }
}
}

const char *TypeTagForDatatypeAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "type_tag_for_datatype";
  }
}

TypeVisibilityAttr *TypeVisibilityAttr::clone(ASTContext &C) const {
  auto *A = new (C) TypeVisibilityAttr(getLocation(), C, visibility, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void TypeVisibilityAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((type_visibility(\"" << TypeVisibilityAttr::ConvertVisibilityTypeToStr(getVisibility()) << "\")))";
    break;
  }
  case 1 : {
    OS << " [[clang::type_visibility(\"" << TypeVisibilityAttr::ConvertVisibilityTypeToStr(getVisibility()) << "\")]]";
    break;
  }
}
}

const char *TypeVisibilityAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "type_visibility";
  case 1:
    return "type_visibility";
  }
}

UnavailableAttr *UnavailableAttr::clone(ASTContext &C) const {
  auto *A = new (C) UnavailableAttr(getLocation(), C, getMessage(), implicitReason, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void UnavailableAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((unavailable(\"" << getMessage() << "\")))";
    break;
  }
}
}

const char *UnavailableAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "unavailable";
  }
}

UnusedAttr *UnusedAttr::clone(ASTContext &C) const {
  auto *A = new (C) UnusedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void UnusedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " [[maybe_unused]]";
    break;
  }
  case 1 : {
    OS << " __attribute__((unused))";
    break;
  }
  case 2 : {
    OS << " [[gnu::unused]]";
    break;
  }
}
}

const char *UnusedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "maybe_unused";
  case 1:
    return "unused";
  case 2:
    return "unused";
  }
}

UsedAttr *UsedAttr::clone(ASTContext &C) const {
  auto *A = new (C) UsedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void UsedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((used))";
    break;
  }
  case 1 : {
    OS << " [[gnu::used]]";
    break;
  }
}
}

const char *UsedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "used";
  case 1:
    return "used";
  }
}

UuidAttr *UuidAttr::clone(ASTContext &C) const {
  auto *A = new (C) UuidAttr(getLocation(), C, getGuid(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void UuidAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __declspec(uuid(\"" << getGuid() << "\"))";
    break;
  }
  case 1 : {
    OS << "[uuid(\"" << getGuid() << "\")]";
    break;
  }
}
}

const char *UuidAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "uuid";
  case 1:
    return "uuid";
  }
}

VecReturnAttr *VecReturnAttr::clone(ASTContext &C) const {
  auto *A = new (C) VecReturnAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void VecReturnAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((vecreturn))";
    break;
  }
}
}

const char *VecReturnAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "vecreturn";
  }
}

VecTypeHintAttr *VecTypeHintAttr::clone(ASTContext &C) const {
  auto *A = new (C) VecTypeHintAttr(getLocation(), C, typeHint, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void VecTypeHintAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((vec_type_hint(" << getTypeHint().getAsString() << ")))";
    break;
  }
}
}

const char *VecTypeHintAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "vec_type_hint";
  }
}

VectorCallAttr *VectorCallAttr::clone(ASTContext &C) const {
  auto *A = new (C) VectorCallAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void VectorCallAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((vectorcall))";
    break;
  }
  case 1 : {
    OS << " __vectorcall";
    break;
  }
  case 2 : {
    OS << " _vectorcall";
    break;
  }
}
}

const char *VectorCallAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "vectorcall";
  case 1:
    return "__vectorcall";
  case 2:
    return "_vectorcall";
  }
}

VisibilityAttr *VisibilityAttr::clone(ASTContext &C) const {
  auto *A = new (C) VisibilityAttr(getLocation(), C, visibility, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void VisibilityAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((visibility(\"" << VisibilityAttr::ConvertVisibilityTypeToStr(getVisibility()) << "\")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::visibility(\"" << VisibilityAttr::ConvertVisibilityTypeToStr(getVisibility()) << "\")]]";
    break;
  }
}
}

const char *VisibilityAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "visibility";
  case 1:
    return "visibility";
  }
}

WarnUnusedAttr *WarnUnusedAttr::clone(ASTContext &C) const {
  auto *A = new (C) WarnUnusedAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void WarnUnusedAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((warn_unused))";
    break;
  }
}
}

const char *WarnUnusedAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "warn_unused";
  }
}

WarnUnusedResultAttr *WarnUnusedResultAttr::clone(ASTContext &C) const {
  auto *A = new (C) WarnUnusedResultAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void WarnUnusedResultAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " [[nodiscard]]";
    break;
  }
  case 1 : {
    OS << " [[clang::warn_unused_result]]";
    break;
  }
  case 2 : {
    OS << " __attribute__((warn_unused_result))";
    break;
  }
  case 3 : {
    OS << " [[gnu::warn_unused_result]]";
    break;
  }
}
}

const char *WarnUnusedResultAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "nodiscard";
  case 1:
    return "warn_unused_result";
  case 2:
    return "warn_unused_result";
  case 3:
    return "warn_unused_result";
  }
}

WeakAttr *WeakAttr::clone(ASTContext &C) const {
  auto *A = new (C) WeakAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void WeakAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((weak))";
    break;
  }
  case 1 : {
    OS << " [[gnu::weak]]";
    break;
  }
}
}

const char *WeakAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "weak";
  case 1:
    return "weak";
  }
}

WeakImportAttr *WeakImportAttr::clone(ASTContext &C) const {
  auto *A = new (C) WeakImportAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void WeakImportAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((weak_import))";
    break;
  }
}
}

const char *WeakImportAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "weak_import";
  }
}

WeakRefAttr *WeakRefAttr::clone(ASTContext &C) const {
  auto *A = new (C) WeakRefAttr(getLocation(), C, getAliasee(), getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void WeakRefAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((weakref(\"" << getAliasee() << "\")))";
    break;
  }
  case 1 : {
    OS << " [[gnu::weakref(\"" << getAliasee() << "\")]]";
    break;
  }
}
}

const char *WeakRefAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "weakref";
  case 1:
    return "weakref";
  }
}

WorkGroupSizeHintAttr *WorkGroupSizeHintAttr::clone(ASTContext &C) const {
  auto *A = new (C) WorkGroupSizeHintAttr(getLocation(), C, xDim, yDim, zDim, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void WorkGroupSizeHintAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((work_group_size_hint(" << getXDim() << ", " << getYDim() << ", " << getZDim() << ")))";
    break;
  }
}
}

const char *WorkGroupSizeHintAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "work_group_size_hint";
  }
}

X86ForceAlignArgPointerAttr *X86ForceAlignArgPointerAttr::clone(ASTContext &C) const {
  auto *A = new (C) X86ForceAlignArgPointerAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void X86ForceAlignArgPointerAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((force_align_arg_pointer))";
    break;
  }
}
}

const char *X86ForceAlignArgPointerAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "force_align_arg_pointer";
  }
}

XRayInstrumentAttr *XRayInstrumentAttr::clone(ASTContext &C) const {
  auto *A = new (C) XRayInstrumentAttr(getLocation(), C, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void XRayInstrumentAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((xray_always_instrument))";
    break;
  }
  case 1 : {
    OS << " [[clang::xray_always_instrument]]";
    break;
  }
  case 2 : {
    OS << " __attribute__((xray_never_instrument))";
    break;
  }
  case 3 : {
    OS << " [[clang::xray_never_instrument]]";
    break;
  }
}
}

const char *XRayInstrumentAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "xray_always_instrument";
  case 1:
    return "xray_always_instrument";
  case 2:
    return "xray_never_instrument";
  case 3:
    return "xray_never_instrument";
  }
}

XRayLogArgsAttr *XRayLogArgsAttr::clone(ASTContext &C) const {
  auto *A = new (C) XRayLogArgsAttr(getLocation(), C, argumentCount, getSpellingListIndex());
  A->Inherited = Inherited;
  A->IsPackExpansion = IsPackExpansion;
  A->Implicit = Implicit;
  return A;
}

void XRayLogArgsAttr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    break;
  case 0 : {
    OS << " __attribute__((xray_log_args(" << getArgumentCount() << ")))";
    break;
  }
  case 1 : {
    OS << " [[clang::xray_log_args(" << getArgumentCount() << ")]]";
    break;
  }
}
}

const char *XRayLogArgsAttr::getSpelling() const {
  switch (SpellingListIndex) {
  default:
    llvm_unreachable("Unknown attribute spelling!");
    return "(No spelling)";
  case 0:
    return "xray_log_args";
  case 1:
    return "xray_log_args";
  }
}

const char *Attr::getSpelling() const {
  switch (getKind()) {
  case attr::AMDGPUFlatWorkGroupSize:
    return cast<AMDGPUFlatWorkGroupSizeAttr>(this)->getSpelling();
  case attr::AMDGPUNumSGPR:
    return cast<AMDGPUNumSGPRAttr>(this)->getSpelling();
  case attr::AMDGPUNumVGPR:
    return cast<AMDGPUNumVGPRAttr>(this)->getSpelling();
  case attr::AMDGPUWavesPerEU:
    return cast<AMDGPUWavesPerEUAttr>(this)->getSpelling();
  case attr::ARMInterrupt:
    return cast<ARMInterruptAttr>(this)->getSpelling();
  case attr::AVRInterrupt:
    return cast<AVRInterruptAttr>(this)->getSpelling();
  case attr::AVRSignal:
    return cast<AVRSignalAttr>(this)->getSpelling();
  case attr::AbiTag:
    return cast<AbiTagAttr>(this)->getSpelling();
  case attr::AcquireCapability:
    return cast<AcquireCapabilityAttr>(this)->getSpelling();
  case attr::AcquiredAfter:
    return cast<AcquiredAfterAttr>(this)->getSpelling();
  case attr::AcquiredBefore:
    return cast<AcquiredBeforeAttr>(this)->getSpelling();
  case attr::Alias:
    return cast<AliasAttr>(this)->getSpelling();
  case attr::AlignMac68k:
    return cast<AlignMac68kAttr>(this)->getSpelling();
  case attr::AlignValue:
    return cast<AlignValueAttr>(this)->getSpelling();
  case attr::Aligned:
    return cast<AlignedAttr>(this)->getSpelling();
  case attr::AllocAlign:
    return cast<AllocAlignAttr>(this)->getSpelling();
  case attr::AllocSize:
    return cast<AllocSizeAttr>(this)->getSpelling();
  case attr::AlwaysInline:
    return cast<AlwaysInlineAttr>(this)->getSpelling();
  case attr::AnalyzerNoReturn:
    return cast<AnalyzerNoReturnAttr>(this)->getSpelling();
  case attr::Annotate:
    return cast<AnnotateAttr>(this)->getSpelling();
  case attr::AnyX86Interrupt:
    return cast<AnyX86InterruptAttr>(this)->getSpelling();
  case attr::ArcWeakrefUnavailable:
    return cast<ArcWeakrefUnavailableAttr>(this)->getSpelling();
  case attr::ArgumentWithTypeTag:
    return cast<ArgumentWithTypeTagAttr>(this)->getSpelling();
  case attr::AsmLabel:
    return cast<AsmLabelAttr>(this)->getSpelling();
  case attr::AssertCapability:
    return cast<AssertCapabilityAttr>(this)->getSpelling();
  case attr::AssertExclusiveLock:
    return cast<AssertExclusiveLockAttr>(this)->getSpelling();
  case attr::AssertSharedLock:
    return cast<AssertSharedLockAttr>(this)->getSpelling();
  case attr::AssumeAligned:
    return cast<AssumeAlignedAttr>(this)->getSpelling();
  case attr::Availability:
    return cast<AvailabilityAttr>(this)->getSpelling();
  case attr::Blocks:
    return cast<BlocksAttr>(this)->getSpelling();
  case attr::C11NoReturn:
    return cast<C11NoReturnAttr>(this)->getSpelling();
  case attr::CDecl:
    return cast<CDeclAttr>(this)->getSpelling();
  case attr::CFAuditedTransfer:
    return cast<CFAuditedTransferAttr>(this)->getSpelling();
  case attr::CFConsumed:
    return cast<CFConsumedAttr>(this)->getSpelling();
  case attr::CFReturnsNotRetained:
    return cast<CFReturnsNotRetainedAttr>(this)->getSpelling();
  case attr::CFReturnsRetained:
    return cast<CFReturnsRetainedAttr>(this)->getSpelling();
  case attr::CFUnknownTransfer:
    return cast<CFUnknownTransferAttr>(this)->getSpelling();
  case attr::CUDAConstant:
    return cast<CUDAConstantAttr>(this)->getSpelling();
  case attr::CUDADevice:
    return cast<CUDADeviceAttr>(this)->getSpelling();
  case attr::CUDAGlobal:
    return cast<CUDAGlobalAttr>(this)->getSpelling();
  case attr::CUDAHost:
    return cast<CUDAHostAttr>(this)->getSpelling();
  case attr::CUDAInvalidTarget:
    return cast<CUDAInvalidTargetAttr>(this)->getSpelling();
  case attr::CUDALaunchBounds:
    return cast<CUDALaunchBoundsAttr>(this)->getSpelling();
  case attr::CUDAShared:
    return cast<CUDASharedAttr>(this)->getSpelling();
  case attr::CXX11NoReturn:
    return cast<CXX11NoReturnAttr>(this)->getSpelling();
  case attr::CallableWhen:
    return cast<CallableWhenAttr>(this)->getSpelling();
  case attr::Capability:
    return cast<CapabilityAttr>(this)->getSpelling();
  case attr::CapturedRecord:
    return cast<CapturedRecordAttr>(this)->getSpelling();
  case attr::CarriesDependency:
    return cast<CarriesDependencyAttr>(this)->getSpelling();
  case attr::Cleanup:
    return cast<CleanupAttr>(this)->getSpelling();
  case attr::Cold:
    return cast<ColdAttr>(this)->getSpelling();
  case attr::Common:
    return cast<CommonAttr>(this)->getSpelling();
  case attr::Const:
    return cast<ConstAttr>(this)->getSpelling();
  case attr::Constructor:
    return cast<ConstructorAttr>(this)->getSpelling();
  case attr::Consumable:
    return cast<ConsumableAttr>(this)->getSpelling();
  case attr::ConsumableAutoCast:
    return cast<ConsumableAutoCastAttr>(this)->getSpelling();
  case attr::ConsumableSetOnRead:
    return cast<ConsumableSetOnReadAttr>(this)->getSpelling();
  case attr::Convergent:
    return cast<ConvergentAttr>(this)->getSpelling();
  case attr::DLLExport:
    return cast<DLLExportAttr>(this)->getSpelling();
  case attr::DLLImport:
    return cast<DLLImportAttr>(this)->getSpelling();
  case attr::Deprecated:
    return cast<DeprecatedAttr>(this)->getSpelling();
  case attr::Destructor:
    return cast<DestructorAttr>(this)->getSpelling();
  case attr::DiagnoseIf:
    return cast<DiagnoseIfAttr>(this)->getSpelling();
  case attr::DisableTailCalls:
    return cast<DisableTailCallsAttr>(this)->getSpelling();
  case attr::EmptyBases:
    return cast<EmptyBasesAttr>(this)->getSpelling();
  case attr::EnableIf:
    return cast<EnableIfAttr>(this)->getSpelling();
  case attr::EnumExtensibility:
    return cast<EnumExtensibilityAttr>(this)->getSpelling();
  case attr::ExclusiveTrylockFunction:
    return cast<ExclusiveTrylockFunctionAttr>(this)->getSpelling();
  case attr::ExternalSourceSymbol:
    return cast<ExternalSourceSymbolAttr>(this)->getSpelling();
  case attr::FallThrough:
    return cast<FallThroughAttr>(this)->getSpelling();
  case attr::FastCall:
    return cast<FastCallAttr>(this)->getSpelling();
  case attr::Final:
    return cast<FinalAttr>(this)->getSpelling();
  case attr::FlagEnum:
    return cast<FlagEnumAttr>(this)->getSpelling();
  case attr::Flatten:
    return cast<FlattenAttr>(this)->getSpelling();
  case attr::Format:
    return cast<FormatAttr>(this)->getSpelling();
  case attr::FormatArg:
    return cast<FormatArgAttr>(this)->getSpelling();
  case attr::GNUInline:
    return cast<GNUInlineAttr>(this)->getSpelling();
  case attr::GuardedBy:
    return cast<GuardedByAttr>(this)->getSpelling();
  case attr::GuardedVar:
    return cast<GuardedVarAttr>(this)->getSpelling();
  case attr::Hot:
    return cast<HotAttr>(this)->getSpelling();
  case attr::IBAction:
    return cast<IBActionAttr>(this)->getSpelling();
  case attr::IBOutlet:
    return cast<IBOutletAttr>(this)->getSpelling();
  case attr::IBOutletCollection:
    return cast<IBOutletCollectionAttr>(this)->getSpelling();
  case attr::IFunc:
    return cast<IFuncAttr>(this)->getSpelling();
  case attr::InitPriority:
    return cast<InitPriorityAttr>(this)->getSpelling();
  case attr::InitSeg:
    return cast<InitSegAttr>(this)->getSpelling();
  case attr::IntelOclBicc:
    return cast<IntelOclBiccAttr>(this)->getSpelling();
  case attr::InternalLinkage:
    return cast<InternalLinkageAttr>(this)->getSpelling();
  case attr::LTOVisibilityPublic:
    return cast<LTOVisibilityPublicAttr>(this)->getSpelling();
  case attr::LayoutVersion:
    return cast<LayoutVersionAttr>(this)->getSpelling();
  case attr::LockReturned:
    return cast<LockReturnedAttr>(this)->getSpelling();
  case attr::LocksExcluded:
    return cast<LocksExcludedAttr>(this)->getSpelling();
  case attr::LoopHint:
    return cast<LoopHintAttr>(this)->getSpelling();
  case attr::MSABI:
    return cast<MSABIAttr>(this)->getSpelling();
  case attr::MSInheritance:
    return cast<MSInheritanceAttr>(this)->getSpelling();
  case attr::MSNoVTable:
    return cast<MSNoVTableAttr>(this)->getSpelling();
  case attr::MSP430Interrupt:
    return cast<MSP430InterruptAttr>(this)->getSpelling();
  case attr::MSStruct:
    return cast<MSStructAttr>(this)->getSpelling();
  case attr::MSVtorDisp:
    return cast<MSVtorDispAttr>(this)->getSpelling();
  case attr::MaxFieldAlignment:
    return cast<MaxFieldAlignmentAttr>(this)->getSpelling();
  case attr::MayAlias:
    return cast<MayAliasAttr>(this)->getSpelling();
  case attr::MinSize:
    return cast<MinSizeAttr>(this)->getSpelling();
  case attr::Mips16:
    return cast<Mips16Attr>(this)->getSpelling();
  case attr::MipsInterrupt:
    return cast<MipsInterruptAttr>(this)->getSpelling();
  case attr::Mode:
    return cast<ModeAttr>(this)->getSpelling();
  case attr::NSConsumed:
    return cast<NSConsumedAttr>(this)->getSpelling();
  case attr::NSConsumesSelf:
    return cast<NSConsumesSelfAttr>(this)->getSpelling();
  case attr::NSReturnsAutoreleased:
    return cast<NSReturnsAutoreleasedAttr>(this)->getSpelling();
  case attr::NSReturnsNotRetained:
    return cast<NSReturnsNotRetainedAttr>(this)->getSpelling();
  case attr::NSReturnsRetained:
    return cast<NSReturnsRetainedAttr>(this)->getSpelling();
  case attr::Naked:
    return cast<NakedAttr>(this)->getSpelling();
  case attr::NoAlias:
    return cast<NoAliasAttr>(this)->getSpelling();
  case attr::NoCommon:
    return cast<NoCommonAttr>(this)->getSpelling();
  case attr::NoDebug:
    return cast<NoDebugAttr>(this)->getSpelling();
  case attr::NoDuplicate:
    return cast<NoDuplicateAttr>(this)->getSpelling();
  case attr::NoInline:
    return cast<NoInlineAttr>(this)->getSpelling();
  case attr::NoInstrumentFunction:
    return cast<NoInstrumentFunctionAttr>(this)->getSpelling();
  case attr::NoMips16:
    return cast<NoMips16Attr>(this)->getSpelling();
  case attr::NoReturn:
    return cast<NoReturnAttr>(this)->getSpelling();
  case attr::NoSanitize:
    return cast<NoSanitizeAttr>(this)->getSpelling();
  case attr::NoSplitStack:
    return cast<NoSplitStackAttr>(this)->getSpelling();
  case attr::NoThreadSafetyAnalysis:
    return cast<NoThreadSafetyAnalysisAttr>(this)->getSpelling();
  case attr::NoThrow:
    return cast<NoThrowAttr>(this)->getSpelling();
  case attr::NonNull:
    return cast<NonNullAttr>(this)->getSpelling();
  case attr::NotTailCalled:
    return cast<NotTailCalledAttr>(this)->getSpelling();
  case attr::OMPCaptureNoInit:
    return cast<OMPCaptureNoInitAttr>(this)->getSpelling();
  case attr::OMPDeclareSimdDecl:
    return cast<OMPDeclareSimdDeclAttr>(this)->getSpelling();
  case attr::OMPDeclareTargetDecl:
    return cast<OMPDeclareTargetDeclAttr>(this)->getSpelling();
  case attr::OMPThreadPrivateDecl:
    return cast<OMPThreadPrivateDeclAttr>(this)->getSpelling();
  case attr::ObjCBoxable:
    return cast<ObjCBoxableAttr>(this)->getSpelling();
  case attr::ObjCBridge:
    return cast<ObjCBridgeAttr>(this)->getSpelling();
  case attr::ObjCBridgeMutable:
    return cast<ObjCBridgeMutableAttr>(this)->getSpelling();
  case attr::ObjCBridgeRelated:
    return cast<ObjCBridgeRelatedAttr>(this)->getSpelling();
  case attr::ObjCDesignatedInitializer:
    return cast<ObjCDesignatedInitializerAttr>(this)->getSpelling();
  case attr::ObjCException:
    return cast<ObjCExceptionAttr>(this)->getSpelling();
  case attr::ObjCExplicitProtocolImpl:
    return cast<ObjCExplicitProtocolImplAttr>(this)->getSpelling();
  case attr::ObjCIndependentClass:
    return cast<ObjCIndependentClassAttr>(this)->getSpelling();
  case attr::ObjCMethodFamily:
    return cast<ObjCMethodFamilyAttr>(this)->getSpelling();
  case attr::ObjCNSObject:
    return cast<ObjCNSObjectAttr>(this)->getSpelling();
  case attr::ObjCPreciseLifetime:
    return cast<ObjCPreciseLifetimeAttr>(this)->getSpelling();
  case attr::ObjCRequiresPropertyDefs:
    return cast<ObjCRequiresPropertyDefsAttr>(this)->getSpelling();
  case attr::ObjCRequiresSuper:
    return cast<ObjCRequiresSuperAttr>(this)->getSpelling();
  case attr::ObjCReturnsInnerPointer:
    return cast<ObjCReturnsInnerPointerAttr>(this)->getSpelling();
  case attr::ObjCRootClass:
    return cast<ObjCRootClassAttr>(this)->getSpelling();
  case attr::ObjCRuntimeName:
    return cast<ObjCRuntimeNameAttr>(this)->getSpelling();
  case attr::ObjCRuntimeVisible:
    return cast<ObjCRuntimeVisibleAttr>(this)->getSpelling();
  case attr::ObjCSubclassingRestricted:
    return cast<ObjCSubclassingRestrictedAttr>(this)->getSpelling();
  case attr::OpenCLAccess:
    return cast<OpenCLAccessAttr>(this)->getSpelling();
  case attr::OpenCLKernel:
    return cast<OpenCLKernelAttr>(this)->getSpelling();
  case attr::OpenCLUnrollHint:
    return cast<OpenCLUnrollHintAttr>(this)->getSpelling();
  case attr::OptimizeNone:
    return cast<OptimizeNoneAttr>(this)->getSpelling();
  case attr::Overloadable:
    return cast<OverloadableAttr>(this)->getSpelling();
  case attr::Override:
    return cast<OverrideAttr>(this)->getSpelling();
  case attr::Ownership:
    return cast<OwnershipAttr>(this)->getSpelling();
  case attr::Packed:
    return cast<PackedAttr>(this)->getSpelling();
  case attr::ParamTypestate:
    return cast<ParamTypestateAttr>(this)->getSpelling();
  case attr::Pascal:
    return cast<PascalAttr>(this)->getSpelling();
  case attr::PassObjectSize:
    return cast<PassObjectSizeAttr>(this)->getSpelling();
  case attr::Pcs:
    return cast<PcsAttr>(this)->getSpelling();
  case attr::PreserveAll:
    return cast<PreserveAllAttr>(this)->getSpelling();
  case attr::PreserveMost:
    return cast<PreserveMostAttr>(this)->getSpelling();
  case attr::PtGuardedBy:
    return cast<PtGuardedByAttr>(this)->getSpelling();
  case attr::PtGuardedVar:
    return cast<PtGuardedVarAttr>(this)->getSpelling();
  case attr::Pure:
    return cast<PureAttr>(this)->getSpelling();
  case attr::RegCall:
    return cast<RegCallAttr>(this)->getSpelling();
  case attr::ReleaseCapability:
    return cast<ReleaseCapabilityAttr>(this)->getSpelling();
  case attr::RenderScriptKernel:
    return cast<RenderScriptKernelAttr>(this)->getSpelling();
  case attr::ReqdWorkGroupSize:
    return cast<ReqdWorkGroupSizeAttr>(this)->getSpelling();
  case attr::RequireConstantInit:
    return cast<RequireConstantInitAttr>(this)->getSpelling();
  case attr::RequiresCapability:
    return cast<RequiresCapabilityAttr>(this)->getSpelling();
  case attr::Restrict:
    return cast<RestrictAttr>(this)->getSpelling();
  case attr::ReturnTypestate:
    return cast<ReturnTypestateAttr>(this)->getSpelling();
  case attr::ReturnsNonNull:
    return cast<ReturnsNonNullAttr>(this)->getSpelling();
  case attr::ReturnsTwice:
    return cast<ReturnsTwiceAttr>(this)->getSpelling();
  case attr::ScopedLockable:
    return cast<ScopedLockableAttr>(this)->getSpelling();
  case attr::Section:
    return cast<SectionAttr>(this)->getSpelling();
  case attr::SelectAny:
    return cast<SelectAnyAttr>(this)->getSpelling();
  case attr::Sentinel:
    return cast<SentinelAttr>(this)->getSpelling();
  case attr::SetTypestate:
    return cast<SetTypestateAttr>(this)->getSpelling();
  case attr::SharedTrylockFunction:
    return cast<SharedTrylockFunctionAttr>(this)->getSpelling();
  case attr::StdCall:
    return cast<StdCallAttr>(this)->getSpelling();
  case attr::Suppress:
    return cast<SuppressAttr>(this)->getSpelling();
  case attr::SwiftCall:
    return cast<SwiftCallAttr>(this)->getSpelling();
  case attr::SwiftContext:
    return cast<SwiftContextAttr>(this)->getSpelling();
  case attr::SwiftErrorResult:
    return cast<SwiftErrorResultAttr>(this)->getSpelling();
  case attr::SwiftIndirectResult:
    return cast<SwiftIndirectResultAttr>(this)->getSpelling();
  case attr::SysVABI:
    return cast<SysVABIAttr>(this)->getSpelling();
  case attr::TLSModel:
    return cast<TLSModelAttr>(this)->getSpelling();
  case attr::Target:
    return cast<TargetAttr>(this)->getSpelling();
  case attr::TestTypestate:
    return cast<TestTypestateAttr>(this)->getSpelling();
  case attr::ThisCall:
    return cast<ThisCallAttr>(this)->getSpelling();
  case attr::Thread:
    return cast<ThreadAttr>(this)->getSpelling();
  case attr::TransparentUnion:
    return cast<TransparentUnionAttr>(this)->getSpelling();
  case attr::TryAcquireCapability:
    return cast<TryAcquireCapabilityAttr>(this)->getSpelling();
  case attr::TypeTagForDatatype:
    return cast<TypeTagForDatatypeAttr>(this)->getSpelling();
  case attr::TypeVisibility:
    return cast<TypeVisibilityAttr>(this)->getSpelling();
  case attr::Unavailable:
    return cast<UnavailableAttr>(this)->getSpelling();
  case attr::Unused:
    return cast<UnusedAttr>(this)->getSpelling();
  case attr::Used:
    return cast<UsedAttr>(this)->getSpelling();
  case attr::Uuid:
    return cast<UuidAttr>(this)->getSpelling();
  case attr::VecReturn:
    return cast<VecReturnAttr>(this)->getSpelling();
  case attr::VecTypeHint:
    return cast<VecTypeHintAttr>(this)->getSpelling();
  case attr::VectorCall:
    return cast<VectorCallAttr>(this)->getSpelling();
  case attr::Visibility:
    return cast<VisibilityAttr>(this)->getSpelling();
  case attr::WarnUnused:
    return cast<WarnUnusedAttr>(this)->getSpelling();
  case attr::WarnUnusedResult:
    return cast<WarnUnusedResultAttr>(this)->getSpelling();
  case attr::Weak:
    return cast<WeakAttr>(this)->getSpelling();
  case attr::WeakImport:
    return cast<WeakImportAttr>(this)->getSpelling();
  case attr::WeakRef:
    return cast<WeakRefAttr>(this)->getSpelling();
  case attr::WorkGroupSizeHint:
    return cast<WorkGroupSizeHintAttr>(this)->getSpelling();
  case attr::X86ForceAlignArgPointer:
    return cast<X86ForceAlignArgPointerAttr>(this)->getSpelling();
  case attr::XRayInstrument:
    return cast<XRayInstrumentAttr>(this)->getSpelling();
  case attr::XRayLogArgs:
    return cast<XRayLogArgsAttr>(this)->getSpelling();
  }
  llvm_unreachable("Unexpected attribute kind!");
}

Attr *Attr::clone(ASTContext &C) const {
  switch (getKind()) {
  case attr::AMDGPUFlatWorkGroupSize:
    return cast<AMDGPUFlatWorkGroupSizeAttr>(this)->clone(C);
  case attr::AMDGPUNumSGPR:
    return cast<AMDGPUNumSGPRAttr>(this)->clone(C);
  case attr::AMDGPUNumVGPR:
    return cast<AMDGPUNumVGPRAttr>(this)->clone(C);
  case attr::AMDGPUWavesPerEU:
    return cast<AMDGPUWavesPerEUAttr>(this)->clone(C);
  case attr::ARMInterrupt:
    return cast<ARMInterruptAttr>(this)->clone(C);
  case attr::AVRInterrupt:
    return cast<AVRInterruptAttr>(this)->clone(C);
  case attr::AVRSignal:
    return cast<AVRSignalAttr>(this)->clone(C);
  case attr::AbiTag:
    return cast<AbiTagAttr>(this)->clone(C);
  case attr::AcquireCapability:
    return cast<AcquireCapabilityAttr>(this)->clone(C);
  case attr::AcquiredAfter:
    return cast<AcquiredAfterAttr>(this)->clone(C);
  case attr::AcquiredBefore:
    return cast<AcquiredBeforeAttr>(this)->clone(C);
  case attr::Alias:
    return cast<AliasAttr>(this)->clone(C);
  case attr::AlignMac68k:
    return cast<AlignMac68kAttr>(this)->clone(C);
  case attr::AlignValue:
    return cast<AlignValueAttr>(this)->clone(C);
  case attr::Aligned:
    return cast<AlignedAttr>(this)->clone(C);
  case attr::AllocAlign:
    return cast<AllocAlignAttr>(this)->clone(C);
  case attr::AllocSize:
    return cast<AllocSizeAttr>(this)->clone(C);
  case attr::AlwaysInline:
    return cast<AlwaysInlineAttr>(this)->clone(C);
  case attr::AnalyzerNoReturn:
    return cast<AnalyzerNoReturnAttr>(this)->clone(C);
  case attr::Annotate:
    return cast<AnnotateAttr>(this)->clone(C);
  case attr::AnyX86Interrupt:
    return cast<AnyX86InterruptAttr>(this)->clone(C);
  case attr::ArcWeakrefUnavailable:
    return cast<ArcWeakrefUnavailableAttr>(this)->clone(C);
  case attr::ArgumentWithTypeTag:
    return cast<ArgumentWithTypeTagAttr>(this)->clone(C);
  case attr::AsmLabel:
    return cast<AsmLabelAttr>(this)->clone(C);
  case attr::AssertCapability:
    return cast<AssertCapabilityAttr>(this)->clone(C);
  case attr::AssertExclusiveLock:
    return cast<AssertExclusiveLockAttr>(this)->clone(C);
  case attr::AssertSharedLock:
    return cast<AssertSharedLockAttr>(this)->clone(C);
  case attr::AssumeAligned:
    return cast<AssumeAlignedAttr>(this)->clone(C);
  case attr::Availability:
    return cast<AvailabilityAttr>(this)->clone(C);
  case attr::Blocks:
    return cast<BlocksAttr>(this)->clone(C);
  case attr::C11NoReturn:
    return cast<C11NoReturnAttr>(this)->clone(C);
  case attr::CDecl:
    return cast<CDeclAttr>(this)->clone(C);
  case attr::CFAuditedTransfer:
    return cast<CFAuditedTransferAttr>(this)->clone(C);
  case attr::CFConsumed:
    return cast<CFConsumedAttr>(this)->clone(C);
  case attr::CFReturnsNotRetained:
    return cast<CFReturnsNotRetainedAttr>(this)->clone(C);
  case attr::CFReturnsRetained:
    return cast<CFReturnsRetainedAttr>(this)->clone(C);
  case attr::CFUnknownTransfer:
    return cast<CFUnknownTransferAttr>(this)->clone(C);
  case attr::CUDAConstant:
    return cast<CUDAConstantAttr>(this)->clone(C);
  case attr::CUDADevice:
    return cast<CUDADeviceAttr>(this)->clone(C);
  case attr::CUDAGlobal:
    return cast<CUDAGlobalAttr>(this)->clone(C);
  case attr::CUDAHost:
    return cast<CUDAHostAttr>(this)->clone(C);
  case attr::CUDAInvalidTarget:
    return cast<CUDAInvalidTargetAttr>(this)->clone(C);
  case attr::CUDALaunchBounds:
    return cast<CUDALaunchBoundsAttr>(this)->clone(C);
  case attr::CUDAShared:
    return cast<CUDASharedAttr>(this)->clone(C);
  case attr::CXX11NoReturn:
    return cast<CXX11NoReturnAttr>(this)->clone(C);
  case attr::CallableWhen:
    return cast<CallableWhenAttr>(this)->clone(C);
  case attr::Capability:
    return cast<CapabilityAttr>(this)->clone(C);
  case attr::CapturedRecord:
    return cast<CapturedRecordAttr>(this)->clone(C);
  case attr::CarriesDependency:
    return cast<CarriesDependencyAttr>(this)->clone(C);
  case attr::Cleanup:
    return cast<CleanupAttr>(this)->clone(C);
  case attr::Cold:
    return cast<ColdAttr>(this)->clone(C);
  case attr::Common:
    return cast<CommonAttr>(this)->clone(C);
  case attr::Const:
    return cast<ConstAttr>(this)->clone(C);
  case attr::Constructor:
    return cast<ConstructorAttr>(this)->clone(C);
  case attr::Consumable:
    return cast<ConsumableAttr>(this)->clone(C);
  case attr::ConsumableAutoCast:
    return cast<ConsumableAutoCastAttr>(this)->clone(C);
  case attr::ConsumableSetOnRead:
    return cast<ConsumableSetOnReadAttr>(this)->clone(C);
  case attr::Convergent:
    return cast<ConvergentAttr>(this)->clone(C);
  case attr::DLLExport:
    return cast<DLLExportAttr>(this)->clone(C);
  case attr::DLLImport:
    return cast<DLLImportAttr>(this)->clone(C);
  case attr::Deprecated:
    return cast<DeprecatedAttr>(this)->clone(C);
  case attr::Destructor:
    return cast<DestructorAttr>(this)->clone(C);
  case attr::DiagnoseIf:
    return cast<DiagnoseIfAttr>(this)->clone(C);
  case attr::DisableTailCalls:
    return cast<DisableTailCallsAttr>(this)->clone(C);
  case attr::EmptyBases:
    return cast<EmptyBasesAttr>(this)->clone(C);
  case attr::EnableIf:
    return cast<EnableIfAttr>(this)->clone(C);
  case attr::EnumExtensibility:
    return cast<EnumExtensibilityAttr>(this)->clone(C);
  case attr::ExclusiveTrylockFunction:
    return cast<ExclusiveTrylockFunctionAttr>(this)->clone(C);
  case attr::ExternalSourceSymbol:
    return cast<ExternalSourceSymbolAttr>(this)->clone(C);
  case attr::FallThrough:
    return cast<FallThroughAttr>(this)->clone(C);
  case attr::FastCall:
    return cast<FastCallAttr>(this)->clone(C);
  case attr::Final:
    return cast<FinalAttr>(this)->clone(C);
  case attr::FlagEnum:
    return cast<FlagEnumAttr>(this)->clone(C);
  case attr::Flatten:
    return cast<FlattenAttr>(this)->clone(C);
  case attr::Format:
    return cast<FormatAttr>(this)->clone(C);
  case attr::FormatArg:
    return cast<FormatArgAttr>(this)->clone(C);
  case attr::GNUInline:
    return cast<GNUInlineAttr>(this)->clone(C);
  case attr::GuardedBy:
    return cast<GuardedByAttr>(this)->clone(C);
  case attr::GuardedVar:
    return cast<GuardedVarAttr>(this)->clone(C);
  case attr::Hot:
    return cast<HotAttr>(this)->clone(C);
  case attr::IBAction:
    return cast<IBActionAttr>(this)->clone(C);
  case attr::IBOutlet:
    return cast<IBOutletAttr>(this)->clone(C);
  case attr::IBOutletCollection:
    return cast<IBOutletCollectionAttr>(this)->clone(C);
  case attr::IFunc:
    return cast<IFuncAttr>(this)->clone(C);
  case attr::InitPriority:
    return cast<InitPriorityAttr>(this)->clone(C);
  case attr::InitSeg:
    return cast<InitSegAttr>(this)->clone(C);
  case attr::IntelOclBicc:
    return cast<IntelOclBiccAttr>(this)->clone(C);
  case attr::InternalLinkage:
    return cast<InternalLinkageAttr>(this)->clone(C);
  case attr::LTOVisibilityPublic:
    return cast<LTOVisibilityPublicAttr>(this)->clone(C);
  case attr::LayoutVersion:
    return cast<LayoutVersionAttr>(this)->clone(C);
  case attr::LockReturned:
    return cast<LockReturnedAttr>(this)->clone(C);
  case attr::LocksExcluded:
    return cast<LocksExcludedAttr>(this)->clone(C);
  case attr::LoopHint:
    return cast<LoopHintAttr>(this)->clone(C);
  case attr::MSABI:
    return cast<MSABIAttr>(this)->clone(C);
  case attr::MSInheritance:
    return cast<MSInheritanceAttr>(this)->clone(C);
  case attr::MSNoVTable:
    return cast<MSNoVTableAttr>(this)->clone(C);
  case attr::MSP430Interrupt:
    return cast<MSP430InterruptAttr>(this)->clone(C);
  case attr::MSStruct:
    return cast<MSStructAttr>(this)->clone(C);
  case attr::MSVtorDisp:
    return cast<MSVtorDispAttr>(this)->clone(C);
  case attr::MaxFieldAlignment:
    return cast<MaxFieldAlignmentAttr>(this)->clone(C);
  case attr::MayAlias:
    return cast<MayAliasAttr>(this)->clone(C);
  case attr::MinSize:
    return cast<MinSizeAttr>(this)->clone(C);
  case attr::Mips16:
    return cast<Mips16Attr>(this)->clone(C);
  case attr::MipsInterrupt:
    return cast<MipsInterruptAttr>(this)->clone(C);
  case attr::Mode:
    return cast<ModeAttr>(this)->clone(C);
  case attr::NSConsumed:
    return cast<NSConsumedAttr>(this)->clone(C);
  case attr::NSConsumesSelf:
    return cast<NSConsumesSelfAttr>(this)->clone(C);
  case attr::NSReturnsAutoreleased:
    return cast<NSReturnsAutoreleasedAttr>(this)->clone(C);
  case attr::NSReturnsNotRetained:
    return cast<NSReturnsNotRetainedAttr>(this)->clone(C);
  case attr::NSReturnsRetained:
    return cast<NSReturnsRetainedAttr>(this)->clone(C);
  case attr::Naked:
    return cast<NakedAttr>(this)->clone(C);
  case attr::NoAlias:
    return cast<NoAliasAttr>(this)->clone(C);
  case attr::NoCommon:
    return cast<NoCommonAttr>(this)->clone(C);
  case attr::NoDebug:
    return cast<NoDebugAttr>(this)->clone(C);
  case attr::NoDuplicate:
    return cast<NoDuplicateAttr>(this)->clone(C);
  case attr::NoInline:
    return cast<NoInlineAttr>(this)->clone(C);
  case attr::NoInstrumentFunction:
    return cast<NoInstrumentFunctionAttr>(this)->clone(C);
  case attr::NoMips16:
    return cast<NoMips16Attr>(this)->clone(C);
  case attr::NoReturn:
    return cast<NoReturnAttr>(this)->clone(C);
  case attr::NoSanitize:
    return cast<NoSanitizeAttr>(this)->clone(C);
  case attr::NoSplitStack:
    return cast<NoSplitStackAttr>(this)->clone(C);
  case attr::NoThreadSafetyAnalysis:
    return cast<NoThreadSafetyAnalysisAttr>(this)->clone(C);
  case attr::NoThrow:
    return cast<NoThrowAttr>(this)->clone(C);
  case attr::NonNull:
    return cast<NonNullAttr>(this)->clone(C);
  case attr::NotTailCalled:
    return cast<NotTailCalledAttr>(this)->clone(C);
  case attr::OMPCaptureNoInit:
    return cast<OMPCaptureNoInitAttr>(this)->clone(C);
  case attr::OMPDeclareSimdDecl:
    return cast<OMPDeclareSimdDeclAttr>(this)->clone(C);
  case attr::OMPDeclareTargetDecl:
    return cast<OMPDeclareTargetDeclAttr>(this)->clone(C);
  case attr::OMPThreadPrivateDecl:
    return cast<OMPThreadPrivateDeclAttr>(this)->clone(C);
  case attr::ObjCBoxable:
    return cast<ObjCBoxableAttr>(this)->clone(C);
  case attr::ObjCBridge:
    return cast<ObjCBridgeAttr>(this)->clone(C);
  case attr::ObjCBridgeMutable:
    return cast<ObjCBridgeMutableAttr>(this)->clone(C);
  case attr::ObjCBridgeRelated:
    return cast<ObjCBridgeRelatedAttr>(this)->clone(C);
  case attr::ObjCDesignatedInitializer:
    return cast<ObjCDesignatedInitializerAttr>(this)->clone(C);
  case attr::ObjCException:
    return cast<ObjCExceptionAttr>(this)->clone(C);
  case attr::ObjCExplicitProtocolImpl:
    return cast<ObjCExplicitProtocolImplAttr>(this)->clone(C);
  case attr::ObjCIndependentClass:
    return cast<ObjCIndependentClassAttr>(this)->clone(C);
  case attr::ObjCMethodFamily:
    return cast<ObjCMethodFamilyAttr>(this)->clone(C);
  case attr::ObjCNSObject:
    return cast<ObjCNSObjectAttr>(this)->clone(C);
  case attr::ObjCPreciseLifetime:
    return cast<ObjCPreciseLifetimeAttr>(this)->clone(C);
  case attr::ObjCRequiresPropertyDefs:
    return cast<ObjCRequiresPropertyDefsAttr>(this)->clone(C);
  case attr::ObjCRequiresSuper:
    return cast<ObjCRequiresSuperAttr>(this)->clone(C);
  case attr::ObjCReturnsInnerPointer:
    return cast<ObjCReturnsInnerPointerAttr>(this)->clone(C);
  case attr::ObjCRootClass:
    return cast<ObjCRootClassAttr>(this)->clone(C);
  case attr::ObjCRuntimeName:
    return cast<ObjCRuntimeNameAttr>(this)->clone(C);
  case attr::ObjCRuntimeVisible:
    return cast<ObjCRuntimeVisibleAttr>(this)->clone(C);
  case attr::ObjCSubclassingRestricted:
    return cast<ObjCSubclassingRestrictedAttr>(this)->clone(C);
  case attr::OpenCLAccess:
    return cast<OpenCLAccessAttr>(this)->clone(C);
  case attr::OpenCLKernel:
    return cast<OpenCLKernelAttr>(this)->clone(C);
  case attr::OpenCLUnrollHint:
    return cast<OpenCLUnrollHintAttr>(this)->clone(C);
  case attr::OptimizeNone:
    return cast<OptimizeNoneAttr>(this)->clone(C);
  case attr::Overloadable:
    return cast<OverloadableAttr>(this)->clone(C);
  case attr::Override:
    return cast<OverrideAttr>(this)->clone(C);
  case attr::Ownership:
    return cast<OwnershipAttr>(this)->clone(C);
  case attr::Packed:
    return cast<PackedAttr>(this)->clone(C);
  case attr::ParamTypestate:
    return cast<ParamTypestateAttr>(this)->clone(C);
  case attr::Pascal:
    return cast<PascalAttr>(this)->clone(C);
  case attr::PassObjectSize:
    return cast<PassObjectSizeAttr>(this)->clone(C);
  case attr::Pcs:
    return cast<PcsAttr>(this)->clone(C);
  case attr::PreserveAll:
    return cast<PreserveAllAttr>(this)->clone(C);
  case attr::PreserveMost:
    return cast<PreserveMostAttr>(this)->clone(C);
  case attr::PtGuardedBy:
    return cast<PtGuardedByAttr>(this)->clone(C);
  case attr::PtGuardedVar:
    return cast<PtGuardedVarAttr>(this)->clone(C);
  case attr::Pure:
    return cast<PureAttr>(this)->clone(C);
  case attr::RegCall:
    return cast<RegCallAttr>(this)->clone(C);
  case attr::ReleaseCapability:
    return cast<ReleaseCapabilityAttr>(this)->clone(C);
  case attr::RenderScriptKernel:
    return cast<RenderScriptKernelAttr>(this)->clone(C);
  case attr::ReqdWorkGroupSize:
    return cast<ReqdWorkGroupSizeAttr>(this)->clone(C);
  case attr::RequireConstantInit:
    return cast<RequireConstantInitAttr>(this)->clone(C);
  case attr::RequiresCapability:
    return cast<RequiresCapabilityAttr>(this)->clone(C);
  case attr::Restrict:
    return cast<RestrictAttr>(this)->clone(C);
  case attr::ReturnTypestate:
    return cast<ReturnTypestateAttr>(this)->clone(C);
  case attr::ReturnsNonNull:
    return cast<ReturnsNonNullAttr>(this)->clone(C);
  case attr::ReturnsTwice:
    return cast<ReturnsTwiceAttr>(this)->clone(C);
  case attr::ScopedLockable:
    return cast<ScopedLockableAttr>(this)->clone(C);
  case attr::Section:
    return cast<SectionAttr>(this)->clone(C);
  case attr::SelectAny:
    return cast<SelectAnyAttr>(this)->clone(C);
  case attr::Sentinel:
    return cast<SentinelAttr>(this)->clone(C);
  case attr::SetTypestate:
    return cast<SetTypestateAttr>(this)->clone(C);
  case attr::SharedTrylockFunction:
    return cast<SharedTrylockFunctionAttr>(this)->clone(C);
  case attr::StdCall:
    return cast<StdCallAttr>(this)->clone(C);
  case attr::Suppress:
    return cast<SuppressAttr>(this)->clone(C);
  case attr::SwiftCall:
    return cast<SwiftCallAttr>(this)->clone(C);
  case attr::SwiftContext:
    return cast<SwiftContextAttr>(this)->clone(C);
  case attr::SwiftErrorResult:
    return cast<SwiftErrorResultAttr>(this)->clone(C);
  case attr::SwiftIndirectResult:
    return cast<SwiftIndirectResultAttr>(this)->clone(C);
  case attr::SysVABI:
    return cast<SysVABIAttr>(this)->clone(C);
  case attr::TLSModel:
    return cast<TLSModelAttr>(this)->clone(C);
  case attr::Target:
    return cast<TargetAttr>(this)->clone(C);
  case attr::TestTypestate:
    return cast<TestTypestateAttr>(this)->clone(C);
  case attr::ThisCall:
    return cast<ThisCallAttr>(this)->clone(C);
  case attr::Thread:
    return cast<ThreadAttr>(this)->clone(C);
  case attr::TransparentUnion:
    return cast<TransparentUnionAttr>(this)->clone(C);
  case attr::TryAcquireCapability:
    return cast<TryAcquireCapabilityAttr>(this)->clone(C);
  case attr::TypeTagForDatatype:
    return cast<TypeTagForDatatypeAttr>(this)->clone(C);
  case attr::TypeVisibility:
    return cast<TypeVisibilityAttr>(this)->clone(C);
  case attr::Unavailable:
    return cast<UnavailableAttr>(this)->clone(C);
  case attr::Unused:
    return cast<UnusedAttr>(this)->clone(C);
  case attr::Used:
    return cast<UsedAttr>(this)->clone(C);
  case attr::Uuid:
    return cast<UuidAttr>(this)->clone(C);
  case attr::VecReturn:
    return cast<VecReturnAttr>(this)->clone(C);
  case attr::VecTypeHint:
    return cast<VecTypeHintAttr>(this)->clone(C);
  case attr::VectorCall:
    return cast<VectorCallAttr>(this)->clone(C);
  case attr::Visibility:
    return cast<VisibilityAttr>(this)->clone(C);
  case attr::WarnUnused:
    return cast<WarnUnusedAttr>(this)->clone(C);
  case attr::WarnUnusedResult:
    return cast<WarnUnusedResultAttr>(this)->clone(C);
  case attr::Weak:
    return cast<WeakAttr>(this)->clone(C);
  case attr::WeakImport:
    return cast<WeakImportAttr>(this)->clone(C);
  case attr::WeakRef:
    return cast<WeakRefAttr>(this)->clone(C);
  case attr::WorkGroupSizeHint:
    return cast<WorkGroupSizeHintAttr>(this)->clone(C);
  case attr::X86ForceAlignArgPointer:
    return cast<X86ForceAlignArgPointerAttr>(this)->clone(C);
  case attr::XRayInstrument:
    return cast<XRayInstrumentAttr>(this)->clone(C);
  case attr::XRayLogArgs:
    return cast<XRayLogArgsAttr>(this)->clone(C);
  }
  llvm_unreachable("Unexpected attribute kind!");
}

void Attr::printPretty(raw_ostream &OS, const PrintingPolicy &Policy) const {
  switch (getKind()) {
  case attr::AMDGPUFlatWorkGroupSize:
    return cast<AMDGPUFlatWorkGroupSizeAttr>(this)->printPretty(OS, Policy);
  case attr::AMDGPUNumSGPR:
    return cast<AMDGPUNumSGPRAttr>(this)->printPretty(OS, Policy);
  case attr::AMDGPUNumVGPR:
    return cast<AMDGPUNumVGPRAttr>(this)->printPretty(OS, Policy);
  case attr::AMDGPUWavesPerEU:
    return cast<AMDGPUWavesPerEUAttr>(this)->printPretty(OS, Policy);
  case attr::ARMInterrupt:
    return cast<ARMInterruptAttr>(this)->printPretty(OS, Policy);
  case attr::AVRInterrupt:
    return cast<AVRInterruptAttr>(this)->printPretty(OS, Policy);
  case attr::AVRSignal:
    return cast<AVRSignalAttr>(this)->printPretty(OS, Policy);
  case attr::AbiTag:
    return cast<AbiTagAttr>(this)->printPretty(OS, Policy);
  case attr::AcquireCapability:
    return cast<AcquireCapabilityAttr>(this)->printPretty(OS, Policy);
  case attr::AcquiredAfter:
    return cast<AcquiredAfterAttr>(this)->printPretty(OS, Policy);
  case attr::AcquiredBefore:
    return cast<AcquiredBeforeAttr>(this)->printPretty(OS, Policy);
  case attr::Alias:
    return cast<AliasAttr>(this)->printPretty(OS, Policy);
  case attr::AlignMac68k:
    return cast<AlignMac68kAttr>(this)->printPretty(OS, Policy);
  case attr::AlignValue:
    return cast<AlignValueAttr>(this)->printPretty(OS, Policy);
  case attr::Aligned:
    return cast<AlignedAttr>(this)->printPretty(OS, Policy);
  case attr::AllocAlign:
    return cast<AllocAlignAttr>(this)->printPretty(OS, Policy);
  case attr::AllocSize:
    return cast<AllocSizeAttr>(this)->printPretty(OS, Policy);
  case attr::AlwaysInline:
    return cast<AlwaysInlineAttr>(this)->printPretty(OS, Policy);
  case attr::AnalyzerNoReturn:
    return cast<AnalyzerNoReturnAttr>(this)->printPretty(OS, Policy);
  case attr::Annotate:
    return cast<AnnotateAttr>(this)->printPretty(OS, Policy);
  case attr::AnyX86Interrupt:
    return cast<AnyX86InterruptAttr>(this)->printPretty(OS, Policy);
  case attr::ArcWeakrefUnavailable:
    return cast<ArcWeakrefUnavailableAttr>(this)->printPretty(OS, Policy);
  case attr::ArgumentWithTypeTag:
    return cast<ArgumentWithTypeTagAttr>(this)->printPretty(OS, Policy);
  case attr::AsmLabel:
    return cast<AsmLabelAttr>(this)->printPretty(OS, Policy);
  case attr::AssertCapability:
    return cast<AssertCapabilityAttr>(this)->printPretty(OS, Policy);
  case attr::AssertExclusiveLock:
    return cast<AssertExclusiveLockAttr>(this)->printPretty(OS, Policy);
  case attr::AssertSharedLock:
    return cast<AssertSharedLockAttr>(this)->printPretty(OS, Policy);
  case attr::AssumeAligned:
    return cast<AssumeAlignedAttr>(this)->printPretty(OS, Policy);
  case attr::Availability:
    return cast<AvailabilityAttr>(this)->printPretty(OS, Policy);
  case attr::Blocks:
    return cast<BlocksAttr>(this)->printPretty(OS, Policy);
  case attr::C11NoReturn:
    return cast<C11NoReturnAttr>(this)->printPretty(OS, Policy);
  case attr::CDecl:
    return cast<CDeclAttr>(this)->printPretty(OS, Policy);
  case attr::CFAuditedTransfer:
    return cast<CFAuditedTransferAttr>(this)->printPretty(OS, Policy);
  case attr::CFConsumed:
    return cast<CFConsumedAttr>(this)->printPretty(OS, Policy);
  case attr::CFReturnsNotRetained:
    return cast<CFReturnsNotRetainedAttr>(this)->printPretty(OS, Policy);
  case attr::CFReturnsRetained:
    return cast<CFReturnsRetainedAttr>(this)->printPretty(OS, Policy);
  case attr::CFUnknownTransfer:
    return cast<CFUnknownTransferAttr>(this)->printPretty(OS, Policy);
  case attr::CUDAConstant:
    return cast<CUDAConstantAttr>(this)->printPretty(OS, Policy);
  case attr::CUDADevice:
    return cast<CUDADeviceAttr>(this)->printPretty(OS, Policy);
  case attr::CUDAGlobal:
    return cast<CUDAGlobalAttr>(this)->printPretty(OS, Policy);
  case attr::CUDAHost:
    return cast<CUDAHostAttr>(this)->printPretty(OS, Policy);
  case attr::CUDAInvalidTarget:
    return cast<CUDAInvalidTargetAttr>(this)->printPretty(OS, Policy);
  case attr::CUDALaunchBounds:
    return cast<CUDALaunchBoundsAttr>(this)->printPretty(OS, Policy);
  case attr::CUDAShared:
    return cast<CUDASharedAttr>(this)->printPretty(OS, Policy);
  case attr::CXX11NoReturn:
    return cast<CXX11NoReturnAttr>(this)->printPretty(OS, Policy);
  case attr::CallableWhen:
    return cast<CallableWhenAttr>(this)->printPretty(OS, Policy);
  case attr::Capability:
    return cast<CapabilityAttr>(this)->printPretty(OS, Policy);
  case attr::CapturedRecord:
    return cast<CapturedRecordAttr>(this)->printPretty(OS, Policy);
  case attr::CarriesDependency:
    return cast<CarriesDependencyAttr>(this)->printPretty(OS, Policy);
  case attr::Cleanup:
    return cast<CleanupAttr>(this)->printPretty(OS, Policy);
  case attr::Cold:
    return cast<ColdAttr>(this)->printPretty(OS, Policy);
  case attr::Common:
    return cast<CommonAttr>(this)->printPretty(OS, Policy);
  case attr::Const:
    return cast<ConstAttr>(this)->printPretty(OS, Policy);
  case attr::Constructor:
    return cast<ConstructorAttr>(this)->printPretty(OS, Policy);
  case attr::Consumable:
    return cast<ConsumableAttr>(this)->printPretty(OS, Policy);
  case attr::ConsumableAutoCast:
    return cast<ConsumableAutoCastAttr>(this)->printPretty(OS, Policy);
  case attr::ConsumableSetOnRead:
    return cast<ConsumableSetOnReadAttr>(this)->printPretty(OS, Policy);
  case attr::Convergent:
    return cast<ConvergentAttr>(this)->printPretty(OS, Policy);
  case attr::DLLExport:
    return cast<DLLExportAttr>(this)->printPretty(OS, Policy);
  case attr::DLLImport:
    return cast<DLLImportAttr>(this)->printPretty(OS, Policy);
  case attr::Deprecated:
    return cast<DeprecatedAttr>(this)->printPretty(OS, Policy);
  case attr::Destructor:
    return cast<DestructorAttr>(this)->printPretty(OS, Policy);
  case attr::DiagnoseIf:
    return cast<DiagnoseIfAttr>(this)->printPretty(OS, Policy);
  case attr::DisableTailCalls:
    return cast<DisableTailCallsAttr>(this)->printPretty(OS, Policy);
  case attr::EmptyBases:
    return cast<EmptyBasesAttr>(this)->printPretty(OS, Policy);
  case attr::EnableIf:
    return cast<EnableIfAttr>(this)->printPretty(OS, Policy);
  case attr::EnumExtensibility:
    return cast<EnumExtensibilityAttr>(this)->printPretty(OS, Policy);
  case attr::ExclusiveTrylockFunction:
    return cast<ExclusiveTrylockFunctionAttr>(this)->printPretty(OS, Policy);
  case attr::ExternalSourceSymbol:
    return cast<ExternalSourceSymbolAttr>(this)->printPretty(OS, Policy);
  case attr::FallThrough:
    return cast<FallThroughAttr>(this)->printPretty(OS, Policy);
  case attr::FastCall:
    return cast<FastCallAttr>(this)->printPretty(OS, Policy);
  case attr::Final:
    return cast<FinalAttr>(this)->printPretty(OS, Policy);
  case attr::FlagEnum:
    return cast<FlagEnumAttr>(this)->printPretty(OS, Policy);
  case attr::Flatten:
    return cast<FlattenAttr>(this)->printPretty(OS, Policy);
  case attr::Format:
    return cast<FormatAttr>(this)->printPretty(OS, Policy);
  case attr::FormatArg:
    return cast<FormatArgAttr>(this)->printPretty(OS, Policy);
  case attr::GNUInline:
    return cast<GNUInlineAttr>(this)->printPretty(OS, Policy);
  case attr::GuardedBy:
    return cast<GuardedByAttr>(this)->printPretty(OS, Policy);
  case attr::GuardedVar:
    return cast<GuardedVarAttr>(this)->printPretty(OS, Policy);
  case attr::Hot:
    return cast<HotAttr>(this)->printPretty(OS, Policy);
  case attr::IBAction:
    return cast<IBActionAttr>(this)->printPretty(OS, Policy);
  case attr::IBOutlet:
    return cast<IBOutletAttr>(this)->printPretty(OS, Policy);
  case attr::IBOutletCollection:
    return cast<IBOutletCollectionAttr>(this)->printPretty(OS, Policy);
  case attr::IFunc:
    return cast<IFuncAttr>(this)->printPretty(OS, Policy);
  case attr::InitPriority:
    return cast<InitPriorityAttr>(this)->printPretty(OS, Policy);
  case attr::InitSeg:
    return cast<InitSegAttr>(this)->printPretty(OS, Policy);
  case attr::IntelOclBicc:
    return cast<IntelOclBiccAttr>(this)->printPretty(OS, Policy);
  case attr::InternalLinkage:
    return cast<InternalLinkageAttr>(this)->printPretty(OS, Policy);
  case attr::LTOVisibilityPublic:
    return cast<LTOVisibilityPublicAttr>(this)->printPretty(OS, Policy);
  case attr::LayoutVersion:
    return cast<LayoutVersionAttr>(this)->printPretty(OS, Policy);
  case attr::LockReturned:
    return cast<LockReturnedAttr>(this)->printPretty(OS, Policy);
  case attr::LocksExcluded:
    return cast<LocksExcludedAttr>(this)->printPretty(OS, Policy);
  case attr::LoopHint:
    return cast<LoopHintAttr>(this)->printPretty(OS, Policy);
  case attr::MSABI:
    return cast<MSABIAttr>(this)->printPretty(OS, Policy);
  case attr::MSInheritance:
    return cast<MSInheritanceAttr>(this)->printPretty(OS, Policy);
  case attr::MSNoVTable:
    return cast<MSNoVTableAttr>(this)->printPretty(OS, Policy);
  case attr::MSP430Interrupt:
    return cast<MSP430InterruptAttr>(this)->printPretty(OS, Policy);
  case attr::MSStruct:
    return cast<MSStructAttr>(this)->printPretty(OS, Policy);
  case attr::MSVtorDisp:
    return cast<MSVtorDispAttr>(this)->printPretty(OS, Policy);
  case attr::MaxFieldAlignment:
    return cast<MaxFieldAlignmentAttr>(this)->printPretty(OS, Policy);
  case attr::MayAlias:
    return cast<MayAliasAttr>(this)->printPretty(OS, Policy);
  case attr::MinSize:
    return cast<MinSizeAttr>(this)->printPretty(OS, Policy);
  case attr::Mips16:
    return cast<Mips16Attr>(this)->printPretty(OS, Policy);
  case attr::MipsInterrupt:
    return cast<MipsInterruptAttr>(this)->printPretty(OS, Policy);
  case attr::Mode:
    return cast<ModeAttr>(this)->printPretty(OS, Policy);
  case attr::NSConsumed:
    return cast<NSConsumedAttr>(this)->printPretty(OS, Policy);
  case attr::NSConsumesSelf:
    return cast<NSConsumesSelfAttr>(this)->printPretty(OS, Policy);
  case attr::NSReturnsAutoreleased:
    return cast<NSReturnsAutoreleasedAttr>(this)->printPretty(OS, Policy);
  case attr::NSReturnsNotRetained:
    return cast<NSReturnsNotRetainedAttr>(this)->printPretty(OS, Policy);
  case attr::NSReturnsRetained:
    return cast<NSReturnsRetainedAttr>(this)->printPretty(OS, Policy);
  case attr::Naked:
    return cast<NakedAttr>(this)->printPretty(OS, Policy);
  case attr::NoAlias:
    return cast<NoAliasAttr>(this)->printPretty(OS, Policy);
  case attr::NoCommon:
    return cast<NoCommonAttr>(this)->printPretty(OS, Policy);
  case attr::NoDebug:
    return cast<NoDebugAttr>(this)->printPretty(OS, Policy);
  case attr::NoDuplicate:
    return cast<NoDuplicateAttr>(this)->printPretty(OS, Policy);
  case attr::NoInline:
    return cast<NoInlineAttr>(this)->printPretty(OS, Policy);
  case attr::NoInstrumentFunction:
    return cast<NoInstrumentFunctionAttr>(this)->printPretty(OS, Policy);
  case attr::NoMips16:
    return cast<NoMips16Attr>(this)->printPretty(OS, Policy);
  case attr::NoReturn:
    return cast<NoReturnAttr>(this)->printPretty(OS, Policy);
  case attr::NoSanitize:
    return cast<NoSanitizeAttr>(this)->printPretty(OS, Policy);
  case attr::NoSplitStack:
    return cast<NoSplitStackAttr>(this)->printPretty(OS, Policy);
  case attr::NoThreadSafetyAnalysis:
    return cast<NoThreadSafetyAnalysisAttr>(this)->printPretty(OS, Policy);
  case attr::NoThrow:
    return cast<NoThrowAttr>(this)->printPretty(OS, Policy);
  case attr::NonNull:
    return cast<NonNullAttr>(this)->printPretty(OS, Policy);
  case attr::NotTailCalled:
    return cast<NotTailCalledAttr>(this)->printPretty(OS, Policy);
  case attr::OMPCaptureNoInit:
    return cast<OMPCaptureNoInitAttr>(this)->printPretty(OS, Policy);
  case attr::OMPDeclareSimdDecl:
    return cast<OMPDeclareSimdDeclAttr>(this)->printPretty(OS, Policy);
  case attr::OMPDeclareTargetDecl:
    return cast<OMPDeclareTargetDeclAttr>(this)->printPretty(OS, Policy);
  case attr::OMPThreadPrivateDecl:
    return cast<OMPThreadPrivateDeclAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCBoxable:
    return cast<ObjCBoxableAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCBridge:
    return cast<ObjCBridgeAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCBridgeMutable:
    return cast<ObjCBridgeMutableAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCBridgeRelated:
    return cast<ObjCBridgeRelatedAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCDesignatedInitializer:
    return cast<ObjCDesignatedInitializerAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCException:
    return cast<ObjCExceptionAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCExplicitProtocolImpl:
    return cast<ObjCExplicitProtocolImplAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCIndependentClass:
    return cast<ObjCIndependentClassAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCMethodFamily:
    return cast<ObjCMethodFamilyAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCNSObject:
    return cast<ObjCNSObjectAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCPreciseLifetime:
    return cast<ObjCPreciseLifetimeAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCRequiresPropertyDefs:
    return cast<ObjCRequiresPropertyDefsAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCRequiresSuper:
    return cast<ObjCRequiresSuperAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCReturnsInnerPointer:
    return cast<ObjCReturnsInnerPointerAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCRootClass:
    return cast<ObjCRootClassAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCRuntimeName:
    return cast<ObjCRuntimeNameAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCRuntimeVisible:
    return cast<ObjCRuntimeVisibleAttr>(this)->printPretty(OS, Policy);
  case attr::ObjCSubclassingRestricted:
    return cast<ObjCSubclassingRestrictedAttr>(this)->printPretty(OS, Policy);
  case attr::OpenCLAccess:
    return cast<OpenCLAccessAttr>(this)->printPretty(OS, Policy);
  case attr::OpenCLKernel:
    return cast<OpenCLKernelAttr>(this)->printPretty(OS, Policy);
  case attr::OpenCLUnrollHint:
    return cast<OpenCLUnrollHintAttr>(this)->printPretty(OS, Policy);
  case attr::OptimizeNone:
    return cast<OptimizeNoneAttr>(this)->printPretty(OS, Policy);
  case attr::Overloadable:
    return cast<OverloadableAttr>(this)->printPretty(OS, Policy);
  case attr::Override:
    return cast<OverrideAttr>(this)->printPretty(OS, Policy);
  case attr::Ownership:
    return cast<OwnershipAttr>(this)->printPretty(OS, Policy);
  case attr::Packed:
    return cast<PackedAttr>(this)->printPretty(OS, Policy);
  case attr::ParamTypestate:
    return cast<ParamTypestateAttr>(this)->printPretty(OS, Policy);
  case attr::Pascal:
    return cast<PascalAttr>(this)->printPretty(OS, Policy);
  case attr::PassObjectSize:
    return cast<PassObjectSizeAttr>(this)->printPretty(OS, Policy);
  case attr::Pcs:
    return cast<PcsAttr>(this)->printPretty(OS, Policy);
  case attr::PreserveAll:
    return cast<PreserveAllAttr>(this)->printPretty(OS, Policy);
  case attr::PreserveMost:
    return cast<PreserveMostAttr>(this)->printPretty(OS, Policy);
  case attr::PtGuardedBy:
    return cast<PtGuardedByAttr>(this)->printPretty(OS, Policy);
  case attr::PtGuardedVar:
    return cast<PtGuardedVarAttr>(this)->printPretty(OS, Policy);
  case attr::Pure:
    return cast<PureAttr>(this)->printPretty(OS, Policy);
  case attr::RegCall:
    return cast<RegCallAttr>(this)->printPretty(OS, Policy);
  case attr::ReleaseCapability:
    return cast<ReleaseCapabilityAttr>(this)->printPretty(OS, Policy);
  case attr::RenderScriptKernel:
    return cast<RenderScriptKernelAttr>(this)->printPretty(OS, Policy);
  case attr::ReqdWorkGroupSize:
    return cast<ReqdWorkGroupSizeAttr>(this)->printPretty(OS, Policy);
  case attr::RequireConstantInit:
    return cast<RequireConstantInitAttr>(this)->printPretty(OS, Policy);
  case attr::RequiresCapability:
    return cast<RequiresCapabilityAttr>(this)->printPretty(OS, Policy);
  case attr::Restrict:
    return cast<RestrictAttr>(this)->printPretty(OS, Policy);
  case attr::ReturnTypestate:
    return cast<ReturnTypestateAttr>(this)->printPretty(OS, Policy);
  case attr::ReturnsNonNull:
    return cast<ReturnsNonNullAttr>(this)->printPretty(OS, Policy);
  case attr::ReturnsTwice:
    return cast<ReturnsTwiceAttr>(this)->printPretty(OS, Policy);
  case attr::ScopedLockable:
    return cast<ScopedLockableAttr>(this)->printPretty(OS, Policy);
  case attr::Section:
    return cast<SectionAttr>(this)->printPretty(OS, Policy);
  case attr::SelectAny:
    return cast<SelectAnyAttr>(this)->printPretty(OS, Policy);
  case attr::Sentinel:
    return cast<SentinelAttr>(this)->printPretty(OS, Policy);
  case attr::SetTypestate:
    return cast<SetTypestateAttr>(this)->printPretty(OS, Policy);
  case attr::SharedTrylockFunction:
    return cast<SharedTrylockFunctionAttr>(this)->printPretty(OS, Policy);
  case attr::StdCall:
    return cast<StdCallAttr>(this)->printPretty(OS, Policy);
  case attr::Suppress:
    return cast<SuppressAttr>(this)->printPretty(OS, Policy);
  case attr::SwiftCall:
    return cast<SwiftCallAttr>(this)->printPretty(OS, Policy);
  case attr::SwiftContext:
    return cast<SwiftContextAttr>(this)->printPretty(OS, Policy);
  case attr::SwiftErrorResult:
    return cast<SwiftErrorResultAttr>(this)->printPretty(OS, Policy);
  case attr::SwiftIndirectResult:
    return cast<SwiftIndirectResultAttr>(this)->printPretty(OS, Policy);
  case attr::SysVABI:
    return cast<SysVABIAttr>(this)->printPretty(OS, Policy);
  case attr::TLSModel:
    return cast<TLSModelAttr>(this)->printPretty(OS, Policy);
  case attr::Target:
    return cast<TargetAttr>(this)->printPretty(OS, Policy);
  case attr::TestTypestate:
    return cast<TestTypestateAttr>(this)->printPretty(OS, Policy);
  case attr::ThisCall:
    return cast<ThisCallAttr>(this)->printPretty(OS, Policy);
  case attr::Thread:
    return cast<ThreadAttr>(this)->printPretty(OS, Policy);
  case attr::TransparentUnion:
    return cast<TransparentUnionAttr>(this)->printPretty(OS, Policy);
  case attr::TryAcquireCapability:
    return cast<TryAcquireCapabilityAttr>(this)->printPretty(OS, Policy);
  case attr::TypeTagForDatatype:
    return cast<TypeTagForDatatypeAttr>(this)->printPretty(OS, Policy);
  case attr::TypeVisibility:
    return cast<TypeVisibilityAttr>(this)->printPretty(OS, Policy);
  case attr::Unavailable:
    return cast<UnavailableAttr>(this)->printPretty(OS, Policy);
  case attr::Unused:
    return cast<UnusedAttr>(this)->printPretty(OS, Policy);
  case attr::Used:
    return cast<UsedAttr>(this)->printPretty(OS, Policy);
  case attr::Uuid:
    return cast<UuidAttr>(this)->printPretty(OS, Policy);
  case attr::VecReturn:
    return cast<VecReturnAttr>(this)->printPretty(OS, Policy);
  case attr::VecTypeHint:
    return cast<VecTypeHintAttr>(this)->printPretty(OS, Policy);
  case attr::VectorCall:
    return cast<VectorCallAttr>(this)->printPretty(OS, Policy);
  case attr::Visibility:
    return cast<VisibilityAttr>(this)->printPretty(OS, Policy);
  case attr::WarnUnused:
    return cast<WarnUnusedAttr>(this)->printPretty(OS, Policy);
  case attr::WarnUnusedResult:
    return cast<WarnUnusedResultAttr>(this)->printPretty(OS, Policy);
  case attr::Weak:
    return cast<WeakAttr>(this)->printPretty(OS, Policy);
  case attr::WeakImport:
    return cast<WeakImportAttr>(this)->printPretty(OS, Policy);
  case attr::WeakRef:
    return cast<WeakRefAttr>(this)->printPretty(OS, Policy);
  case attr::WorkGroupSizeHint:
    return cast<WorkGroupSizeHintAttr>(this)->printPretty(OS, Policy);
  case attr::X86ForceAlignArgPointer:
    return cast<X86ForceAlignArgPointerAttr>(this)->printPretty(OS, Policy);
  case attr::XRayInstrument:
    return cast<XRayInstrumentAttr>(this)->printPretty(OS, Policy);
  case attr::XRayLogArgs:
    return cast<XRayLogArgsAttr>(this)->printPretty(OS, Policy);
  }
  llvm_unreachable("Unexpected attribute kind!");
}

