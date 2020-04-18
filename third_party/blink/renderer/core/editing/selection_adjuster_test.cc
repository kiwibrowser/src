// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/selection_adjuster.h"

#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/html/forms/text_control_element.h"

namespace blink {

class SelectionAdjusterTest : public EditingTestBase {};

TEST_F(SelectionAdjusterTest, AdjustShadowToCollpasedInDOMTree) {
  const SelectionInDOMTree& selection = SetSelectionTextToBody(
      "<span><template data-mode=\"open\">a|bc</template></span>^");
  const SelectionInDOMTree& result =
      SelectionAdjuster::AdjustSelectionToAvoidCrossingShadowBoundaries(
          selection);
  EXPECT_EQ("<span></span>|", GetSelectionTextFromBody(result));
}

TEST_F(SelectionAdjusterTest, AdjustShadowToCollpasedInFlatTree) {
  SetBodyContent("<input value=abc>");
  const auto& input = ToTextControl(*GetDocument().QuerySelector("input"));
  const SelectionInFlatTree& selection =
      SelectionInFlatTree::Builder()
          .Collapse(PositionInFlatTree::AfterNode(input))
          .Extend(
              PositionInFlatTree(*input.InnerEditorElement()->firstChild(), 1))
          .Build();
  const SelectionInFlatTree& result =
      SelectionAdjuster::AdjustSelectionToAvoidCrossingShadowBoundaries(
          selection);
  EXPECT_EQ("<input value=\"abc\"><div>abc</div></input>|",
            GetSelectionTextInFlatTreeFromBody(result));
}

}  // namespace blink
