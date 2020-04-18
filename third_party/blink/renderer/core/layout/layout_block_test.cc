// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/element_traversal.h"
#include "third_party/blink/renderer/core/layout/layout_block.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

namespace blink {

class LayoutBlockTest : public RenderingTest {};

TEST_F(LayoutBlockTest, LayoutNameCalledWithNullStyle) {
  LayoutObject* obj = LayoutBlockFlow::CreateAnonymous(&GetDocument());
  EXPECT_FALSE(obj->Style());
  EXPECT_STREQ("LayoutBlockFlow (anonymous)",
               obj->DecoratedName().Ascii().data());
  obj->Destroy();
}

TEST_F(LayoutBlockTest, WidthAvailableToChildrenChanged) {
  ScopedOverlayScrollbarsForTest overlay_scrollbars(false);
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <div id='list' style='overflow-y:auto; width:150px; height:100px'>
      <div style='height:20px'>Item</div>
      <div style='height:20px'>Item</div>
      <div style='height:20px'>Item</div>
      <div style='height:20px'>Item</div>
      <div style='height:20px'>Item</div>
      <div style='height:20px'>Item</div>
    </div>
  )HTML");
  Element* list_element = GetDocument().getElementById("list");
  ASSERT_TRUE(list_element);
  LayoutBox* list_box = ToLayoutBox(list_element->GetLayoutObject());
  Element* item_element = ElementTraversal::FirstChild(*list_element);
  ASSERT_TRUE(item_element);
  ASSERT_GT(list_box->VerticalScrollbarWidth(), 0);
  ASSERT_EQ(item_element->OffsetWidth(),
            150 - list_box->VerticalScrollbarWidth());

  DummyExceptionStateForTesting exception_state;
  list_element->style()->setCSSText(&GetDocument(), "width:150px;height:100px;",
                                    exception_state);
  ASSERT_FALSE(exception_state.HadException());
  GetDocument().View()->UpdateAllLifecyclePhases();
  ASSERT_EQ(list_box->VerticalScrollbarWidth(), 0);
  ASSERT_EQ(item_element->OffsetWidth(), 150);
}

TEST_F(LayoutBlockTest, OverflowWithTransformAndPerspective) {
  SetBodyInnerHTML(R"HTML(
    <div id='target' style='width: 100px; height: 100px; overflow: scroll;
        perspective: 200px;'>
      <div style='transform: rotateY(-45deg); width: 140px; height: 100px'>
      </div>
    </div>
  )HTML");
  LayoutBox* scroller =
      ToLayoutBox(GetDocument().getElementById("target")->GetLayoutObject());
  EXPECT_EQ(119.5, scroller->LayoutOverflowRect().Width().ToFloat());
}

}  // namespace blink
