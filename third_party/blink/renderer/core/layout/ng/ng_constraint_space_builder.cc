// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space_builder.h"

#include "third_party/blink/renderer/core/layout/ng/exclusions/ng_exclusion_space.h"

namespace blink {

NGConstraintSpaceBuilder::NGConstraintSpaceBuilder(
    const NGConstraintSpace& parent_space)
    : NGConstraintSpaceBuilder(parent_space.GetWritingMode(),
                               parent_space.InitialContainingBlockSize()) {
  parent_percentage_resolution_size_ = parent_space.PercentageResolutionSize();
  is_intermediate_layout_ = parent_space.IsIntermediateLayout();
}

NGConstraintSpaceBuilder::NGConstraintSpaceBuilder(WritingMode writing_mode,
                                                   NGPhysicalSize icb_size)
    : initial_containing_block_size_(icb_size),
      parent_writing_mode_(writing_mode) {}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetAvailableSize(
    NGLogicalSize available_size) {
  available_size_ = available_size;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetPercentageResolutionSize(
    NGLogicalSize percentage_resolution_size) {
  percentage_resolution_size_ = percentage_resolution_size;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetTextDirection(
    TextDirection text_direction) {
  text_direction_ = text_direction;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetMarginStrut(
    const NGMarginStrut& margin_strut) {
  margin_strut_ = margin_strut;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetBfcOffset(
    const NGBfcOffset& bfc_offset) {
  bfc_offset_ = bfc_offset;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetFloatsBfcOffset(
    const base::Optional<NGBfcOffset>& floats_bfc_offset) {
  floats_bfc_offset_ = floats_bfc_offset;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetClearanceOffset(
    LayoutUnit clearance_offset) {
  clearance_offset_ = clearance_offset;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetExclusionSpace(
    const NGExclusionSpace& exclusion_space) {
  exclusion_space_ = &exclusion_space;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetIsFixedSizeInline(
    bool is_fixed_size_inline) {
  is_fixed_size_inline_ = is_fixed_size_inline;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetIsFixedSizeBlock(
    bool is_fixed_size_block) {
  is_fixed_size_block_ = is_fixed_size_block;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetFixedSizeBlockIsDefinite(
    bool fixed_size_block_is_definite) {
  fixed_size_block_is_definite_ = fixed_size_block_is_definite;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetIsShrinkToFit(
    bool shrink_to_fit) {
  is_shrink_to_fit_ = shrink_to_fit;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetIsIntermediateLayout(
    bool is_intermediate_layout) {
  is_intermediate_layout_ = is_intermediate_layout;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetFragmentationType(
    NGFragmentationType fragmentation_type) {
  fragmentation_type_ = fragmentation_type;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetIsNewFormattingContext(
    bool is_new_fc) {
  is_new_fc_ = is_new_fc;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetIsAnonymous(
    bool is_anonymous) {
  is_anonymous_ = is_anonymous;
  return *this;
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::SetUseFirstLineStyle(
    bool use_first_line_style) {
  use_first_line_style_ = use_first_line_style;
  return *this;
}

void NGConstraintSpaceBuilder::AddBaselineRequests(
    const Vector<NGBaselineRequest>& requests) {
  DCHECK(baseline_requests_.IsEmpty());
  baseline_requests_.AppendVector(requests);
}

NGConstraintSpaceBuilder& NGConstraintSpaceBuilder::AddBaselineRequest(
    const NGBaselineRequest& request) {
  for (const auto& existing : baseline_requests_) {
    if (existing == request)
      return *this;
  }
  baseline_requests_.push_back(request);
  return *this;
}

scoped_refptr<NGConstraintSpace> NGConstraintSpaceBuilder::ToConstraintSpace(
    WritingMode out_writing_mode) {
  // Whether the child and the containing block are parallel to each other.
  // Example: vertical-rl and vertical-lr
  bool is_in_parallel_flow =
      IsParallelWritingMode(parent_writing_mode_, out_writing_mode);

  NGLogicalSize available_size = available_size_;
  NGLogicalSize percentage_resolution_size = percentage_resolution_size_;
  NGLogicalSize parent_percentage_resolution_size =
      parent_percentage_resolution_size_.value_or(percentage_resolution_size);
  bool is_fixed_size_inline = is_fixed_size_inline_;
  bool is_fixed_size_block = is_fixed_size_block_;
  bool fixed_size_block_is_definite = fixed_size_block_is_definite_;
  if (!is_in_parallel_flow) {
    available_size.Flip();
    percentage_resolution_size.Flip();
    parent_percentage_resolution_size.Flip();
    is_fixed_size_inline = is_fixed_size_block_;
    is_fixed_size_block = is_fixed_size_inline_;
    fixed_size_block_is_definite = true;
  }

  // If inline size is indefinite, use size of initial containing block.
  // https://www.w3.org/TR/css-writing-modes-3/#orthogonal-auto
  if (available_size.inline_size == NGSizeIndefinite) {
    DCHECK(!is_in_parallel_flow);
    if (out_writing_mode == WritingMode::kHorizontalTb) {
      available_size.inline_size = initial_containing_block_size_.width;
    } else {
      available_size.inline_size = initial_containing_block_size_.height;
    }
  }
  if (percentage_resolution_size.inline_size == NGSizeIndefinite) {
    DCHECK(!is_in_parallel_flow);
    if (out_writing_mode == WritingMode::kHorizontalTb) {
      percentage_resolution_size.inline_size =
          initial_containing_block_size_.width;
    } else {
      percentage_resolution_size.inline_size =
          initial_containing_block_size_.height;
    }
  }

  DEFINE_STATIC_LOCAL(NGExclusionSpace, empty_exclusion_space, ());

  DCHECK(!is_new_fc_ || !adjoining_floats_);

  const NGExclusionSpace& exclusion_space = (is_new_fc_ || !exclusion_space_)
                                                ? empty_exclusion_space
                                                : *exclusion_space_;
  NGBfcOffset bfc_offset = is_new_fc_ ? NGBfcOffset() : bfc_offset_;
  NGMarginStrut margin_strut = is_new_fc_ ? NGMarginStrut() : margin_strut_;
  LayoutUnit clearance_offset =
      is_new_fc_ ? LayoutUnit::Min() : clearance_offset_;
  base::Optional<NGBfcOffset> floats_bfc_offset =
      is_new_fc_ ? base::nullopt : floats_bfc_offset_;

  if (floats_bfc_offset) {
    floats_bfc_offset = NGBfcOffset(
        {bfc_offset.line_offset, floats_bfc_offset.value().block_offset});
  }

  return base::AdoptRef(new NGConstraintSpace(
      out_writing_mode, !is_in_parallel_flow, text_direction_, available_size,
      percentage_resolution_size, parent_percentage_resolution_size.inline_size,
      initial_containing_block_size_, fragmentainer_block_size_,
      fragmentainer_space_at_bfc_start_, is_fixed_size_inline,
      is_fixed_size_block, fixed_size_block_is_definite, is_shrink_to_fit_,
      is_intermediate_layout_, fragmentation_type_,
      separate_leading_fragmentainer_margins_, is_new_fc_, is_anonymous_,
      use_first_line_style_, should_force_clearance_, adjoining_floats_,
      margin_strut, bfc_offset, floats_bfc_offset, exclusion_space,
      clearance_offset, baseline_requests_));
}

}  // namespace blink
