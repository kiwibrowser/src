// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_offset_mapping_builder.h"

#include "third_party/blink/renderer/core/layout/layout_text.h"
#include "third_party/blink/renderer/core/layout/layout_text_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_offset_mapping.h"

namespace blink {

namespace {

// Returns the type of a unit-length simple offset mapping.
NGOffsetMappingUnitType GetUnitLengthMappingType(unsigned value) {
  if (value == 0u)
    return NGOffsetMappingUnitType::kCollapsed;
  if (value == 1u)
    return NGOffsetMappingUnitType::kIdentity;
  return NGOffsetMappingUnitType::kExpanded;
}

// Returns the associated node of a possibly null LayoutObject.
const Node* GetAssociatedNode(const LayoutObject* layout_object) {
  if (!layout_object)
    return nullptr;
  if (!layout_object->IsText() ||
      !ToLayoutText(layout_object)->IsTextFragment())
    return layout_object->NonPseudoNode();
  const LayoutTextFragment* fragment = ToLayoutTextFragment(layout_object);
  return fragment->AssociatedTextNode();
}

// Finds the offset mapping unit starting from index |start|.
std::pair<NGOffsetMappingUnitType, unsigned> GetMappingUnitTypeAndEnd(
    const Vector<unsigned>& mapping,
    const Vector<const LayoutObject*>& annotation,
    unsigned start) {
  DCHECK_LT(start + 1, mapping.size());
  NGOffsetMappingUnitType type =
      GetUnitLengthMappingType(mapping[start + 1] - mapping[start]);
  if (type == NGOffsetMappingUnitType::kExpanded)
    return std::make_pair(type, start + 1);

  unsigned end = start + 1;
  for (; end + 1 < mapping.size(); ++end) {
    if (annotation[end] != annotation[start])
      break;
    NGOffsetMappingUnitType next_type =
        GetUnitLengthMappingType(mapping[end + 1] - mapping[end]);
    if (next_type != type)
      break;
  }
  return std::make_pair(type, end);
}

// If |layout_object| is the remaining text of a text node, returns the start
// offset; In all other cases, returns 0.
unsigned GetRemainingTextOffset(const LayoutObject* layout_object) {
  if (!layout_object || !layout_object->IsText())
    return 0;
  return ToLayoutText(layout_object)->TextStartOffset();
}

}  // namespace

NGOffsetMappingBuilder::NGOffsetMappingBuilder() {
  mapping_.push_back(0);
}

NGOffsetMappingBuilder::SourceNodeScope::SourceNodeScope(
    NGOffsetMappingBuilder* builder,
    const LayoutObject* node)
    : auto_reset_(&builder->current_source_node_, node) {
#if DCHECK_IS_ON()
  builder_ = builder;
  if (!node)
    return;
  // We allow at most one scope with non-null node at any time.
  DCHECK(!builder->has_nonnull_node_scope_);
  builder->has_nonnull_node_scope_ = true;
#endif
}

NGOffsetMappingBuilder::SourceNodeScope::~SourceNodeScope() {
#if DCHECK_IS_ON()
  if (builder_->current_source_node_)
    builder_->has_nonnull_node_scope_ = false;
#endif
}

void NGOffsetMappingBuilder::AppendIdentityMapping(unsigned length) {
  DCHECK_GT(length, 0u);
  DCHECK(!mapping_.IsEmpty());
  for (unsigned i = 0; i < length; ++i) {
    unsigned next = mapping_.back() + 1;
    mapping_.push_back(next);
  }
  annotation_.resize(annotation_.size() + length);
  std::fill(annotation_.end() - length, annotation_.end(),
            current_source_node_);
}

void NGOffsetMappingBuilder::AppendCollapsedMapping(unsigned length) {
  DCHECK_GT(length, 0u);
  DCHECK(!mapping_.IsEmpty());
  const unsigned back = mapping_.back();
  for (unsigned i = 0; i < length; ++i)
    mapping_.push_back(back);
  annotation_.resize(annotation_.size() + length);
  std::fill(annotation_.end() - length, annotation_.end(),
            current_source_node_);
}

void NGOffsetMappingBuilder::CollapseTrailingSpace(unsigned skip_length) {
  DCHECK(!mapping_.IsEmpty());

  // Find the |skipped_count + 1|-st last uncollapsed character. By collapsing
  // it, all mapping values beyond this position are decremented by 1.
  unsigned skipped_count = 0;
  for (unsigned i = mapping_.size() - 1; skipped_count <= skip_length; --i) {
    DCHECK_GT(i, 0u);
    if (mapping_[i] != mapping_[i - 1])
      ++skipped_count;
    --mapping_[i];
  }
}

void NGOffsetMappingBuilder::Concatenate(const NGOffsetMappingBuilder& other) {
  DCHECK(!mapping_.IsEmpty());
  DCHECK(!other.mapping_.IsEmpty());
  const unsigned shift_amount = mapping_.back();
  for (unsigned i = 1; i < other.mapping_.size(); ++i)
    mapping_.push_back(other.mapping_[i] + shift_amount);
  annotation_.AppendVector(other.annotation_);
}

void NGOffsetMappingBuilder::Composite(const NGOffsetMappingBuilder& other) {
  DCHECK(!mapping_.IsEmpty());
  DCHECK_EQ(mapping_.back() + 1, other.mapping_.size());
  for (unsigned i = 0; i < mapping_.size(); ++i)
    mapping_[i] = other.mapping_[mapping_[i]];
}

void NGOffsetMappingBuilder::SetDestinationString(String string) {
  DCHECK_EQ(mapping_.back(), string.length());
  destination_string_ = string;
}

NGOffsetMapping NGOffsetMappingBuilder::Build() {
  NGOffsetMapping::UnitVector units;
  NGOffsetMapping::RangeMap ranges;

  // Information of current owner node
  const Node* nonnull_owner = nullptr;
  unsigned processed_length = 0;
  unsigned unit_range_start = 0;
  unsigned remaining_text_offset = 0;

  // Information of non-atomic inlines that we are currently in
  using NodeAndStart = std::pair<const Node*, unsigned>;
  Vector<NodeAndStart> current_inlines;

  // Sentinels
  annotation_.push_back(nullptr);
  boundaries_.push_back(InlineBoundary({nullptr, mapping_.size() - 1, false}));

  const InlineBoundary* boundary_it = boundaries_.begin();
  unsigned unit_start = 0;
  while (unit_start + 1 < mapping_.size() ||
         std::next(boundary_it) != boundaries_.end()) {
    // Handle boundaries of non-atomic inline nodes
    if (std::next(boundary_it) != boundaries_.end() &&
        boundary_it->offset <= unit_start) {
      const InlineBoundary& boundary = *boundary_it++;
      const Node* node = GetAssociatedNode(boundary.node);
      // Skip generated content
      if (!node)
        continue;

      // Enter non-atomic inline
      if (boundary.is_enter) {
        current_inlines.push_back(NodeAndStart({node, units.size()}));
        continue;
      }

      // Exit non-atomic inline
      DCHECK(current_inlines.size());
      DCHECK_EQ(node, current_inlines.back().first);
      const unsigned node_unit_range_start = current_inlines.back().second;
      if (units.size() > node_unit_range_start) {
        ranges.insert(node,
                      std::make_pair(node_unit_range_start, units.size()));
      }
      current_inlines.pop_back();
      continue;
    }

    // Create mapping units and/or unit ranges, if any
    DCHECK_LT(unit_start, annotation_.size());
    const Node* node = GetAssociatedNode(annotation_[unit_start]);
    const auto type_and_end =
        GetMappingUnitTypeAndEnd(mapping_, annotation_, unit_start);
    unsigned unit_end = type_and_end.second;
    // Create unit only for non-generated content.
    if (node) {
      if (node != nonnull_owner) {
        nonnull_owner = node;
        processed_length = 0;
      }

      if (!unit_start ||
          node != GetAssociatedNode(annotation_[unit_start - 1])) {
        unit_range_start = units.size();
        // We get a non-zero |remaining_text_offset| only when |current_node| is
        // a text node that has blockified ::first-letter style, and we are at
        // the remaining text of |current_node|.
        remaining_text_offset = GetRemainingTextOffset(annotation_[unit_start]);
      }

      NGOffsetMappingUnitType type = type_and_end.first;
      unsigned dom_start = processed_length + remaining_text_offset;
      unsigned dom_end = unit_end - unit_start + dom_start;
      unsigned text_content_start = mapping_[unit_start];
      unsigned text_content_end = mapping_[unit_end];
      units.emplace_back(type, *node, dom_start, dom_end, text_content_start,
                         text_content_end);

      processed_length += dom_end - dom_start;

      if (GetAssociatedNode(annotation_[unit_end]) != node) {
        auto iter = ranges.find(node);
        if (iter != ranges.end()) {
          // We are here if characters of |node| were appended inconsecutively,
          // separated by generated characters. An example is BiDi-overridden
          // text node with 'white-space: pre' style, where generated BiDi-
          // control characters are inserted around '\n' characters.
          DCHECK_EQ(iter->value.second, unit_range_start);
          iter->value.second = units.size();
        } else {
          ranges.insert(node, std::make_pair(unit_range_start, units.size()));
        }
      }
    }
    unit_start = unit_end;
  }

  return NGOffsetMapping(std::move(units), std::move(ranges),
                         destination_string_);
}

void NGOffsetMappingBuilder::EnterInline(const LayoutObject& node) {
  boundaries_.push_back(InlineBoundary({&node, mapping_.size() - 1, true}));
}

void NGOffsetMappingBuilder::ExitInline(const LayoutObject& node) {
  boundaries_.push_back(InlineBoundary({&node, mapping_.size() - 1, false}));
}

Vector<unsigned> NGOffsetMappingBuilder::DumpOffsetMappingForTesting() const {
  return mapping_;
}

Vector<const LayoutObject*> NGOffsetMappingBuilder::DumpAnnotationForTesting()
    const {
  return annotation_;
}

}  // namespace blink
