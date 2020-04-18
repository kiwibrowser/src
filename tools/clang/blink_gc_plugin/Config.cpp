// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "Config.h"

#include <cassert>

#include "clang/AST/AST.h"

using namespace clang;

const char kNewOperatorName[] = "operator new";
const char kCreateName[] = "Create";
const char kTraceName[] = "Trace";
const char kTraceWrappersName[] = "TraceWrappers";
const char kFinalizeName[] = "FinalizeGarbageCollectedObject";
const char kTraceAfterDispatchName[] = "TraceAfterDispatch";
const char kRegisterWeakMembersName[] = "RegisterWeakMembers";
const char kHeapAllocatorName[] = "HeapAllocator";
const char kTraceIfNeededName[] = "TraceIfNeeded";
const char kVisitorDispatcherName[] = "VisitorDispatcher";
const char kVisitorVarName[] = "visitor";
const char kAdjustAndMarkName[] = "AdjustAndMark";
const char kIsHeapObjectAliveName[] = "IsHeapObjectAlive";
const char kIsEagerlyFinalizedName[] = "IsEagerlyFinalizedMarker";
const char kConstIteratorName[] = "const_iterator";
const char kIteratorName[] = "iterator";
const char kConstReverseIteratorName[] = "const_reverse_iterator";
const char kReverseIteratorName[] = "reverse_iterator";

const char* kIgnoredTraceWrapperNames[] = {
    "blink::ScriptWrappableVisitor::TraceWrappers",
    "blink::WrapperMarkingData::TraceWrappers"};

bool Config::IsTemplateInstantiation(CXXRecordDecl* record) {
  ClassTemplateSpecializationDecl* spec =
      dyn_cast<clang::ClassTemplateSpecializationDecl>(record);
  if (!spec)
    return false;
  switch (spec->getTemplateSpecializationKind()) {
    case TSK_ImplicitInstantiation:
    case TSK_ExplicitInstantiationDefinition:
      return true;
    case TSK_Undeclared:
    case TSK_ExplicitSpecialization:
      return false;
    // TODO: unsupported cases.
    case TSK_ExplicitInstantiationDeclaration:
      return false;
  }
  assert(false && "Unknown template specialization kind");
  return false;
}

// static
Config::TraceWrappersMethodType Config::GetTraceWrappersMethodType(
    const clang::FunctionDecl* method) {
  if (method->getNumParams() != 1)
    return NOT_TRACE_WRAPPERS_METHOD;

  const std::string& name = method->getNameAsString();
  const std::string& full_name = method->getQualifiedNameAsString();
  for (size_t i = 0; i < (sizeof(kIgnoredTraceWrapperNames) /
                          sizeof(kIgnoredTraceWrapperNames[0]));
       i++) {
    if (full_name == kIgnoredTraceWrapperNames[i])
      return NOT_TRACE_WRAPPERS_METHOD;
  }

  if (name == kTraceWrappersName)
    return TRACE_WRAPPERS_METHOD;

  return NOT_TRACE_WRAPPERS_METHOD;
}

// static
bool Config::IsTraceWrappersMethod(const clang::FunctionDecl* method) {
  return GetTraceWrappersMethodType(method) != NOT_TRACE_WRAPPERS_METHOD;
}
