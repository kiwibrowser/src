// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment_traversal.h"

#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment.h"
#include "third_party/blink/renderer/core/testing/core_unit_test_helper.h"

namespace blink {

class NGPaintFragmentTraversalTest : public RenderingTest,
                                     private ScopedLayoutNGForTest {
 public:
  NGPaintFragmentTraversalTest()
      : RenderingTest(nullptr), ScopedLayoutNGForTest(true) {}

 protected:
  void SetUpHtml(const char* container_id, const char* html) {
    SetBodyInnerHTML(html);
    layout_block_flow_ =
        ToLayoutBlockFlow(GetLayoutObjectByElementId(container_id));
    root_fragment_ = layout_block_flow_->PaintFragment();
  }

  const Vector<std::unique_ptr<NGPaintFragment>>& RootChildren() const {
    return root_fragment_->Children();
  }

  LayoutBlockFlow* layout_block_flow_;
  NGPaintFragment* root_fragment_;
};

TEST_F(NGPaintFragmentTraversalTest, PreviousLineOf) {
  SetUpHtml("t", "<div id=t>foo<br>bar</div>");
  ASSERT_EQ(2u, RootChildren().size());
  EXPECT_EQ(nullptr,
            NGPaintFragmentTraversal::PreviousLineOf(*RootChildren()[0]));
  EXPECT_EQ(RootChildren()[0].get(),
            NGPaintFragmentTraversal::PreviousLineOf(*RootChildren()[1]));
}

TEST_F(NGPaintFragmentTraversalTest, PreviousLineInListItem) {
  SetUpHtml("t", "<ul><li id=t>foo</li></ul>");
  ASSERT_EQ(2u, RootChildren().size());
  ASSERT_TRUE(RootChildren()[0]->PhysicalFragment().IsListMarker());
  EXPECT_EQ(nullptr,
            NGPaintFragmentTraversal::PreviousLineOf(*RootChildren()[1]));
}

TEST_F(NGPaintFragmentTraversalTest, InlineDescendantsOf) {
  SetUpHtml("t",
            "<ul>"
            "<li id=t style='position: absolute'>"
            "<span style='float: right'>float</span>"
            "<span style='position: absolute'>oof</span>"
            "text<br>"
            "<span style='display: inline-block'>inline block</span>"
            "</li>"
            "</ul>");

  // Tests that floats, out-of-flow positioned and descendants of atomic inlines
  // are excluded.
  auto descendants =
      NGPaintFragmentTraversal::InlineDescendantsOf(*root_fragment_);
  ASSERT_EQ(6u, descendants.size());
  // TODO(layout-dev): This list marker is not in any line box. Should it be
  // treated as inline?
  EXPECT_TRUE(descendants[0].fragment->PhysicalFragment().IsListMarker());
  EXPECT_TRUE(descendants[1].fragment->PhysicalFragment().IsLineBox());
  EXPECT_TRUE(descendants[2].fragment->PhysicalFragment().IsText());  // "text"
  EXPECT_TRUE(descendants[3].fragment->PhysicalFragment().IsText());  // "br"
  EXPECT_TRUE(descendants[4].fragment->PhysicalFragment().IsLineBox());
  EXPECT_TRUE(descendants[5].fragment->PhysicalFragment().IsAtomicInline());
}

}  // namespace blink
