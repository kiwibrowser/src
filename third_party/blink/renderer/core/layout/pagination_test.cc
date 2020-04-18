// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/core/layout/layout_multi_column_flow_thread.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"

namespace blink {

class PaginationStrutTest : public RenderingTest {
 public:
  int StrutForBox(const char* block_id) {
    LayoutObject* layout_object = GetLayoutObjectByElementId(block_id);
    if (!layout_object || !layout_object->IsBox())
      return std::numeric_limits<int>::min();
    LayoutBox* box = ToLayoutBox(layout_object);
    return box->PaginationStrut().ToInt();
  }

  int StrutForLine(const char* container_id, int line_index) {
    LayoutObject* layout_object = GetLayoutObjectByElementId(container_id);
    if (!layout_object || !layout_object->IsLayoutBlockFlow())
      return std::numeric_limits<int>::min();
    LayoutBlockFlow* block = ToLayoutBlockFlow(layout_object);
    if (block->MultiColumnFlowThread())
      block = block->MultiColumnFlowThread();
    for (RootInlineBox *line = block->FirstRootBox(); line;
         line = line->NextRootBox(), line_index--) {
      if (line_index)
        continue;
      return line->PaginationStrut().ToInt();
    }
    return std::numeric_limits<int>::min();
  }
};

TEST_F(PaginationStrutTest, LineWithStrut) {
  SetBodyInnerHTML(R"HTML(
    <div id='paged' style='overflow:-webkit-paged-y; height:200px;
    line-height:150px;'>
        line1<br>
        line2<br>
    </div>
  )HTML");
  EXPECT_EQ(0, StrutForLine("paged", 0));
  EXPECT_EQ(50, StrutForLine("paged", 1));
}

TEST_F(PaginationStrutTest, BlockWithStrut) {
  SetBodyInnerHTML(R"HTML(
    <div style='overflow:-webkit-paged-y; height:200px; line-height:150px;'>
        <div id='block1'>line1</div>
        <div id='block2'>line2</div>
    </div>
  )HTML");
  EXPECT_EQ(0, StrutForBox("block1"));
  EXPECT_EQ(0, StrutForLine("block1", 0));
  EXPECT_EQ(50, StrutForBox("block2"));
  EXPECT_EQ(0, StrutForLine("block2", 0));
}

TEST_F(PaginationStrutTest, FloatWithStrut) {
  SetBodyInnerHTML(R"HTML(
    <div style='overflow:-webkit-paged-y; height:200px; line-height:150px;'>
        <div style='height:120px;'></div>
        <div id='float' style='float:left;'>line</div>
    </div>
  )HTML");
  EXPECT_EQ(80, StrutForBox("float"));
  EXPECT_EQ(0, StrutForLine("float", 0));
}

TEST_F(PaginationStrutTest, UnbreakableBlockWithStrut) {
  SetBodyInnerHTML(R"HTML(
    <style>img { display:block; width:100px; height:150px; outline:4px
    solid blue; padding:0; border:none; margin:0; }</style>
    <div style='overflow:-webkit-paged-y; height:200px;'>
        <img id='img1'>
        <img id='img2'>
    </div>
  )HTML");
  EXPECT_EQ(0, StrutForBox("img1"));
  EXPECT_EQ(50, StrutForBox("img2"));
}

TEST_F(PaginationStrutTest, BreakBefore) {
  SetBodyInnerHTML(R"HTML(
    <div style='overflow:-webkit-paged-y; height:400px; line-height:50px;'>
        <div id='block1'>line1</div>
        <div id='block2' style='break-before:page;'>line2</div>
    </div>
  )HTML");
  EXPECT_EQ(0, StrutForBox("block1"));
  EXPECT_EQ(0, StrutForLine("block1", 0));
  EXPECT_EQ(350, StrutForBox("block2"));
  EXPECT_EQ(0, StrutForLine("block2", 0));
}

TEST_F(PaginationStrutTest, BlockWithStrutPropagatedFromInnerBlock) {
  SetBodyInnerHTML(R"HTML(
    <div style='overflow:-webkit-paged-y; height:200px; line-height:150px;'>
        <div id='block1'>line1</div>
        <div id='block2' style='padding-top:2px;'>
            <div id='innerBlock' style='padding-top:2px;'>line2</div>
        </div>
    </div>
  )HTML");
  EXPECT_EQ(0, StrutForBox("block1"));
  EXPECT_EQ(0, StrutForLine("block1", 0));
  EXPECT_EQ(50, StrutForBox("block2"));
  EXPECT_EQ(0, StrutForBox("innerBlock"));
  EXPECT_EQ(0, StrutForLine("innerBlock", 0));
}

TEST_F(PaginationStrutTest, BlockWithStrutPropagatedFromUnbreakableInnerBlock) {
  SetBodyInnerHTML(R"HTML(
    <div style='overflow:-webkit-paged-y; height:400px; line-height:150px;'>
        <div id='block1'>line1</div>
        <div id='block2' style='padding-top:2px;'>
            <div id='innerBlock' style='padding-top:2px;
    break-inside:avoid;'>
                line2<br>
                line3<br>
            </div>
        </div>
    </div>
  )HTML");
  EXPECT_EQ(0, StrutForBox("block1"));
  EXPECT_EQ(0, StrutForLine("block1", 0));
  EXPECT_EQ(250, StrutForBox("block2"));
  EXPECT_EQ(0, StrutForBox("innerBlock"));
  EXPECT_EQ(0, StrutForLine("innerBlock", 0));
  EXPECT_EQ(0, StrutForLine("innerBlock", 1));
}

TEST_F(PaginationStrutTest, InnerBlockWithBreakBefore) {
  SetBodyInnerHTML(R"HTML(
    <div style='overflow:-webkit-paged-y; height:200px; line-height:150px;'>
        <div id='block1'>line1</div>
        <div id='block2' style='padding-top:2px;'>
            <div id='innerBlock' style='padding-top:2px;
    break-before:page;'>line2</div>
        </div>
    </div>
  )HTML");
  EXPECT_EQ(0, StrutForBox("block1"));
  EXPECT_EQ(0, StrutForLine("block1", 0));
  // There's no class A break point before #innerBlock (they only exist
  // *between* siblings), so the break is propagated and applied before #block2.
  EXPECT_EQ(50, StrutForBox("block2"));
  EXPECT_EQ(0, StrutForBox("innerBlock"));
  EXPECT_EQ(0, StrutForLine("innerBlock", 0));
}

}  // namespace blink
