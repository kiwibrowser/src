// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BUDGET_WORKER_NAVIGATOR_BUDGET_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BUDGET_WORKER_NAVIGATOR_BUDGET_H_

#include "third_party/blink/renderer/core/workers/worker_navigator.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"

namespace blink {

class BudgetService;
class ExecutionContext;
class WorkerNavigator;

// This exposes the budget object on the WorkerNavigator partial interface.
class WorkerNavigatorBudget final
    : public GarbageCollected<WorkerNavigatorBudget>,
      public Supplement<WorkerNavigator> {
  USING_GARBAGE_COLLECTED_MIXIN(WorkerNavigatorBudget);
  WTF_MAKE_NONCOPYABLE(WorkerNavigatorBudget);

 public:
  static const char kSupplementName[];

  static WorkerNavigatorBudget& From(WorkerNavigator& worker_navigator);

  static BudgetService* budget(ExecutionContext* context,
                               WorkerNavigator& worker_navigator);
  BudgetService* budget(ExecutionContext* context);

  void Trace(blink::Visitor* visitor) override;

 private:
  explicit WorkerNavigatorBudget(WorkerNavigator& worker_navigator);

  Member<BudgetService> budget_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BUDGET_WORKER_NAVIGATOR_BUDGET_H_
