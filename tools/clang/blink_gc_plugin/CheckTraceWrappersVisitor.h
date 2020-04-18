// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_BLINK_GC_PLUGIN_CHECK_TRACE_WRAPPERS_VISITOR_H_
#define TOOLS_BLINK_GC_PLUGIN_CHECK_TRACE_WRAPPERS_VISITOR_H_

#include <string>

#include "RecordInfo.h"
#include "clang/AST/AST.h"
#include "clang/AST/RecursiveASTVisitor.h"

class RecordCache;
class RecordInfo;

// This visitor checks a wrapper tracing method by traversing its body.
// - A base is wrapper traced if a base-qualified call to a trace method is
//   found.
class CheckTraceWrappersVisitor
    : public clang::RecursiveASTVisitor<CheckTraceWrappersVisitor> {
 public:
  CheckTraceWrappersVisitor(clang::CXXMethodDecl* trace,
                            RecordInfo* info,
                            RecordCache* cache);

  bool VisitCallExpr(clang::CallExpr* call);

 private:
  bool IsTraceWrappersCallName(const std::string& name);

  bool CheckTraceBaseCall(clang::CallExpr* call);

  clang::CXXMethodDecl* trace_wrappers_;
  RecordInfo* info_;
  RecordCache* cache_;
};

#endif  // TOOLS_BLINK_GC_PLUGIN_CHECK_TRACE_WRAPPERS_VISITOR_H_
