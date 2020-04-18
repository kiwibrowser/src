// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_offset_mapping.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/first_letter_pseudo_element.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/position.h"
#include "third_party/blink/renderer/core/layout/layout_text_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_node.h"
#include "third_party/blink/renderer/core/layout/ng/layout_ng_block_flow.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_test.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

class NGOffsetMappingTest : public NGLayoutTest {
 protected:
  void SetUp() override {
    NGLayoutTest::SetUp();
    style_ = ComputedStyle::Create();
    style_->GetFont().Update(nullptr);
  }

  void SetupHtml(const char* id, String html) {
    SetBodyInnerHTML(html);
    layout_block_flow_ = ToLayoutBlockFlow(GetLayoutObjectByElementId(id));
    DCHECK(layout_block_flow_->IsLayoutNGMixin());
    layout_object_ = layout_block_flow_->FirstChild();
    style_ = layout_object_->Style();
  }

  const NGOffsetMapping& GetOffsetMapping() const {
    const NGOffsetMapping* map =
        NGInlineNode(layout_block_flow_).ComputeOffsetMappingIfNeeded();
    CHECK(map);
    return *map;
  }

  bool IsOffsetMappingStored() const {
    return layout_block_flow_->GetNGInlineNodeData()->offset_mapping.get();
  }

  const LayoutText* GetLayoutTextUnder(const char* parent_id) {
    Element* parent = GetDocument().getElementById(parent_id);
    return ToLayoutText(parent->firstChild()->GetLayoutObject());
  }

  const NGOffsetMappingUnit* GetUnitForPosition(
      const Position& position) const {
    return GetOffsetMapping().GetMappingUnitForPosition(position);
  }

  base::Optional<unsigned> GetTextContentOffset(
      const Position& position) const {
    return GetOffsetMapping().GetTextContentOffset(position);
  }

  Position StartOfNextNonCollapsedContent(const Position& position) const {
    return GetOffsetMapping().StartOfNextNonCollapsedContent(position);
  }

  Position EndOfLastNonCollapsedContent(const Position& position) const {
    return GetOffsetMapping().EndOfLastNonCollapsedContent(position);
  }

  bool IsBeforeNonCollapsedContent(const Position& position) const {
    return GetOffsetMapping().IsBeforeNonCollapsedContent(position);
  }

  bool IsAfterNonCollapsedContent(const Position& position) const {
    return GetOffsetMapping().IsAfterNonCollapsedContent(position);
  }

  Position GetFirstPosition(unsigned offset) const {
    return GetOffsetMapping().GetFirstPosition(offset);
  }

  Position GetLastPosition(unsigned offset) const {
    return GetOffsetMapping().GetLastPosition(offset);
  }

  scoped_refptr<const ComputedStyle> style_;
  LayoutBlockFlow* layout_block_flow_ = nullptr;
  LayoutObject* layout_object_ = nullptr;
  FontCachePurgePreventer purge_preventer_;
};

// TODO(layout-dev): Remove this unused parameterization.
class ParameterizedNGOffsetMappingTest
    : public testing::WithParamInterface<bool>,
      public NGOffsetMappingTest {
 public:
  ParameterizedNGOffsetMappingTest() {}
};

INSTANTIATE_TEST_CASE_P(All, ParameterizedNGOffsetMappingTest, testing::Bool());

#define TEST_UNIT(unit, type, owner, dom_start, dom_end, text_content_start, \
                  text_content_end)                                          \
  EXPECT_EQ(type, unit.GetType());                                           \
  EXPECT_EQ(owner, &unit.GetOwner());                                        \
  EXPECT_EQ(dom_start, unit.DOMStart());                                     \
  EXPECT_EQ(dom_end, unit.DOMEnd());                                         \
  EXPECT_EQ(text_content_start, unit.TextContentStart());                    \
  EXPECT_EQ(text_content_end, unit.TextContentEnd())

#define TEST_RANGE(ranges, owner, start, end) \
  ASSERT_TRUE(ranges.Contains(owner));        \
  EXPECT_EQ(start, ranges.at(owner).first);   \
  EXPECT_EQ(end, ranges.at(owner).second)

TEST_P(ParameterizedNGOffsetMappingTest, StoredResult) {
  SetupHtml("t", "<div id=t>foo</div>");
  EXPECT_FALSE(IsOffsetMappingStored());
  GetOffsetMapping();
  EXPECT_TRUE(IsOffsetMappingStored());
}

TEST_P(ParameterizedNGOffsetMappingTest, NGInlineFormattingContextOf) {
  SetBodyInnerHTML(
      "<div id=container>"
      "  foo"
      "  <span id=inline-block style='display:inline-block'>blah</span>"
      "  <span id=inline-span>bar</span>"
      "</div>");

  const Element* container = GetElementById("container");
  const Element* inline_block = GetElementById("inline-block");
  const Element* inline_span = GetElementById("inline-span");
  const Node* blah = inline_block->firstChild();
  const Node* foo = inline_block->previousSibling();
  const Node* bar = inline_span->firstChild();

  EXPECT_EQ(nullptr,
            NGInlineFormattingContextOf(Position::BeforeNode(*container)));
  EXPECT_EQ(nullptr,
            NGInlineFormattingContextOf(Position::AfterNode(*container)));

  const LayoutObject* container_object = container->GetLayoutObject();
  EXPECT_EQ(container_object, NGInlineFormattingContextOf(Position(foo, 0)));
  EXPECT_EQ(container_object, NGInlineFormattingContextOf(Position(bar, 0)));
  EXPECT_EQ(container_object,
            NGInlineFormattingContextOf(Position::BeforeNode(*inline_block)));
  EXPECT_EQ(container_object,
            NGInlineFormattingContextOf(Position::AfterNode(*inline_block)));
  EXPECT_EQ(container_object,
            NGInlineFormattingContextOf(Position::BeforeNode(*inline_span)));
  EXPECT_EQ(container_object,
            NGInlineFormattingContextOf(Position::AfterNode(*inline_span)));

  const LayoutObject* inline_block_object = inline_block->GetLayoutObject();
  EXPECT_EQ(inline_block_object,
            NGInlineFormattingContextOf(Position(blah, 0)));
}

TEST_P(ParameterizedNGOffsetMappingTest, OneTextNode) {
  SetupHtml("t", "<div id=t>foo</div>");
  const Node* foo_node = layout_object_->GetNode();
  const NGOffsetMapping& result = GetOffsetMapping();

  EXPECT_EQ("foo", result.GetText());

  ASSERT_EQ(1u, result.GetUnits().size());
  TEST_UNIT(result.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, foo_node,
            0u, 3u, 0u, 3u);

  ASSERT_EQ(1u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), foo_node, 0u, 1u);

  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 0)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 1)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 2)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 3)));

  EXPECT_EQ(0u, *GetTextContentOffset(Position(foo_node, 0)));
  EXPECT_EQ(1u, *GetTextContentOffset(Position(foo_node, 1)));
  EXPECT_EQ(2u, *GetTextContentOffset(Position(foo_node, 2)));
  EXPECT_EQ(3u, *GetTextContentOffset(Position(foo_node, 3)));

  EXPECT_EQ(Position(foo_node, 0), GetFirstPosition(0));
  EXPECT_EQ(Position(foo_node, 1), GetFirstPosition(1));
  EXPECT_EQ(Position(foo_node, 2), GetFirstPosition(2));
  EXPECT_EQ(Position(foo_node, 3), GetFirstPosition(3));

  EXPECT_EQ(Position(foo_node, 0), GetLastPosition(0));
  EXPECT_EQ(Position(foo_node, 1), GetLastPosition(1));
  EXPECT_EQ(Position(foo_node, 2), GetLastPosition(2));
  EXPECT_EQ(Position(foo_node, 3), GetLastPosition(3));

  EXPECT_EQ(Position(foo_node, 0),
            StartOfNextNonCollapsedContent(Position(foo_node, 0)));
  EXPECT_EQ(Position(foo_node, 1),
            StartOfNextNonCollapsedContent(Position(foo_node, 1)));
  EXPECT_EQ(Position(foo_node, 2),
            StartOfNextNonCollapsedContent(Position(foo_node, 2)));
  EXPECT_TRUE(StartOfNextNonCollapsedContent(Position(foo_node, 3)).IsNull());

  EXPECT_TRUE(EndOfLastNonCollapsedContent(Position(foo_node, 0)).IsNull());
  EXPECT_EQ(Position(foo_node, 1),
            EndOfLastNonCollapsedContent(Position(foo_node, 1)));
  EXPECT_EQ(Position(foo_node, 2),
            EndOfLastNonCollapsedContent(Position(foo_node, 2)));
  EXPECT_EQ(Position(foo_node, 3),
            EndOfLastNonCollapsedContent(Position(foo_node, 3)));

  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(foo_node, 0)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(foo_node, 1)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(foo_node, 2)));
  EXPECT_FALSE(
      IsBeforeNonCollapsedContent(Position(foo_node, 3)));  // false at node end

  // false at node start
  EXPECT_FALSE(IsAfterNonCollapsedContent(Position(foo_node, 0)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(foo_node, 1)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(foo_node, 2)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(foo_node, 3)));
}

TEST_P(ParameterizedNGOffsetMappingTest, TwoTextNodes) {
  SetupHtml("t", "<div id=t>foo<span id=s>bar</span></div>");
  const LayoutText* foo = ToLayoutText(layout_object_);
  const LayoutText* bar = GetLayoutTextUnder("s");
  const Node* foo_node = foo->GetNode();
  const Node* bar_node = bar->GetNode();
  const Node* span = GetElementById("s");
  const NGOffsetMapping& result = GetOffsetMapping();

  EXPECT_EQ("foobar", result.GetText());

  ASSERT_EQ(2u, result.GetUnits().size());
  TEST_UNIT(result.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, foo_node,
            0u, 3u, 0u, 3u);
  TEST_UNIT(result.GetUnits()[1], NGOffsetMappingUnitType::kIdentity, bar_node,
            0u, 3u, 3u, 6u);

  ASSERT_EQ(3u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), foo_node, 0u, 1u);
  TEST_RANGE(result.GetRanges(), bar_node, 1u, 2u);
  TEST_RANGE(result.GetRanges(), span, 1u, 2u);

  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 0)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 1)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 2)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 3)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(bar_node, 0)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(bar_node, 1)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(bar_node, 2)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(bar_node, 3)));

  EXPECT_EQ(0u, *GetTextContentOffset(Position(foo_node, 0)));
  EXPECT_EQ(1u, *GetTextContentOffset(Position(foo_node, 1)));
  EXPECT_EQ(2u, *GetTextContentOffset(Position(foo_node, 2)));
  EXPECT_EQ(3u, *GetTextContentOffset(Position(foo_node, 3)));
  EXPECT_EQ(3u, *GetTextContentOffset(Position(bar_node, 0)));
  EXPECT_EQ(4u, *GetTextContentOffset(Position(bar_node, 1)));
  EXPECT_EQ(5u, *GetTextContentOffset(Position(bar_node, 2)));
  EXPECT_EQ(6u, *GetTextContentOffset(Position(bar_node, 3)));

  EXPECT_EQ(Position(foo_node, 3), GetFirstPosition(3));
  EXPECT_EQ(Position(bar_node, 0), GetLastPosition(3));

  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(foo_node, 0)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(foo_node, 1)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(foo_node, 2)));
  EXPECT_FALSE(
      IsBeforeNonCollapsedContent(Position(foo_node, 3)));  // false at node end

  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(bar_node, 0)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(bar_node, 1)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(bar_node, 2)));
  EXPECT_FALSE(
      IsBeforeNonCollapsedContent(Position(bar_node, 3)));  // false at node end

  // false at node start
  EXPECT_FALSE(IsAfterNonCollapsedContent(Position(foo_node, 0)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(foo_node, 1)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(foo_node, 2)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(foo_node, 3)));

  // false at node start
  EXPECT_FALSE(IsAfterNonCollapsedContent(Position(bar_node, 0)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(bar_node, 1)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(bar_node, 2)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(bar_node, 3)));
}

TEST_P(ParameterizedNGOffsetMappingTest, BRBetweenTextNodes) {
  SetupHtml("t", u"<div id=t>foo<br>bar</div>");
  const LayoutText* foo = ToLayoutText(layout_object_);
  const LayoutText* br = ToLayoutText(foo->NextSibling());
  const LayoutText* bar = ToLayoutText(br->NextSibling());
  const Node* foo_node = foo->GetNode();
  const Node* br_node = br->GetNode();
  const Node* bar_node = bar->GetNode();
  const NGOffsetMapping& result = GetOffsetMapping();

  EXPECT_EQ("foo\nbar", result.GetText());

  ASSERT_EQ(3u, result.GetUnits().size());
  TEST_UNIT(result.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, foo_node,
            0u, 3u, 0u, 3u);
  TEST_UNIT(result.GetUnits()[1], NGOffsetMappingUnitType::kIdentity, br_node,
            0u, 1u, 3u, 4u);
  TEST_UNIT(result.GetUnits()[2], NGOffsetMappingUnitType::kIdentity, bar_node,
            0u, 3u, 4u, 7u);

  ASSERT_EQ(3u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), foo_node, 0u, 1u);
  TEST_RANGE(result.GetRanges(), br_node, 1u, 2u);
  TEST_RANGE(result.GetRanges(), bar_node, 2u, 3u);

  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 0)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 1)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 2)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 3)));
  EXPECT_EQ(&result.GetUnits()[1],
            GetUnitForPosition(Position::BeforeNode(*br_node)));
  EXPECT_EQ(&result.GetUnits()[1],
            GetUnitForPosition(Position::AfterNode(*br_node)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 0)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 1)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 2)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 3)));

  EXPECT_EQ(0u, *GetTextContentOffset(Position(foo_node, 0)));
  EXPECT_EQ(1u, *GetTextContentOffset(Position(foo_node, 1)));
  EXPECT_EQ(2u, *GetTextContentOffset(Position(foo_node, 2)));
  EXPECT_EQ(3u, *GetTextContentOffset(Position(foo_node, 3)));
  EXPECT_EQ(3u, *GetTextContentOffset(Position::BeforeNode(*br_node)));
  EXPECT_EQ(4u, *GetTextContentOffset(Position::AfterNode(*br_node)));
  EXPECT_EQ(4u, *GetTextContentOffset(Position(bar_node, 0)));
  EXPECT_EQ(5u, *GetTextContentOffset(Position(bar_node, 1)));
  EXPECT_EQ(6u, *GetTextContentOffset(Position(bar_node, 2)));
  EXPECT_EQ(7u, *GetTextContentOffset(Position(bar_node, 3)));

  EXPECT_EQ(Position(foo_node, 3), GetFirstPosition(3));
  EXPECT_EQ(Position::BeforeNode(*br_node), GetLastPosition(3));
  EXPECT_EQ(Position::AfterNode(*br_node), GetFirstPosition(4));
  EXPECT_EQ(Position(bar_node, 0), GetLastPosition(4));
}

TEST_P(ParameterizedNGOffsetMappingTest, OneTextNodeWithCollapsedSpace) {
  SetupHtml("t", "<div id=t>foo  bar</div>");
  const Node* node = layout_object_->GetNode();
  const NGOffsetMapping& result = GetOffsetMapping();

  EXPECT_EQ("foo bar", result.GetText());

  ASSERT_EQ(3u, result.GetUnits().size());
  TEST_UNIT(result.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, node, 0u,
            4u, 0u, 4u);
  TEST_UNIT(result.GetUnits()[1], NGOffsetMappingUnitType::kCollapsed, node, 4u,
            5u, 4u, 4u);
  TEST_UNIT(result.GetUnits()[2], NGOffsetMappingUnitType::kIdentity, node, 5u,
            8u, 4u, 7u);

  ASSERT_EQ(1u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), node, 0u, 3u);

  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(node, 0)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(node, 1)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(node, 2)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(node, 3)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(node, 4)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(node, 5)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(node, 6)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(node, 7)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(node, 8)));

  EXPECT_EQ(0u, *GetTextContentOffset(Position(node, 0)));
  EXPECT_EQ(1u, *GetTextContentOffset(Position(node, 1)));
  EXPECT_EQ(2u, *GetTextContentOffset(Position(node, 2)));
  EXPECT_EQ(3u, *GetTextContentOffset(Position(node, 3)));
  EXPECT_EQ(4u, *GetTextContentOffset(Position(node, 4)));
  EXPECT_EQ(4u, *GetTextContentOffset(Position(node, 5)));
  EXPECT_EQ(5u, *GetTextContentOffset(Position(node, 6)));
  EXPECT_EQ(6u, *GetTextContentOffset(Position(node, 7)));
  EXPECT_EQ(7u, *GetTextContentOffset(Position(node, 8)));

  EXPECT_EQ(Position(node, 4), GetFirstPosition(4));
  EXPECT_EQ(Position(node, 5), GetLastPosition(4));

  EXPECT_EQ(Position(node, 3),
            StartOfNextNonCollapsedContent(Position(node, 3)));
  EXPECT_EQ(Position(node, 5),
            StartOfNextNonCollapsedContent(Position(node, 4)));
  EXPECT_EQ(Position(node, 5),
            StartOfNextNonCollapsedContent(Position(node, 5)));

  EXPECT_EQ(Position(node, 3), EndOfLastNonCollapsedContent(Position(node, 3)));
  EXPECT_EQ(Position(node, 4), EndOfLastNonCollapsedContent(Position(node, 4)));
  EXPECT_EQ(Position(node, 4), EndOfLastNonCollapsedContent(Position(node, 5)));

  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(node, 0)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(node, 1)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(node, 2)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(node, 3)));
  EXPECT_FALSE(IsBeforeNonCollapsedContent(Position(node, 4)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(node, 5)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(node, 6)));
  EXPECT_TRUE(IsBeforeNonCollapsedContent(Position(node, 7)));
  EXPECT_FALSE(IsBeforeNonCollapsedContent(Position(node, 8)));

  EXPECT_FALSE(IsAfterNonCollapsedContent(Position(node, 0)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(node, 1)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(node, 2)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(node, 3)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(node, 4)));
  EXPECT_FALSE(IsAfterNonCollapsedContent(Position(node, 5)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(node, 6)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(node, 7)));
  EXPECT_TRUE(IsAfterNonCollapsedContent(Position(node, 8)));
}

TEST_P(ParameterizedNGOffsetMappingTest, FullyCollapsedWhiteSpaceNode) {
  SetupHtml("t",
            "<div id=t>"
            "<span id=s1>foo </span>"
            " "
            "<span id=s2>bar</span>"
            "</div>");
  const LayoutText* foo = GetLayoutTextUnder("s1");
  const LayoutText* bar = GetLayoutTextUnder("s2");
  const LayoutText* space = ToLayoutText(layout_object_->NextSibling());
  const Node* foo_node = foo->GetNode();
  const Node* bar_node = bar->GetNode();
  const Node* space_node = space->GetNode();
  const Node* span1 = GetElementById("s1");
  const Node* span2 = GetElementById("s2");
  const NGOffsetMapping& result = GetOffsetMapping();

  EXPECT_EQ("foo bar", result.GetText());

  ASSERT_EQ(3u, result.GetUnits().size());
  TEST_UNIT(result.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, foo_node,
            0u, 4u, 0u, 4u);
  TEST_UNIT(result.GetUnits()[1], NGOffsetMappingUnitType::kCollapsed,
            space_node, 0u, 1u, 4u, 4u);
  TEST_UNIT(result.GetUnits()[2], NGOffsetMappingUnitType::kIdentity, bar_node,
            0u, 3u, 4u, 7u);

  ASSERT_EQ(5u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), foo_node, 0u, 1u);
  TEST_RANGE(result.GetRanges(), span1, 0u, 1u);
  TEST_RANGE(result.GetRanges(), space_node, 1u, 2u);
  TEST_RANGE(result.GetRanges(), bar_node, 2u, 3u);
  TEST_RANGE(result.GetRanges(), span2, 2u, 3u);

  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 0)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 1)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 2)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 3)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 4)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(space_node, 0)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(space_node, 1)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 0)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 1)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 2)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 3)));

  EXPECT_EQ(0u, *GetTextContentOffset(Position(foo_node, 0)));
  EXPECT_EQ(1u, *GetTextContentOffset(Position(foo_node, 1)));
  EXPECT_EQ(2u, *GetTextContentOffset(Position(foo_node, 2)));
  EXPECT_EQ(3u, *GetTextContentOffset(Position(foo_node, 3)));
  EXPECT_EQ(4u, *GetTextContentOffset(Position(foo_node, 4)));
  EXPECT_EQ(4u, *GetTextContentOffset(Position(space_node, 0)));
  EXPECT_EQ(4u, *GetTextContentOffset(Position(space_node, 1)));
  EXPECT_EQ(4u, *GetTextContentOffset(Position(bar_node, 0)));
  EXPECT_EQ(5u, *GetTextContentOffset(Position(bar_node, 1)));
  EXPECT_EQ(6u, *GetTextContentOffset(Position(bar_node, 2)));
  EXPECT_EQ(7u, *GetTextContentOffset(Position(bar_node, 3)));

  EXPECT_EQ(Position(foo_node, 4), GetFirstPosition(4));
  EXPECT_EQ(Position(bar_node, 0), GetLastPosition(4));

  EXPECT_TRUE(EndOfLastNonCollapsedContent(Position(space_node, 1u)).IsNull());
  EXPECT_TRUE(
      StartOfNextNonCollapsedContent(Position(space_node, 0u)).IsNull());
}

TEST_P(ParameterizedNGOffsetMappingTest, ReplacedElement) {
  SetupHtml("t", "<div id=t>foo <img> bar</div>");
  const LayoutText* foo = ToLayoutText(layout_object_);
  const LayoutObject* img = foo->NextSibling();
  const LayoutText* bar = ToLayoutText(img->NextSibling());
  const Node* foo_node = foo->GetNode();
  const Node* img_node = img->GetNode();
  const Node* bar_node = bar->GetNode();
  const NGOffsetMapping& result = GetOffsetMapping();

  ASSERT_EQ(3u, result.GetUnits().size());
  TEST_UNIT(result.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, foo_node,
            0u, 4u, 0u, 4u);
  TEST_UNIT(result.GetUnits()[1], NGOffsetMappingUnitType::kIdentity, img_node,
            0u, 1u, 4u, 5u);
  TEST_UNIT(result.GetUnits()[2], NGOffsetMappingUnitType::kIdentity, bar_node,
            0u, 4u, 5u, 9u);

  ASSERT_EQ(3u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), foo_node, 0u, 1u);
  TEST_RANGE(result.GetRanges(), img_node, 1u, 2u);
  TEST_RANGE(result.GetRanges(), bar_node, 2u, 3u);

  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 0)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 1)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 2)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 3)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 4)));
  EXPECT_EQ(&result.GetUnits()[1],
            GetUnitForPosition(Position::BeforeNode(*img_node)));
  EXPECT_EQ(&result.GetUnits()[1],
            GetUnitForPosition(Position::AfterNode(*img_node)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 0)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 1)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 2)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 3)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(bar_node, 4)));

  EXPECT_EQ(0u, *GetTextContentOffset(Position(foo_node, 0)));
  EXPECT_EQ(1u, *GetTextContentOffset(Position(foo_node, 1)));
  EXPECT_EQ(2u, *GetTextContentOffset(Position(foo_node, 2)));
  EXPECT_EQ(3u, *GetTextContentOffset(Position(foo_node, 3)));
  EXPECT_EQ(4u, *GetTextContentOffset(Position(foo_node, 4)));
  EXPECT_EQ(4u, *GetTextContentOffset(Position::BeforeNode(*img_node)));
  EXPECT_EQ(5u, *GetTextContentOffset(Position::AfterNode(*img_node)));
  EXPECT_EQ(5u, *GetTextContentOffset(Position(bar_node, 0)));
  EXPECT_EQ(6u, *GetTextContentOffset(Position(bar_node, 1)));
  EXPECT_EQ(7u, *GetTextContentOffset(Position(bar_node, 2)));
  EXPECT_EQ(8u, *GetTextContentOffset(Position(bar_node, 3)));
  EXPECT_EQ(9u, *GetTextContentOffset(Position(bar_node, 4)));

  EXPECT_EQ(Position(foo_node, 4), GetFirstPosition(4));
  EXPECT_EQ(Position::BeforeNode(*img_node), GetLastPosition(4));
  EXPECT_EQ(Position::AfterNode(*img_node), GetFirstPosition(5));
  EXPECT_EQ(Position(bar_node, 0), GetLastPosition(5));
}

TEST_P(ParameterizedNGOffsetMappingTest, FirstLetter) {
  SetupHtml("t",
            "<style>div:first-letter{color:red}</style>"
            "<div id=t>foo</div>");
  Element* div = GetDocument().getElementById("t");
  const Node* foo_node = div->firstChild();
  const NGOffsetMapping& result = GetOffsetMapping();

  ASSERT_EQ(2u, result.GetUnits().size());
  TEST_UNIT(result.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, foo_node,
            0u, 1u, 0u, 1u);
  // first leter and remaining text are always in different mapping units.
  TEST_UNIT(result.GetUnits()[1], NGOffsetMappingUnitType::kIdentity, foo_node,
            1u, 3u, 1u, 3u);

  ASSERT_EQ(1u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), foo_node, 0u, 2u);

  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 0)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(foo_node, 1)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(foo_node, 2)));

  EXPECT_EQ(0u, *GetTextContentOffset(Position(foo_node, 0)));
  EXPECT_EQ(1u, *GetTextContentOffset(Position(foo_node, 1)));
  EXPECT_EQ(2u, *GetTextContentOffset(Position(foo_node, 2)));

  EXPECT_EQ(Position(foo_node, 1), GetFirstPosition(1));
  EXPECT_EQ(Position(foo_node, 1), GetLastPosition(1));
}

TEST_P(ParameterizedNGOffsetMappingTest, FirstLetterWithLeadingSpace) {
  SetupHtml("t",
            "<style>div:first-letter{color:red}</style>"
            "<div id=t>  foo</div>");
  Element* div = GetDocument().getElementById("t");
  const Node* foo_node = div->firstChild();
  const NGOffsetMapping& result = GetOffsetMapping();

  ASSERT_EQ(3u, result.GetUnits().size());
  TEST_UNIT(result.GetUnits()[0], NGOffsetMappingUnitType::kCollapsed, foo_node,
            0u, 2u, 0u, 0u);
  TEST_UNIT(result.GetUnits()[1], NGOffsetMappingUnitType::kIdentity, foo_node,
            2u, 3u, 0u, 1u);
  // first leter and remaining text are always in different mapping units.
  TEST_UNIT(result.GetUnits()[2], NGOffsetMappingUnitType::kIdentity, foo_node,
            3u, 5u, 1u, 3u);

  ASSERT_EQ(1u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), foo_node, 0u, 3u);

  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 0)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(foo_node, 1)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(foo_node, 2)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(foo_node, 3)));
  EXPECT_EQ(&result.GetUnits()[2], GetUnitForPosition(Position(foo_node, 4)));

  EXPECT_EQ(0u, *GetTextContentOffset(Position(foo_node, 0)));
  EXPECT_EQ(0u, *GetTextContentOffset(Position(foo_node, 1)));
  EXPECT_EQ(0u, *GetTextContentOffset(Position(foo_node, 2)));
  EXPECT_EQ(1u, *GetTextContentOffset(Position(foo_node, 3)));
  EXPECT_EQ(2u, *GetTextContentOffset(Position(foo_node, 4)));

  EXPECT_EQ(Position(foo_node, 0), GetFirstPosition(0));
  EXPECT_EQ(Position(foo_node, 2), GetLastPosition(0));
}

TEST_P(ParameterizedNGOffsetMappingTest, FirstLetterWithoutRemainingText) {
  SetupHtml("t",
            "<style>div:first-letter{color:red}</style>"
            "<div id=t>  f</div>");
  Element* div = GetDocument().getElementById("t");
  const Node* text_node = div->firstChild();
  const NGOffsetMapping& result = GetOffsetMapping();

  ASSERT_EQ(2u, result.GetUnits().size());
  TEST_UNIT(result.GetUnits()[0], NGOffsetMappingUnitType::kCollapsed,
            text_node, 0u, 2u, 0u, 0u);
  TEST_UNIT(result.GetUnits()[1], NGOffsetMappingUnitType::kIdentity, text_node,
            2u, 3u, 0u, 1u);

  ASSERT_EQ(1u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), text_node, 0u, 2u);

  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(text_node, 0)));
  EXPECT_EQ(&result.GetUnits()[0], GetUnitForPosition(Position(text_node, 1)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(text_node, 2)));
  EXPECT_EQ(&result.GetUnits()[1], GetUnitForPosition(Position(text_node, 3)));

  EXPECT_EQ(0u, *GetTextContentOffset(Position(text_node, 0)));
  EXPECT_EQ(0u, *GetTextContentOffset(Position(text_node, 1)));
  EXPECT_EQ(0u, *GetTextContentOffset(Position(text_node, 2)));
  EXPECT_EQ(1u, *GetTextContentOffset(Position(text_node, 3)));

  EXPECT_EQ(Position(text_node, 0), GetFirstPosition(0));
  EXPECT_EQ(Position(text_node, 2), GetLastPosition(0));
}

TEST_P(ParameterizedNGOffsetMappingTest, FirstLetterInDifferentBlock) {
  SetupHtml("t",
            "<style>:first-letter{float:right}</style><div id=t>foo</div>");
  Element* div = GetDocument().getElementById("t");
  const Node* text_node = div->firstChild();

  auto* mapping0 = NGOffsetMapping::GetFor(Position(text_node, 0));
  auto* mapping1 = NGOffsetMapping::GetFor(Position(text_node, 1));
  auto* mapping2 = NGOffsetMapping::GetFor(Position(text_node, 2));
  auto* mapping3 = NGOffsetMapping::GetFor(Position(text_node, 3));

  ASSERT_TRUE(mapping0);
  ASSERT_TRUE(mapping1);
  ASSERT_TRUE(mapping2);
  ASSERT_TRUE(mapping3);

  // GetNGOffsetmappingFor() returns different mappings for offset 0 and other
  // offsets, because first-letter is laid out in a different block.
  EXPECT_NE(mapping0, mapping1);
  EXPECT_EQ(mapping1, mapping2);
  EXPECT_EQ(mapping2, mapping3);

  const NGOffsetMapping& first_letter_result = *mapping0;
  ASSERT_EQ(1u, first_letter_result.GetUnits().size());
  TEST_UNIT(first_letter_result.GetUnits()[0],
            NGOffsetMappingUnitType::kIdentity, text_node, 0u, 1u, 0u, 1u);
  ASSERT_EQ(1u, first_letter_result.GetRanges().size());
  TEST_RANGE(first_letter_result.GetRanges(), text_node, 0u, 1u);

  const NGOffsetMapping& remaining_text_result = *mapping1;
  ASSERT_EQ(1u, remaining_text_result.GetUnits().size());
  TEST_UNIT(remaining_text_result.GetUnits()[0],
            NGOffsetMappingUnitType::kIdentity, text_node, 1u, 3u, 1u, 3u);
  ASSERT_EQ(1u, remaining_text_result.GetRanges().size());
  TEST_RANGE(remaining_text_result.GetRanges(), text_node, 0u, 1u);

  EXPECT_EQ(
      &first_letter_result.GetUnits()[0],
      first_letter_result.GetMappingUnitForPosition(Position(text_node, 0)));
  EXPECT_EQ(
      &remaining_text_result.GetUnits()[0],
      remaining_text_result.GetMappingUnitForPosition(Position(text_node, 1)));
  EXPECT_EQ(
      &remaining_text_result.GetUnits()[0],
      remaining_text_result.GetMappingUnitForPosition(Position(text_node, 2)));
  EXPECT_EQ(
      &remaining_text_result.GetUnits()[0],
      remaining_text_result.GetMappingUnitForPosition(Position(text_node, 3)));

  EXPECT_EQ(0u,
            *first_letter_result.GetTextContentOffset(Position(text_node, 0)));
  EXPECT_EQ(
      1u, *remaining_text_result.GetTextContentOffset(Position(text_node, 1)));
  EXPECT_EQ(
      2u, *remaining_text_result.GetTextContentOffset(Position(text_node, 2)));
  EXPECT_EQ(
      3u, *remaining_text_result.GetTextContentOffset(Position(text_node, 3)));

  EXPECT_EQ(Position(text_node, 1), first_letter_result.GetFirstPosition(1));
  EXPECT_EQ(Position(text_node, 1), first_letter_result.GetLastPosition(1));
  EXPECT_EQ(Position(text_node, 1), remaining_text_result.GetFirstPosition(1));
  EXPECT_EQ(Position(text_node, 1), remaining_text_result.GetLastPosition(1));
}

TEST_P(ParameterizedNGOffsetMappingTest, WhiteSpaceTextNodeWithoutLayoutText) {
  SetupHtml("t", "<div id=t> <span>foo</span></div>");
  Element* div = GetDocument().getElementById("t");
  const Node* text_node = div->firstChild();

  EXPECT_TRUE(EndOfLastNonCollapsedContent(Position(text_node, 1u)).IsNull());
  EXPECT_TRUE(StartOfNextNonCollapsedContent(Position(text_node, 0u)).IsNull());
}

TEST_P(ParameterizedNGOffsetMappingTest,
       OneContainerWithLeadingAndTrailingSpaces) {
  SetupHtml("t", "<div id=t><span id=s>  foo  </span></div>");
  const Node* span = GetElementById("s");
  const Node* text = span->firstChild();
  const NGOffsetMapping& result = GetOffsetMapping();

  // 3 units in total:
  // - collapsed unit for leading spaces
  // - identity unit for "foo"
  // - collapsed unit for trailing spaces

  ASSERT_EQ(2u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), span, 0u, 3u);
  TEST_RANGE(result.GetRanges(), text, 0u, 3u);

  auto unit_range = result.GetMappingUnitsForDOMRange(
      EphemeralRange(Position::BeforeNode(*span), Position::AfterNode(*span)));
  EXPECT_EQ(result.GetUnits().begin(), unit_range.begin());
  EXPECT_EQ(result.GetUnits().end(), unit_range.end());

  EXPECT_EQ(0u, *GetTextContentOffset(Position::BeforeNode(*span)));
  EXPECT_EQ(3u, *GetTextContentOffset(Position::AfterNode(*span)));
}

TEST_P(ParameterizedNGOffsetMappingTest, ContainerWithGeneratedContent) {
  SetupHtml("t",
            "<style>#s::before{content:'bar'} #s::after{content:'baz'}</style>"
            "<div id=t><span id=s>foo</span></div>");
  const Node* span = GetElementById("s");
  const Node* text = span->firstChild();
  const NGOffsetMapping& result = GetOffsetMapping();

  ASSERT_EQ(2u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), span, 0u, 1u);
  TEST_RANGE(result.GetRanges(), text, 0u, 1u);

  auto unit_range = result.GetMappingUnitsForDOMRange(
      EphemeralRange(Position::BeforeNode(*span), Position::AfterNode(*span)));
  EXPECT_EQ(result.GetUnits().begin(), unit_range.begin());
  EXPECT_EQ(result.GetUnits().end(), unit_range.end());

  // Offset mapping for inline containers skips generated content.
  EXPECT_EQ(3u, *GetTextContentOffset(Position::BeforeNode(*span)));
  EXPECT_EQ(6u, *GetTextContentOffset(Position::AfterNode(*span)));
}

TEST_P(ParameterizedNGOffsetMappingTest, Table) {
  SetupHtml("t", "<table><tr><td id=t>  foo  </td></tr></table>");

  const Node* foo_node = layout_object_->GetNode();
  const NGOffsetMapping& result = GetOffsetMapping();

  EXPECT_EQ("foo", result.GetText());

  ASSERT_EQ(3u, result.GetUnits().size());
  TEST_UNIT(result.GetUnits()[0], NGOffsetMappingUnitType::kCollapsed, foo_node,
            0u, 2u, 0u, 0u);
  TEST_UNIT(result.GetUnits()[1], NGOffsetMappingUnitType::kIdentity, foo_node,
            2u, 5u, 0u, 3u);
  TEST_UNIT(result.GetUnits()[2], NGOffsetMappingUnitType::kCollapsed, foo_node,
            5u, 7u, 3u, 3u);

  ASSERT_EQ(1u, result.GetRanges().size());
  TEST_RANGE(result.GetRanges(), foo_node, 0u, 3u);
}

TEST_P(ParameterizedNGOffsetMappingTest, GetMappingForInlineBlock) {
  SetupHtml("t",
            "<div id=t>foo"
            "<span style='display: inline-block' id=span> bar </span>"
            "baz</div>");

  const Element* div = GetElementById("t");
  const Element* span = GetElementById("span");

  const NGOffsetMapping* div_mapping =
      NGOffsetMapping::GetFor(Position(div->firstChild(), 0));
  const NGOffsetMapping* span_mapping =
      NGOffsetMapping::GetFor(Position(span->firstChild(), 0));

  // NGOffsetMapping::GetFor for Before/AfterAnchor of an inline block should
  // return the mapping of the containing block, not of the inline block itself.

  const NGOffsetMapping* span_before_mapping =
      NGOffsetMapping::GetFor(Position::BeforeNode(*span));
  EXPECT_EQ(div_mapping, span_before_mapping);
  EXPECT_NE(span_mapping, span_before_mapping);

  const NGOffsetMapping* span_after_mapping =
      NGOffsetMapping::GetFor(Position::AfterNode(*span));
  EXPECT_EQ(div_mapping, span_after_mapping);
  EXPECT_NE(span_mapping, span_after_mapping);
}

TEST_P(ParameterizedNGOffsetMappingTest, NoWrapSpaceAndCollapsibleSpace) {
  SetupHtml("t",
            "<div id=t>"
            "<span style='white-space: nowrap' id=span>foo </span>"
            " bar"
            "</div>");

  const Element* span = GetElementById("span");
  const Node* foo = span->firstChild();
  const Node* bar = span->nextSibling();
  const NGOffsetMapping& mapping = GetOffsetMapping();

  // NGInlineItemsBuilder inserts a ZWS to indicate break opportunity.
  EXPECT_EQ(String(u"foo \u200Bbar"), mapping.GetText());

  // Should't map any character in DOM to the generated ZWS.
  ASSERT_EQ(3u, mapping.GetUnits().size());
  TEST_UNIT(mapping.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, foo, 0u,
            4u, 0u, 4u);
  TEST_UNIT(mapping.GetUnits()[1], NGOffsetMappingUnitType::kCollapsed, bar, 0u,
            1u, 5u, 5u);
  TEST_UNIT(mapping.GetUnits()[2], NGOffsetMappingUnitType::kIdentity, bar, 1u,
            4u, 5u, 8u);
}

TEST_P(ParameterizedNGOffsetMappingTest, BiDiAroundForcedBreakInPreLine) {
  SetupHtml("t",
            "<div id=t style='white-space: pre-line'>"
            "<bdo dir=rtl id=bdo>foo\nbar</bdo></div>");

  const Node* text = GetElementById("bdo")->firstChild();
  const NGOffsetMapping& mapping = GetOffsetMapping();

  EXPECT_EQ(String(u"\u202Efoo\u202C"
                   u"\n"
                   u"\u202Ebar\u202C"),
            mapping.GetText());

  // Offset mapping should skip generated BiDi control characters.
  ASSERT_EQ(3u, mapping.GetUnits().size());
  TEST_UNIT(mapping.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, text, 0u,
            3u, 1u, 4u);  // "foo"
  TEST_UNIT(mapping.GetUnits()[1], NGOffsetMappingUnitType::kIdentity, text, 3u,
            4u, 5u, 6u);  // "\n"
  TEST_UNIT(mapping.GetUnits()[2], NGOffsetMappingUnitType::kIdentity, text, 4u,
            7u, 7u, 10u);  // "bar"
  TEST_RANGE(mapping.GetRanges(), text, 0u, 3u);
}

TEST_P(ParameterizedNGOffsetMappingTest, BiDiAroundForcedBreakInPreWrap) {
  SetupHtml("t",
            "<div id=t style='white-space: pre-wrap'>"
            "<bdo dir=rtl id=bdo>foo\nbar</bdo></div>");

  const Node* text = GetElementById("bdo")->firstChild();
  const NGOffsetMapping& mapping = GetOffsetMapping();

  EXPECT_EQ(String(u"\u202Efoo\u202C"
                   u"\n"
                   u"\u202Ebar\u202C"),
            mapping.GetText());

  // Offset mapping should skip generated BiDi control characters.
  ASSERT_EQ(3u, mapping.GetUnits().size());
  TEST_UNIT(mapping.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, text, 0u,
            3u, 1u, 4u);  // "foo"
  TEST_UNIT(mapping.GetUnits()[1], NGOffsetMappingUnitType::kIdentity, text, 3u,
            4u, 5u, 6u);  // "\n"
  TEST_UNIT(mapping.GetUnits()[2], NGOffsetMappingUnitType::kIdentity, text, 4u,
            7u, 7u, 10u);  // "bar"
  TEST_RANGE(mapping.GetRanges(), text, 0u, 3u);
}

TEST_P(ParameterizedNGOffsetMappingTest, BiDiAroundForcedBreakInPre) {
  SetupHtml("t",
            "<div id=t style='white-space: pre'>"
            "<bdo dir=rtl id=bdo>foo\nbar</bdo></div>");

  const Node* text = GetElementById("bdo")->firstChild();
  const NGOffsetMapping& mapping = GetOffsetMapping();

  EXPECT_EQ(String(u"\u202Efoo\u202C"
                   u"\n"
                   u"\u202Ebar\u202C"),
            mapping.GetText());

  // Offset mapping should skip generated BiDi control characters.
  ASSERT_EQ(3u, mapping.GetUnits().size());
  TEST_UNIT(mapping.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, text, 0u,
            3u, 1u, 4u);  // "foo"
  TEST_UNIT(mapping.GetUnits()[1], NGOffsetMappingUnitType::kIdentity, text, 3u,
            4u, 5u, 6u);  // "\n"
  TEST_UNIT(mapping.GetUnits()[2], NGOffsetMappingUnitType::kIdentity, text, 4u,
            7u, 7u, 10u);  // "bar"
  TEST_RANGE(mapping.GetRanges(), text, 0u, 3u);
}

TEST_P(ParameterizedNGOffsetMappingTest, SoftHyphen) {
  LoadAhem();
  SetupHtml(
      "t",
      "<div id=t style='font: 10px/10px Ahem; width: 40px'>abc&shy;def</div>");

  const Node* text = GetElementById("t")->firstChild();
  const NGOffsetMapping& mapping = GetOffsetMapping();

  // Line wrapping and hyphenation are oblivious to offset mapping.
  ASSERT_EQ(1u, mapping.GetUnits().size());
  TEST_UNIT(mapping.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, text, 0u,
            7u, 0u, 7u);
  TEST_RANGE(mapping.GetRanges(), text, 0u, 1u);
}

TEST_P(ParameterizedNGOffsetMappingTest, TextOverflowEllipsis) {
  LoadAhem();
  SetupHtml("t",
            "<div id=t style='font: 10px/10px Ahem; width: 30px; overflow: "
            "hidden; text-overflow: ellipsis'>123456</div>");

  const Node* text = GetElementById("t")->firstChild();
  const NGOffsetMapping& mapping = GetOffsetMapping();

  // Ellipsis is oblivious to offset mapping.
  ASSERT_EQ(1u, mapping.GetUnits().size());
  TEST_UNIT(mapping.GetUnits()[0], NGOffsetMappingUnitType::kIdentity, text, 0u,
            6u, 0u, 6u);
  TEST_RANGE(mapping.GetRanges(), text, 0u, 1u);
}

}  // namespace blink
