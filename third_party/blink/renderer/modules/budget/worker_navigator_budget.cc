// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/budget/worker_navigator_budget.h"

#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_navigator.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/modules/budget/budget_service.h"

namespace blink {

WorkerNavigatorBudget::WorkerNavigatorBudget(WorkerNavigator& worker_navigator)
    : Supplement<WorkerNavigator>(worker_navigator) {}

// static
const char WorkerNavigatorBudget::kSupplementName[] = "WorkerNavigatorBudget";

// static
WorkerNavigatorBudget& WorkerNavigatorBudget::From(
    WorkerNavigator& worker_navigator) {
  // Get the unique WorkerNavigatorBudget associated with this workerNavigator.
  WorkerNavigatorBudget* worker_navigator_budget =
      Supplement<WorkerNavigator>::From<WorkerNavigatorBudget>(
          worker_navigator);
  if (!worker_navigator_budget) {
    // If there isn't one already, create it now and associate it.
    worker_navigator_budget = new WorkerNavigatorBudget(worker_navigator);
    ProvideTo(worker_navigator, worker_navigator_budget);
  }
  return *worker_navigator_budget;
}

BudgetService* WorkerNavigatorBudget::budget(ExecutionContext* context) {
  if (!budget_) {
    budget_ = BudgetService::Create(context->GetInterfaceProvider());
  }
  return budget_.Get();
}

// static
BudgetService* WorkerNavigatorBudget::budget(
    ExecutionContext* context,
    WorkerNavigator& worker_navigator) {
  return WorkerNavigatorBudget::From(worker_navigator).budget(context);
}

void WorkerNavigatorBudget::Trace(blink::Visitor* visitor) {
  visitor->Trace(budget_);
  Supplement<WorkerNavigator>::Trace(visitor);
}

}  // namespace blink
