// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/budget/budget_state.h"

namespace blink {

BudgetState::BudgetState() : budget_at_(0), time_(DOMTimeStamp()) {}

BudgetState::BudgetState(double budget_at, DOMTimeStamp time)
    : budget_at_(budget_at), time_(time) {}

BudgetState::BudgetState(const BudgetState& other)
    : budget_at_(other.budget_at_), time_(other.time_) {}

}  // namespace blink
