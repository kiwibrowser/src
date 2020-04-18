// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGLineBoxFragmentBuilder_h
#define NGLineBoxFragmentBuilder_h

#include "third_party/blink/renderer/core/layout/ng/geometry/ng_logical_offset.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_node.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_line_height_metrics.h"
#include "third_party/blink/renderer/core/layout/ng/ng_container_fragment_builder.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_result.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class ComputedStyle;
class NGInlineBreakToken;
class NGPhysicalFragment;
struct NGPositionedFloat;

class CORE_EXPORT NGLineBoxFragmentBuilder final
    : public NGContainerFragmentBuilder {
  STACK_ALLOCATED();
 public:
  NGLineBoxFragmentBuilder(NGInlineNode,
                           scoped_refptr<const ComputedStyle>,
                           WritingMode,
                           TextDirection);
  ~NGLineBoxFragmentBuilder() override;

  void Reset();

  LayoutUnit LineHeight() const;
  LayoutUnit ComputeBlockSize() const;

  const NGLineHeightMetrics& Metrics() const { return metrics_; }
  void SetMetrics(const NGLineHeightMetrics&);

  void SetBaseDirection(TextDirection);

  void SwapPositionedFloats(Vector<NGPositionedFloat>*);

  // Set the break token for the fragment to build.
  // A finished break token will be attached if not set.
  void SetBreakToken(scoped_refptr<NGInlineBreakToken>);

  // A data struct to keep NGLayoutResult or fragment until the box tree
  // structures and child offsets are finalized.
  struct Child {
    DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

    scoped_refptr<NGLayoutResult> layout_result;
    scoped_refptr<NGPhysicalFragment> fragment;
    LayoutObject* out_of_flow_positioned_box = nullptr;
    LayoutObject* out_of_flow_containing_box = nullptr;
    NGLogicalOffset offset;
    LayoutUnit inline_size;
    unsigned box_data_index = 0;
    UBiDiLevel bidi_level = 0xff;

    // Empty constructor needed for |resize()|.
    Child() = default;
    // Create a placeholder. A placeholder does not have a fragment nor a bidi
    // level.
    Child(NGLogicalOffset offset) : offset(offset) {}
    // Crete a bidi control. A bidi control does not have a fragment, but has
    // bidi level and affects bidi reordering.
    Child(UBiDiLevel bidi_level) : bidi_level(bidi_level) {}
    // Create an in-flow |NGLayoutResult|.
    Child(scoped_refptr<NGLayoutResult> layout_result,
          NGLogicalOffset offset,
          LayoutUnit inline_size,
          UBiDiLevel bidi_level)
        : layout_result(std::move(layout_result)),
          offset(offset),
          inline_size(inline_size),
          bidi_level(bidi_level) {}
    // Create an in-flow |NGPhysicalFragment|.
    Child(scoped_refptr<NGPhysicalFragment> fragment,
          NGLogicalOffset offset,
          LayoutUnit inline_size,
          UBiDiLevel bidi_level)
        : fragment(std::move(fragment)),
          offset(offset),
          inline_size(inline_size),
          bidi_level(bidi_level) {}
    Child(scoped_refptr<NGPhysicalFragment> fragment,
          LayoutUnit block_offset,
          LayoutUnit inline_size,
          UBiDiLevel bidi_level)
        : fragment(std::move(fragment)),
          offset({LayoutUnit(), block_offset}),
          inline_size(inline_size),
          bidi_level(bidi_level) {}
    // Create an out-of-flow positioned object.
    Child(LayoutObject* out_of_flow_positioned_box,
          LayoutObject* out_of_flow_containing_box,
          UBiDiLevel bidi_level)
        : out_of_flow_positioned_box(out_of_flow_positioned_box),
          out_of_flow_containing_box(out_of_flow_containing_box),
          bidi_level(bidi_level) {}

    bool HasInFlowFragment() const { return layout_result || fragment; }
    bool HasOutOfFlowFragment() const { return out_of_flow_positioned_box; }
    bool HasFragment() const {
      return HasInFlowFragment() || HasOutOfFlowFragment();
    }
    bool HasBidiLevel() const { return bidi_level != 0xff; }
    const NGPhysicalFragment* PhysicalFragment() const;
  };

  // A vector of Child.
  // Unlike the fragment builder, chlidren are mutable.
  // Callers can add to the fragment builder in a batch once finalized.
  class ChildList {
    STACK_ALLOCATED();

   public:
    ChildList() = default;
    void operator=(ChildList&& other) {
      children_ = std::move(other.children_);
    }

    Child& operator[](unsigned i) { return children_[i]; }

    unsigned size() const { return children_.size(); }
    bool IsEmpty() const { return children_.IsEmpty(); }
    void ReserveInitialCapacity(unsigned capacity) {
      children_.ReserveInitialCapacity(capacity);
    }
    void clear() { children_.clear(); }
    void resize(size_t size) { children_.resize(size); }

    using iterator = Vector<Child, 16>::iterator;
    iterator begin() { return children_.begin(); }
    iterator end() { return children_.end(); }
    using const_iterator = Vector<Child, 16>::const_iterator;
    const_iterator begin() const { return children_.begin(); }
    const_iterator end() const { return children_.end(); }
    using reverse_iterator = Vector<Child, 16>::reverse_iterator;
    reverse_iterator rbegin() { return children_.rbegin(); }
    reverse_iterator rend() { return children_.rend(); }
    using const_reverse_iterator = Vector<Child, 16>::const_reverse_iterator;
    const_reverse_iterator rbegin() const { return children_.rbegin(); }
    const_reverse_iterator rend() const { return children_.rend(); }

    Child* FirstInFlowChild();
    Child* LastInFlowChild();

    // Add a child. Accepts all constructor arguments for |Child|.
    template <class... Args>
    void AddChild(Args&&... args) {
      children_.push_back(Child(std::forward<Args>(args)...));
    }
    void InsertChild(unsigned,
                     scoped_refptr<NGLayoutResult>,
                     const NGLogicalOffset&,
                     LayoutUnit inline_size,
                     UBiDiLevel);

    void MoveInInlineDirection(LayoutUnit, unsigned start, unsigned end);
    void MoveInBlockDirection(LayoutUnit);
    void MoveInBlockDirection(LayoutUnit, unsigned start, unsigned end);

   private:
    Vector<Child, 16> children_;
  };

  // Add all items in ChildList. Skips null Child if any.
  void AddChildren(ChildList&);

  // Creates the fragment. Can only be called once.
  scoped_refptr<NGLayoutResult> ToLineBoxFragment();

 private:
  NGInlineNode node_;

  NGLineHeightMetrics metrics_;
  Vector<NGPositionedFloat> positioned_floats_;

  scoped_refptr<NGInlineBreakToken> break_token_;

  TextDirection base_direction_;
};

}  // namespace blink

#endif  // NGLineBoxFragmentBuilder
