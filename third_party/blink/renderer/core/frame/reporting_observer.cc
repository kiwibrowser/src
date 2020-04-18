// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/reporting_observer.h"

#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/report.h"
#include "third_party/blink/renderer/core/frame/reporting_context.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

ReportingObserver* ReportingObserver::Create(
    ExecutionContext* execution_context,
    V8ReportingObserverCallback* callback) {
  return new ReportingObserver(execution_context, callback);
}

ReportingObserver::ReportingObserver(ExecutionContext* execution_context,
                                     V8ReportingObserverCallback* callback)
    : execution_context_(execution_context), callback_(callback) {}

void ReportingObserver::ReportToCallback(
    const HeapVector<Member<Report>>& reports) {
  callback_->InvokeAndReportException(this, reports, this);
}

void ReportingObserver::observe() {
  ReportingContext::From(execution_context_)->RegisterObserver(this);
}

void ReportingObserver::disconnect() {
  ReportingContext::From(execution_context_)->UnregisterObserver(this);
}

void ReportingObserver::Trace(blink::Visitor* visitor) {
  visitor->Trace(execution_context_);
  visitor->Trace(callback_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
