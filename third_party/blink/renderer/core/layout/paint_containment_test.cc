// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/layout/layout_block.h"
#include "third_party/blink/renderer/core/layout/layout_inline.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"

namespace blink {

class PaintContainmentTest : public RenderingTest {
 private:
  void SetUp() override {
    RenderingTest::SetUp();
    EnableCompositing();
  }
};

static void CheckIsClippingStackingContextAndContainer(
    LayoutBoxModelObject& obj) {
  EXPECT_TRUE(obj.CanContainFixedPositionObjects());
  EXPECT_TRUE(obj.HasClipRelatedProperty());
  EXPECT_TRUE(obj.Style()->ContainsPaint());

  // TODO(leviw): Ideally, we wouldn't require a paint layer to handle the
  // clipping and stacking performed by paint containment.
  DCHECK(obj.Layer());
  PaintLayer* layer = obj.Layer();
  EXPECT_TRUE(layer->StackingNode() &&
              layer->StackingNode()->IsStackingContext());
}

TEST_F(PaintContainmentTest, BlockPaintContainment) {
  SetBodyInnerHTML("<div id='div' style='contain: paint'></div>");
  Element* div = GetDocument().getElementById(AtomicString("div"));
  DCHECK(div);
  LayoutObject* obj = div->GetLayoutObject();
  DCHECK(obj);
  DCHECK(obj->IsLayoutBlock());
  LayoutBlock& block = ToLayoutBlock(*obj);
  EXPECT_TRUE(block.CreatesNewFormattingContext());
  EXPECT_FALSE(block.CanBeScrolledAndHasScrollableArea());
  CheckIsClippingStackingContextAndContainer(block);
}

TEST_F(PaintContainmentTest, InlinePaintContainment) {
  SetBodyInnerHTML(
      "<div><span id='test' style='contain: paint'>Foo</span></div>");
  Element* span = GetDocument().getElementById(AtomicString("test"));
  DCHECK(span);
  // The inline should have been coerced into a block in StyleAdjuster.
  LayoutObject* obj = span->GetLayoutObject();
  DCHECK(obj);
  DCHECK(obj->IsLayoutBlock());
  LayoutBlock& layout_block = ToLayoutBlock(*obj);
  CheckIsClippingStackingContextAndContainer(layout_block);
}

TEST_F(PaintContainmentTest, SvgWithContainmentShouldNotCrash) {
  // SVG doesn't currently support PaintLayers and should not crash with
  // layer-related properties.
  SetBodyInnerHTML("<svg><text y='20' style='contain: paint'>Foo</text></svg>");
  SetBodyInnerHTML(
      "<svg><foreignObject style='contain: paint'>Foo</foreignObject></svg>");
  SetBodyInnerHTML(
      "<svg><foreignObject><span style='contain: "
      "paint'>Foo</span></foreignObject></svg>");
}

}  // namespace blink
