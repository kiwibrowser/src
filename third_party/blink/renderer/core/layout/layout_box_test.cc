// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/layout_box.h"

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/html/html_body_element.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/layout/layout_image.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/graphics/test/stub_image.h"

namespace blink {

class LayoutBoxTest : public RenderingTest {};

TEST_F(LayoutBoxTest, BackgroundObscuredInRect) {
  SetBodyInnerHTML(R"HTML(
    <style>.column { width: 295.4px; padding-left: 10.4px; }
    .white-background { background: red; position: relative; overflow:
    hidden; border-radius: 1px; }
    .black-background { height: 100px; background: black; color: white; }
    </style>
    <div class='column'> <div> <div id='target' class='white-background'>
    <div class='black-background'></div> </div> </div> </div>
  )HTML");
  LayoutObject* layout_object = GetLayoutObjectByElementId("target");
  ASSERT_TRUE(layout_object);
  ASSERT_TRUE(layout_object->BackgroundIsKnownToBeObscured());
}

TEST_F(LayoutBoxTest, BackgroundNotObscuredWithCssClippedChild) {
  SetBodyInnerHTML(R"HTML(
    <style>
      #parent {
        position: relative;
        width: 200px;
        height: 200px;
        background-color: green;
      }
      #child {
        position: absolute;
        width: 100%;
        height: 100%;
        background-color: blue;
        /* clip the 200x200 box to a centered, 100x100 square. */
        clip: rect(50px, 150px, 150px, 50px);
      }
    </style>
    <div id="parent">
      <div id="child"></div>
    </div>
  )HTML");
  auto* child = GetLayoutObjectByElementId("child");
  EXPECT_FALSE(child->BackgroundIsKnownToBeObscured());

  auto* parent = GetLayoutObjectByElementId("parent");
  EXPECT_FALSE(parent->BackgroundIsKnownToBeObscured());
}

TEST_F(LayoutBoxTest, BackgroundNotObscuredWithCssClippedGrandChild) {
  SetBodyInnerHTML(R"HTML(
    <style>
      #parent {
        position: relative;
        width: 200px;
        height: 200px;
        background-color: green;
      }
      #child {
        position: absolute;
        width: 100%;
        height: 100%;
        /* clip the 200x200 box to a centered, 100x100 square. */
        clip: rect(50px, 150px, 150px, 50px);
      }
      #grandchild {
        position: absolute;
        width: 100%;
        height: 100%;
        background-color: blue;
      }
    </style>
    <div id="parent">
      <div id="child">
        <div id="grandchild"></div>
      </div>
    </div>
  )HTML");
  auto* grandchild = GetLayoutObjectByElementId("grandchild");
  EXPECT_FALSE(grandchild->BackgroundIsKnownToBeObscured());

  auto* child = GetLayoutObjectByElementId("child");
  EXPECT_FALSE(child->BackgroundIsKnownToBeObscured());

  auto* parent = GetLayoutObjectByElementId("parent");
  EXPECT_FALSE(parent->BackgroundIsKnownToBeObscured());
}

TEST_F(LayoutBoxTest, BackgroundRect) {
  SetBodyInnerHTML(R"HTML(
    <style>div { position: absolute; width: 100px; height: 100px; padding:
    10px; border: 10px solid black; overflow: scroll; }
    #target1 { background:
    url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUg) border-box, green
    content-box;}
    #target2 { background:
    url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUg) content-box, green
    local border-box;}
    #target3 { background:
    url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUg) content-box, rgba(0,
    255, 0, 0.5) border-box;}
    #target4 { background-image:
    url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUg), none;
               background-clip: content-box, border-box;
               background-blend-mode: normal, multiply;
               background-color: green; }
    #target5 { background: none border-box, green content-box;}
    #target6 { background: green content-box local; }
    </style>
    <div id='target1'></div>
    <div id='target2'></div>
    <div id='target3'></div>
    <div id='target4'></div>
    <div id='target5'></div>
    <div id='target6'></div>
  )HTML");

  // #target1's opaque background color only fills the content box but its
  // translucent image extends to the borders.
  LayoutBox* layout_box = ToLayoutBox(GetLayoutObjectByElementId("target1"));
  EXPECT_EQ(LayoutRect(20, 20, 100, 100),
            layout_box->BackgroundRect(kBackgroundKnownOpaqueRect));
  EXPECT_EQ(LayoutRect(0, 0, 140, 140),
            layout_box->BackgroundRect(kBackgroundClipRect));

  // #target2's background color is opaque but only fills the padding-box
  // because it has local attachment. This eclipses the content-box image.
  layout_box = ToLayoutBox(GetLayoutObjectByElementId("target2"));
  EXPECT_EQ(LayoutRect(10, 10, 120, 120),
            layout_box->BackgroundRect(kBackgroundKnownOpaqueRect));
  EXPECT_EQ(LayoutRect(10, 10, 120, 120),
            layout_box->BackgroundRect(kBackgroundClipRect));

  // #target3's background color is not opaque so we only have a clip rect.
  layout_box = ToLayoutBox(GetLayoutObjectByElementId("target3"));
  EXPECT_TRUE(layout_box->BackgroundRect(kBackgroundKnownOpaqueRect).IsEmpty());
  EXPECT_EQ(LayoutRect(0, 0, 140, 140),
            layout_box->BackgroundRect(kBackgroundClipRect));

  // #target4's background color has a blend mode so it isn't opaque.
  layout_box = ToLayoutBox(GetLayoutObjectByElementId("target4"));
  EXPECT_TRUE(layout_box->BackgroundRect(kBackgroundKnownOpaqueRect).IsEmpty());
  EXPECT_EQ(LayoutRect(0, 0, 140, 140),
            layout_box->BackgroundRect(kBackgroundClipRect));

  // #target5's solid background only covers the content-box but it has a "none"
  // background covering the border box.
  layout_box = ToLayoutBox(GetLayoutObjectByElementId("target5"));
  EXPECT_EQ(LayoutRect(20, 20, 100, 100),
            layout_box->BackgroundRect(kBackgroundKnownOpaqueRect));
  EXPECT_EQ(LayoutRect(0, 0, 140, 140),
            layout_box->BackgroundRect(kBackgroundClipRect));

  // Because it can scroll due to local attachment, the opaque local background
  // in #target6 is treated as padding box for the clip rect, but remains the
  // content box for the known opaque rect.
  layout_box = ToLayoutBox(GetLayoutObjectByElementId("target6"));
  EXPECT_EQ(LayoutRect(20, 20, 100, 100),
            layout_box->BackgroundRect(kBackgroundKnownOpaqueRect));
  EXPECT_EQ(LayoutRect(10, 10, 120, 120),
            layout_box->BackgroundRect(kBackgroundClipRect));
}

TEST_F(LayoutBoxTest, LocationContainer) {
  SetBodyInnerHTML(R"HTML(
    <div id='div'>
      <b>Inline content<img id='img'></b>
    </div>
    <table id='table'>
      <tbody id='tbody'>
        <tr id='row'>
          <td id='cell' style='width: 100px; height: 80px'></td>
        </tr>
      </tbody>
    </table>
  )HTML");

  const LayoutBox* body = GetDocument().body()->GetLayoutBox();
  const LayoutBox* div = ToLayoutBox(GetLayoutObjectByElementId("div"));
  const LayoutBox* img = ToLayoutBox(GetLayoutObjectByElementId("img"));
  const LayoutBox* table = ToLayoutBox(GetLayoutObjectByElementId("table"));
  const LayoutBox* tbody = ToLayoutBox(GetLayoutObjectByElementId("tbody"));
  const LayoutBox* row = ToLayoutBox(GetLayoutObjectByElementId("row"));
  const LayoutBox* cell = ToLayoutBox(GetLayoutObjectByElementId("cell"));

  EXPECT_EQ(body, div->LocationContainer());
  EXPECT_EQ(div, img->LocationContainer());
  EXPECT_EQ(body, table->LocationContainer());
  EXPECT_EQ(table, tbody->LocationContainer());
  EXPECT_EQ(tbody, row->LocationContainer());
  EXPECT_EQ(tbody, cell->LocationContainer());
}

TEST_F(LayoutBoxTest, TopLeftLocationFlipped) {
  SetBodyInnerHTML(R"HTML(
    <div style='width: 600px; height: 200px; writing-mode: vertical-rl'>
      <div id='box1' style='width: 100px'></div>
      <div id='box2' style='width: 200px'></div>
    </div>
  )HTML");

  const LayoutBox* box1 = ToLayoutBox(GetLayoutObjectByElementId("box1"));
  EXPECT_EQ(LayoutPoint(0, 0), box1->Location());
  EXPECT_EQ(LayoutPoint(500, 0), box1->PhysicalLocation());

  const LayoutBox* box2 = ToLayoutBox(GetLayoutObjectByElementId("box2"));
  EXPECT_EQ(LayoutPoint(100, 0), box2->Location());
  EXPECT_EQ(LayoutPoint(300, 0), box2->PhysicalLocation());
}

TEST_F(LayoutBoxTest, TableRowCellTopLeftLocationFlipped) {
  GetDocument().SetCompatibilityMode(Document::kQuirksMode);
  SetBodyInnerHTML(R"HTML(
    <div style='writing-mode: vertical-rl'>
      <table style='border-spacing: 0'>
        <thead><tr><td style='width: 50px'></td></tr></thead>
        <tbody>
          <tr id='row1'>
            <td id='cell1' style='width: 100px; height: 80px'></td>
          </tr>
          <tr id='row2'>
            <td id='cell2' style='width: 300px; height: 80px'></td>
          </tr>
        </tbody>
      </table>
    </div>
  )HTML");

  // location and physicalLocation of a table row or a table cell should be
  // relative to the containing section.

  const LayoutBox* row1 = ToLayoutBox(GetLayoutObjectByElementId("row1"));
  EXPECT_EQ(LayoutPoint(0, 0), row1->Location());
  EXPECT_EQ(LayoutPoint(300, 0), row1->PhysicalLocation());

  const LayoutBox* cell1 = ToLayoutBox(GetLayoutObjectByElementId("cell1"));
  EXPECT_EQ(LayoutPoint(0, 0), cell1->Location());
  EXPECT_EQ(LayoutPoint(300, 0), cell1->PhysicalLocation());

  const LayoutBox* row2 = ToLayoutBox(GetLayoutObjectByElementId("row2"));
  EXPECT_EQ(LayoutPoint(100, 0), row2->Location());
  EXPECT_EQ(LayoutPoint(0, 0), row2->PhysicalLocation());

  const LayoutBox* cell2 = ToLayoutBox(GetLayoutObjectByElementId("cell2"));
  EXPECT_EQ(LayoutPoint(100, 0), cell2->Location());
  EXPECT_EQ(LayoutPoint(0, 0), cell2->PhysicalLocation());
}

TEST_F(LayoutBoxTest, LocationContainerOfSVG) {
  SetBodyInnerHTML(R"HTML(
    <svg id='svg' style='writing-mode:vertical-rl' width='500' height='500'>
      <foreignObject x='44' y='77' width='100' height='80' id='foreign'>
        <div id='child' style='width: 33px; height: 55px'>
        </div>
      </foreignObject>
    </svg>
  )HTML");
  const LayoutBox* svg_root = ToLayoutBox(GetLayoutObjectByElementId("svg"));
  const LayoutBox* foreign = ToLayoutBox(GetLayoutObjectByElementId("foreign"));
  const LayoutBox* child = ToLayoutBox(GetLayoutObjectByElementId("child"));

  EXPECT_EQ(GetDocument().body()->GetLayoutObject(),
            svg_root->LocationContainer());

  // The foreign object's location is not affected by SVGRoot's writing-mode.
  EXPECT_FALSE(foreign->LocationContainer());
  EXPECT_EQ(LayoutRect(44, 77, 100, 80), foreign->FrameRect());
  EXPECT_EQ(LayoutPoint(44, 77), foreign->PhysicalLocation());
  // The writing mode style should be still be inherited.
  EXPECT_TRUE(foreign->HasFlippedBlocksWritingMode());

  // The child of the foreign object is affected by writing-mode.
  EXPECT_EQ(foreign, child->LocationContainer());
  EXPECT_EQ(LayoutRect(0, 0, 33, 55), child->FrameRect());
  EXPECT_EQ(LayoutPoint(67, 0), child->PhysicalLocation());
  EXPECT_TRUE(child->HasFlippedBlocksWritingMode());
}

TEST_F(LayoutBoxTest, ControlClip) {
  SetBodyInnerHTML(R"HTML(
    <style>
      * { margin: 0; }
      #target {
        position: relative;
        width: 100px; height: 50px;
      }
    </style>
    <input id='target' type='button' value='some text'/>
  )HTML");
  LayoutBox* target = ToLayoutBox(GetLayoutObjectByElementId("target"));
  EXPECT_TRUE(target->HasControlClip());
  EXPECT_TRUE(target->HasClipRelatedProperty());
  EXPECT_TRUE(target->ShouldClipOverflow());
#if defined(OS_MACOSX)
  EXPECT_EQ(LayoutRect(0, 0, 100, 18), target->ClippingRect(LayoutPoint()));
#else
  EXPECT_EQ(LayoutRect(2, 2, 96, 46), target->ClippingRect(LayoutPoint()));
#endif
}

TEST_F(LayoutBoxTest, LocalVisualRectWithMask) {
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  SetBodyInnerHTML(R"HTML(
    <div id='target' style='-webkit-mask-image: url(#a);
         width: 100px; height: 100px; background: blue'>
      <div style='width: 300px; height: 10px; background: green'></div>
    </div>
  )HTML");

  LayoutBox* target = ToLayoutBox(GetLayoutObjectByElementId("target"));
  EXPECT_TRUE(target->HasMask());
  EXPECT_EQ(LayoutRect(0, 0, 100, 100), target->LocalVisualRect());
  EXPECT_EQ(LayoutRect(0, 0, 100, 100), target->VisualOverflowRect());
}

TEST_F(LayoutBoxTest, LocalVisualRectWithMaskAndOverflowClip) {
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  SetBodyInnerHTML(R"HTML(
    <div id='target' style='-webkit-mask-image: url(#a); overflow: hidden;
         width: 100px; height: 100px; background: blue'>
      <div style='width: 300px; height: 10px; background: green'></div>
    </div>
  )HTML");

  LayoutBox* target = ToLayoutBox(GetLayoutObjectByElementId("target"));
  EXPECT_TRUE(target->HasMask());
  EXPECT_TRUE(target->HasOverflowClip());
  EXPECT_EQ(LayoutRect(0, 0, 100, 100), target->LocalVisualRect());
  EXPECT_EQ(LayoutRect(0, 0, 100, 100), target->VisualOverflowRect());
}

TEST_F(LayoutBoxTest, LocalVisualRectWithMaskWithOutset) {
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  SetBodyInnerHTML(R"HTML(
    <div id='target' style='-webkit-mask-box-image-source: url(#a);
    -webkit-mask-box-image-outset: 10px 20px;
         width: 100px; height: 100px; background: blue'>
      <div style='width: 300px; height: 10px; background: green'></div>
    </div>
  )HTML");

  LayoutBox* target = ToLayoutBox(GetLayoutObjectByElementId("target"));
  EXPECT_TRUE(target->HasMask());
  EXPECT_EQ(LayoutRect(-20, -10, 140, 120), target->LocalVisualRect());
  EXPECT_EQ(LayoutRect(-20, -10, 140, 120), target->VisualOverflowRect());
}

TEST_F(LayoutBoxTest, LocalVisualRectWithMaskWithOutsetAndOverflowClip) {
  if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled())
    return;

  SetBodyInnerHTML(R"HTML(
    <div id='target' style='-webkit-mask-box-image-source: url(#a);
    -webkit-mask-box-image-outset: 10px 20px; overflow: hidden;
         width: 100px; height: 100px; background: blue'>
      <div style='width: 300px; height: 10px; background: green'></div>
    </div>
  )HTML");

  LayoutBox* target = ToLayoutBox(GetLayoutObjectByElementId("target"));
  EXPECT_TRUE(target->HasMask());
  EXPECT_TRUE(target->HasOverflowClip());
  EXPECT_EQ(LayoutRect(-20, -10, 140, 120), target->LocalVisualRect());
  EXPECT_EQ(LayoutRect(-20, -10, 140, 120), target->VisualOverflowRect());
}

TEST_F(LayoutBoxTest, ContentsVisualOverflowPropagation) {
  SetBodyInnerHTML(R"HTML(
    <style>
      div { width: 100px; height: 100px }
    </style>
    <div id='a'>
      <div style='height: 50px'></div>
      <div id='b' style='writing-mode: vertical-rl; margin-left: 60px'>
        <div style='width: 30px'></div>
        <div id='c' style='margin-top: 40px'>
          <div style='width: 10px'></div>
          <div style='margin-top: 20px; margin-left: 10px'></div>
        </div>
        <div id='d' style='writing-mode: vertical-lr; margin-top: 40px'>
          <div style='width: 10px'></div>
          <div style='margin-top: 20px'></div>
        </div>
      </div>
    </div>
  )HTML");

  auto* c = ToLayoutBox(GetLayoutObjectByElementId("c"));
  EXPECT_EQ(LayoutRect(0, 0, 100, 100), c->SelfVisualOverflowRect());
  EXPECT_EQ(LayoutRect(10, 20, 100, 100), c->ContentsVisualOverflowRect());
  EXPECT_EQ(LayoutRect(0, 0, 110, 120), c->VisualOverflowRect());
  // C and its parent b have the same blocks direction.
  EXPECT_EQ(LayoutRect(0, 0, 110, 120), c->VisualOverflowRectForPropagation());

  auto* d = ToLayoutBox(GetLayoutObjectByElementId("d"));
  EXPECT_EQ(LayoutRect(0, 0, 100, 100), d->SelfVisualOverflowRect());
  EXPECT_EQ(LayoutRect(10, 20, 100, 100), d->ContentsVisualOverflowRect());
  EXPECT_EQ(LayoutRect(0, 0, 110, 120), d->VisualOverflowRect());
  // D and its parent b have different blocks direction.
  EXPECT_EQ(LayoutRect(-10, 0, 110, 120),
            d->VisualOverflowRectForPropagation());

  auto* b = ToLayoutBox(GetLayoutObjectByElementId("b"));
  EXPECT_EQ(LayoutRect(0, 0, 100, 100), b->SelfVisualOverflowRect());
  // Union of VisualOverflowRectForPropagations offset by locations of c and d.
  EXPECT_EQ(LayoutRect(30, 40, 200, 120), b->ContentsVisualOverflowRect());
  EXPECT_EQ(LayoutRect(0, 0, 230, 160), b->VisualOverflowRect());
  // B and its parent A have different blocks direction.
  EXPECT_EQ(LayoutRect(-130, 0, 230, 160),
            b->VisualOverflowRectForPropagation());

  auto* a = ToLayoutBox(GetLayoutObjectByElementId("a"));
  EXPECT_EQ(LayoutRect(0, 0, 100, 100), a->SelfVisualOverflowRect());
  EXPECT_EQ(LayoutRect(-70, 50, 230, 160), a->ContentsVisualOverflowRect());
  EXPECT_EQ(LayoutRect(-70, 0, 230, 210), a->VisualOverflowRect());
}

TEST_F(LayoutBoxTest, HitTestContainPaint) {
  SetBodyInnerHTML(R"HTML(
    <div id='container' style='width: 100px; height: 200px; contain: paint'>
      <div id='child' style='width: 300px; height: 400px;'></div>
    </div>
  )HTML");

  auto* child = GetDocument().getElementById("child");
  EXPECT_EQ(GetDocument().documentElement(), HitTest(1, 1));
  EXPECT_EQ(child, HitTest(10, 10));
  EXPECT_EQ(GetDocument().FirstBodyElement(), HitTest(150, 10));
  EXPECT_EQ(GetDocument().documentElement(), HitTest(10, 250));
}

class AnimatedImage : public StubImage {
 public:
  bool MaybeAnimated() override { return true; }
};

TEST_F(LayoutBoxTest, DeferredInvalidation) {
  SetBodyInnerHTML("<img id='image' style='width: 100px; height: 100px;'/>");
  auto* obj = ToLayoutBox(GetLayoutObjectByElementId("image"));
  ASSERT_TRUE(obj);

  // Inject an animated image since deferred invalidations are only done for
  // animated images.
  auto* image =
      ImageResourceContent::CreateLoaded(base::AdoptRef(new AnimatedImage()));
  ToLayoutImage(obj)->ImageResource()->SetImageResource(image);
  ASSERT_TRUE(ToLayoutImage(obj)->CachedImage()->GetImage()->MaybeAnimated());

  // CanDeferInvalidation::kYes results in a deferred invalidation.
  obj->ClearPaintInvalidationFlags();
  EXPECT_EQ(obj->FullPaintInvalidationReason(), PaintInvalidationReason::kNone);
  obj->ImageChanged(image, ImageResourceObserver::CanDeferInvalidation::kYes);
  EXPECT_EQ(obj->FullPaintInvalidationReason(),
            PaintInvalidationReason::kDelayedFull);

  // CanDeferInvalidation::kNo results in a immediate invalidation.
  obj->ClearPaintInvalidationFlags();
  EXPECT_EQ(obj->FullPaintInvalidationReason(), PaintInvalidationReason::kNone);
  obj->ImageChanged(image, ImageResourceObserver::CanDeferInvalidation::kNo);
  EXPECT_EQ(obj->FullPaintInvalidationReason(),
            PaintInvalidationReason::kImage);
}

TEST_F(LayoutBoxTest, MarkerContainerLayoutOverflowRect) {
  SetBodyInnerHTML(R"HTML(
    <style>
      html { font-size: 16px; }
    </style>
    <div id='target' style='display: list-item;'>
      <div style='overflow: hidden; line-height:100px;'>hello</div>
    </div>
  )HTML");

  LayoutBox* marker_container =
      ToLayoutBox(GetLayoutObjectByElementId("target")->SlowFirstChild());
  // Unit marker_container's frame_rect which y-pos starts from 0 and marker's
  // frame_rect.
  EXPECT_TRUE(marker_container->LayoutOverflowRect().Height() > LayoutUnit(50));
}

}  // namespace blink
