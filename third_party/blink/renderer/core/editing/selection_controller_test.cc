// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/selection_controller.h"

#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/editing/visible_selection.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/input/event_handler.h"

namespace blink {

class SelectionControllerTest : public EditingTestBase {
 protected:
  SelectionControllerTest() = default;

  VisibleSelection VisibleSelectionInDOMTree() const {
    return Selection().ComputeVisibleSelectionInDOMTree();
  }

  VisibleSelectionInFlatTree GetVisibleSelectionInFlatTree() const {
    return Selection().GetSelectionInFlatTree();
  }

  void SetCaretAtHitTestResult(const HitTestResult&);
  void SetNonDirectionalSelectionIfNeeded(const SelectionInFlatTree&,
                                          TextGranularity);

 private:
  DISALLOW_COPY_AND_ASSIGN(SelectionControllerTest);
};

void SelectionControllerTest::SetCaretAtHitTestResult(
    const HitTestResult& hit_test_result) {
  GetFrame().GetEventHandler().GetSelectionController().SetCaretAtHitTestResult(
      hit_test_result);
}

void SelectionControllerTest::SetNonDirectionalSelectionIfNeeded(
    const SelectionInFlatTree& new_selection,
    TextGranularity granularity) {
  GetFrame()
      .GetEventHandler()
      .GetSelectionController()
      .SetNonDirectionalSelectionIfNeeded(
          new_selection,
          SetSelectionOptions::Builder().SetGranularity(granularity).Build(),
          SelectionController::kDoNotAdjustEndpoints);
}

TEST_F(SelectionControllerTest, setNonDirectionalSelectionIfNeeded) {
  const char* body_content = "<span id=top>top</span><span id=host></span>";
  const char* shadow_content = "<span id=bottom>bottom</span>";
  SetBodyContent(body_content);
  ShadowRoot* shadow_root = SetShadowContent(shadow_content, "host");

  Node* top = GetDocument().getElementById("top")->firstChild();
  Node* bottom = shadow_root->getElementById("bottom")->firstChild();

  // top to bottom
  SetNonDirectionalSelectionIfNeeded(SelectionInFlatTree::Builder()
                                         .Collapse(PositionInFlatTree(top, 1))
                                         .Extend(PositionInFlatTree(bottom, 3))
                                         .Build(),
                                     TextGranularity::kCharacter);
  EXPECT_EQ(VisibleSelectionInDOMTree().Start(),
            VisibleSelectionInDOMTree().Base());
  EXPECT_EQ(VisibleSelectionInDOMTree().End(),
            VisibleSelectionInDOMTree().Extent());
  EXPECT_EQ(Position(top, 1), VisibleSelectionInDOMTree().Start());
  EXPECT_EQ(Position(top, 3), VisibleSelectionInDOMTree().End());

  EXPECT_EQ(PositionInFlatTree(top, 1), GetVisibleSelectionInFlatTree().Base());
  EXPECT_EQ(PositionInFlatTree(bottom, 3),
            GetVisibleSelectionInFlatTree().Extent());
  EXPECT_EQ(PositionInFlatTree(top, 1),
            GetVisibleSelectionInFlatTree().Start());
  EXPECT_EQ(PositionInFlatTree(bottom, 3),
            GetVisibleSelectionInFlatTree().End());

  // bottom to top
  SetNonDirectionalSelectionIfNeeded(
      SelectionInFlatTree::Builder()
          .Collapse(PositionInFlatTree(bottom, 3))
          .Extend(PositionInFlatTree(top, 1))
          .Build(),
      TextGranularity::kCharacter);
  EXPECT_EQ(VisibleSelectionInDOMTree().End(),
            VisibleSelectionInDOMTree().Base());
  EXPECT_EQ(VisibleSelectionInDOMTree().Start(),
            VisibleSelectionInDOMTree().Extent());
  EXPECT_EQ(Position(bottom, 0), VisibleSelectionInDOMTree().Start());
  EXPECT_EQ(Position(bottom, 3), VisibleSelectionInDOMTree().End());

  EXPECT_EQ(PositionInFlatTree(bottom, 3),
            GetVisibleSelectionInFlatTree().Base());
  EXPECT_EQ(PositionInFlatTree(top, 1),
            GetVisibleSelectionInFlatTree().Extent());
  EXPECT_EQ(PositionInFlatTree(top, 1),
            GetVisibleSelectionInFlatTree().Start());
  EXPECT_EQ(PositionInFlatTree(bottom, 3),
            GetVisibleSelectionInFlatTree().End());
}

TEST_F(SelectionControllerTest, setCaretAtHitTestResult) {
  const char* body_content = "<div id='sample' contenteditable>sample</div>";
  SetBodyContent(body_content);
  GetDocument().GetSettings()->SetScriptEnabled(true);
  Element* script = GetDocument().CreateRawElement(HTMLNames::scriptTag);
  script->SetInnerHTMLFromString(
      "var sample = document.getElementById('sample');"
      "sample.addEventListener('onselectstart', "
      "  event => elem.parentNode.removeChild(elem));");
  GetDocument().body()->AppendChild(script);
  GetDocument().View()->UpdateAllLifecyclePhases();
  GetFrame().GetEventHandler().GetSelectionController().HandleGestureLongPress(
      GetFrame().GetEventHandler().HitTestResultAtPoint(IntPoint(8, 8)));
}

// For http://crbug.com/704827
TEST_F(SelectionControllerTest, setCaretAtHitTestResultWithNullPosition) {
  SetBodyContent(
      "<style>"
      "#sample:before {content: '&nbsp;'}"
      "#sample { user-select: none; }"
      "</style>"
      "<div id=sample></div>");
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Hit "&nbsp;" in before pseudo element of "sample".
  SetCaretAtHitTestResult(
      GetFrame().GetEventHandler().HitTestResultAtPoint(IntPoint(10, 10)));

  EXPECT_TRUE(Selection().GetSelectionInDOMTree().IsNone());
}

// For http://crbug.com/759971
TEST_F(SelectionControllerTest,
       SetCaretAtHitTestResultWithDisconnectedPosition) {
  GetDocument().GetSettings()->SetScriptEnabled(true);
  Element* script = GetDocument().CreateRawElement(HTMLNames::scriptTag);
  script->SetInnerHTMLFromString(
      "document.designMode = 'on';"
      "const selection = window.getSelection();"
      "const html = document.getElementsByTagName('html')[0];"
      "selection.collapse(html);"
      "const range = selection.getRangeAt(0);"

      "function selectstart() {"
      "  const body = document.getElementsByTagName('body')[0];"
      "  range.surroundContents(body);"
      "  range.deleteContents();"
      "}"
      "document.addEventListener('selectstart', selectstart);");
  GetDocument().body()->AppendChild(script);
  GetDocument().View()->UpdateAllLifecyclePhases();

  // Simulate a tap somewhere in the document
  blink::WebMouseEvent mouse_event(
      blink::WebInputEvent::kMouseDown,
      blink::WebInputEvent::kIsCompatibilityEventForTouch,
      blink::WebInputEvent::GetStaticTimeStampForTests());
  // Frame scale defaults to 0, which would cause a divide-by-zero problem.
  mouse_event.SetFrameScale(1);
  GetFrame().GetEventHandler().GetSelectionController().HandleMousePressEvent(
      MouseEventWithHitTestResults(
          mouse_event,
          GetFrame().GetEventHandler().HitTestResultAtPoint(IntPoint(0, 0))));

  // The original bug was that this test would cause
  // TextSuggestionController::HandlePotentialMisspelledWordTap() to crash. So
  // the primary thing this test cases tests is that we can get here without
  // crashing.

  // Verify no selection was set.
  EXPECT_TRUE(Selection().GetSelectionInDOMTree().IsNone());
}

// For http://crbug.com/700368
TEST_F(SelectionControllerTest, AdjustSelectionWithTrailingWhitespace) {
  SetBodyContent(
      "<input type=checkbox>"
      "<div style='user-select:none'>abc</div>");
  Element* const input = GetDocument().QuerySelector("input");

  const VisibleSelectionInFlatTree& selection =
      CreateVisibleSelectionWithGranularity(
          SelectionInFlatTree::Builder()
              .Collapse(PositionInFlatTree::BeforeNode(*input))
              .Extend(PositionInFlatTree::AfterNode(*input))
              .Build(),
          TextGranularity::kWord);
  const SelectionInFlatTree& result =
      AdjustSelectionWithTrailingWhitespace(selection.AsSelection());

  EXPECT_EQ(PositionInFlatTree::BeforeNode(*input),
            result.ComputeStartPosition());
  EXPECT_EQ(PositionInFlatTree::AfterNode(*input), result.ComputeEndPosition());
}

}  // namespace blink
