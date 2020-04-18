// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/svg/layout_svg_root.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/layout/svg/layout_svg_shape.h"
#include "third_party/blink/renderer/core/layout/svg/svg_layout_support.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"

namespace blink {

using LayoutSVGRootTest = RenderingTest;

TEST_F(LayoutSVGRootTest, VisualRectMappingWithoutViewportClipWithBorder) {
  SetBodyInnerHTML(R"HTML(
    <svg id='root' style='border: 10px solid red; width: 200px; height:
    100px; overflow: visible' viewBox='0 0 200 100'>
       <rect id='rect' x='80' y='80' width='100' height='100'/>
    </svg>
  )HTML");

  const LayoutSVGRoot& root =
      *ToLayoutSVGRoot(GetLayoutObjectByElementId("root"));
  const LayoutSVGShape& svg_rect =
      *ToLayoutSVGShape(GetLayoutObjectByElementId("rect"));

  LayoutRect rect = SVGLayoutSupport::VisualRectInAncestorSpace(svg_rect, root);
  // (80, 80, 100, 100) added by root's content rect offset from border rect,
  // not clipped.
  EXPECT_EQ(LayoutRect(90, 90, 100, 100), rect);

  LayoutRect root_visual_rect =
      static_cast<const LayoutObject&>(root).LocalVisualRect();
  // SVG root's overflow includes overflow from descendants.
  EXPECT_EQ(LayoutRect(0, 0, 220, 190), root_visual_rect);

  rect = root_visual_rect;
  EXPECT_TRUE(root.MapToVisualRectInAncestorSpace(&root, rect));
  EXPECT_EQ(LayoutRect(0, 0, 220, 190), rect);
}

TEST_F(LayoutSVGRootTest, VisualRectMappingWithViewportClipAndBorder) {
  SetBodyInnerHTML(R"HTML(
    <svg id='root' style='border: 10px solid red; width: 200px; height:
    100px; overflow: hidden' viewBox='0 0 200 100'>
       <rect id='rect' x='80' y='80' width='100' height='100'/>
    </svg>
  )HTML");

  const LayoutSVGRoot& root =
      *ToLayoutSVGRoot(GetLayoutObjectByElementId("root"));
  const LayoutSVGShape& svg_rect =
      *ToLayoutSVGShape(GetLayoutObjectByElementId("rect"));

  LayoutRect rect = SVGLayoutSupport::VisualRectInAncestorSpace(svg_rect, root);
  // (80, 80, 100, 100) added by root's content rect offset from border rect,
  // clipped by (10, 10, 200, 100).
  EXPECT_EQ(LayoutRect(90, 90, 100, 20), rect);

  LayoutRect root_visual_rect =
      static_cast<const LayoutObject&>(root).LocalVisualRect();
  // SVG root with overflow:hidden doesn't include overflow from children, just
  // border box rect.
  EXPECT_EQ(LayoutRect(0, 0, 220, 120), root_visual_rect);

  rect = root_visual_rect;
  EXPECT_TRUE(root.MapToVisualRectInAncestorSpace(&root, rect));
  // LayoutSVGRoot should not apply overflow clip on its own rect.
  EXPECT_EQ(LayoutRect(0, 0, 220, 120), rect);
}

TEST_F(LayoutSVGRootTest, VisualRectMappingWithViewportClipWithoutBorder) {
  SetBodyInnerHTML(R"HTML(
    <svg id='root' style='width: 200px; height: 100px; overflow: hidden'
    viewBox='0 0 200 100'>
       <rect id='rect' x='80' y='80' width='100' height='100'/>
    </svg>
  )HTML");

  const LayoutSVGRoot& root =
      *ToLayoutSVGRoot(GetLayoutObjectByElementId("root"));
  const LayoutSVGShape& svg_rect =
      *ToLayoutSVGShape(GetLayoutObjectByElementId("rect"));

  LayoutRect rect = SVGLayoutSupport::VisualRectInAncestorSpace(svg_rect, root);
  // (80, 80, 100, 100) clipped by (0, 0, 200, 100).
  EXPECT_EQ(LayoutRect(80, 80, 100, 20), rect);

  LayoutRect root_visual_rect =
      static_cast<const LayoutObject&>(root).LocalVisualRect();
  // SVG root doesn't have box decoration background, so just use clipped
  // overflow of children.
  EXPECT_EQ(LayoutRect(80, 80, 100, 20), root_visual_rect);

  rect = root_visual_rect;
  EXPECT_TRUE(root.MapToVisualRectInAncestorSpace(&root, rect));
  EXPECT_EQ(LayoutRect(80, 80, 100, 20), rect);
}

TEST_F(LayoutSVGRootTest,
       PaintedOutputOfObjectHasNoEffectRegardlessOfSizeEmpty) {
  SetBodyInnerHTML(R"HTML(
    <svg id="svg" width="100.1%" height="16">
      <rect width="100%" height="16" fill="#fff"></rect>
    </svg>
  )HTML");

  const LayoutSVGRoot& root =
      *ToLayoutSVGRoot(GetLayoutObjectByElementId("svg"));
  EXPECT_TRUE(root.PaintedOutputOfObjectHasNoEffectRegardlessOfSize());
}

TEST_F(LayoutSVGRootTest,
       PaintedOutputOfObjectHasNoEffectRegardlessOfSizeMask) {
  SetBodyInnerHTML(R"HTML(
    <svg id="svg" width="16" height="16" mask="url(#test)">
      <rect width="100%" height="16" fill="#fff"></rect>
      <defs>
        <mask id="test">
          <g>
            <rect width="100%" height="100%" fill="#ffffff" style=""></rect>
          </g>
        </mask>
      </defs>
    </svg>
  )HTML");

  const LayoutSVGRoot& root =
      *ToLayoutSVGRoot(GetLayoutObjectByElementId("svg"));
  EXPECT_FALSE(root.PaintedOutputOfObjectHasNoEffectRegardlessOfSize());
}

}  // namespace blink
