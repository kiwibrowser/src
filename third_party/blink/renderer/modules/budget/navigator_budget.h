// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BUDGET_NAVIGATOR_BUDGET_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BUDGET_NAVIGATOR_BUDGET_H_

#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/platform/supplementable.h"

namespace blink {

class BudgetService;
class Navigator;

// This exposes the budget object on the Navigator partial interface.
class NavigatorBudget final : public GarbageCollected<NavigatorBudget>,
                              public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorBudget);
  WTF_MAKE_NONCOPYABLE(NavigatorBudget);

 public:
  static const char kSupplementName[];

  static NavigatorBudget& From(Navigator& navigator);

  static BudgetService* budget(ExecutionContext* context, Navigator& navigator);
  BudgetService* budget(ExecutionContext* context);

  void Trace(blink::Visitor* visitor) override;

 private:
  explicit NavigatorBudget(Navigator& navigator);

  Member<BudgetService> budget_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BUDGET_NAVIGATOR_BUDGET_H_
