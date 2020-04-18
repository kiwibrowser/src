// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_caret_position.h"

#include "third_party/blink/renderer/core/editing/text_affinity.h"
#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_fragment_traversal.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_offset_mapping.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_text_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_test.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_box_fragment.h"
#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment.h"

namespace blink {

class NGCaretPositionTest : public NGLayoutTest {
 public:
  NGCaretPositionTest() : NGLayoutTest() {}

  void SetUp() override {
    NGLayoutTest::SetUp();
    LoadAhem();
  }

 protected:
  void SetInlineFormattingContext(const char* id,
                                  const char* html,
                                  unsigned width,
                                  TextDirection dir = TextDirection::kLtr) {
    const char* pattern =
        dir == TextDirection::kLtr
            ? "<div id='%s' style='font: 10px/10px Ahem; width: %u0px; "
              "word-break: break-all'>%s</div>"
            : "<bdo dir=rtl id='%s' style='font: 10px/10px Ahem; width: %u0px; "
              "word-break: break-all; display: block'>%s</bdo>";
    SetBodyInnerHTML(String::Format(pattern, id, width, html));
    container_ = GetElementById(id);
    DCHECK(container_);
    context_ = ToLayoutBlockFlow(container_->GetLayoutObject());
    DCHECK(context_);
    DCHECK(context_->IsLayoutNGMixin());
    root_fragment_ = context_->CurrentFragment();
    DCHECK(root_fragment_);
  }

  NGCaretPosition ComputeNGCaretPosition(unsigned offset,
                                         TextAffinity affinity) const {
    return blink::ComputeNGCaretPosition(*context_, offset, affinity);
  }

  const NGPhysicalFragment* FragmentOf(const Node* node) const {
    auto fragments = NGInlineFragmentTraversal::SelfFragmentsOf(
        *root_fragment_, node->GetLayoutObject());
    DCHECK_EQ(1u, fragments.size());
    return fragments.front().fragment.get();
  }

  Persistent<Element> container_;
  const LayoutBlockFlow* context_;
  const NGPhysicalBoxFragment* root_fragment_;
};

#define TEST_CARET(caret, fragment_, type_, offset_)                         \
  {                                                                          \
    EXPECT_EQ(&caret.fragment->PhysicalFragment(), fragment_)                \
        << caret.fragment->PhysicalFragment().ToString();                    \
    EXPECT_EQ(caret.position_type, NGCaretPositionType::type_);              \
    EXPECT_EQ(caret.text_offset, offset_) << caret.text_offset.value_or(-1); \
  }

TEST_F(NGCaretPositionTest, CaretPositionInOneLineOfText) {
  SetInlineFormattingContext("t", "foo", 3);
  const Node* text = container_->firstChild();
  const NGPhysicalFragment* text_fragment = FragmentOf(text);

  // Beginning of line
  TEST_CARET(ComputeNGCaretPosition(0, TextAffinity::kDownstream),
             text_fragment, kAtTextOffset, base::Optional<unsigned>(0));
  TEST_CARET(ComputeNGCaretPosition(0, TextAffinity::kUpstream), text_fragment,
             kAtTextOffset, base::Optional<unsigned>(0));

  // Middle in the line
  TEST_CARET(ComputeNGCaretPosition(1, TextAffinity::kDownstream),
             text_fragment, kAtTextOffset, base::Optional<unsigned>(1));
  TEST_CARET(ComputeNGCaretPosition(1, TextAffinity::kUpstream), text_fragment,
             kAtTextOffset, base::Optional<unsigned>(1));

  // End of line
  TEST_CARET(ComputeNGCaretPosition(3, TextAffinity::kDownstream),
             text_fragment, kAtTextOffset, base::Optional<unsigned>(3));
  TEST_CARET(ComputeNGCaretPosition(3, TextAffinity::kUpstream), text_fragment,
             kAtTextOffset, base::Optional<unsigned>(3));
}

TEST_F(NGCaretPositionTest, CaretPositionAtSoftLineWrap) {
  SetInlineFormattingContext("t", "foobar", 3);
  const Node* text = container_->firstChild();
  const auto text_fragments = NGInlineFragmentTraversal::SelfFragmentsOf(
      *root_fragment_, text->GetLayoutObject());
  const NGPhysicalFragment* foo_fragment = text_fragments[0].fragment.get();
  const NGPhysicalFragment* bar_fragment = text_fragments[1].fragment.get();

  TEST_CARET(ComputeNGCaretPosition(3, TextAffinity::kDownstream), bar_fragment,
             kAtTextOffset, base::Optional<unsigned>(3));
  TEST_CARET(ComputeNGCaretPosition(3, TextAffinity::kUpstream), foo_fragment,
             kAtTextOffset, base::Optional<unsigned>(3));
}

TEST_F(NGCaretPositionTest, CaretPositionAtSoftLineWrapWithSpace) {
  SetInlineFormattingContext("t", "foo bar", 3);
  const Node* text = container_->firstChild();
  const auto text_fragments = NGInlineFragmentTraversal::SelfFragmentsOf(
      *root_fragment_, text->GetLayoutObject());
  const NGPhysicalFragment* foo_fragment = text_fragments[0].fragment.get();
  const NGPhysicalFragment* bar_fragment = text_fragments[1].fragment.get();

  // Before the space
  TEST_CARET(ComputeNGCaretPosition(3, TextAffinity::kDownstream), foo_fragment,
             kAtTextOffset, base::Optional<unsigned>(3));
  TEST_CARET(ComputeNGCaretPosition(3, TextAffinity::kUpstream), foo_fragment,
             kAtTextOffset, base::Optional<unsigned>(3));

  // After the space
  TEST_CARET(ComputeNGCaretPosition(4, TextAffinity::kDownstream), bar_fragment,
             kAtTextOffset, base::Optional<unsigned>(4));
  TEST_CARET(ComputeNGCaretPosition(4, TextAffinity::kUpstream), bar_fragment,
             kAtTextOffset, base::Optional<unsigned>(4));
}

TEST_F(NGCaretPositionTest, CaretPositionAtForcedLineBreak) {
  SetInlineFormattingContext("t", "foo<br>bar", 3);
  const Node* foo = container_->firstChild();
  const Node* br = foo->nextSibling();
  const Node* bar = br->nextSibling();
  const NGPhysicalFragment* foo_fragment = FragmentOf(foo);
  const NGPhysicalFragment* bar_fragment = FragmentOf(bar);

  // Before the BR
  TEST_CARET(ComputeNGCaretPosition(3, TextAffinity::kDownstream), foo_fragment,
             kAtTextOffset, base::Optional<unsigned>(3));
  TEST_CARET(ComputeNGCaretPosition(3, TextAffinity::kUpstream), foo_fragment,
             kAtTextOffset, base::Optional<unsigned>(3));

  // After the BR
  TEST_CARET(ComputeNGCaretPosition(4, TextAffinity::kDownstream), bar_fragment,
             kAtTextOffset, base::Optional<unsigned>(4));
  TEST_CARET(ComputeNGCaretPosition(4, TextAffinity::kUpstream), bar_fragment,
             kAtTextOffset, base::Optional<unsigned>(4));
}

TEST_F(NGCaretPositionTest, CaretPositionAtEmptyLine) {
  SetInlineFormattingContext("f", "foo<br><br>bar", 3);
  const Node* foo = container_->firstChild();
  const Node* br1 = foo->nextSibling();
  const Node* br2 = br1->nextSibling();
  const NGPhysicalFragment* br2_fragment = FragmentOf(br2);

  TEST_CARET(ComputeNGCaretPosition(4, TextAffinity::kDownstream), br2_fragment,
             kAtTextOffset, base::Optional<unsigned>(4));
  TEST_CARET(ComputeNGCaretPosition(4, TextAffinity::kUpstream), br2_fragment,
             kAtTextOffset, base::Optional<unsigned>(4));
}

TEST_F(NGCaretPositionTest, CaretPositionInOneLineOfImage) {
  SetInlineFormattingContext("t", "<img>", 3);
  const Node* img = container_->firstChild();
  const NGPhysicalFragment* img_fragment = FragmentOf(img);

  // Before the image
  TEST_CARET(ComputeNGCaretPosition(0, TextAffinity::kDownstream), img_fragment,
             kBeforeBox, base::nullopt);
  TEST_CARET(ComputeNGCaretPosition(0, TextAffinity::kUpstream), img_fragment,
             kBeforeBox, base::nullopt);

  // After the image
  TEST_CARET(ComputeNGCaretPosition(1, TextAffinity::kDownstream), img_fragment,
             kAfterBox, base::nullopt);
  TEST_CARET(ComputeNGCaretPosition(1, TextAffinity::kUpstream), img_fragment,
             kAfterBox, base::nullopt);
}

TEST_F(NGCaretPositionTest, CaretPositionAtSoftLineWrapBetweenImages) {
  SetInlineFormattingContext("t",
                             "<img id=img1><img id=img2>"
                             "<style>img{width: 1em; height: 1em}</style>",
                             1);
  const Node* img1 = container_->firstChild();
  const Node* img2 = img1->nextSibling();
  const NGPhysicalFragment* img1_fragment = FragmentOf(img1);
  const NGPhysicalFragment* img2_fragment = FragmentOf(img2);

  TEST_CARET(ComputeNGCaretPosition(1, TextAffinity::kDownstream),
             img2_fragment, kBeforeBox, base::nullopt);
  TEST_CARET(ComputeNGCaretPosition(1, TextAffinity::kUpstream), img1_fragment,
             kAfterBox, base::nullopt);
}

TEST_F(NGCaretPositionTest,
       CaretPositionAtSoftLineWrapBetweenMultipleTextNodes) {
  SetInlineFormattingContext("t",
                             "<span>A</span>"
                             "<span>B</span>"
                             "<span id=span-c>C</span>"
                             "<span id=span-d>D</span>"
                             "<span>E</span>"
                             "<span>F</span>",
                             3);
  const Node* text_c = GetElementById("span-c")->firstChild();
  const Node* text_d = GetElementById("span-d")->firstChild();
  const NGPhysicalFragment* fragment_c = FragmentOf(text_c);
  const NGPhysicalFragment* fragment_d = FragmentOf(text_d);

  const Position wrap_position(text_c, 1);
  const NGOffsetMapping& mapping = *NGOffsetMapping::GetFor(wrap_position);
  const unsigned wrap_offset =
      mapping.GetTextContentOffset(wrap_position).value();

  TEST_CARET(ComputeNGCaretPosition(wrap_offset, TextAffinity::kUpstream),
             fragment_c, kAtTextOffset, base::Optional<unsigned>(wrap_offset));
  TEST_CARET(ComputeNGCaretPosition(wrap_offset, TextAffinity::kDownstream),
             fragment_d, kAtTextOffset, base::Optional<unsigned>(wrap_offset));
}

TEST_F(NGCaretPositionTest,
       CaretPositionAtSoftLineWrapBetweenMultipleTextNodesRtl) {
  SetInlineFormattingContext("t",
                             "<span>A</span>"
                             "<span>B</span>"
                             "<span id=span-c>C</span>"
                             "<span id=span-d>D</span>"
                             "<span>E</span>"
                             "<span>F</span>",
                             3, TextDirection::kRtl);
  const Node* text_c = GetElementById("span-c")->firstChild();
  const Node* text_d = GetElementById("span-d")->firstChild();
  const NGPhysicalFragment* fragment_c = FragmentOf(text_c);
  const NGPhysicalFragment* fragment_d = FragmentOf(text_d);

  const Position wrap_position(text_c, 1);
  const NGOffsetMapping& mapping = *NGOffsetMapping::GetFor(wrap_position);
  const unsigned wrap_offset =
      mapping.GetTextContentOffset(wrap_position).value();

  TEST_CARET(ComputeNGCaretPosition(wrap_offset, TextAffinity::kUpstream),
             fragment_c, kAtTextOffset, base::Optional<unsigned>(wrap_offset));
  TEST_CARET(ComputeNGCaretPosition(wrap_offset, TextAffinity::kDownstream),
             fragment_d, kAtTextOffset, base::Optional<unsigned>(wrap_offset));
}

TEST_F(NGCaretPositionTest, CaretPositionAtSoftLineWrapBetweenDeepTextNodes) {
  SetInlineFormattingContext(
      "t",
      "<style>span {border: 1px solid black}</style>"
      "<span>A</span>"
      "<span>B</span>"
      "<span id=span-c>C</span>"
      "<span id=span-d>D</span>"
      "<span>E</span>"
      "<span>F</span>",
      4);  // Wider space to allow border and 3 characters
  const Node* text_c = GetElementById("span-c")->firstChild();
  const Node* text_d = GetElementById("span-d")->firstChild();
  const NGPhysicalFragment* fragment_c = FragmentOf(text_c);
  const NGPhysicalFragment* fragment_d = FragmentOf(text_d);

  const Position wrap_position(text_c, 1);
  const NGOffsetMapping& mapping = *NGOffsetMapping::GetFor(wrap_position);
  const unsigned wrap_offset =
      mapping.GetTextContentOffset(wrap_position).value();

  TEST_CARET(ComputeNGCaretPosition(wrap_offset, TextAffinity::kUpstream),
             fragment_c, kAtTextOffset, base::Optional<unsigned>(wrap_offset));
  TEST_CARET(ComputeNGCaretPosition(wrap_offset, TextAffinity::kDownstream),
             fragment_d, kAtTextOffset, base::Optional<unsigned>(wrap_offset));
}

}  // namespace blink
