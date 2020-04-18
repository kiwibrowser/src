// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_node.h"

#include <algorithm>
#include <memory>

#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/layout_text.h"
#include "third_party/blink/renderer/core/layout/ng/inline/layout_ng_text.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_bidi_paragraph.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_break_token.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_item.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_items_builder.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_layout_algorithm.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_line_breaker.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_offset_mapping.h"
#include "third_party/blink/renderer/core/layout/ng/legacy_layout_tree_walking.h"
#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space_builder.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_result.h"
#include "third_party/blink/renderer/core/layout/ng/ng_length_utils.h"
#include "third_party/blink/renderer/core/layout/ng/ng_positioned_float.h"
#include "third_party/blink/renderer/core/layout/ng/ng_unpositioned_float.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/fonts/shaping/harf_buzz_shaper.h"
#include "third_party/blink/renderer/platform/fonts/shaping/shape_result_spacing.h"
#include "third_party/blink/renderer/platform/wtf/text/character_names.h"

namespace blink {

namespace {

// Templated helper function for CollectInlinesInternal().
template <typename OffsetMappingBuilder>
void ClearNeedsLayoutIfUpdatingLayout(LayoutObject* node) {
  node->ClearNeedsLayout();
  node->ClearNeedsCollectInlines();
  // Reset previous items if they cannot be reused to prevent stale items
  // for subsequent layouts. Items that can be reused have already been
  // added to the builder.
  if (node->IsLayoutNGText())
    ToLayoutNGText(node)->ClearInlineItems();
}

template <>
void ClearNeedsLayoutIfUpdatingLayout<NGOffsetMappingBuilder>(LayoutObject*) {}

// The function is templated to indicate the purpose of collected inlines:
// - With EmptyOffsetMappingBuilder: updating layout;
// - With NGOffsetMappingBuilder: building offset mapping on clean layout.
//
// This allows code sharing between the two purposes with slightly different
// behaviors. For example, we clear a LayoutObject's need layout flags when
// updating layout, but don't do that when building offset mapping.
//
// There are also performance considerations, since template saves the overhead
// for condition checking and branching.
template <typename OffsetMappingBuilder>
void CollectInlinesInternal(
    LayoutBlockFlow* block,
    NGInlineItemsBuilderTemplate<OffsetMappingBuilder>* builder,
    String* previous_text) {
  builder->EnterBlock(block->Style());
  LayoutObject* node = GetLayoutObjectForFirstChildNode(block);
  while (node) {
    if (node->IsText()) {
      LayoutText* layout_text = ToLayoutText(node);

      // If the LayoutText element hasn't changed, reuse the existing items.

      // if the last ended with space and this starts with space, do not allow
      // reuse. builder->MightCollapseWithPreceding(*previous_text)
      bool item_reused = false;
      if (node->IsLayoutNGText() && ToLayoutNGText(node)->HasValidLayout() &&
          previous_text) {
        item_reused = builder->Append(*previous_text, ToLayoutNGText(node),
                                      ToLayoutNGText(node)->InlineItems());
      }

      // If not create a new item as needed.
      if (!item_reused) {
        if (UNLIKELY(layout_text->IsWordBreak()))
          builder->AppendBreakOpportunity(node->Style(), layout_text);
        else
          builder->Append(layout_text->GetText(), node->Style(), layout_text);
      }
      ClearNeedsLayoutIfUpdatingLayout<OffsetMappingBuilder>(layout_text);

    } else if (node->IsFloating()) {
      // Add floats and positioned objects in the same way as atomic inlines.
      // Because these objects need positions, they will be handled in
      // NGInlineLayoutAlgorithm.
      builder->AppendOpaque(NGInlineItem::kFloating,
                            kObjectReplacementCharacter, nullptr, node);

    } else if (node->IsOutOfFlowPositioned()) {
      builder->AppendOpaque(NGInlineItem::kOutOfFlowPositioned, nullptr, node);

    } else if (node->IsAtomicInlineLevel()) {
      if (node->IsLayoutNGListMarker()) {
        // LayoutNGListItem produces the 'outside' list marker as an inline
        // block. This is an out-of-flow item whose position is computed
        // automatically.
        builder->AppendOpaque(NGInlineItem::kListMarker, node->Style(), node);
      } else {
        // For atomic inlines add a unicode "object replacement character" to
        // signal the presence of a non-text object to the unicode bidi
        // algorithm.
        builder->AppendAtomicInline(node->Style(), node);
      }

    } else {
      // Because we're collecting from LayoutObject tree, block-level children
      // should not appear. LayoutObject tree should have created an anonymous
      // box to prevent having inline/block-mixed children.
      DCHECK(node->IsInline());

      builder->EnterInline(node);

      // Traverse to children if they exist.
      if (LayoutObject* child = node->SlowFirstChild()) {
        node = child;
        continue;

      } else {
        // An empty inline node.
        ClearNeedsLayoutIfUpdatingLayout<OffsetMappingBuilder>(node);
      }

      builder->ExitInline(node);
    }

    // Find the next sibling, or parent, until we reach |block|.
    while (true) {
      if (LayoutObject* next = node->NextSibling()) {
        node = next;
        break;
      }
      node = GetLayoutObjectForParentNode(node);
      if (node == block) {
        // Set |node| to |nullptr| to break out of the outer loop.
        node = nullptr;
        break;
      }
      DCHECK(node->IsInline());
      builder->ExitInline(node);
      ClearNeedsLayoutIfUpdatingLayout<OffsetMappingBuilder>(node);
    }
  }
  builder->ExitBlock();
}

static bool NeedsShaping(const NGInlineItem& item) {
  return item.Type() == NGInlineItem::kText && !item.TextShapeResult();
}

// Determine if reshape is needed for ::first-line style.
bool FirstLineNeedsReshape(const ComputedStyle& first_line_style,
                           const ComputedStyle& base_style) {
  const Font& base_font = base_style.GetFont();
  const Font& first_line_font = first_line_style.GetFont();
  return &base_font != &first_line_font && base_font != first_line_font;
}

// Make a string to the specified length, either by truncating if longer, or
// appending space characters if shorter.
void TruncateOrPadText(String* text, unsigned length) {
  if (text->length() > length) {
    *text = text->Substring(0, length);
  } else if (text->length() < length) {
    StringBuilder builder;
    builder.ReserveCapacity(length);
    builder.Append(*text);
    while (builder.length() < length)
      builder.Append(kSpaceCharacter);
    *text = builder.ToString();
  }
}

}  // namespace

NGInlineNode::NGInlineNode(LayoutBlockFlow* block)
    : NGLayoutInputNode(block, kInline) {
  DCHECK(block);
  DCHECK(block->IsLayoutNGMixin());
  if (!block->HasNGInlineNodeData())
    block->ResetNGInlineNodeData();
}

bool NGInlineNode::InLineHeightQuirksMode() const {
  return GetDocument().InLineHeightQuirksMode();
}

bool NGInlineNode::CanContainFirstFormattedLine() const {
  // TODO(kojii): In LayoutNG, leading OOF creates an anonymous block box,
  // and that |LayoutBlockFlow::CanContainFirstFormattedLine()| does not work.
  // crbug.com/734554
  LayoutObject* layout_object = GetLayoutBlockFlow();
  if (!layout_object->IsAnonymousBlock())
    return true;
  for (;;) {
    layout_object = layout_object->PreviousSibling();
    if (!layout_object)
      return true;
    if (!layout_object->IsFloatingOrOutOfFlowPositioned())
      return false;
  }
}

NGInlineNodeData* NGInlineNode::MutableData() {
  return ToLayoutBlockFlow(box_)->GetNGInlineNodeData();
}

bool NGInlineNode::IsPrepareLayoutFinished() const {
  const NGInlineNodeData* data = ToLayoutBlockFlow(box_)->GetNGInlineNodeData();
  return data && !data->text_content.IsNull();
}

const NGInlineNodeData& NGInlineNode::Data() const {
  DCHECK(IsPrepareLayoutFinished() &&
         !GetLayoutBlockFlow()->NeedsCollectInlines());
  return *ToLayoutBlockFlow(box_)->GetNGInlineNodeData();
}

void NGInlineNode::InvalidatePrepareLayoutForTest() {
  GetLayoutBlockFlow()->ResetNGInlineNodeData();
  DCHECK(!IsPrepareLayoutFinished());
}

void NGInlineNode::PrepareLayoutIfNeeded() {
  std::unique_ptr<NGInlineNodeData> previous_data;
  LayoutBlockFlow* block_flow = GetLayoutBlockFlow();
  if (IsPrepareLayoutFinished()) {
    if (!block_flow->NeedsCollectInlines())
      return;

    previous_data.reset(block_flow->TakeNGInlineNodeData());
    block_flow->ResetNGInlineNodeData();
  }

  // Scan list of siblings collecting all in-flow non-atomic inlines. A single
  // NGInlineNode represent a collection of adjacent non-atomic inlines.
  NGInlineNodeData* data = MutableData();
  DCHECK(data);
  CollectInlines(data, previous_data.get());
  SegmentText(data);
  ShapeText(data, previous_data.get());
  ShapeTextForFirstLineIfNeeded(data);
  AssociateItemsWithInlines(data);
  DCHECK_EQ(data, MutableData());

  block_flow->ClearNeedsCollectInlines();

#if DCHECK_IS_ON()
  // ComputeOffsetMappingIfNeeded() runs some integrity checks as part of
  // creating offset mapping. Run the check, and discard the result.
  DCHECK(!data->offset_mapping);
  ComputeOffsetMappingIfNeeded();
  DCHECK(data->offset_mapping);
  data->offset_mapping.reset();
#endif
}

const NGInlineNodeData& NGInlineNode::EnsureData() {
  PrepareLayoutIfNeeded();
  return Data();
}

const NGOffsetMapping* NGInlineNode::ComputeOffsetMappingIfNeeded() {
  DCHECK(!GetLayoutBlockFlow()->GetDocument().NeedsLayoutTreeUpdate());

  NGInlineNodeData* data = MutableData();
  if (!data->offset_mapping) {
    // TODO(xiaochengh): ComputeOffsetMappingIfNeeded() discards the
    // NGInlineItems and text content built by |builder|, because they are
    // already there in NGInlineNodeData. For efficiency, we should make
    // |builder| not construct items and text content.
    Vector<NGInlineItem> items;
    NGInlineItemsBuilderForOffsetMapping builder(&items);
    CollectInlinesInternal(GetLayoutBlockFlow(), &builder, nullptr);
    String text = builder.ToString();

    // The trailing space of the text for offset mapping may be removed. If not,
    // share the string instance.
    if (text == data->text_content)
      text = data->text_content;

    // TODO(xiaochengh): This doesn't compute offset mapping correctly when
    // text-transform CSS property changes text length.
    NGOffsetMappingBuilder& mapping_builder = builder.GetOffsetMappingBuilder();
    mapping_builder.SetDestinationString(text);
    data->offset_mapping =
        std::make_unique<NGOffsetMapping>(mapping_builder.Build());
  }

  return data->offset_mapping.get();
}

// Depth-first-scan of all LayoutInline and LayoutText nodes that make up this
// NGInlineNode object. Collects LayoutText items, merging them up into the
// parent LayoutInline where possible, and joining all text content in a single
// string to allow bidi resolution and shaping of the entire block.
void NGInlineNode::CollectInlines(NGInlineNodeData* data,
                                  NGInlineNodeData* previous_data) {
  DCHECK(data->text_content.IsNull());
  DCHECK(data->items.IsEmpty());
  LayoutBlockFlow* block = GetLayoutBlockFlow();
  block->WillCollectInlines();

  String* previous_text =
      previous_data ? &previous_data->text_content : nullptr;
  NGInlineItemsBuilder builder(&data->items);
  CollectInlinesInternal(block, &builder, previous_text);
  data->text_content = builder.ToString();

  // Set |is_bidi_enabled_| for all UTF-16 strings for now, because at this
  // point the string may or may not contain RTL characters.
  // |SegmentText()| will analyze the text and reset |is_bidi_enabled_| if it
  // doesn't contain any RTL characters.
  data->is_bidi_enabled_ =
      !data->text_content.Is8Bit() || builder.HasBidiControls();
  data->is_empty_inline_ = builder.IsEmptyInline();
}

void NGInlineNode::SegmentText(NGInlineNodeData* data) {
  if (!data->is_bidi_enabled_) {
    data->SetBaseDirection(TextDirection::kLtr);
    return;
  }

  NGBidiParagraph bidi;
  data->text_content.Ensure16Bit();
  if (!bidi.SetParagraph(data->text_content, Style())) {
    // On failure, give up bidi resolving and reordering.
    data->is_bidi_enabled_ = false;
    data->SetBaseDirection(TextDirection::kLtr);
    return;
  }

  data->SetBaseDirection(bidi.BaseDirection());

  if (bidi.IsUnidirectional() && IsLtr(bidi.BaseDirection())) {
    // All runs are LTR, no need to reorder.
    data->is_bidi_enabled_ = false;
    return;
  }

  Vector<NGInlineItem>& items = data->items;
  unsigned item_index = 0;
  for (unsigned start = 0; start < data->text_content.length();) {
    UBiDiLevel level;
    unsigned end = bidi.GetLogicalRun(start, &level);
    DCHECK_EQ(items[item_index].start_offset_, start);
    item_index = NGInlineItem::SetBidiLevel(items, item_index, end, level);
    start = end;
  }
#if DCHECK_IS_ON()
  // Check all items have bidi levels, except trailing non-length items.
  // Items that do not create break opportunities such as kOutOfFlowPositioned
  // do not have corresponding characters, and that they do not have bidi level
  // assigned.
  while (item_index < items.size() && !items[item_index].Length())
    item_index++;
  DCHECK_EQ(item_index, items.size());
#endif
}

void NGInlineNode::ShapeText(NGInlineItemsData* data,
                             NGInlineItemsData* previous_data) {
  // TODO(eae): Add support for shaping latin-1 text?
  data->text_content.Ensure16Bit();
  ShapeText(data->text_content, &data->items,
            previous_data ? &previous_data->text_content : nullptr);
}

void NGInlineNode::ShapeText(const String& text_content,
                             Vector<NGInlineItem>* items,
                             const String* previous_text) {
  // Provide full context of the entire node to the shaper.
  HarfBuzzShaper shaper(text_content.Characters16(), text_content.length());
  ShapeResultSpacing<String> spacing(text_content);

  for (unsigned index = 0; index < items->size();) {
    NGInlineItem& start_item = (*items)[index];
    if (start_item.Type() != NGInlineItem::kText) {
      index++;
      continue;
    }

    const Font& font = start_item.Style()->GetFont();
    TextDirection direction = start_item.Direction();
    unsigned end_index = index + 1;
    unsigned end_offset = start_item.EndOffset();
    for (; end_index < items->size(); end_index++) {
      const NGInlineItem& item = (*items)[end_index];

      if (item.Type() == NGInlineItem::kControl) {
        // Do not shape across control characters (line breaks, zero width
        // spaces, etc).
        break;
      }
      if (item.Type() == NGInlineItem::kText) {
        // Shape adjacent items together if the font and direction matches to
        // allow ligatures and kerning to apply.
        // TODO(kojii): Figure out the exact conditions under which this
        // behavior is desirable.
        if (font != item.Style()->GetFont() || direction != item.Direction())
          break;
        end_offset = item.EndOffset();
      } else if (item.Type() == NGInlineItem::kOpenTag ||
                 item.Type() == NGInlineItem::kCloseTag ||
                 item.Type() == NGInlineItem::kOutOfFlowPositioned) {
        // These items are opaque to shaping.
        // Opaque items cannot have text, such as Object Replacement Characters,
        // since such characters can affect shaping.
        DCHECK_EQ(0u, item.Length());
      } else {
        break;
      }
    }

    // Shaping a single item. Skip if the existing results remain valid.
    if (previous_text && end_offset == start_item.EndOffset() &&
        !NeedsShaping(start_item)) {
      DCHECK_EQ(start_item.StartOffset(),
                start_item.TextShapeResult()->StartIndexForResult());
      DCHECK_EQ(start_item.EndOffset(),
                start_item.TextShapeResult()->EndIndexForResult());
      index++;
      continue;
    }

    // Results may only be reused if all items in the range remain valid.
    bool has_valid_shape_results = true;
    for (unsigned item_index = index; item_index < end_index; item_index++) {
      if (NeedsShaping((*items)[item_index])) {
        has_valid_shape_results = false;
        break;
      }
    }

    // When shaping across multiple items checking whether the individual
    // items has valid shape results isn't sufficient as items may have been
    // re-ordered or removed.
    // TODO(layout-dev): It would probably be faster to check for removed or
    // moved items but for now comparing the string itself will do.
    unsigned text_start = start_item.StartOffset();
    DCHECK_GE(end_offset, text_start);
    unsigned text_length = end_offset - text_start;
    if (has_valid_shape_results && previous_text &&
        end_offset <= previous_text->length() &&
        StringView(text_content, text_start, text_length) ==
            StringView(*previous_text, text_start, text_length)) {
      index = end_index;
      continue;
    }

    // Shape each item with the full context of the entire node.
    scoped_refptr<ShapeResult> shape_result =
        shaper.Shape(&font, direction, start_item.StartOffset(), end_offset);
    if (UNLIKELY(spacing.SetSpacing(font.GetFontDescription())))
      shape_result->ApplySpacing(spacing);

    // If the text is from one item, use the ShapeResult as is.
    if (end_offset == start_item.EndOffset()) {
      start_item.shape_result_ = std::move(shape_result);
      index++;
      continue;
    }

    // If the text is from multiple items, split the ShapeResult to
    // corresponding items.
    for (; index < end_index; index++) {
      NGInlineItem& item = (*items)[index];
      if (item.Type() != NGInlineItem::kText)
        continue;

      // We don't use SafeToBreak API here because this is not a line break.
      // The ShapeResult is broken into multiple results, but they must look
      // like they were not broken.
      //
      // When multiple code units shape to one glyph, such as ligatures, the
      // item that has its first code unit keeps the glyph.
      item.shape_result_ =
          shape_result->SubRange(item.StartOffset(), item.EndOffset());
    }
  }
}

// Create Vector<NGInlineItem> with :first-line rules applied if needed.
void NGInlineNode::ShapeTextForFirstLineIfNeeded(NGInlineNodeData* data) {
  // First check if the document has any :first-line rules.
  DCHECK(!data->first_line_items_);
  LayoutObject* layout_object = GetLayoutObject();
  if (!layout_object->GetDocument().GetStyleEngine().UsesFirstLineRules())
    return;

  // Check if :first-line rules make any differences in the style.
  const ComputedStyle* block_style = layout_object->Style();
  const ComputedStyle* first_line_style = layout_object->FirstLineStyle();
  if (block_style == first_line_style)
    return;

  auto first_line_items = std::make_unique<NGInlineItemsData>();
  first_line_items->text_content = data->text_content;
  bool needs_reshape = false;
  if (first_line_style->TextTransform() != block_style->TextTransform()) {
    // TODO(kojii): This logic assumes that text-transform is applied only to
    // ::first-line, and does not work when the base style has text-transform
    // and ::first-line has different text-transform.
    first_line_style->ApplyTextTransform(&first_line_items->text_content);
    if (first_line_items->text_content != data->text_content) {
      // TODO(kojii): When text-transform changes the length, we need to adjust
      // offset in NGInlineItem, or re-collect inlines. Other classes such as
      // line breaker need to support the scenario too. For now, we force the
      // string to be the same length to prevent them from crashing. This may
      // result in a missing or a duplicate character if the length changes.
      TruncateOrPadText(&first_line_items->text_content,
                        data->text_content.length());
      needs_reshape = true;
    }
  }

  first_line_items->items.AppendVector(data->items);
  for (auto& item : first_line_items->items) {
    if (item.style_) {
      DCHECK(item.layout_object_);
      item.style_ = item.layout_object_->FirstLineStyle();
      item.SetStyleVariant(NGStyleVariant::kFirstLine);
    }
  }

  // Re-shape if the font is different.
  if (needs_reshape || FirstLineNeedsReshape(*first_line_style, *block_style))
    ShapeText(first_line_items.get());

  data->first_line_items_ = std::move(first_line_items);
}

void NGInlineNode::AssociateItemsWithInlines(NGInlineNodeData* data) {
  LayoutObject* last_object = nullptr;
  for (auto& item : data->items) {
    LayoutObject* object = item.GetLayoutObject();
    if (object && object->IsLayoutNGText()) {
      LayoutNGText* layout_text = ToLayoutNGText(object);
      if (object != last_object)
        layout_text->ClearInlineItems();
      layout_text->AddInlineItem(&item);
    }
    last_object = object;
  }
}

scoped_refptr<NGLayoutResult> NGInlineNode::Layout(
    const NGConstraintSpace& constraint_space,
    NGBreakToken* break_token) {
  PrepareLayoutIfNeeded();

  NGInlineLayoutAlgorithm algorithm(*this, constraint_space,
                                    ToNGInlineBreakToken(break_token));
  return algorithm.Layout();
}

static LayoutUnit ComputeContentSize(NGInlineNode node,
                                     const MinMaxSizeInput& input,
                                     NGLineBreakerMode mode) {
  const ComputedStyle& style = node.Style();
  WritingMode writing_mode = style.GetWritingMode();
  LayoutUnit available_inline_size =
      mode == NGLineBreakerMode::kMaxContent ? LayoutUnit::Max() : LayoutUnit();

  scoped_refptr<NGConstraintSpace> space =
      NGConstraintSpaceBuilder(writing_mode, node.InitialContainingBlockSize())
          .SetTextDirection(style.Direction())
          .SetAvailableSize({available_inline_size, NGSizeIndefinite})
          .SetIsIntermediateLayout(true)
          .ToConstraintSpace(writing_mode);

  Vector<NGPositionedFloat> positioned_floats;
  Vector<scoped_refptr<NGUnpositionedFloat>> unpositioned_floats;

  scoped_refptr<NGInlineBreakToken> break_token;
  NGLineInfo line_info;
  NGExclusionSpace empty_exclusion_space;
  NGLineLayoutOpportunity line_opportunity(available_inline_size);
  LayoutUnit result;
  LayoutUnit previous_floats_inline_size =
      input.float_left_inline_size + input.float_right_inline_size;
  while (!break_token || !break_token->IsFinished()) {
    unpositioned_floats.clear();

    NGLineBreaker line_breaker(node, mode, *space, &positioned_floats,
                               &unpositioned_floats,
                               nullptr /* container_builder */,
                               &empty_exclusion_space, 0u, break_token.get());
    if (!line_breaker.NextLine(line_opportunity, &line_info))
      break;

    break_token = line_breaker.CreateBreakToken(line_info, nullptr);
    LayoutUnit inline_size = line_info.TextIndent();
    for (const NGInlineItemResult item_result : line_info.Results())
      inline_size += item_result.inline_size;

    // There should be no positioned floats while determining the min/max sizes.
    DCHECK_EQ(positioned_floats.size(), 0u);

    // These variables are only used for the max-content calculation.
    LayoutUnit floats_inline_size = mode == NGLineBreakerMode::kMaxContent
                                        ? previous_floats_inline_size
                                        : LayoutUnit();
    EFloat previous_float_type = EFloat::kNone;

    // Earlier floats can only be assumed to affect the first line, so clear
    // them now.
    previous_floats_inline_size = LayoutUnit();

    for (const auto& unpositioned_float : unpositioned_floats) {
      NGBlockNode float_node = unpositioned_float->node;
      const ComputedStyle& float_style = float_node.Style();

      MinMaxSizeInput zero_input;  // Floats don't intrude into floats.
      MinMaxSize child_sizes = ComputeMinAndMaxContentContribution(
          writing_mode, float_node, zero_input);
      LayoutUnit child_inline_margins =
          ComputeMinMaxMargins(style, float_node).InlineSum();

      if (mode == NGLineBreakerMode::kMinContent) {
        result = std::max(result, child_sizes.min_size + child_inline_margins);
      } else {
        const EClear float_clear = float_style.Clear();

        // If this float clears the previous float we start a new "line".
        // This is subtly different to block layout which will only reset either
        // the left or the right float size trackers.
        if ((previous_float_type == EFloat::kLeft &&
             (float_clear == EClear::kBoth || float_clear == EClear::kLeft)) ||
            (previous_float_type == EFloat::kRight &&
             (float_clear == EClear::kBoth || float_clear == EClear::kRight))) {
          result = std::max(result, inline_size + floats_inline_size);
          floats_inline_size = LayoutUnit();
        }

        floats_inline_size += child_sizes.max_size + child_inline_margins;
        previous_float_type = float_style.Floating();
      }
    }

    // NOTE: floats_inline_size will be zero for the min-content calculation,
    // and will just take the inline size of the un-breakable line.
    result = std::max(result, inline_size + floats_inline_size);
  }

  return result;
}

MinMaxSize NGInlineNode::ComputeMinMaxSize(const MinMaxSizeInput& input) {
  PrepareLayoutIfNeeded();

  // Run line breaking with 0 and indefinite available width.

  // TODO(kojii): There are several ways to make this more efficient and faster
  // than runnning two line breaking.

  // Compute the max of inline sizes of all line boxes with 0 available inline
  // size. This gives the min-content, the width where lines wrap at every
  // break opportunity.
  MinMaxSize sizes;
  sizes.min_size =
      ComputeContentSize(*this, input, NGLineBreakerMode::kMinContent);

  // Compute the sum of inline sizes of all inline boxes with no line breaks.
  // TODO(kojii): NGConstraintSpaceBuilder does not allow NGSizeIndefinite
  // inline available size. We can allow it, or make this more efficient
  // without using NGLineBreaker.
  sizes.max_size =
      ComputeContentSize(*this, input, NGLineBreakerMode::kMaxContent);

  // Negative text-indent can make min > max. Ensure min is the minimum size.
  sizes.min_size = std::min(sizes.min_size, sizes.max_size);

  return sizes;
}

void NGInlineNode::CheckConsistency() const {
#if DCHECK_IS_ON()
  const Vector<NGInlineItem>& items = Data().items;
  for (const NGInlineItem& item : items) {
    DCHECK(!item.GetLayoutObject() || !item.Style() ||
           item.Style() == item.GetLayoutObject()->Style());
  }
#endif
}

String NGInlineNode::ToString() const {
  return String::Format("NGInlineNode");
}

}  // namespace blink
