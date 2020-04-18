// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/page/scrolling/snap_coordinator.h"

#include <gtest/gtest.h>
#include <memory>
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/paint_layer_scrollable_area.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/scroll/scroll_types.h"

namespace blink {

using HTMLNames::styleAttr;

class SnapCoordinatorTest : public testing::Test {
 protected:
  void SetUp() override {
    page_holder_ = DummyPageHolder::Create();

    SetHTML(R"HTML(
      <style>
          #snap-container {
              height: 1000px;
              width: 1000px;
              overflow: scroll;
              scroll-snap-type: both mandatory;
          }
          #snap-element-fixed-position {
               position: fixed;
          }
      </style>
      <body>
        <div id='snap-container'>
          <div id='snap-element'></div>
          <div id='intermediate'>
             <div id='nested-snap-element'></div>
          </div>
          <div id='snap-element-fixed-position'></div>
          <div style='width:2000px; height:2000px;'></div>
        </div>
      </body>
    )HTML");
    GetDocument().UpdateStyleAndLayout();
  }

  void TearDown() override { page_holder_ = nullptr; }

  Document& GetDocument() { return page_holder_->GetDocument(); }

  void SetHTML(const char* html_content) {
    GetDocument().documentElement()->SetInnerHTMLFromString(html_content);
  }

  Element& SnapContainer() {
    return *GetDocument().getElementById("snap-container");
  }

  unsigned SizeOfSnapAreas(const ContainerNode& node) {
    if (node.GetLayoutBox()->SnapAreas())
      return node.GetLayoutBox()->SnapAreas()->size();
    return 0U;
  }

  void SetUpSingleSnapArea() {
    SetHTML(R"HTML(
      <style>
      #scroller {
        width: 140px;
        height: 160px;
        padding: 0px;
        scroll-snap-type: both mandatory;
        scroll-padding: 10px;
        overflow: scroll;
      }
      #container {
        margin: 0px;
        padding: 0px;
        width: 500px;
        height: 500px;
      }
      #area {
        position: relative;
        top: 200px;
        left: 200px;
        width: 100px;
        height: 100px;
        scroll-margin: 8px;
      }
      </style>
      <div id='scroller'>
        <div id='container'>
          <div id="area"></div>
        </div>
      </div>
      )HTML");
    GetDocument().UpdateStyleAndLayout();
  }

  std::unique_ptr<DummyPageHolder> page_holder_;
};

TEST_F(SnapCoordinatorTest, SimpleSnapElement) {
  Element& snap_element = *GetDocument().getElementById("snap-element");
  snap_element.setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();

  EXPECT_EQ(1U, SizeOfSnapAreas(SnapContainer()));
}

TEST_F(SnapCoordinatorTest, NestedSnapElement) {
  Element& snap_element = *GetDocument().getElementById("nested-snap-element");
  snap_element.setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();

  EXPECT_EQ(1U, SizeOfSnapAreas(SnapContainer()));
}

TEST_F(SnapCoordinatorTest, NestedSnapElementCaptured) {
  Element& snap_element = *GetDocument().getElementById("nested-snap-element");
  snap_element.setAttribute(styleAttr, "scroll-snap-align: start;");

  Element* intermediate = GetDocument().getElementById("intermediate");
  intermediate->setAttribute(styleAttr, "overflow: scroll;");

  GetDocument().UpdateStyleAndLayout();

  // Intermediate scroller captures nested snap elements first so ancestor
  // does not get them.
  EXPECT_EQ(0U, SizeOfSnapAreas(SnapContainer()));
  EXPECT_EQ(1U, SizeOfSnapAreas(*intermediate));
}

TEST_F(SnapCoordinatorTest, PositionFixedSnapElement) {
  Element& snap_element =
      *GetDocument().getElementById("snap-element-fixed-position");
  snap_element.setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();

  // Position fixed elements are contained in document and not its immediate
  // ancestor scroller. They cannot be a valid snap destination so they should
  // not contribute snap points to their immediate snap container or document
  // See: https://lists.w3.org/Archives/Public/www-style/2015Jun/0376.html
  EXPECT_EQ(0U, SizeOfSnapAreas(SnapContainer()));

  Element* body = GetDocument().ViewportDefiningElement();
  EXPECT_EQ(0U, SizeOfSnapAreas(*body));
}

TEST_F(SnapCoordinatorTest, UpdateStyleForSnapElement) {
  Element& snap_element = *GetDocument().getElementById("snap-element");
  snap_element.setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();

  EXPECT_EQ(1U, SizeOfSnapAreas(SnapContainer()));

  snap_element.remove();
  GetDocument().UpdateStyleAndLayout();

  EXPECT_EQ(0U, SizeOfSnapAreas(SnapContainer()));

  // Add a new snap element
  Element& container = *GetDocument().getElementById("snap-container");
  container.SetInnerHTMLFromString(R"HTML(
    <div style='scroll-snap-align: start;'>
        <div style='width:2000px; height:2000px;'></div>
    </div>
  )HTML");
  GetDocument().UpdateStyleAndLayout();

  EXPECT_EQ(1U, SizeOfSnapAreas(SnapContainer()));
}

TEST_F(SnapCoordinatorTest, LayoutViewCapturesWhenBodyElementViewportDefining) {
  SetHTML(R"HTML(
    <style>
    body {
        overflow: scroll;
        scroll-snap-type: both mandatory;
        height: 1000px;
        width: 1000px;
        margin: 5px;
    }
    </style>
    <body>
        <div id='snap-element' style='scroll-snap-align: start;></div>
        <div id='intermediate'>
            <div id='nested-snap-element'
                style='scroll-snap-align: start;'></div>
        </div>
        <div style='width:2000px; height:2000px;'></div>
    </body>
  )HTML");

  GetDocument().UpdateStyleAndLayout();

  // Sanity check that body is the viewport defining element
  EXPECT_EQ(GetDocument().body(), GetDocument().ViewportDefiningElement());

  // When body is viewport defining and overflows then any snap points on the
  // body element will be captured by layout view as the snap container.
  EXPECT_EQ(2U, SizeOfSnapAreas(GetDocument()));
  EXPECT_EQ(0U, SizeOfSnapAreas(*(GetDocument().body())));
  EXPECT_EQ(0U, SizeOfSnapAreas(*(GetDocument().documentElement())));
}

TEST_F(SnapCoordinatorTest,
       LayoutViewCapturesWhenDocumentElementViewportDefining) {
  SetHTML(R"HTML(
    <style>
    :root {
        overflow: scroll;
        scroll-snap-type: both mandatory;
        height: 500px;
        width: 500px;
    }
    body {
        margin: 5px;
    }
    </style>
    <html>
       <body>
           <div id='snap-element' style='scroll-snap-align: start;></div>
           <div id='intermediate'>
             <div id='nested-snap-element'
                 style='scroll-snap-align: start;'></div>
          </div>
          <div style='width:2000px; height:2000px;'></div>
       </body>
    </html>
  )HTML");

  GetDocument().UpdateStyleAndLayout();

  // Sanity check that document element is the viewport defining element
  EXPECT_EQ(GetDocument().documentElement(),
            GetDocument().ViewportDefiningElement());

  // When document is viewport defining and overflows then any snap points on
  // the document element will be captured by layout view as the snap
  // container.
  EXPECT_EQ(2U, SizeOfSnapAreas(GetDocument()));
  EXPECT_EQ(0U, SizeOfSnapAreas(*(GetDocument().body())));
  EXPECT_EQ(0U, SizeOfSnapAreas(*(GetDocument().documentElement())));
}

TEST_F(SnapCoordinatorTest,
       BodyCapturesWhenBodyOverflowAndDocumentElementViewportDefining) {
  SetHTML(R"HTML(
    <style>
    :root {
        overflow: scroll;
        scroll-snap-type: both mandatory;
        height: 500px;
        width: 500px;
    }
    body {
        overflow: scroll;
        scroll-snap-type: both mandatory;
        height: 1000px;
        width: 1000px;
        margin: 5px;
    }
    </style>
    <html>
       <body style='overflow: scroll; scroll-snap-type: both mandatory;
    height:1000px; width:1000px;'>
           <div id='snap-element' style='scroll-snap-align: start;></div>
           <div id='intermediate'>
             <div id='nested-snap-element'
                 style='scroll-snap-align: start;'></div>
          </div>
          <div style='width:2000px; height:2000px;'></div>
       </body>
    </html>
  )HTML");

  GetDocument().UpdateStyleAndLayout();

  // Sanity check that document element is the viewport defining element
  EXPECT_EQ(GetDocument().documentElement(),
            GetDocument().ViewportDefiningElement());

  // When body and document elements are both scrollable then body element
  // should capture snap points defined on it as opposed to layout view.
  Element& body = *GetDocument().body();
  EXPECT_EQ(2U, SizeOfSnapAreas(body));
}

#define EXPECT_EQ_CONTAINER(expected, actual)                          \
  {                                                                    \
    EXPECT_EQ(expected.max_position().x(), actual.max_position().x()); \
    EXPECT_EQ(expected.max_position().y(), actual.max_position().y()); \
    EXPECT_EQ(expected.scroll_snap_type(), actual.scroll_snap_type()); \
    EXPECT_EQ(expected.proximity_range(), actual.proximity_range());   \
    EXPECT_EQ(expected.size(), actual.size());                         \
  }

#define EXPECT_EQ_AREA(expected, actual)                             \
  {                                                                  \
    EXPECT_EQ(expected.snap_axis, actual.snap_axis);                 \
    EXPECT_EQ(expected.snap_position.x(), actual.snap_position.x()); \
    EXPECT_EQ(expected.snap_position.y(), actual.snap_position.y()); \
    EXPECT_EQ(expected.visible_region, actual.visible_region);       \
    EXPECT_EQ(expected.must_snap, actual.must_snap);                 \
  }

// The following tests check the snap data are correctly calculated.
TEST_F(SnapCoordinatorTest, StartAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller_element = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*scroller_element->GetLayoutBox());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  ScrollableArea* scrollable_area =
      scroller_element->GetLayoutBox()->GetScrollableArea();
  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  // (#area.left - #area.scroll-margin) - (#scroller.scroll-padding)
  double snap_position_x = (200 - 8) - 10;
  // (#area.top - #area.scroll-margin) - (#scroller.scroll-padding)
  double snap_position_y = (200 - 8) - 10;

  double width = scroller_element->clientWidth();
  double height = scroller_element->clientHeight();
  gfx::RectF visible_region(202 - width, 202 - height, 96 + width, 96 + height);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));
  SnapAreaData expected_area(
      SnapAxis::kBoth, gfx::ScrollOffset(snap_position_x, snap_position_y),
      visible_region, must_snap);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

TEST_F(SnapCoordinatorTest, ScrolledStartAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* scroller_element = GetDocument().getElementById("scroller");
  ScrollableArea* scrollable_area =
      scroller_element->GetLayoutBox()->GetScrollableArea();
  scroller_element->scrollBy(20, 20);
  EXPECT_EQ(FloatPoint(20, 20), scrollable_area->ScrollPosition());
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*scroller_element->GetLayoutBox());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  // (#area.left - #area.scroll-margin) - (#scroller.scroll-padding)
  double snap_position_x = (200 - 8) - 10;
  // (#area.top - #area.scroll-margin) - (#scroller.scroll-padding)
  double snap_position_y = (200 - 8) - 10;

  double width = scroller_element->clientWidth();
  double height = scroller_element->clientHeight();
  gfx::RectF visible_region(202 - width, 202 - height, 96 + width, 96 + height);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));
  SnapAreaData expected_area(
      SnapAxis::kBoth, gfx::ScrollOffset(snap_position_x, snap_position_y),
      visible_region, must_snap);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

TEST_F(SnapCoordinatorTest, ScrolledStartAlignmentCalculationOnViewport) {
  SetHTML(R"HTML(
    <style>
    body {
      margin: 0px;
      scroll-snap-type: both mandatory;
      overflow: scroll;
    }
    #container {
      width: 1000px;
      height: 1000px;
    }
    #area {
      position: relative;
      top: 200px;
      left: 200px;
      width: 100px;
      height: 100px;
    }
    </style>
    <div id='container'>
    <div id="area"></div>
    </div>
    )HTML");
  GetDocument().UpdateStyleAndLayout();

  Element* body = GetDocument().body();
  EXPECT_EQ(body, GetDocument().ViewportDefiningElement());
  ScrollableArea* scrollable_area =
      GetDocument().View()->LayoutViewportScrollableArea();
  body->scrollBy(20, 20);
  EXPECT_EQ(FloatPoint(20, 20), scrollable_area->ScrollPosition());
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*GetDocument().GetLayoutView());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));

  double width = body->clientWidth();
  double height = body->clientHeight();
  gfx::RectF visible_region(200 - width, 200 - height, 100 + width,
                            100 + height);
  SnapAreaData expected_area(SnapAxis::kBoth, gfx::ScrollOffset(200, 200),
                             visible_region, false);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

TEST_F(SnapCoordinatorTest, StartAlignmentCalculationWithBoxModel) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr,
                             "scroll-snap-align: start; margin: 2px; border: "
                             "9px solid; padding: 5px;");
  Element* scroller_element = GetDocument().getElementById("scroller");
  scroller_element->setAttribute(
      styleAttr, "margin: 3px; border: 10px solid; padding: 4px;");
  GetDocument().UpdateStyleAndLayout();
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*scroller_element->GetLayoutBox());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  ScrollableArea* scrollable_area =
      scroller_element->GetLayoutBox()->GetScrollableArea();
  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  // (#scroller.padding + #area.left + #area.margin - #area.scroll-margin)
  //  - (#scroller.scroll-padding)
  double snap_position_x = (4 + 200 + 2 - 8) - 10;
  // (#scroller.padding + #area.top + #area.margin - #area.scroll-margin)
  //  - (#scroller.scroll-padding)
  double snap_position_y = (4 + 200 + 2 - 8) - 10;

  double width = scroller_element->clientWidth();
  double height = scroller_element->clientHeight();
  gfx::RectF visible_region(208 - width, 208 - height, 124 + width,
                            124 + height);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));
  SnapAreaData expected_area(
      SnapAxis::kBoth, gfx::ScrollOffset(snap_position_x, snap_position_y),
      visible_region, must_snap);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

TEST_F(SnapCoordinatorTest, NegativeMarginStartAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr,
                             "scroll-snap-align: start; scroll-margin: -8px;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller_element = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*scroller_element->GetLayoutBox());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  ScrollableArea* scrollable_area =
      scroller_element->GetLayoutBox()->GetScrollableArea();
  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  // (#area.left - #area.scroll-margin) - (#scroller.scroll-padding)
  double snap_position_x = (200 - (-8)) - 10;
  // (#area.top - #area.scroll-margin) - (#scroller.scroll-padding)
  double snap_position_y = (200 - (-8)) - 10;

  double width = scroller_element->clientWidth();
  double height = scroller_element->clientHeight();
  gfx::RectF visible_region(218 - width, 218 - height, 64 + width, 64 + height);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));
  SnapAreaData expected_area(
      SnapAxis::kBoth, gfx::ScrollOffset(snap_position_x, snap_position_y),
      visible_region, must_snap);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

TEST_F(SnapCoordinatorTest, CenterAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr, "scroll-snap-align: center;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller_element = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*scroller_element->GetLayoutBox());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  ScrollableArea* scrollable_area =
      scroller_element->GetLayoutBox()->GetScrollableArea();
  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  double width = scroller_element->clientWidth();
  double height = scroller_element->clientHeight();
  // (#area.left + #area.right) / 2 - #scroller.width / 2
  double snap_position_x = (200 + (200 + 100)) / 2 - width / 2;
  // (#area.top + #area.bottom) / 2 - #scroller.height / 2
  double snap_position_y = (200 + (200 + 100)) / 2 - height / 2;

  gfx::RectF visible_region(202 - width, 202 - height, 96 + width, 96 + height);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));
  SnapAreaData expected_area(
      SnapAxis::kBoth, gfx::ScrollOffset(snap_position_x, snap_position_y),
      visible_region, must_snap);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

TEST_F(SnapCoordinatorTest, AsymmetricalCenterAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr,
                             R"HTML(
        scroll-snap-align: center;
        scroll-margin-top: 2px;
        scroll-margin-right: 4px;
        scroll-margin-bottom: 6px;
        scroll-margin-left: 8px;
      )HTML");
  Element* scroller_element = GetDocument().getElementById("scroller");
  scroller_element->setAttribute(styleAttr,
                                 R"HTML(
        scroll-padding-top: 10px;
        scroll-padding-right: 12px;
        scroll-padding-bottom: 14px;
        scroll-padding-left: 16px;
      )HTML");
  GetDocument().UpdateStyleAndLayout();
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*scroller_element->GetLayoutBox());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  ScrollableArea* scrollable_area =
      scroller_element->GetLayoutBox()->GetScrollableArea();
  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  double width = scroller_element->clientWidth();
  double height = scroller_element->clientHeight();
  // (#area.left - #area.scroll-margin-left +
  //  #area.right + #area.scroll-margin-right) / 2 -
  // (#scroller.left + #scroller.scroll-padding-left +
  //  #scroller.right - #scroller.scroll-padding-right) / 2
  double snap_position_x =
      (200 - 8 + (200 + 100 + 4)) / 2 - (0 + 16 + width - 12) / 2;

  // (#area.top - #area.scroll-margin-top +
  //  #area.bottom + #area.scroll-margin-bottom) / 2 -
  // (#scroller.top + #scroller.scroll-padding-top +
  //  #scroller.bottom - #scroller.scroll-padding-bottom) / 2
  double snap_position_y =
      (200 - 2 + (200 + 100 + 6)) / 2 - (0 + 10 + height - 14) / 2;

  gfx::RectF visible_region(204 - width, 212 - height, 84 + width, 84 + height);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));
  SnapAreaData expected_area(
      SnapAxis::kBoth, gfx::ScrollOffset(snap_position_x, snap_position_y),
      visible_region, must_snap);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

TEST_F(SnapCoordinatorTest, EndAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr, "scroll-snap-align: end;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller_element = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*scroller_element->GetLayoutBox());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  ScrollableArea* scrollable_area =
      scroller_element->GetLayoutBox()->GetScrollableArea();
  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  double width = scroller_element->clientWidth();
  double height = scroller_element->clientHeight();
  // (#area.right + #area.scroll-margin)
  // - (#scroller.right - #scroller.scroll-padding)
  double snap_position_x = (200 + 100 + 8) - (width - 10);

  // (#area.bottom + #area.scroll-margin)
  // - (#scroller.bottom - #scroller.scroll-padding)
  double snap_position_y = (200 + 100 + 8) - (height - 10);

  gfx::RectF visible_region(202 - width, 202 - height, 96 + width, 96 + height);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));
  SnapAreaData expected_area(
      SnapAxis::kBoth, gfx::ScrollOffset(snap_position_x, snap_position_y),
      visible_region, must_snap);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

TEST_F(SnapCoordinatorTest, EndAlignmentCalculationWithBoxModel) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(
      styleAttr,
      "scroll-snap-align: end; margin: 2px; border: 9px solid; padding: 5px;");
  Element* scroller_element = GetDocument().getElementById("scroller");
  scroller_element->setAttribute(
      styleAttr, "margin: 3px; border: 10px solid; padding: 4px;");
  GetDocument().UpdateStyleAndLayout();
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*scroller_element->GetLayoutBox());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  ScrollableArea* scrollable_area =
      scroller_element->GetLayoutBox()->GetScrollableArea();
  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  double width = scroller_element->clientWidth();
  double height = scroller_element->clientHeight();
  // (#scroller.padding + #area.left + #area.margin + #area.width
  //   + 2 x (#area.border + #area.padding)) + #area.scroll-margin)
  //  - (#scroller.width - #scroller.scroll-padding)
  double snap_position_x = (4 + 200 + 2 + 100 + 2 * (9 + 5) + 8) - (width - 10);
  // (#scroller.padding + #area.top + #area.height + #area.margin
  //   + 2 x (#area.border + #area.padding)) + #area.scroll-margin)
  //  - (#scroller.height - #scroller.scroll-padding)
  double snap_position_y =
      (4 + 200 + 2 + 100 + 2 * (9 + 5) + 8) - (height - 10);

  gfx::RectF visible_region(208 - width, 208 - height, 124 + width,
                            124 + height);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));
  SnapAreaData expected_area(
      SnapAxis::kBoth, gfx::ScrollOffset(snap_position_x, snap_position_y),
      visible_region, must_snap);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

TEST_F(SnapCoordinatorTest, ScaledEndAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr,
                             "scroll-snap-align: end; transform: scale(4, 4);");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller_element = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*scroller_element->GetLayoutBox());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  ScrollableArea* scrollable_area =
      scroller_element->GetLayoutBox()->GetScrollableArea();
  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  double width = scroller_element->clientWidth();
  double height = scroller_element->clientHeight();
  // The area is scaled from center, so it pushes the area's top-left corner to
  // (50, 50).
  // (#area.right + #area.scroll-margin)
  // - (#scroller.right - #scroller.scroll-padding)
  double snap_position_x = (50 + 400 + 8) - (width - 10);

  // (#area.bottom + #area.scroll-margin)
  // - (#scroller.bottom - #scroller.scroll-padding)
  double snap_position_y = (50 + 400 + 8) - (height - 10);

  gfx::RectF visible_region(52 - width, 52 - height, 396 + width, 396 + height);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));
  SnapAreaData expected_area(
      SnapAxis::kBoth, gfx::ScrollOffset(snap_position_x, snap_position_y),
      visible_region, must_snap);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

TEST_F(SnapCoordinatorTest, VerticalRlStartAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr,
                             "scroll-snap-align: start; left: -200px;");
  Element* scroller_element = GetDocument().getElementById("scroller");
  scroller_element->setAttribute(styleAttr, "writing-mode: vertical-rl;");
  GetDocument().UpdateStyleAndLayout();
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*scroller_element->GetLayoutBox());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  ScrollableArea* scrollable_area =
      scroller_element->GetLayoutBox()->GetScrollableArea();
  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  double width = scroller_element->clientWidth();
  double height = scroller_element->clientHeight();
  // Under vertical-rl writing mode, 'start' should align to the right.
  // (#area.right + #area.scroll-margin)
  // - (#scroller.right - #scroller.scroll-padding)
  double snap_position_x = (200 + 100 + 8) - (width - 10);

  // (#area.top - #area.scroll-margin) - (#scroller.scroll-padding)
  double snap_position_y = (200 - 8) - 10;

  gfx::RectF visible_region(202 - width, 202 - height, 96 + width, 96 + height);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));
  SnapAreaData expected_area(
      SnapAxis::kBoth, gfx::ScrollOffset(snap_position_x, snap_position_y),
      visible_region, must_snap);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

// TODO(sunyunjia): Also add a test for vertical and rtl page.

TEST_F(SnapCoordinatorTest, OverflowedSnapPositionCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr,
                             "left: 0px; top: 0px; scroll-snap-align: end;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller_element = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  base::Optional<SnapContainerData> data =
      snap_coordinator->GetSnapContainerData(*scroller_element->GetLayoutBox());
  EXPECT_TRUE(data.has_value());
  SnapContainerData actual_container = data.value();

  ScrollableArea* scrollable_area =
      scroller_element->GetLayoutBox()->GetScrollableArea();
  FloatPoint max_position = ScrollOffsetToPosition(
      scrollable_area->MaximumScrollOffset(), scrollable_area->ScrollOrigin());

  double width = scroller_element->clientWidth();
  double height = scroller_element->clientHeight();
  // (#area.right + #area.scroll-margin)
  //  - (#scroller.right - #scroller.scroll-padding)
  // = (100 + 8) - (clientWidth - 10) < 0
  // As scrollPosition cannot be set to a negative number, we set it to 0.
  double snap_position_x = 0;

  // (#area.bottom + #area.scroll-margin)
  //  - (#scroller.bottom - #scroller.scroll-padding)
  // = (100 + 8) - (clientHeight - 10) < 0
  // As scrollPosition cannot be set to a negative number, we set it to 0.
  double snap_position_y = 0;

  gfx::RectF visible_region(2 - width, 2 - height, 96 + width, 96 + height);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(max_position.X(), max_position.Y()));
  SnapAreaData expected_area(
      SnapAxis::kBoth, gfx::ScrollOffset(snap_position_x, snap_position_y),
      visible_region, must_snap);
  expected_container.AddSnapAreaData(expected_area);

  EXPECT_EQ_CONTAINER(expected_container, actual_container);
  EXPECT_EQ_AREA(expected_area, actual_container.at(0));
}

// The following tests check GetSnapPositionForPoint().
TEST_F(SnapCoordinatorTest, SnapsIfScrolledAndSnappingAxesMatch) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  Element* scroller_element = GetDocument().getElementById("scroller");
  area_element->setAttribute(styleAttr, "scroll-snap-align: start;");
  scroller_element->setAttribute(styleAttr, "scroll-snap-type: x mandatory");
  GetDocument().UpdateStyleAndLayout();

  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  LayoutBox* snap_container = scroller_element->GetLayoutBox();
  FloatPoint snap_position = snap_coordinator->GetSnapPositionForPoint(
      *snap_container, FloatPoint(150, 150), true, false);
  EXPECT_EQ(200 - 8 - 10, snap_position.X());
  EXPECT_EQ(150, snap_position.Y());
}

TEST_F(SnapCoordinatorTest, DoesNotSnapOnNonSnappingAxis) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  Element* scroller_element = GetDocument().getElementById("scroller");
  area_element->setAttribute(styleAttr, "scroll-snap-align: start;");
  scroller_element->setAttribute(styleAttr, "scroll-snap-type: y mandatory");
  GetDocument().UpdateStyleAndLayout();

  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  LayoutBox* snap_container = scroller_element->GetLayoutBox();
  FloatPoint snap_position = snap_coordinator->GetSnapPositionForPoint(
      *snap_container, FloatPoint(150, 150), true, false);
  EXPECT_EQ(150, snap_position.X());
  EXPECT_EQ(150, snap_position.Y());
}

TEST_F(SnapCoordinatorTest, DoesNotSnapOnEmptyContainer) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  Element* scroller_element = GetDocument().getElementById("scroller");
  area_element->setAttribute(styleAttr, "scroll-snap-align: none;");
  scroller_element->setAttribute(styleAttr, "scroll-snap-type: x mandatory");
  GetDocument().UpdateStyleAndLayout();

  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  LayoutBox* snap_container = scroller_element->GetLayoutBox();
  FloatPoint snap_position = snap_coordinator->GetSnapPositionForPoint(
      *snap_container, FloatPoint(150, 150), true, false);
  EXPECT_EQ(150, snap_position.X());
  EXPECT_EQ(150, snap_position.Y());
}

TEST_F(SnapCoordinatorTest, DoesNotSnapOnNonSnapContainer) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  Element* scroller_element = GetDocument().getElementById("scroller");
  area_element->setAttribute(styleAttr, "scroll-snap-align: start;");
  scroller_element->setAttribute(styleAttr, "scroll-snap-type: none");
  GetDocument().UpdateStyleAndLayout();

  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  LayoutBox* snap_container = scroller_element->GetLayoutBox();
  FloatPoint snap_position = snap_coordinator->GetSnapPositionForPoint(
      *snap_container, FloatPoint(150, 150), true, false);
  EXPECT_EQ(150, snap_position.X());
  EXPECT_EQ(150, snap_position.Y());
}

}  // namespace
