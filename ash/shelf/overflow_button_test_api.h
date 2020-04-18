// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_OVERFLOW_BUTTON_TEST_API_H_
#define ASH_SHELF_OVERFLOW_BUTTON_TEST_API_H_

#include <iosfwd>

#include "ash/shelf/overflow_button.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash {

// Use the api in this class to test OverflowButton.
class OverflowButtonTestApi {
 public:
  using ChevronDirection = OverflowButton::ChevronDirection;

  explicit OverflowButtonTestApi(OverflowButton* overflow_button);

  testing::AssertionResult ChevronDirectionMatches(ChevronDirection expected);

 private:
  OverflowButton* overflow_button_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(OverflowButtonTestApi);
};

}  // namespace ash

#endif  // ASH_SHELF_OVERFLOW_BUTTON_TEST_API_H_
