// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment_traversal.h"

#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_text_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_box_fragment.h"
#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment.h"

namespace blink {

namespace {

// Preorder traverse |container|, and collect the fragments satisfying
// |filter| into |results|.
// |filter|.IsTraverse(NGPaintFragment) returns true to traverse children.
// |filter|.IsCollectible(NGPaintFragment) returns true to collect fragment.

template <typename Filter>
void CollectPaintFragments(const NGPaintFragment& container,
                           NGPhysicalOffset offset_to_container_box,
                           Filter& filter,
                           Vector<NGPaintFragmentWithContainerOffset>* result) {
  for (const auto& child : container.Children()) {
    NGPaintFragmentWithContainerOffset fragment_with_offset{
        child.get(), child->Offset() + offset_to_container_box};
    if (filter.IsCollectible(child.get())) {
      result->push_back(fragment_with_offset);
    }
    if (filter.IsTraverse(child.get())) {
      CollectPaintFragments(*child.get(), fragment_with_offset.container_offset,
                            filter, result);
    }
  }
}

// Does not collect fragments with SelfPaintingLayer or their descendants.
class NotSelfPaintingFilter {
 public:
  bool IsCollectible(const NGPaintFragment* fragment) const {
    return !fragment->HasSelfPaintingLayer();
  }
  bool IsTraverse(const NGPaintFragment* fragment) const {
    return !fragment->HasSelfPaintingLayer();
  }
};

// Collects line box and inline fragments.
class InlineFilter {
 public:
  bool IsCollectible(const NGPaintFragment* fragment) const {
    return fragment->PhysicalFragment().IsInline() ||
           fragment->PhysicalFragment().IsLineBox();
  }
  bool IsTraverse(const NGPaintFragment* fragment) {
    return fragment->PhysicalFragment().IsContainer() &&
           !fragment->PhysicalFragment().IsBlockLayoutRoot();
  }
};

// Collect only fragments that belong to this LayoutObject.
class LayoutObjectFilter {
 public:
  explicit LayoutObjectFilter(const LayoutObject* layout_object)
      : layout_object_(layout_object) {
    DCHECK(layout_object);
  };
  bool IsCollectible(const NGPaintFragment* fragment) const {
    return fragment->GetLayoutObject() == layout_object_;
  }
  bool IsTraverse(const NGPaintFragment*) const { return true; }

 private:
  const LayoutObject* layout_object_;
};

// ------ Helpers for traversing inline fragments ------

bool IsLineBreak(const NGPaintFragmentTraversalContext& fragment) {
  DCHECK(!fragment.IsNull());
  const NGPhysicalFragment& physical_fragment =
      fragment.GetFragment()->PhysicalFragment();
  DCHECK(physical_fragment.IsInline());
  if (!physical_fragment.IsText())
    return false;
  return ToNGPhysicalTextFragment(physical_fragment).IsLineBreak();
}

bool IsInlineLeaf(const NGPaintFragmentTraversalContext& fragment) {
  DCHECK(!fragment.IsNull());
  const NGPhysicalFragment& physical_fragment =
      fragment.GetFragment()->PhysicalFragment();
  if (!physical_fragment.IsInline())
    return false;
  return physical_fragment.IsText() || physical_fragment.IsAtomicInline();
}

NGPaintFragmentTraversalContext FirstInclusiveLeafDescendantOf(
    const NGPaintFragmentTraversalContext& fragment) {
  DCHECK(!fragment.IsNull());
  if (IsInlineLeaf(fragment))
    return fragment;
  const auto& children = fragment.GetFragment()->Children();
  for (unsigned i = 0; i < children.size(); ++i) {
    NGPaintFragmentTraversalContext maybe_leaf =
        FirstInclusiveLeafDescendantOf({fragment.GetFragment(), i});
    if (!maybe_leaf.IsNull())
      return maybe_leaf;
  }
  return NGPaintFragmentTraversalContext();
}

NGPaintFragmentTraversalContext LastInclusiveLeafDescendantOf(
    const NGPaintFragmentTraversalContext& fragment) {
  DCHECK(!fragment.IsNull());
  if (IsInlineLeaf(fragment))
    return fragment;
  const auto& children = fragment.GetFragment()->Children();
  for (unsigned i = children.size(); i != 0u; --i) {
    NGPaintFragmentTraversalContext maybe_leaf =
        LastInclusiveLeafDescendantOf({fragment.GetFragment(), i - 1});
    if (!maybe_leaf.IsNull())
      return maybe_leaf;
  }
  return NGPaintFragmentTraversalContext();
}

NGPaintFragmentTraversalContext PreviousSiblingOf(
    const NGPaintFragmentTraversalContext& fragment) {
  if (!fragment.parent || fragment.index == 0u)
    return NGPaintFragmentTraversalContext();
  return {fragment.parent, fragment.index - 1};
}

NGPaintFragmentTraversalContext NextSiblingOf(
    const NGPaintFragmentTraversalContext& fragment) {
  if (!fragment.parent)
    return NGPaintFragmentTraversalContext();
  if (fragment.index + 1 == fragment.parent->Children().size())
    return NGPaintFragmentTraversalContext();
  return {fragment.parent, fragment.index + 1};
}

}  // namespace

Vector<NGPaintFragmentWithContainerOffset>
NGPaintFragmentTraversal::DescendantsOf(const NGPaintFragment& container) {
  Vector<NGPaintFragmentWithContainerOffset> result;
  NotSelfPaintingFilter filter;
  CollectPaintFragments(container, NGPhysicalOffset(), filter, &result);
  return result;
}

Vector<NGPaintFragmentWithContainerOffset>
NGPaintFragmentTraversal::InlineDescendantsOf(
    const NGPaintFragment& container) {
  Vector<NGPaintFragmentWithContainerOffset> result;
  InlineFilter filter;
  CollectPaintFragments(container, NGPhysicalOffset(), filter, &result);
  return result;
}

Vector<NGPaintFragmentWithContainerOffset>
NGPaintFragmentTraversal::SelfFragmentsOf(const NGPaintFragment& container,
                                          const LayoutObject* target) {
  Vector<NGPaintFragmentWithContainerOffset> result;
  LayoutObjectFilter filter(target);
  CollectPaintFragments(container, NGPhysicalOffset(), filter, &result);
  return result;
}

NGPaintFragment* NGPaintFragmentTraversal::PreviousLineOf(
    const NGPaintFragment& line) {
  DCHECK(line.PhysicalFragment().IsLineBox());
  NGPaintFragment* parent = line.Parent();
  DCHECK(parent);
  NGPaintFragment* previous_line = nullptr;
  for (const auto& sibling : parent->Children()) {
    if (sibling.get() == &line)
      return previous_line;
    if (sibling->PhysicalFragment().IsLineBox())
      previous_line = sibling.get();
  }
  NOTREACHED();
  return nullptr;
}

const NGPaintFragment* NGPaintFragmentTraversalContext::GetFragment() const {
  if (!parent)
    return nullptr;
  return parent->Children()[index].get();
}

// static
NGPaintFragmentTraversalContext NGPaintFragmentTraversalContext::Create(
    const NGPaintFragment* fragment) {
  if (!fragment)
    return NGPaintFragmentTraversalContext();

  DCHECK(fragment->Parent());
  const auto& siblings = fragment->Parent()->Children();
  const auto* self_iter = std::find_if(
      siblings.begin(), siblings.end(),
      [&fragment](const auto& sibling) { return fragment == sibling.get(); });
  DCHECK_NE(self_iter, siblings.end());
  return {fragment->Parent(), self_iter - siblings.begin()};
}

NGPaintFragmentTraversalContext NGPaintFragmentTraversal::PreviousInlineLeafOf(
    const NGPaintFragmentTraversalContext& fragment) {
  DCHECK(!fragment.IsNull());
  DCHECK(fragment.GetFragment()->PhysicalFragment().IsInline());
  for (auto sibling = PreviousSiblingOf(fragment); !sibling.IsNull();
       sibling = PreviousSiblingOf(sibling)) {
    NGPaintFragmentTraversalContext maybe_leaf =
        LastInclusiveLeafDescendantOf(sibling);
    if (!maybe_leaf.IsNull())
      return maybe_leaf;
  }
  DCHECK(fragment.parent);
  if (fragment.parent->PhysicalFragment().IsLineBox())
    return NGPaintFragmentTraversalContext();
  return PreviousInlineLeafOf(
      NGPaintFragmentTraversalContext::Create(fragment.parent));
}

NGPaintFragmentTraversalContext NGPaintFragmentTraversal::NextInlineLeafOf(
    const NGPaintFragmentTraversalContext& fragment) {
  DCHECK(!fragment.IsNull());
  DCHECK(fragment.GetFragment()->PhysicalFragment().IsInline());
  for (auto sibling = NextSiblingOf(fragment); !sibling.IsNull();
       sibling = NextSiblingOf(sibling)) {
    NGPaintFragmentTraversalContext maybe_leaf =
        FirstInclusiveLeafDescendantOf(sibling);
    if (!maybe_leaf.IsNull())
      return maybe_leaf;
  }
  DCHECK(fragment.parent);
  if (fragment.parent->PhysicalFragment().IsLineBox())
    return NGPaintFragmentTraversalContext();
  return NextInlineLeafOf(
      NGPaintFragmentTraversalContext::Create(fragment.parent));
}

NGPaintFragmentTraversalContext
NGPaintFragmentTraversal::PreviousInlineLeafOfIgnoringLineBreak(
    const NGPaintFragmentTraversalContext& fragment) {
  NGPaintFragmentTraversalContext runner = PreviousInlineLeafOf(fragment);
  while (!runner.IsNull() && IsLineBreak(runner))
    runner = PreviousInlineLeafOf(runner);
  return runner;
}

NGPaintFragmentTraversalContext
NGPaintFragmentTraversal::NextInlineLeafOfIgnoringLineBreak(
    const NGPaintFragmentTraversalContext& fragment) {
  NGPaintFragmentTraversalContext runner = NextInlineLeafOf(fragment);
  while (!runner.IsNull() && IsLineBreak(runner))
    runner = NextInlineLeafOf(runner);
  return runner;
}

}  // namespace blink
