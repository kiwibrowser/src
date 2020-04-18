// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shelf/overflow_button_test_api.h"

namespace ash {
namespace {

std::string ToString(OverflowButtonTestApi::ChevronDirection direction) {
  switch (direction) {
    case OverflowButtonTestApi::ChevronDirection::UP:
      return "UP";
    case OverflowButtonTestApi::ChevronDirection::DOWN:
      return "DOWN";
    case OverflowButtonTestApi::ChevronDirection::LEFT:
      return "LEFT";
    case OverflowButtonTestApi::ChevronDirection::RIGHT:
      return "RIGHT";
  }
}

}  // namespace

OverflowButtonTestApi::OverflowButtonTestApi(OverflowButton* overflow_button)
    : overflow_button_(overflow_button) {}

testing::AssertionResult OverflowButtonTestApi::ChevronDirectionMatches(
    ChevronDirection expected) {
  ChevronDirection actual = overflow_button_->GetChevronDirection();
  testing::AssertionResult result =
      actual == expected ? testing::AssertionSuccess() << "Expected == Actual"
                         : testing::AssertionFailure() << "Expected != Actual";
  result << ": " << ToString(expected) << " vs " << ToString(actual);
  return result;
}

}  // namespace ash
