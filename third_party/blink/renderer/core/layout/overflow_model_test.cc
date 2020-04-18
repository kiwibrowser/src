/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/layout/overflow_model.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"

namespace blink {
namespace {

LayoutRect InitialLayoutOverflow() {
  return LayoutRect(10, 10, 80, 80);
}

LayoutRect InitialVisualOverflow() {
  return LayoutRect(0, 0, 100, 100);
}

class SimpleOverflowModelTest : public testing::Test {
 protected:
  SimpleOverflowModelTest()
      : overflow_(InitialLayoutOverflow(), InitialVisualOverflow()) {}
  SimpleOverflowModel overflow_;
};

TEST_F(SimpleOverflowModelTest, InitialOverflowRects) {
  EXPECT_EQ(InitialLayoutOverflow(), overflow_.LayoutOverflowRect());
  EXPECT_EQ(InitialVisualOverflow(), overflow_.VisualOverflowRect());
}

TEST_F(SimpleOverflowModelTest, AddLayoutOverflowOutsideExpandsRect) {
  overflow_.AddLayoutOverflow(LayoutRect(0, 10, 30, 10));
  EXPECT_EQ(LayoutRect(0, 10, 90, 80), overflow_.LayoutOverflowRect());
}

TEST_F(SimpleOverflowModelTest, AddLayoutOverflowInsideDoesNotAffectRect) {
  overflow_.AddLayoutOverflow(LayoutRect(50, 50, 10, 20));
  EXPECT_EQ(InitialLayoutOverflow(), overflow_.LayoutOverflowRect());
}

TEST_F(SimpleOverflowModelTest, AddLayoutOverflowEmpty) {
  // This test documents the existing behavior so that we are aware when/if
  // it changes. It would also be reasonable for addLayoutOverflow to be
  // a no-op in this situation.
  overflow_.AddLayoutOverflow(LayoutRect(200, 200, 0, 0));
  EXPECT_EQ(LayoutRect(10, 10, 190, 190), overflow_.LayoutOverflowRect());
}

TEST_F(SimpleOverflowModelTest, AddLayoutOverflowDoesNotAffectVisualOverflow) {
  overflow_.AddLayoutOverflow(LayoutRect(300, 300, 300, 300));
  EXPECT_EQ(InitialVisualOverflow(), overflow_.VisualOverflowRect());
}

TEST_F(SimpleOverflowModelTest, AddVisualOverflowOutsideExpandsRect) {
  overflow_.AddVisualOverflow(LayoutRect(150, -50, 10, 10));
  EXPECT_EQ(LayoutRect(0, -50, 160, 150), overflow_.VisualOverflowRect());
}

TEST_F(SimpleOverflowModelTest, AddVisualOverflowInsideDoesNotAffectRect) {
  overflow_.AddVisualOverflow(LayoutRect(0, 10, 90, 90));
  EXPECT_EQ(InitialVisualOverflow(), overflow_.VisualOverflowRect());
}

TEST_F(SimpleOverflowModelTest, AddVisualOverflowEmpty) {
  overflow_.SetVisualOverflow(LayoutRect(0, 0, 600, 0));
  overflow_.AddVisualOverflow(LayoutRect(100, -50, 100, 100));
  overflow_.AddVisualOverflow(LayoutRect(300, 300, 0, 10000));
  EXPECT_EQ(LayoutRect(100, -50, 100, 100), overflow_.VisualOverflowRect());
}

TEST_F(SimpleOverflowModelTest, AddVisualOverflowDoesNotAffectLayoutOverflow) {
  overflow_.AddVisualOverflow(LayoutRect(300, 300, 300, 300));
  EXPECT_EQ(InitialLayoutOverflow(), overflow_.LayoutOverflowRect());
}

TEST_F(SimpleOverflowModelTest, MoveAffectsLayoutOverflow) {
  overflow_.Move(LayoutUnit(500), LayoutUnit(100));
  EXPECT_EQ(LayoutRect(510, 110, 80, 80), overflow_.LayoutOverflowRect());
}

TEST_F(SimpleOverflowModelTest, MoveAffectsVisualOverflow) {
  overflow_.Move(LayoutUnit(500), LayoutUnit(100));
  EXPECT_EQ(LayoutRect(500, 100, 100, 100), overflow_.VisualOverflowRect());
}

class BoxOverflowModelTest : public testing::Test {
 protected:
  BoxOverflowModelTest()
      : overflow_(InitialLayoutOverflow(), InitialVisualOverflow()) {}
  BoxOverflowModel overflow_;
};

TEST_F(BoxOverflowModelTest, InitialOverflowRects) {
  EXPECT_EQ(InitialLayoutOverflow(), overflow_.LayoutOverflowRect());
  EXPECT_EQ(InitialVisualOverflow(), overflow_.SelfVisualOverflowRect());
  EXPECT_TRUE(overflow_.ContentsVisualOverflowRect().IsEmpty());
}

TEST_F(BoxOverflowModelTest, AddLayoutOverflowOutsideExpandsRect) {
  overflow_.AddLayoutOverflow(LayoutRect(0, 10, 30, 10));
  EXPECT_EQ(LayoutRect(0, 10, 90, 80), overflow_.LayoutOverflowRect());
}

TEST_F(BoxOverflowModelTest, AddLayoutOverflowInsideDoesNotAffectRect) {
  overflow_.AddLayoutOverflow(LayoutRect(50, 50, 10, 20));
  EXPECT_EQ(InitialLayoutOverflow(), overflow_.LayoutOverflowRect());
}

TEST_F(BoxOverflowModelTest, AddLayoutOverflowEmpty) {
  // This test documents the existing behavior so that we are aware when/if
  // it changes. It would also be reasonable for addLayoutOverflow to be
  // a no-op in this situation.
  overflow_.AddLayoutOverflow(LayoutRect(200, 200, 0, 0));
  EXPECT_EQ(LayoutRect(10, 10, 190, 190), overflow_.LayoutOverflowRect());
}

TEST_F(BoxOverflowModelTest, AddLayoutOverflowDoesNotAffectSelfVisualOverflow) {
  overflow_.AddLayoutOverflow(LayoutRect(300, 300, 300, 300));
  EXPECT_EQ(InitialVisualOverflow(), overflow_.SelfVisualOverflowRect());
}

TEST_F(BoxOverflowModelTest,
       AddLayoutOverflowDoesNotAffectContentsVisualOverflow) {
  overflow_.AddLayoutOverflow(LayoutRect(300, 300, 300, 300));
  EXPECT_TRUE(overflow_.ContentsVisualOverflowRect().IsEmpty());
}

TEST_F(BoxOverflowModelTest, AddSelfVisualOverflowOutsideExpandsRect) {
  overflow_.AddSelfVisualOverflow(LayoutRect(150, -50, 10, 10));
  EXPECT_EQ(LayoutRect(0, -50, 160, 150), overflow_.SelfVisualOverflowRect());
}

TEST_F(BoxOverflowModelTest, AddSelfVisualOverflowInsideDoesNotAffectRect) {
  overflow_.AddSelfVisualOverflow(LayoutRect(0, 10, 90, 90));
  EXPECT_EQ(InitialVisualOverflow(), overflow_.SelfVisualOverflowRect());
}

TEST_F(BoxOverflowModelTest, AddSelfVisualOverflowEmpty) {
  BoxOverflowModel overflow(LayoutRect(), LayoutRect(0, 0, 600, 0));
  overflow.AddSelfVisualOverflow(LayoutRect(100, -50, 100, 100));
  overflow.AddSelfVisualOverflow(LayoutRect(300, 300, 0, 10000));
  EXPECT_EQ(LayoutRect(100, -50, 100, 100), overflow.SelfVisualOverflowRect());
}

TEST_F(BoxOverflowModelTest, AddSelfVisualOverflowDoesNotAffectLayoutOverflow) {
  overflow_.AddSelfVisualOverflow(LayoutRect(300, 300, 300, 300));
  EXPECT_EQ(InitialLayoutOverflow(), overflow_.LayoutOverflowRect());
}

TEST_F(BoxOverflowModelTest,
       AddSelfVisualOverflowDoesNotAffectContentsVisualOverflow) {
  overflow_.AddSelfVisualOverflow(LayoutRect(300, 300, 300, 300));
  EXPECT_TRUE(overflow_.ContentsVisualOverflowRect().IsEmpty());
}

TEST_F(BoxOverflowModelTest, AddContentsVisualOverflowFirstCall) {
  overflow_.AddContentsVisualOverflow(LayoutRect(0, 0, 10, 10));
  EXPECT_EQ(LayoutRect(0, 0, 10, 10), overflow_.ContentsVisualOverflowRect());
}

TEST_F(BoxOverflowModelTest, AddContentsVisualOverflowUnitesRects) {
  overflow_.AddContentsVisualOverflow(LayoutRect(0, 0, 10, 10));
  overflow_.AddContentsVisualOverflow(LayoutRect(80, 80, 10, 10));
  EXPECT_EQ(LayoutRect(0, 0, 90, 90), overflow_.ContentsVisualOverflowRect());
}

TEST_F(BoxOverflowModelTest, AddContentsVisualOverflowRectWithinRect) {
  overflow_.AddContentsVisualOverflow(LayoutRect(0, 0, 10, 10));
  overflow_.AddContentsVisualOverflow(LayoutRect(2, 2, 5, 5));
  EXPECT_EQ(LayoutRect(0, 0, 10, 10), overflow_.ContentsVisualOverflowRect());
}

TEST_F(BoxOverflowModelTest, AddContentsVisualOverflowEmpty) {
  overflow_.AddContentsVisualOverflow(LayoutRect(0, 0, 10, 10));
  overflow_.AddContentsVisualOverflow(LayoutRect(20, 20, 0, 0));
  EXPECT_EQ(LayoutRect(0, 0, 10, 10), overflow_.ContentsVisualOverflowRect());
}

TEST_F(BoxOverflowModelTest, MoveAffectsLayoutOverflow) {
  overflow_.Move(LayoutUnit(500), LayoutUnit(100));
  EXPECT_EQ(LayoutRect(510, 110, 80, 80), overflow_.LayoutOverflowRect());
}

TEST_F(BoxOverflowModelTest, MoveAffectsSelfVisualOverflow) {
  overflow_.Move(LayoutUnit(500), LayoutUnit(100));
  EXPECT_EQ(LayoutRect(500, 100, 100, 100), overflow_.SelfVisualOverflowRect());
}

TEST_F(BoxOverflowModelTest, MoveAffectsContentsVisualOverflow) {
  overflow_.AddContentsVisualOverflow(LayoutRect(0, 0, 10, 10));
  overflow_.Move(LayoutUnit(500), LayoutUnit(100));
  EXPECT_EQ(LayoutRect(500, 100, 10, 10),
            overflow_.ContentsVisualOverflowRect());
}

}  // anonymous namespace
}  // namespace blink
