// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/test/test_utils.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/transform.h"

namespace ui {

//TODO(avallee): Make this into a predicate and add some matrix pretty printing.
void CheckApproximatelyEqual(const gfx::Transform& lhs,
                             const gfx::Transform& rhs) {
  unsigned int errors = 0;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      EXPECT_FLOAT_EQ(lhs.matrix().get(i, j), rhs.matrix().get(i, j))
        << "(i, j) = (" << i << ", " << j << "), error count: " << ++errors;
    }
  }

  if (errors) {
    ADD_FAILURE() << "Expected matrix:\n"
                  << lhs.ToString() << "\n"
                  << "Actual matrix:\n"
                  << rhs.ToString();
  }
}

void CheckApproximatelyEqual(const gfx::Rect& lhs, const gfx::Rect& rhs) {
  EXPECT_FLOAT_EQ(lhs.x(), rhs.x());
  EXPECT_FLOAT_EQ(lhs.y(), rhs.y());
  EXPECT_FLOAT_EQ(lhs.width(), rhs.width());
  EXPECT_FLOAT_EQ(lhs.height(), rhs.height());
}

}  // namespace ui
