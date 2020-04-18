// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/visible_position.h"

#include "third_party/blink/renderer/core/css/css_style_declaration.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/editing/visible_units.h"

namespace blink {

class VisiblePositionTest : public EditingTestBase {};

TEST_F(VisiblePositionTest, ShadowV0DistributedNodes) {
  const char* body_content =
      "<p id='host'>00<b id='one'>11</b><b id='two'>22</b>33</p>";
  const char* shadow_content =
      "<a><span id='s4'>44</span><content select=#two></content><span "
      "id='s5'>55</span><content select=#one></content><span "
      "id='s6'>66</span></a>";
  SetBodyContent(body_content);
  ShadowRoot* shadow_root = SetShadowContent(shadow_content, "host");

  Element* body = GetDocument().body();
  Element* one = body->QuerySelector("#one");
  Element* two = body->QuerySelector("#two");
  Element* four = shadow_root->QuerySelector("#s4");
  Element* five = shadow_root->QuerySelector("#s5");

  EXPECT_EQ(Position(one->firstChild(), 0),
            CanonicalPositionOf(Position(one, 0)));
  EXPECT_EQ(Position(one->firstChild(), 0),
            CreateVisiblePosition(Position(one, 0)).DeepEquivalent());
  EXPECT_EQ(Position(one->firstChild(), 2),
            CanonicalPositionOf(Position(two, 0)));
  EXPECT_EQ(Position(one->firstChild(), 2),
            CreateVisiblePosition(Position(two, 0)).DeepEquivalent());

  EXPECT_EQ(PositionInFlatTree(five->firstChild(), 2),
            CanonicalPositionOf(PositionInFlatTree(one, 0)));
  EXPECT_EQ(PositionInFlatTree(five->firstChild(), 2),
            CreateVisiblePosition(PositionInFlatTree(one, 0)).DeepEquivalent());
  EXPECT_EQ(PositionInFlatTree(four->firstChild(), 2),
            CanonicalPositionOf(PositionInFlatTree(two, 0)));
  EXPECT_EQ(PositionInFlatTree(four->firstChild(), 2),
            CreateVisiblePosition(PositionInFlatTree(two, 0)).DeepEquivalent());
}

#if DCHECK_IS_ON()

TEST_F(VisiblePositionTest, NullIsValid) {
  EXPECT_TRUE(VisiblePosition().IsValid());
}

TEST_F(VisiblePositionTest, NonNullIsValidBeforeMutation) {
  SetBodyContent("<p>one</p>");

  Element* paragraph = GetDocument().QuerySelector("p");
  Position position(paragraph->firstChild(), 1);
  EXPECT_TRUE(CreateVisiblePosition(position).IsValid());
}

TEST_F(VisiblePositionTest, NonNullInvalidatedAfterDOMChange) {
  SetBodyContent("<p>one</p>");

  Element* paragraph = GetDocument().QuerySelector("p");
  Position position(paragraph->firstChild(), 1);
  VisiblePosition null_visible_position;
  VisiblePosition non_null_visible_position = CreateVisiblePosition(position);

  Element* div = GetDocument().CreateRawElement(HTMLNames::divTag);
  GetDocument().body()->AppendChild(div);

  EXPECT_TRUE(null_visible_position.IsValid());
  EXPECT_FALSE(non_null_visible_position.IsValid());

  UpdateAllLifecyclePhases();

  // Invalid VisiblePosition can never become valid again.
  EXPECT_FALSE(non_null_visible_position.IsValid());
}

TEST_F(VisiblePositionTest, NonNullInvalidatedAfterStyleChange) {
  SetBodyContent("<div>one</div><p>two</p>");

  Element* paragraph = GetDocument().QuerySelector("p");
  Element* div = GetDocument().QuerySelector("div");
  Position position(paragraph->firstChild(), 1);

  VisiblePosition visible_position1 = CreateVisiblePosition(position);
  div->style()->setProperty(&GetDocument(), "color", "red", "important",
                            ASSERT_NO_EXCEPTION);
  EXPECT_FALSE(visible_position1.IsValid());

  UpdateAllLifecyclePhases();

  VisiblePosition visible_position2 = CreateVisiblePosition(position);
  div->style()->setProperty(&GetDocument(), "display", "none", "important",
                            ASSERT_NO_EXCEPTION);
  EXPECT_FALSE(visible_position2.IsValid());

  UpdateAllLifecyclePhases();

  // Invalid VisiblePosition can never become valid again.
  EXPECT_FALSE(visible_position1.IsValid());
  EXPECT_FALSE(visible_position2.IsValid());
}

#endif

}  // namespace blink
