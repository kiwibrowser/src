// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/reporting_context.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/report.h"
#include "third_party/blink/renderer/core/frame/reporting_observer.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

// static
const char ReportingContext::kSupplementName[] = "ReportingContext";

ReportingContext::ReportingContext(ExecutionContext& context)
    : Supplement<ExecutionContext>(context), execution_context_(context) {}

// static
ReportingContext* ReportingContext::From(ExecutionContext* context) {
  ReportingContext* reporting_context =
      Supplement<ExecutionContext>::From<ReportingContext>(context);
  if (!reporting_context) {
    reporting_context = new ReportingContext(*context);
    Supplement<ExecutionContext>::ProvideTo(*context, reporting_context);
  }
  return reporting_context;
}

void ReportingContext::QueueReport(Report* report) {
  if (!ObserverExists())
    return;

  reports_.push_back(report);

  // When the first report of a batch is queued, make a task to report the whole
  // batch (in the queue) to all ReportingObservers.
  if (reports_.size() == 1) {
    execution_context_->GetTaskRunner(TaskType::kMiscPlatformAPI)
        ->PostTask(FROM_HERE, WTF::Bind(&ReportingContext::SendReports,
                                        WrapWeakPersistent(this)));
  }
}

void ReportingContext::SendReports() {
  // The reports queued to be sent to callbacks are copied (and cleared) before
  // being sent to observers, since additional reports may be queued as a result
  // of the callbacks.
  auto reports_to_send = reports_;
  reports_.clear();
  for (auto observer : observers_)
    observer->ReportToCallback(reports_to_send);
}

void ReportingContext::RegisterObserver(ReportingObserver* observer) {
  observers_.insert(observer);
}

void ReportingContext::UnregisterObserver(ReportingObserver* observer) {
  observers_.erase(observer);
}

bool ReportingContext::ObserverExists() {
  return observers_.size();
}

void ReportingContext::Trace(blink::Visitor* visitor) {
  visitor->Trace(observers_);
  visitor->Trace(reports_);
  visitor->Trace(execution_context_);
  Supplement<ExecutionContext>::Trace(visitor);
}

}  // namespace blink
