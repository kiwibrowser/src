// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/geometry/insets.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/insets_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"

TEST(InsetsTest, InsetsDefault) {
  gfx::Insets insets;
  EXPECT_EQ(0, insets.top());
  EXPECT_EQ(0, insets.left());
  EXPECT_EQ(0, insets.bottom());
  EXPECT_EQ(0, insets.right());
  EXPECT_EQ(0, insets.width());
  EXPECT_EQ(0, insets.height());
  EXPECT_TRUE(insets.IsEmpty());
}

TEST(InsetsTest, Insets) {
  gfx::Insets insets(1, 2, 3, 4);
  EXPECT_EQ(1, insets.top());
  EXPECT_EQ(2, insets.left());
  EXPECT_EQ(3, insets.bottom());
  EXPECT_EQ(4, insets.right());
  EXPECT_EQ(6, insets.width());  // Left + right.
  EXPECT_EQ(4, insets.height());  // Top + bottom.
  EXPECT_FALSE(insets.IsEmpty());
}

TEST(InsetsTest, Set) {
  gfx::Insets insets;
  insets.Set(1, 2, 3, 4);
  EXPECT_EQ(1, insets.top());
  EXPECT_EQ(2, insets.left());
  EXPECT_EQ(3, insets.bottom());
  EXPECT_EQ(4, insets.right());
}

TEST(InsetsTest, Operators) {
  gfx::Insets insets;
  insets.Set(1, 2, 3, 4);
  insets += gfx::Insets(5, 6, 7, 8);
  EXPECT_EQ(6, insets.top());
  EXPECT_EQ(8, insets.left());
  EXPECT_EQ(10, insets.bottom());
  EXPECT_EQ(12, insets.right());

  insets -= gfx::Insets(-1, 0, 1, 2);
  EXPECT_EQ(7, insets.top());
  EXPECT_EQ(8, insets.left());
  EXPECT_EQ(9, insets.bottom());
  EXPECT_EQ(10, insets.right());

  insets = gfx::Insets(10, 10, 10, 10) + gfx::Insets(5, 5, 0, -20);
  EXPECT_EQ(15, insets.top());
  EXPECT_EQ(15, insets.left());
  EXPECT_EQ(10, insets.bottom());
  EXPECT_EQ(-10, insets.right());

  insets = gfx::Insets(10, 10, 10, 10) - gfx::Insets(5, 5, 0, -20);
  EXPECT_EQ(5, insets.top());
  EXPECT_EQ(5, insets.left());
  EXPECT_EQ(10, insets.bottom());
  EXPECT_EQ(30, insets.right());
}

TEST(InsetsFTest, Operators) {
  gfx::InsetsF insets;
  insets.Set(1.f, 2.5f, 3.3f, 4.1f);
  insets += gfx::InsetsF(5.8f, 6.7f, 7.6f, 8.5f);
  EXPECT_FLOAT_EQ(6.8f, insets.top());
  EXPECT_FLOAT_EQ(9.2f, insets.left());
  EXPECT_FLOAT_EQ(10.9f, insets.bottom());
  EXPECT_FLOAT_EQ(12.6f, insets.right());

  insets -= gfx::InsetsF(-1.f, 0, 1.1f, 2.2f);
  EXPECT_FLOAT_EQ(7.8f, insets.top());
  EXPECT_FLOAT_EQ(9.2f, insets.left());
  EXPECT_FLOAT_EQ(9.8f, insets.bottom());
  EXPECT_FLOAT_EQ(10.4f, insets.right());

  insets = gfx::InsetsF(10, 10.1f, 10.01f, 10.001f) +
           gfx::InsetsF(5.5f, 5.f, 0, -20.2f);
  EXPECT_FLOAT_EQ(15.5f, insets.top());
  EXPECT_FLOAT_EQ(15.1f, insets.left());
  EXPECT_FLOAT_EQ(10.01f, insets.bottom());
  EXPECT_FLOAT_EQ(-10.199f, insets.right());

  insets = gfx::InsetsF(10, 10.1f, 10.01f, 10.001f) -
           gfx::InsetsF(5.5f, 5.f, 0, -20.2f);
  EXPECT_FLOAT_EQ(4.5f, insets.top());
  EXPECT_FLOAT_EQ(5.1f, insets.left());
  EXPECT_FLOAT_EQ(10.01f, insets.bottom());
  EXPECT_FLOAT_EQ(30.201f, insets.right());
}

TEST(InsetsTest, Equality) {
  gfx::Insets insets1;
  insets1.Set(1, 2, 3, 4);
  gfx::Insets insets2;
  // Test operator== and operator!=.
  EXPECT_FALSE(insets1 == insets2);
  EXPECT_TRUE(insets1 != insets2);

  insets2.Set(1, 2, 3, 4);
  EXPECT_TRUE(insets1 == insets2);
  EXPECT_FALSE(insets1 != insets2);
}

TEST(InsetsTest, ToString) {
  gfx::Insets insets(1, 2, 3, 4);
  EXPECT_EQ("1,2,3,4", insets.ToString());
}

TEST(InsetsTest, Offset) {
  const gfx::Insets insets(1, 2, 3, 4);
  const gfx::Rect rect(5, 6, 7, 8);
  const gfx::Vector2d vector(9, 10);

  // Whether you inset then offset the rect, offset then inset the rect, or
  // offset the insets then apply to the rect, the outcome should be the same.
  gfx::Rect inset_first = rect;
  inset_first.Inset(insets);
  inset_first.Offset(vector);

  gfx::Rect offset_first = rect;
  offset_first.Offset(vector);
  offset_first.Inset(insets);

  gfx::Rect inset_by_offset = rect;
  inset_by_offset.Inset(insets.Offset(vector));

  EXPECT_EQ(inset_first, offset_first);
  EXPECT_EQ(inset_by_offset, inset_first);
}
