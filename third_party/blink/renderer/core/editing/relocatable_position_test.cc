// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/relocatable_position.h"

#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/editing/visible_position.h"

namespace blink {

class RelocatablePositionTest : public EditingTestBase {};

TEST_F(RelocatablePositionTest, position) {
  SetBodyContent("<b>foo</b><textarea>bar</textarea>");
  Node* boldface = GetDocument().QuerySelector("b");
  Node* textarea = GetDocument().QuerySelector("textarea");

  RelocatablePosition relocatable_position(
      Position(textarea, PositionAnchorType::kBeforeAnchor));
  textarea->remove();
  GetDocument().UpdateStyleAndLayout();

  // RelocatablePosition should track the given Position even if its original
  // anchor node is moved away from the document.
  Position expected_position(boldface, PositionAnchorType::kAfterAnchor);
  Position tracked_position = relocatable_position.GetPosition();
  EXPECT_TRUE(tracked_position.AnchorNode()->isConnected());
  EXPECT_EQ(CreateVisiblePosition(expected_position).DeepEquivalent(),
            CreateVisiblePosition(tracked_position).DeepEquivalent());
}

}  // namespace blink
