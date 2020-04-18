// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_BUDGET_BUDGET_STATE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_BUDGET_BUDGET_STATE_H_

#include "third_party/blink/renderer/core/dom/dom_time_stamp.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace blink {

class BudgetService;

// This exposes the BudgetState interface which is returned from BudgetService
// when there is a GetBudget call.
class BudgetState final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  BudgetState();
  BudgetState(double budget_at, DOMTimeStamp);
  BudgetState(const BudgetState& other);

  double budgetAt() const { return budget_at_; }
  DOMTimeStamp time() const { return time_; }

  void SetBudgetAt(const double budget_at) { budget_at_ = budget_at; }
  void SetTime(const DOMTimeStamp& time) { time_ = time; }

 private:
  double budget_at_;
  DOMTimeStamp time_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_BUDGET_BUDGET_STATE_H_
