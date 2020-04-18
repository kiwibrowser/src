// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "CheckTraceWrappersVisitor.h"

#include <vector>

#include "Config.h"

using namespace clang;

CheckTraceWrappersVisitor::CheckTraceWrappersVisitor(CXXMethodDecl* trace,
                                                     RecordInfo* info,
                                                     RecordCache* cache)
    : trace_wrappers_(trace), info_(info), cache_(cache) {}

bool CheckTraceWrappersVisitor::VisitCallExpr(CallExpr* call) {
  CheckTraceBaseCall(call);
  return true;
}

bool CheckTraceWrappersVisitor::IsTraceWrappersCallName(
    const std::string& name) {
  // See CheckTraceVisitor::IsTraceCallName.
  return name == trace_wrappers_->getName();
}

bool CheckTraceWrappersVisitor::CheckTraceBaseCall(CallExpr* call) {
  // Checks for "Base::TraceWrappers(visitor)"-like calls.

  // For example, if we've got "Base::TraceWrappers(visitor)" as |call|,
  // callee_record will be "Base", and func_name will be "TraceWrappers".
  CXXRecordDecl* callee_record = nullptr;
  std::string func_name;

  if (MemberExpr* callee = dyn_cast<MemberExpr>(call->getCallee())) {
    if (!callee->hasQualifier())
      return false;

    FunctionDecl* trace_decl = dyn_cast<FunctionDecl>(callee->getMemberDecl());
    if (!trace_decl || !Config::IsTraceWrappersMethod(trace_decl))
      return false;

    const Type* type = callee->getQualifier()->getAsType();
    if (!type)
      return false;

    callee_record = type->getAsCXXRecordDecl();
    func_name = trace_decl->getName();
  }

  if (!callee_record)
    return false;

  if (!IsTraceWrappersCallName(func_name))
    return false;

  for (auto& base : info_->GetBases()) {
    // We want to deal with omitted TraceWrappers() function in an intermediary
    // class in the class hierarchy, e.g.:
    //     class A : public TraceWrapperBase<A> { TraceWrappers() { ... } };
    //     class B : public A {
    //       /* No TraceWrappers(); have nothing to trace. */
    //     };
    //     class C : public B { TraceWrappers() { B::TraceWrappers(visitor); } }
    // where, B::TraceWrappers() is actually A::TraceWrappers(), and in some
    // cases we get A as |callee_record| instead of B. We somehow need to mark B
    // as wrapper traced if we find A::TraceWrappers() call.
    //
    // To solve this, here we keep going up the class hierarchy as long as
    // they are not required to have a trace method. The implementation is
    // a simple DFS, where |base_records| represents the set of base classes
    // we need to visit.

    std::vector<CXXRecordDecl*> base_records;
    base_records.push_back(base.first);

    while (!base_records.empty()) {
      CXXRecordDecl* base_record = base_records.back();
      base_records.pop_back();

      if (base_record == callee_record) {
        // If we find a matching trace method, pretend the user has written
        // a correct trace() method of the base; in the example above, we
        // find A::trace() here and mark B as correctly traced.
        base.second.MarkWrapperTraced();
        return true;
      }

      if (RecordInfo* base_info = cache_->Lookup(base_record)) {
        if (!base_info->RequiresTraceWrappersMethod()) {
          // If this base class is not required to have a trace method, then
          // the actual trace method may be defined in an ancestor.
          for (auto& inner_base : base_info->GetBases())
            base_records.push_back(inner_base.first);
        }
      }
    }
  }

  return false;
}
