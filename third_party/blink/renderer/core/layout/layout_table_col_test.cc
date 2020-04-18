// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/layout_table_col.h"

#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {

using LayoutTableColTest = RenderingTest;

TEST_F(LayoutTableColTest, LocalVisualRectSPv1) {
  ScopedSlimmingPaintV175ForTest spv175(false);
  SetBodyInnerHTML(R"HTML(
    <table id='table' style='width: 200px; height: 200px'>
      <col id='col1' style='visibility: hidden'>
      <col id='col2' style='visibility: collapse'>
      <col id='col3'>
      <tr><td></td><td></td></tr>
    </table>
  )HTML");

  auto table_local_visual_rect =
      GetLayoutObjectByElementId("table")->LocalVisualRect();
  EXPECT_NE(LayoutRect(), table_local_visual_rect);
  EXPECT_EQ(table_local_visual_rect,
            GetLayoutObjectByElementId("col1")->LocalVisualRect());
  EXPECT_EQ(table_local_visual_rect,
            GetLayoutObjectByElementId("col2")->LocalVisualRect());
  EXPECT_EQ(table_local_visual_rect,
            GetLayoutObjectByElementId("col3")->LocalVisualRect());
}

TEST_F(LayoutTableColTest, LocalVisualRectSPv175) {
  ScopedSlimmingPaintV175ForTest spv175(true);
  SetBodyInnerHTML(R"HTML(
    <table style='width: 200px; height: 200px'>
      <col id='col1' style='visibility: hidden'>
      <col id='col2' style='visibility: collapse'>
      <col id='col3'>
      <tr><td></td><td></td></tr>
    </table>
  )HTML");

  EXPECT_EQ(LayoutRect(),
            GetLayoutObjectByElementId("col1")->LocalVisualRect());
  EXPECT_EQ(LayoutRect(),
            GetLayoutObjectByElementId("col2")->LocalVisualRect());
  EXPECT_EQ(LayoutRect(),
            GetLayoutObjectByElementId("col3")->LocalVisualRect());
}

}  // namespace blink
