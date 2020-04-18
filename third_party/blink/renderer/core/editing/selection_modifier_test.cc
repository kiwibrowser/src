// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/selection_modifier.h"

#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"

namespace blink {

class SelectionModifierTest : public EditingTestBase {};

TEST_F(SelectionModifierTest, ExtendForwardByWordNone) {
  SetBodyContent("abc");
  SelectionModifier modifier(GetFrame(), SelectionInDOMTree());
  modifier.Modify(SelectionModifyAlteration::kExtend,
                  SelectionModifyDirection::kForward, TextGranularity::kWord);
  // We should not crash here. See http://crbug.com/832061
  EXPECT_EQ(SelectionInDOMTree(), modifier.Selection().AsSelection());
}

TEST_F(SelectionModifierTest, leftPositionOf) {
  const char* body_content =
      "<b id=zero>0</b><p id=host><b id=one>1</b><b id=two>22</b></p><b "
      "id=three>333</b>";
  const char* shadow_content =
      "<b id=four>4444</b><content select=#two></content><content "
      "select=#one></content><b id=five>55555</b>";
  SetBodyContent(body_content);
  ShadowRoot* shadow_root = SetShadowContent(shadow_content, "host");

  Element* one = GetDocument().getElementById("one");
  Element* two = GetDocument().getElementById("two");
  Element* three = GetDocument().getElementById("three");
  Element* four = shadow_root->getElementById("four");
  Element* five = shadow_root->getElementById("five");

  EXPECT_EQ(
      Position(two->firstChild(), 1),
      LeftPositionOf(CreateVisiblePosition(Position(one, 0))).DeepEquivalent());
  EXPECT_EQ(PositionInFlatTree(two->firstChild(), 1),
            LeftPositionOf(CreateVisiblePosition(PositionInFlatTree(one, 0)))
                .DeepEquivalent());

  EXPECT_EQ(
      Position(one->firstChild(), 0),
      LeftPositionOf(CreateVisiblePosition(Position(two, 0))).DeepEquivalent());
  EXPECT_EQ(PositionInFlatTree(four->firstChild(), 3),
            LeftPositionOf(CreateVisiblePosition(PositionInFlatTree(two, 0)))
                .DeepEquivalent());

  EXPECT_EQ(Position(two->firstChild(), 2),
            LeftPositionOf(CreateVisiblePosition(Position(three, 0)))
                .DeepEquivalent());
  EXPECT_EQ(PositionInFlatTree(five->firstChild(), 5),
            LeftPositionOf(CreateVisiblePosition(PositionInFlatTree(three, 0)))
                .DeepEquivalent());
}

TEST_F(SelectionModifierTest, MoveForwardByWordNone) {
  SetBodyContent("abc");
  SelectionModifier modifier(GetFrame(), SelectionInDOMTree());
  modifier.Modify(SelectionModifyAlteration::kMove,
                  SelectionModifyDirection::kForward, TextGranularity::kWord);
  // We should not crash here. See http://crbug.com/832061
  EXPECT_EQ(SelectionInDOMTree(), modifier.Selection().AsSelection());
}

TEST_F(SelectionModifierTest, rightPositionOf) {
  const char* body_content =
      "<b id=zero>0</b><p id=host><b id=one>1</b><b id=two>22</b></p><b "
      "id=three>333</b>";
  const char* shadow_content =
      "<p id=four>4444</p><content select=#two></content><content "
      "select=#one></content><p id=five>55555</p>";
  SetBodyContent(body_content);
  ShadowRoot* shadow_root = SetShadowContent(shadow_content, "host");

  Node* one = GetDocument().getElementById("one")->firstChild();
  Node* two = GetDocument().getElementById("two")->firstChild();
  Node* three = GetDocument().getElementById("three")->firstChild();
  Node* four = shadow_root->getElementById("four")->firstChild();
  Node* five = shadow_root->getElementById("five")->firstChild();

  EXPECT_EQ(Position(), RightPositionOf(CreateVisiblePosition(Position(one, 1)))
                            .DeepEquivalent());
  EXPECT_EQ(PositionInFlatTree(five, 0),
            RightPositionOf(CreateVisiblePosition(PositionInFlatTree(one, 1)))
                .DeepEquivalent());

  EXPECT_EQ(Position(one, 1),
            RightPositionOf(CreateVisiblePosition(Position(two, 2)))
                .DeepEquivalent());
  EXPECT_EQ(PositionInFlatTree(one, 1),
            RightPositionOf(CreateVisiblePosition(PositionInFlatTree(two, 2)))
                .DeepEquivalent());

  EXPECT_EQ(Position(five, 0),
            RightPositionOf(CreateVisiblePosition(Position(four, 4)))
                .DeepEquivalent());
  EXPECT_EQ(PositionInFlatTree(two, 0),
            RightPositionOf(CreateVisiblePosition(PositionInFlatTree(four, 4)))
                .DeepEquivalent());

  EXPECT_EQ(Position(),
            RightPositionOf(CreateVisiblePosition(Position(five, 5)))
                .DeepEquivalent());
  EXPECT_EQ(PositionInFlatTree(three, 0),
            RightPositionOf(CreateVisiblePosition(PositionInFlatTree(five, 5)))
                .DeepEquivalent());
}

}  // namespace blink
