// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/fragment_data.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"

namespace blink {

class FragmentDataTest : public RenderingTest {
 protected:
  bool HasRareData(const FragmentData& data) { return !!data.rare_data_; }
};

TEST_F(FragmentDataTest, LocationInBackingAndSelectionVisualRect) {
  FragmentData fragment;

  // Default LocationInBacking and SelectionVisualRect should not create
  // RareData.
  fragment.SetVisualRect(LayoutRect(10, 20, 30, 400));
  fragment.SetLocationInBacking(LayoutPoint(10, 20));
  fragment.SetSelectionVisualRect(LayoutRect());
  EXPECT_FALSE(HasRareData(fragment));
  EXPECT_EQ(LayoutPoint(10, 20), fragment.LocationInBacking());
  EXPECT_EQ(LayoutRect(), fragment.SelectionVisualRect());

  // Non-Default LocationInBacking and SelectionVisualRect create RareData.
  fragment.SetLocationInBacking(LayoutPoint(20, 30));
  fragment.SetSelectionVisualRect(LayoutRect(1, 2, 3, 4));
  EXPECT_TRUE(HasRareData(fragment));
  EXPECT_EQ(LayoutPoint(20, 30), fragment.LocationInBacking());
  EXPECT_EQ(LayoutRect(1, 2, 3, 4), fragment.SelectionVisualRect());

  // PaintProperties should store default LocationInBacking and
  // SelectionVisualRect once it's created.
  fragment.SetLocationInBacking(LayoutPoint(10, 20));
  fragment.SetSelectionVisualRect(LayoutRect());
  EXPECT_TRUE(HasRareData(fragment));
  EXPECT_EQ(LayoutPoint(10, 20), fragment.LocationInBacking());
  EXPECT_EQ(LayoutRect(), fragment.SelectionVisualRect());
}

TEST_F(FragmentDataTest, PreEffectClipProperties) {
  SetBodyInnerHTML(R"HTML(
    <style>
      #target {
        width: 400px; height: 400px; position: absolute;
        clip: rect(0, 50px, 100px, 0);
        clip-path: inset(0%);
      }
    </style>
    <div id='target'></div>
  )HTML");

  LayoutObject* target = GetLayoutObjectByElementId("target");
  const ObjectPaintProperties* properties =
      target->FirstFragment().PaintProperties();
  EXPECT_TRUE(properties->ClipPathClip());
  EXPECT_TRUE(properties->CssClip());
  EXPECT_EQ(properties->ClipPathClip(), properties->CssClip()->Parent());
  EXPECT_EQ(properties->ClipPathClip()->Parent(),
            target->FirstFragment().PreEffectProperties().Clip());
}

}  // namespace blink
