// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/selection_modifier.h"

#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"

namespace blink {

class SelectionModifierCharacterTest : public EditingTestBase {};

// Regression test for crbug.com/834850
TEST_F(SelectionModifierCharacterTest, MoveLeftTowardsListMarkerNoCrash) {
  const SelectionInDOMTree selection =
      SetSelectionTextToBody("<ol contenteditable><li>|<br></li></ol>");
  SelectionModifier modifier(GetFrame(), selection);
  modifier.Modify(SelectionModifyAlteration::kMove,
                  SelectionModifyDirection::kLeft, TextGranularity::kCharacter);
  // Shouldn't crash here.
  EXPECT_EQ("<ol contenteditable><li>|<br></li></ol>",
            GetSelectionTextFromBody(modifier.Selection().AsSelection()));
}

}  // namespace blink
