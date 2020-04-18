// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/editing/rendered_position.h"

#include "build/build_config.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/position_with_affinity.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/html/forms/text_control_element.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/paint/compositing/composited_selection.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/platform/testing/use_mock_scrollbar_settings.h"

namespace blink {

class RenderedPositionTest : public EditingTestBase {
 public:
  void SetUp() override {
    EditingTestBase::SetUp();
    GetPage().GetSettings().SetAcceleratedCompositingEnabled(true);
    GetDocument().View()->SetParentVisible(true);
    GetDocument().View()->SetSelfVisible(true);
    LoadAhem();
  }

  void FocusAndSelectAll(Element* focus, const Node& select) {
    DCHECK(focus);
    focus->focus();
    Selection().SetSelection(
        SelectionInDOMTree::Builder().SelectAllChildren(select).Build(),
        SetSelectionOptions::Builder().SetShouldShowHandle(true).Build());
    UpdateAllLifecyclePhases();
  }

  void FocusAndSelectAll(TextControlElement* target) {
    FocusAndSelectAll(target, *target->InnerEditorElement());
  }

 private:
  UseMockScrollbarSettings mock_scrollbars_;
};

TEST_F(RenderedPositionTest, ComputeCompositedSelection) {
  SetBodyContent(R"HTML(
      <!DOCTYPE html>
      input {
        font: 10px/1 Ahem;
        padding: 0;
        border: 0;
      }
      <input id=target width=20 value='test test test test test tes tes test'
      style='width: 100px; height: 20px;'>
  )HTML");

  FocusAndSelectAll(ToHTMLInputElement(GetDocument().getElementById("target")));

  const CompositedSelection& composited_selection =
      ComputeCompositedSelection(Selection());
  EXPECT_FALSE(composited_selection.start.hidden);
  EXPECT_TRUE(composited_selection.end.hidden);
}

TEST_F(RenderedPositionTest, PositionInScrollableRoot) {
  SetBodyContent(R"HTML(
      <!DOCTYPE html>
      <style>
        body {
           margin: 0;
           height: 2000px;
           width: 2000px;
        }
        input {
          font: 10px/1 Ahem;
          padding: 0;
          border: 0;
          width: 100px;
          height: 20px;
          position: absolute;
          top: 900px;
          left: 1000px;
        }
      </style>
      <input id=target width=20 value='test test test test test tes tes test'>
  )HTML");

  FocusAndSelectAll(ToHTMLInputElement(GetDocument().getElementById("target")));

  ScrollableArea* root_scroller = GetDocument().View()->GetScrollableArea();
  root_scroller->SetScrollOffset(ScrollOffset(800, 500), kProgrammaticScroll);
  ASSERT_EQ(ScrollOffset(800, 500), root_scroller->GetScrollOffset());

  UpdateAllLifecyclePhases();

  const CompositedSelection& composited_selection =
      ComputeCompositedSelection(Selection());

  // Top-left corner should be around (1000, 905) - 10px centered in 20px
  // height.
  EXPECT_EQ(FloatPoint(1000, 905),
            composited_selection.start.edge_top_in_layer);
  EXPECT_EQ(FloatPoint(1000, 915),
            composited_selection.start.edge_bottom_in_layer);
  EXPECT_EQ(FloatPoint(1369, 905), composited_selection.end.edge_top_in_layer);
  EXPECT_EQ(FloatPoint(1369, 915),
            composited_selection.end.edge_bottom_in_layer);
}

TEST_F(RenderedPositionTest, PositionInScroller) {
  SetBodyContent(R"HTML(
      <!DOCTYPE html>
      <style>
        body {
           margin: 0;
           height: 2000px;
           width: 2000px;
        }
        input {
          font: 10px/1 Ahem;
          padding: 0;
          border: 0;
          width: 100px;
          height: 20px;
          position: absolute;
          top: 900px;
          left: 1000px;
        }

        #scroller {
          width: 300px;
          height: 300px;
          position: absolute;
          left: 300px;
          top: 400px;
          overflow: scroll;
          border: 200px;
          will-change: transform;
        }

        #space {
          width: 2000px;
          height: 2000px;
        }
      </style>
      <div id="scroller">
        <div id="space"></div>
        <input id=target width=20 value='test test test test test tes tes test'>
      </div>
  )HTML");

  FocusAndSelectAll(ToHTMLInputElement(GetDocument().getElementById("target")));

  Element* e = GetDocument().getElementById("scroller");
  PaintLayerScrollableArea* scroller =
      ToLayoutBox(e->GetLayoutObject())->GetScrollableArea();
  scroller->SetScrollOffset(ScrollOffset(900, 800), kProgrammaticScroll);
  ASSERT_EQ(ScrollOffset(900, 800), scroller->GetScrollOffset());

  UpdateAllLifecyclePhases();

  const CompositedSelection& composited_selection =
      ComputeCompositedSelection(Selection());

  // Top-left corner should be around (1000, 905) - 10px centered in 20px
  // height.
  EXPECT_EQ(FloatPoint(1000, 905),
            composited_selection.start.edge_top_in_layer);
  EXPECT_EQ(FloatPoint(1000, 915),
            composited_selection.start.edge_bottom_in_layer);
  EXPECT_EQ(FloatPoint(1369, 905), composited_selection.end.edge_top_in_layer);
  EXPECT_EQ(FloatPoint(1369, 915),
            composited_selection.end.edge_bottom_in_layer);
}

// crbug.com/807930
TEST_F(RenderedPositionTest, ContentEditableLinebreak) {
  SetBodyContent(
      "<div style='font: 10px/10px Ahem;' contenteditable>"
      "test<br><br></div>");
  Element* target = GetDocument().QuerySelector("div");
  FocusAndSelectAll(target, *target);
  const CompositedSelection& composited_selection =
      ComputeCompositedSelection(Selection());
  EXPECT_EQ(composited_selection.start.edge_top_in_layer,
            FloatPoint(8.0f, 8.0f));
  EXPECT_EQ(composited_selection.start.edge_bottom_in_layer,
            FloatPoint(8.0f, 18.0f));
  EXPECT_EQ(composited_selection.end.edge_top_in_layer,
            FloatPoint(8.0f, 18.0f));
  EXPECT_EQ(composited_selection.end.edge_bottom_in_layer,
            FloatPoint(8.0f, 28.0f));
}

// crbug.com/807930
TEST_F(RenderedPositionTest, TextAreaLinebreak) {
  SetBodyContent(
      "<textarea style='font: 10px/10px Ahem;'>"
      "test\n</textarea>");
  FocusAndSelectAll(ToTextControl(GetDocument().QuerySelector("textarea")));
  const CompositedSelection& composited_selection =
      ComputeCompositedSelection(Selection());
  EXPECT_EQ(composited_selection.start.edge_top_in_layer,
            FloatPoint(11.0f, 11.0f));
  EXPECT_EQ(composited_selection.start.edge_bottom_in_layer,
            FloatPoint(11.0f, 21.0f));
  EXPECT_EQ(composited_selection.end.edge_top_in_layer,
            FloatPoint(11.0f, 21.0f));
  EXPECT_EQ(composited_selection.end.edge_bottom_in_layer,
            FloatPoint(11.0f, 31.0f));
}

// crbug.com/815099
TEST_F(RenderedPositionTest, CaretBeforeSoftWrap) {
  SetBodyContent(
      "<div style='font: 10px/10px Ahem; width:20px;' "
      "contenteditable>foo</div>");
  Element* target = GetDocument().QuerySelector("div");
  target->focus();
  Node* text_foo = target->firstChild();
  Selection().SetSelection(
      SelectionInDOMTree::Builder()
          .Collapse(
              PositionWithAffinity({text_foo, 2}, TextAffinity::kUpstream))
          .Build(),
      SetSelectionOptions::Builder().SetShouldShowHandle(true).Build());
  UpdateAllLifecyclePhases();
  const CompositedSelection& composited_selection =
      ComputeCompositedSelection(Selection());
  EXPECT_EQ(composited_selection.start.edge_top_in_layer,
            FloatPoint(27.0f, 8.0f));
  EXPECT_EQ(composited_selection.start.edge_bottom_in_layer,
            FloatPoint(27.0f, 18.0f));
  EXPECT_EQ(composited_selection.end.edge_top_in_layer,
            FloatPoint(27.0f, 8.0f));
  EXPECT_EQ(composited_selection.end.edge_bottom_in_layer,
            FloatPoint(27.0f, 18.0f));
}

TEST_F(RenderedPositionTest, CaretAfterSoftWrap) {
  SetBodyContent(
      "<div style='font: 10px/10px Ahem; width:20px;' "
      "contenteditable>foo</div>");
  Element* target = GetDocument().QuerySelector("div");
  target->focus();
  Node* text_foo = target->firstChild();
  Selection().SetSelection(
      SelectionInDOMTree::Builder()
          .Collapse(
              PositionWithAffinity({text_foo, 2}, TextAffinity::kDownstream))
          .Build(),
      SetSelectionOptions::Builder().SetShouldShowHandle(true).Build());
  UpdateAllLifecyclePhases();
  const CompositedSelection& composited_selection =
      ComputeCompositedSelection(Selection());
  EXPECT_EQ(composited_selection.start.edge_top_in_layer,
            FloatPoint(8.0f, 18.0f));
  EXPECT_EQ(composited_selection.start.edge_bottom_in_layer,
            FloatPoint(8.0f, 28.0f));
  EXPECT_EQ(composited_selection.end.edge_top_in_layer,
            FloatPoint(8.0f, 18.0f));
  EXPECT_EQ(composited_selection.end.edge_bottom_in_layer,
            FloatPoint(8.0f, 28.0f));
}

// crbug.com/834686
TEST_F(RenderedPositionTest, RangeBeginAtBlockEnd) {
  const SelectionInDOMTree& selection = SetSelectionTextToBody(
      "<div style='font: 10px/10px Ahem;'>"
      "<div>foo\n^</div><div>ba|r</div></div>");
  Selection().SetSelection(
      selection,
      SetSelectionOptions::Builder().SetShouldShowHandle(true).Build());
  Element* target = GetDocument().QuerySelector("div");
  target->focus();
  UpdateAllLifecyclePhases();
  const CompositedSelection& composited_selection =
      ComputeCompositedSelection(Selection());
  EXPECT_EQ(composited_selection.start.edge_top_in_layer,
            FloatPoint(38.0f, 8.0f));
  EXPECT_EQ(composited_selection.start.edge_bottom_in_layer,
            FloatPoint(38.0f, 18.0f));
  EXPECT_EQ(composited_selection.end.edge_top_in_layer,
            FloatPoint(28.0f, 18.0f));
  EXPECT_EQ(composited_selection.end.edge_bottom_in_layer,
            FloatPoint(28.0f, 28.0f));
}

}  // namespace blink
