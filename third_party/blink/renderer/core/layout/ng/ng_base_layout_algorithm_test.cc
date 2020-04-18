// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_base_layout_algorithm_test.h"

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/layout/ng/layout_ng_block_flow.h"
#include "third_party/blink/renderer/core/layout/ng/ng_block_layout_algorithm.h"
#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space_builder.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_result.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_box_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_fragment.h"

namespace blink {

void NGBaseLayoutAlgorithmTest::SetUp() {
  NGLayoutTest::SetUp();
  EnableCompositing();
}

std::pair<scoped_refptr<NGPhysicalBoxFragment>,
          scoped_refptr<NGConstraintSpace>>
NGBaseLayoutAlgorithmTest::RunBlockLayoutAlgorithmForElement(Element* element) {
  LayoutBlockFlow* block_flow = ToLayoutBlockFlow(element->GetLayoutObject());
  NGBlockNode node(block_flow);
  scoped_refptr<NGConstraintSpace> space =
      NGConstraintSpace::CreateFromLayoutObject(*block_flow);

  scoped_refptr<NGLayoutResult> result =
      NGBlockLayoutAlgorithm(node, *space).Layout();
  return std::make_pair(
      ToNGPhysicalBoxFragment(result->PhysicalFragment().get()),
      std::move(space));
}

scoped_refptr<const NGPhysicalBoxFragment>
NGBaseLayoutAlgorithmTest::GetBoxFragmentByElementId(const char* id) {
  LayoutObject* layout_object = GetLayoutObjectByElementId(id);
  CHECK(layout_object && layout_object->IsLayoutNGMixin());
  scoped_refptr<const NGPhysicalBoxFragment> fragment =
      ToLayoutBlockFlow(layout_object)->CurrentFragment();
  CHECK(fragment);
  return fragment;
}

const NGPhysicalBoxFragment* NGBaseLayoutAlgorithmTest::CurrentFragmentFor(
    const LayoutNGBlockFlow* block_flow) {
  return block_flow->CurrentFragment();
}

const NGPhysicalBoxFragment* FragmentChildIterator::NextChild() {
  if (!parent_)
    return nullptr;
  if (index_ >= parent_->Children().size())
    return nullptr;
  while (parent_->Children()[index_]->Type() !=
         NGPhysicalFragment::kFragmentBox) {
    ++index_;
    if (index_ >= parent_->Children().size())
      return nullptr;
  }
  return ToNGPhysicalBoxFragment(parent_->Children()[index_++].get());
}

scoped_refptr<NGConstraintSpace> ConstructBlockLayoutTestConstraintSpace(
    WritingMode writing_mode,
    TextDirection direction,
    NGLogicalSize size,
    bool shrink_to_fit,
    bool is_new_formatting_context,
    LayoutUnit fragmentainer_space_available) {
  NGFragmentationType block_fragmentation =
      fragmentainer_space_available != LayoutUnit()
          ? NGFragmentationType::kFragmentColumn
          : NGFragmentationType::kFragmentNone;

  return NGConstraintSpaceBuilder(
             writing_mode,
             /* icb_size */ size.ConvertToPhysical(writing_mode))
      .SetAvailableSize(size)
      .SetPercentageResolutionSize(size)
      .SetTextDirection(direction)
      .SetIsShrinkToFit(shrink_to_fit)
      .SetIsNewFormattingContext(is_new_formatting_context)
      .SetFragmentainerSpaceAtBfcStart(fragmentainer_space_available)
      .SetFragmentationType(block_fragmentation)
      .ToConstraintSpace(writing_mode);
}

}  // namespace blink
