// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutNGMixin_h
#define LayoutNGMixin_h

#include <type_traits>

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/layout_box_model_object.h"

namespace blink {

enum class NGBaselineAlgorithmType;
class NGBreakToken;
class NGConstraintSpace;
class NGLayoutResult;
class NGPaintFragment;
class NGPhysicalFragment;
struct NGBaseline;
struct NGInlineNodeData;

// This mixin holds code shared between LayoutNG subclasses of
// LayoutBlockFlow.

template <typename Base>
class CORE_TEMPLATE_CLASS_EXPORT LayoutNGMixin : public Base {
 public:
  explicit LayoutNGMixin(Element* element);
  ~LayoutNGMixin() override;

  NGInlineNodeData* TakeNGInlineNodeData() override;
  NGInlineNodeData* GetNGInlineNodeData() const override;
  void ResetNGInlineNodeData() override;
  bool HasNGInlineNodeData() const override {
    return ng_inline_node_data_.get();
  }

  LayoutUnit FirstLineBoxBaseline() const override;
  LayoutUnit InlineBlockBaseline(LineDirectionMode) const override;

  void InvalidateDisplayItemClients(PaintInvalidationReason) const override;

  void Paint(const PaintInfo&, const LayoutPoint&) const override;

  bool NodeAtPoint(HitTestResult&,
                   const HitTestLocation& location_in_container,
                   const LayoutPoint& accumulated_offset,
                   HitTestAction) override;

  PositionWithAffinity PositionForPoint(const LayoutPoint&) const override;

  // Returns the last layout result for this block flow with the given
  // constraint space and break token, or null if it is not up-to-date or
  // otherwise unavailable.
  scoped_refptr<NGLayoutResult> CachedLayoutResult(
      const NGConstraintSpace&,
      NGBreakToken*) const override;
  const NGConstraintSpace* CachedConstraintSpace() const override;

  void SetCachedLayoutResult(const NGConstraintSpace&,
                             NGBreakToken*,
                             scoped_refptr<NGLayoutResult>) override;
  // For testing only.
  scoped_refptr<NGLayoutResult> CachedLayoutResultForTesting() override;

  NGPaintFragment* PaintFragment() const override {
    return paint_fragment_.get();
  }
  void SetPaintFragment(scoped_refptr<const NGPhysicalFragment>) override;
  void ClearPaintFragment() override;

 protected:
  bool IsOfType(LayoutObject::LayoutObjectType) const override;

  void AddOverflowFromChildren() override;

  void AddOutlineRects(
      Vector<LayoutRect>&,
      const LayoutPoint& additional_offset,
      LayoutObject::IncludeBlockVisualOverflowOrNot) const override;

  const NGPhysicalBoxFragment* CurrentFragment() const override;

  const NGBaseline* FragmentBaseline(NGBaselineAlgorithmType) const;

  std::unique_ptr<NGInlineNodeData> ng_inline_node_data_;

  scoped_refptr<NGLayoutResult> cached_result_;
  scoped_refptr<const NGConstraintSpace> cached_constraint_space_;
  std::unique_ptr<NGPaintFragment> paint_fragment_;

  friend class NGBaseLayoutAlgorithmTest;
};

}  // namespace blink

#endif  // LayoutNGMixin_h
