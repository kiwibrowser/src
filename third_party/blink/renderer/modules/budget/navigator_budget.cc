// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/budget/navigator_budget.h"

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/modules/budget/budget_service.h"

namespace blink {

NavigatorBudget::NavigatorBudget(Navigator& navigator)
    : Supplement<Navigator>(navigator) {}

// static
const char NavigatorBudget::kSupplementName[] = "NavigatorBudget";

// static
NavigatorBudget& NavigatorBudget::From(Navigator& navigator) {
  // Get the unique NavigatorBudget associated with this navigator.
  NavigatorBudget* navigator_budget =
      Supplement<Navigator>::From<NavigatorBudget>(navigator);
  if (!navigator_budget) {
    // If there isn't one already, create it now and associate it.
    navigator_budget = new NavigatorBudget(navigator);
    ProvideTo(navigator, navigator_budget);
  }
  return *navigator_budget;
}

BudgetService* NavigatorBudget::budget(ExecutionContext* context) {
  if (!budget_) {
    if (auto* interface_provider = context->GetInterfaceProvider()) {
      budget_ = BudgetService::Create(interface_provider);
    }
  }
  return budget_.Get();
}

// static
BudgetService* NavigatorBudget::budget(ExecutionContext* context,
                                       Navigator& navigator) {
  return NavigatorBudget::From(navigator).budget(context);
}

void NavigatorBudget::Trace(blink::Visitor* visitor) {
  visitor->Trace(budget_);
  Supplement<Navigator>::Trace(visitor);
}

}  // namespace blink
