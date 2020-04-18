// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/layout_inline.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {

class LayoutInlineTest : public RenderingTest {};

// Helper class to run the same test code with and without LayoutNG
class ParameterizedLayoutInlineTest : public testing::WithParamInterface<bool>,
                                      private ScopedLayoutNGForTest,
                                      public LayoutInlineTest {
 public:
  ParameterizedLayoutInlineTest() : ScopedLayoutNGForTest(GetParam()) {}

 protected:
  bool LayoutNGEnabled() const { return GetParam(); }
};

INSTANTIATE_TEST_CASE_P(All, ParameterizedLayoutInlineTest, testing::Bool());

TEST_P(ParameterizedLayoutInlineTest, LinesBoundingBox) {
  LoadAhem();
  SetBodyInnerHTML(
      "<style>"
      "html { font-family: Ahem; font-size: 13px; }"
      // LayoutNG requires box decorations at this moment. crbug.com/789390
      "span { background-color: yellow; }"
      ".vertical { writing-mode: vertical-rl; }"
      "</style>"
      "<p><span id=ltr1>abc<br>xyz</span></p>"
      "<p><span id=ltr2>12 345 6789</span></p>"
      "<p dir=rtl><span id=rtl1>abc<br>xyz</span></p>"
      "<p dir=rtl><span id=rtl2>12 345 6789</span></p>"
      "<p class=vertical><span id=vertical>abc<br>xyz</span></p>");
  EXPECT_EQ(
      LayoutRect(LayoutPoint(0, 0), LayoutSize(39, 26)),
      ToLayoutInline(GetLayoutObjectByElementId("ltr1"))->LinesBoundingBox());
  EXPECT_EQ(
      LayoutRect(LayoutPoint(0, 0), LayoutSize(143, 13)),
      ToLayoutInline(GetLayoutObjectByElementId("ltr2"))->LinesBoundingBox());
  EXPECT_EQ(
      LayoutRect(LayoutPoint(745, 0), LayoutSize(39, 26)),
      ToLayoutInline(GetLayoutObjectByElementId("rtl1"))->LinesBoundingBox());
  EXPECT_EQ(
      LayoutRect(LayoutPoint(641, 0), LayoutSize(143, 13)),
      ToLayoutInline(GetLayoutObjectByElementId("rtl2"))->LinesBoundingBox());
  EXPECT_EQ(LayoutRect(LayoutPoint(0, 0), LayoutSize(26, 39)),
            ToLayoutInline(GetLayoutObjectByElementId("vertical"))
                ->LinesBoundingBox());
}

TEST_F(LayoutInlineTest, SimpleContinuation) {
  SetBodyInnerHTML(
      "<span id='splitInline'><i id='before'></i><h1 id='blockChild'></h1><i "
      "id='after'></i></span>");

  LayoutInline* split_inline_part1 =
      ToLayoutInline(GetLayoutObjectByElementId("splitInline"));
  ASSERT_TRUE(split_inline_part1);
  ASSERT_TRUE(split_inline_part1->FirstChild());
  EXPECT_EQ(split_inline_part1->FirstChild(),
            GetLayoutObjectByElementId("before"));
  EXPECT_FALSE(split_inline_part1->FirstChild()->NextSibling());

  LayoutBlockFlow* block =
      ToLayoutBlockFlow(split_inline_part1->Continuation());
  ASSERT_TRUE(block);
  ASSERT_TRUE(block->FirstChild());
  EXPECT_EQ(block->FirstChild(), GetLayoutObjectByElementId("blockChild"));
  EXPECT_FALSE(block->FirstChild()->NextSibling());

  LayoutInline* split_inline_part2 = ToLayoutInline(block->Continuation());
  ASSERT_TRUE(split_inline_part2);
  ASSERT_TRUE(split_inline_part2->FirstChild());
  EXPECT_EQ(split_inline_part2->FirstChild(),
            GetLayoutObjectByElementId("after"));
  EXPECT_FALSE(split_inline_part2->FirstChild()->NextSibling());
  EXPECT_FALSE(split_inline_part2->Continuation());
}

TEST_F(LayoutInlineTest, RegionHitTest) {
  SetBodyInnerHTML(R"HTML(
    <div><span id='lotsOfBoxes'>
    This is a test line<br>This is a test line<br>This is a test line<br>
    This is a test line<br>This is a test line<br>This is a test line<br>
    This is a test line<br>This is a test line<br>This is a test line<br>
    This is a test line<br>This is a test line<br>This is a test line<br>
    This is a test line<br>This is a test line<br>This is a test line<br>
    This is a test line<br>This is a test line<br>This is a test line<br>
    </span></div>
  )HTML");

  GetDocument().View()->UpdateAllLifecyclePhases();

  LayoutInline* lots_of_boxes =
      ToLayoutInline(GetLayoutObjectByElementId("lotsOfBoxes"));
  ASSERT_TRUE(lots_of_boxes);

  HitTestRequest hit_request(HitTestRequest::kTouchEvent |
                             HitTestRequest::kListBased);
  LayoutPoint hit_location(2, 5);
  LayoutRectOutsets padding(2, 1, 2, 1);
  HitTestResult hit_result(hit_request, hit_location, padding);
  LayoutPoint hit_offset;

  bool hit_outcome = lots_of_boxes->HitTestCulledInline(
      hit_result, hit_result.GetHitTestLocation(), hit_offset);
  // Assert checks that we both hit something and that the area covered
  // by "something" totally contains the hit region.
  EXPECT_TRUE(hit_outcome);
}

// crbug.com/844746
TEST_P(ParameterizedLayoutInlineTest, RelativePositionedHitTest) {
  LoadAhem();
  SetBodyInnerHTML(
      "<div style='font: 10px/10px Ahem'>"
      "  <span style='position: relative'>XXX</span>"
      "</div>");

  HitTestRequest hit_request(HitTestRequest::kReadOnly |
                             HitTestRequest::kActive);
  const LayoutPoint container_offset(8, 8);
  const LayoutPoint hit_location(18, 15);

  Element* div = GetDocument().QuerySelector("div");
  Element* span = GetDocument().QuerySelector("span");
  Node* text = span->firstChild();

  // Shouldn't hit anything in SPAN as it's in another paint layer
  {
    LayoutObject* layout_div = div->GetLayoutObject();
    HitTestResult hit_result(hit_request, hit_location);
    bool hit_outcome = layout_div->HitTestAllPhases(hit_result, hit_location,
                                                    container_offset);
    EXPECT_TRUE(hit_outcome);
    EXPECT_EQ(div, hit_result.InnerNode());
  }

  // SPAN and its descendants can be hit only with a hit test that starts from
  // the SPAN itself.
  {
    LayoutObject* layout_span = span->GetLayoutObject();
    HitTestResult hit_result(hit_request, hit_location);
    bool hit_outcome = layout_span->HitTestAllPhases(hit_result, hit_location,
                                                     container_offset);
    EXPECT_TRUE(hit_outcome);
    EXPECT_EQ(text, hit_result.InnerNode());
  }

  // Hit test from LayoutView to verify that everything works together.
  {
    HitTestResult hit_result(hit_request, hit_location);
    bool hit_outcome = GetLayoutView().HitTest(hit_result);
    EXPECT_TRUE(hit_outcome);
    EXPECT_EQ(text, hit_result.InnerNode());
  }
}

TEST_P(ParameterizedLayoutInlineTest, MultilineRelativePositionedHitTest) {
  LoadAhem();
  SetBodyInnerHTML(
      "<div style='font: 10px/10px Ahem; width: 30px'>"
      "  <span id=span style='position: relative'>"
      "    XXX"
      "    <span id=line2 style='background-color: red'>YYY</span>"
      "    <img style='width: 10px; height: 10px; vertical-align: bottom'>"
      "  </span>"
      "</div>");

  LayoutObject* layout_span = GetLayoutObjectByElementId("span");
  HitTestRequest hit_request(HitTestRequest::kReadOnly |
                             HitTestRequest::kActive |
                             HitTestRequest::kIgnorePointerEventsNone);
  const LayoutPoint container_offset(8, 8);

  // Hit test first line
  {
    LayoutPoint hit_location(13, 13);
    Node* target = GetElementById("span")->firstChild();

    HitTestResult hit_result(hit_request, hit_location);
    bool hit_outcome = layout_span->HitTestAllPhases(hit_result, hit_location,
                                                     container_offset);
    EXPECT_TRUE(hit_outcome);
    EXPECT_EQ(target, hit_result.InnerNode());

    // Initiate a hit test from LayoutView to verify the "natural" process.
    HitTestResult layout_view_hit_result(hit_request, hit_location);
    bool layout_view_hit_outcome =
        GetLayoutView().HitTest(layout_view_hit_result);
    EXPECT_TRUE(layout_view_hit_outcome);
    EXPECT_EQ(target, layout_view_hit_result.InnerNode());
  }

  // Hit test second line
  {
    LayoutPoint hit_location(13, 23);
    Node* target = GetElementById("line2")->firstChild();

    HitTestResult hit_result(hit_request, hit_location);
    bool hit_outcome = layout_span->HitTestAllPhases(hit_result, hit_location,
                                                     container_offset);
    EXPECT_TRUE(hit_outcome);
    EXPECT_EQ(target, hit_result.InnerNode());

    // Initiate a hit test from LayoutView to verify the "natural" process.
    HitTestResult layout_view_hit_result(hit_request, hit_location);
    bool layout_view_hit_outcome =
        GetLayoutView().HitTest(layout_view_hit_result);
    EXPECT_TRUE(layout_view_hit_outcome);
    EXPECT_EQ(target, layout_view_hit_result.InnerNode());
  }

  // Hit test image in third line
  {
    LayoutPoint hit_location(13, 33);
    Node* target = GetDocument().QuerySelector("img");

    HitTestResult hit_result(hit_request, hit_location);
    bool hit_outcome = layout_span->HitTestAllPhases(hit_result, hit_location,
                                                     container_offset);
    EXPECT_TRUE(hit_outcome);
    EXPECT_EQ(target, hit_result.InnerNode());

    // Initiate a hit test from LayoutView to verify the "natural" process.
    HitTestResult layout_view_hit_result(hit_request, hit_location);
    bool layout_view_hit_outcome =
        GetLayoutView().HitTest(layout_view_hit_result);
    EXPECT_TRUE(layout_view_hit_outcome);
    EXPECT_EQ(target, layout_view_hit_result.InnerNode());
  }
}

}  // namespace blink
