// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_node.h"
#include "third_party/blink/renderer/core/layout/ng/ng_block_layout_algorithm.h"
#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space_builder.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_result.h"
#include "third_party/blink/renderer/core/testing/sim/sim_compositor.h"
#include "third_party/blink/renderer/core/testing/sim/sim_request.h"
#include "third_party/blink/renderer/core/testing/sim/sim_test.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/text/character_names.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class NGInlineLayoutTest : public SimTest {
 public:
  scoped_refptr<NGConstraintSpace> ConstraintSpaceForElement(
      LayoutBlockFlow* block_flow) {
    return NGConstraintSpaceBuilder(
               block_flow->Style()->GetWritingMode(),
               /* icb_size */ {NGSizeIndefinite, NGSizeIndefinite})
        .SetAvailableSize(NGLogicalSize(LayoutUnit(), LayoutUnit()))
        .SetPercentageResolutionSize(NGLogicalSize(LayoutUnit(), LayoutUnit()))
        .SetTextDirection(block_flow->Style()->Direction())
        .ToConstraintSpace(block_flow->Style()->GetWritingMode());
  }
};

TEST_F(NGInlineLayoutTest, BlockWithSingleTextNode) {
  ScopedLayoutNGForTest layout_ng(true);

  SimRequest main_resource("https://example.com/", "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete(
      "<div id=\"target\">Hello <strong>World</strong>!</div>");

  Compositor().BeginFrame();
  ASSERT_FALSE(Compositor().NeedsBeginFrame());

  Element* target = GetDocument().getElementById("target");
  LayoutBlockFlow* block_flow = ToLayoutBlockFlow(target->GetLayoutObject());
  scoped_refptr<NGConstraintSpace> constraint_space =
      ConstraintSpaceForElement(block_flow);
  NGBlockNode node(block_flow);

  scoped_refptr<NGLayoutResult> result =
      NGBlockLayoutAlgorithm(node, *constraint_space).Layout();
  EXPECT_TRUE(result);

  String expected_text("Hello World!");
  NGInlineNode first_child = ToNGInlineNode(node.FirstChild());
  EXPECT_EQ(expected_text,
            StringView(first_child.ItemsData(false).text_content, 0, 12));
}

TEST_F(NGInlineLayoutTest, BlockWithTextAndAtomicInline) {
  ScopedLayoutNGForTest layout_ng(true);

  SimRequest main_resource("https://example.com/", "text/html");
  LoadURL("https://example.com/");
  main_resource.Complete("<div id=\"target\">Hello <img>.</div>");

  Compositor().BeginFrame();
  ASSERT_FALSE(Compositor().NeedsBeginFrame());

  Element* target = GetDocument().getElementById("target");
  LayoutBlockFlow* block_flow = ToLayoutBlockFlow(target->GetLayoutObject());
  scoped_refptr<NGConstraintSpace> constraint_space =
      ConstraintSpaceForElement(block_flow);
  NGBlockNode node(block_flow);

  scoped_refptr<NGLayoutResult> result =
      NGBlockLayoutAlgorithm(node, *constraint_space).Layout();
  EXPECT_TRUE(result);

  String expected_text("Hello ");
  expected_text.append(kObjectReplacementCharacter);
  expected_text.append(".");
  NGInlineNode first_child = ToNGInlineNode(node.FirstChild());
  EXPECT_EQ(expected_text,
            StringView(first_child.ItemsData(false).text_content, 0, 8));

  // Delete the line box tree to avoid leaks in the test.
  block_flow->DeleteLineBoxTree();
}

}  // namespace blink
