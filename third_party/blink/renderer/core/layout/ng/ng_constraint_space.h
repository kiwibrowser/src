// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGConstraintSpace_h
#define NGConstraintSpace_h

#include "base/optional.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/exclusions/ng_exclusion_space.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_bfc_offset.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_size.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_margin_strut.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_size.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_baseline.h"
#include "third_party/blink/renderer/core/layout/ng/ng_floats_utils.h"
#include "third_party/blink/renderer/platform/text/text_direction.h"
#include "third_party/blink/renderer/platform/text/writing_mode.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class LayoutBox;

enum NGFragmentationType {
  kFragmentNone,
  kFragmentPage,
  kFragmentColumn,
  kFragmentRegion
};

// The NGConstraintSpace represents a set of constraints and available space
// which a layout algorithm may produce a NGFragment within.
class CORE_EXPORT NGConstraintSpace final
    : public RefCounted<NGConstraintSpace> {
 public:
  // Creates NGConstraintSpace representing LayoutObject's containing block.
  // This should live on NGBlockNode or another layout bridge and probably take
  // a root NGConstraintSpace.
  static scoped_refptr<NGConstraintSpace> CreateFromLayoutObject(
      const LayoutBox&);

  const NGExclusionSpace& ExclusionSpace() const { return *exclusion_space_; }

  TextDirection Direction() const {
    return static_cast<TextDirection>(direction_);
  }

  WritingMode GetWritingMode() const {
    return static_cast<WritingMode>(writing_mode_);
  }

  bool IsOrthogonalWritingModeRoot() const {
    return is_orthogonal_writing_mode_root_;
  }

  // The size to use for percentage resolution.
  // See: https://drafts.csswg.org/css-sizing/#percentage-sizing
  NGLogicalSize PercentageResolutionSize() const {
    return percentage_resolution_size_;
  }

  // The size to use for percentage resolution for margin/border/padding.
  // They are always get computed relative to the inline size, in the parent
  // writing mode.
  LayoutUnit PercentageResolutionInlineSizeForParentWritingMode() const;

  // Parent's PercentageResolutionInlineSize().
  // This is not always available.
  LayoutUnit ParentPercentageResolutionInlineSize() const;

  // The available space size.
  // See: https://drafts.csswg.org/css-sizing/#available
  NGLogicalSize AvailableSize() const { return available_size_; }

  NGPhysicalSize InitialContainingBlockSize() const {
    return initial_containing_block_size_;
  }

  LayoutUnit FragmentainerBlockSize() const {
    return fragmentainer_block_size_;
  }

  // Return the block space that was available in the current fragmentainer at
  // the start of the current block formatting context. Note that if the start
  // of the current block formatting context is in a previous fragmentainer, the
  // size of the current fragmentainer is returned instead.
  LayoutUnit FragmentainerSpaceAtBfcStart() const {
    DCHECK(HasBlockFragmentation());
    return fragmentainer_space_at_bfc_start_;
  }

  // Whether the current constraint space is for the newly established
  // Formatting Context.
  bool IsNewFormattingContext() const { return is_new_fc_; }

  // Return true if we are to separate (i.e. honor, rather than collapse)
  // block-start margins at the beginning of fragmentainers. This only makes a
  // difference if we're block-fragmented (pagination, multicol, etc.). Then
  // block-start margins at the beginning of a fragmentainers are to be
  // truncated to 0 if they occur after a soft (unforced) break.
  bool HasSeparateLeadingFragmentainerMargins() const {
    return separate_leading_fragmentainer_margins_;
  }

  // Whether the fragment produced from layout should be anonymous, (e.g. it
  // may be a column in a multi-column layout). In such cases it shouldn't have
  // any borders or padding.
  bool IsAnonymous() const { return is_anonymous_; }

  // Whether to use the ':first-line' style or not.
  // Note, this is not about the first line of the content to layout, but
  // whether the constraint space itself is on the first line, such as when it's
  // an inline block.
  // Also note this is true only when the document has ':first-line' rules.
  bool UseFirstLineStyle() const { return use_first_line_style_; }

  // Some layout modes “stretch” their children to a fixed size (e.g. flex,
  // grid). These flags represented whether a layout needs to produce a
  // fragment that satisfies a fixed constraint in the inline and block
  // direction respectively.
  //
  // If these flags are true, the AvailableSize() is interpreted as the fixed
  // border-box size of this box in the respective dimension.
  bool IsFixedSizeInline() const { return is_fixed_size_inline_; }

  bool IsFixedSizeBlock() const { return is_fixed_size_block_; }

  // Whether a fixed block size should be considered definite.
  bool FixedSizeBlockIsDefinite() const {
    return fixed_size_block_is_definite_;
  }

  // Whether an auto inline-size should be interpreted as shrink-to-fit
  // (ie. fit-content). This is used for inline-block, floats, etc.
  bool IsShrinkToFit() const { return is_shrink_to_fit_; }

  // Whether this constraint space is used for an intermediate layout in a
  // multi-pass layout. In such a case, we should not copy back the resulting
  // layout data to the legacy tree or create a paint fragment from it.
  bool IsIntermediateLayout() const { return is_intermediate_layout_; }

  // If specified a layout should produce a Fragment which fragments at the
  // blockSize if possible.
  NGFragmentationType BlockFragmentationType() const;

  // Return true if this contraint space participates in a fragmentation
  // context.
  bool HasBlockFragmentation() const {
    return BlockFragmentationType() != kFragmentNone;
  }

  NGMarginStrut MarginStrut() const { return margin_strut_; }

  // The BfcOffset is where the MarginStrut is placed within the block
  // formatting context.
  //
  // The current layout or a descendant layout may "resolve" the BFC offset,
  // i.e. decide where the current fragment should be placed within the BFC.
  //
  // This is done by:
  //   bfc_block_offset =
  //     space.BfcOffset().block_offset + space.MarginStrut().Sum();
  //
  // The BFC offset can get "resolved" in many circumstances (including, but
  // not limited to):
  //   - block_start border or padding in the current layout.
  //   - Text content, atomic inlines, (see NGLineBreaker).
  //   - The current layout having a block_size.
  //   - Clearance before a child.
  NGBfcOffset BfcOffset() const { return bfc_offset_; }

  // If present, and the current layout hasn't resolved its BFC offset yet (see
  // BfcOffset), the layout should position all of its unpositioned floats at
  // this offset. This value is the BFC offset that we calculated in the
  // previous pass, a pass which aborted once the BFC offset got resolved,
  // because we had walked past content (i.e. floats) that depended on it being
  // resolved.
  //
  // This value should be propogated to child layouts if the current layout
  // hasn't resolved its BFC offset yet.
  //
  // This value is calculated *after* an initial pass of the tree, and should
  // only be present during subsequent passes.
  base::Optional<NGBfcOffset> FloatsBfcOffset() const {
    return floats_bfc_offset_;
  }

  // Return the types (none, left, right, both) of preceding adjoining
  // floats. These are floats that are added while the in-flow BFC offset is
  // still unknown. The floats may or may not be unpositioned (pending). That
  // depends on which layout pass we're in. They are typically positioned if
  // FloatsBfcOffset() is known. Adjoining floats should be treated differently
  // when calculating clearance on a block with adjoining block-start margin.
  // (in such cases we will know up front that the block will need clearance,
  // since, if it doesn't, the float will be pulled along with the block, and
  // the block will fail to clear).
  NGFloatTypes AdjoiningFloatTypes() const { return adjoining_floats_; }

  bool HasClearanceOffset() const {
    return clearance_offset_ != LayoutUnit::Min();
  }
  LayoutUnit ClearanceOffset() const { return clearance_offset_; }

  // Return true if the fragment needs to have clearance applied to it,
  // regardless of its hypothetical position. The fragment will then go exactly
  // below the relevant floats. This happens when a cleared child gets separated
  // from floats that would otherwise be adjoining; example:
  //
  // <div id="container">
  //   <div id="float" style="float:left; width:100px; height:100px;"></div>
  //   <div id="clearee" style="clear:left; margin-top:12345px;">text</div>
  // </div>
  //
  // Clearance separates #clearee from #container, and #float is positioned at
  // the block-start content edge of #container. Without clearance, margins
  // would have been adjoining and the large margin on #clearee would have
  // pulled both #container and #float along with it. No margin, no matter how
  // large, would ever be able to pull #clearee below the float then. But we
  // have clearance, the margins are separated, and in this case we know that we
  // have clearance even before we have laid out (because of the adjoing
  // float). So it would just be wrong to check for clearance when we position
  // #clearee. Nothing can prevent clearance here. A large margin on the cleared
  // child will be canceled out with negative clearance.
  bool ShouldForceClearance() const { return should_force_clearance_; }

  const Vector<NGBaselineRequest>& BaselineRequests() const {
    return baseline_requests_;
  }

  bool operator==(const NGConstraintSpace&) const;
  bool operator!=(const NGConstraintSpace&) const;

  String ToString() const;

 private:
  friend class NGConstraintSpaceBuilder;
  // Default constructor.
  NGConstraintSpace(WritingMode,
                    bool is_orthogonal_writing_mode_root,
                    TextDirection,
                    NGLogicalSize available_size,
                    NGLogicalSize percentage_resolution_size,
                    LayoutUnit parent_percentage_resolution_inline_size,
                    NGPhysicalSize initial_containing_block_size,
                    LayoutUnit fragmentainer_block_size,
                    LayoutUnit fragmentainer_space_at_bfc_start,
                    bool is_fixed_size_inline,
                    bool is_fixed_size_block,
                    bool fixed_size_block_is_definite,
                    bool is_shrink_to_fit,
                    bool is_intermediate_layout,
                    NGFragmentationType block_direction_fragmentation_type,
                    bool separate_leading_fragmentainer_margins_,
                    bool is_new_fc,
                    bool is_anonymous,
                    bool use_first_line_style,
                    bool should_force_clearance,
                    NGFloatTypes adjoining_floats,
                    const NGMarginStrut& margin_strut,
                    const NGBfcOffset& bfc_offset,
                    const base::Optional<NGBfcOffset>& floats_bfc_offset,
                    const NGExclusionSpace& exclusion_space,
                    LayoutUnit clearance_offset,
                    Vector<NGBaselineRequest>& baseline_requests);

  NGLogicalSize available_size_;
  NGLogicalSize percentage_resolution_size_;
  LayoutUnit parent_percentage_resolution_inline_size_;
  NGPhysicalSize initial_containing_block_size_;

  LayoutUnit fragmentainer_block_size_;
  LayoutUnit fragmentainer_space_at_bfc_start_;

  unsigned is_fixed_size_inline_ : 1;
  unsigned is_fixed_size_block_ : 1;
  unsigned fixed_size_block_is_definite_ : 1;

  unsigned is_shrink_to_fit_ : 1;
  unsigned is_intermediate_layout_ : 1;

  unsigned block_direction_fragmentation_type_ : 2;
  unsigned separate_leading_fragmentainer_margins_ : 1;

  // Whether the current constraint space is for the newly established
  // formatting Context
  unsigned is_new_fc_ : 1;

  unsigned is_anonymous_ : 1;
  unsigned use_first_line_style_ : 1;
  unsigned should_force_clearance_ : 1;
  unsigned adjoining_floats_ : 2;  //  NGFloatTypes

  unsigned writing_mode_ : 3;
  unsigned is_orthogonal_writing_mode_root_ : 1;
  unsigned direction_ : 1;

  NGMarginStrut margin_strut_;
  NGBfcOffset bfc_offset_;
  base::Optional<NGBfcOffset> floats_bfc_offset_;

  const std::unique_ptr<const NGExclusionSpace> exclusion_space_;
  LayoutUnit clearance_offset_;

  Vector<NGBaselineRequest> baseline_requests_;
};

inline std::ostream& operator<<(std::ostream& stream,
                                const NGConstraintSpace& value) {
  return stream << value.ToString();
}

}  // namespace blink

#endif  // NGConstraintSpace_h
