// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_fragment_traversal.h"

#include "third_party/blink/renderer/core/layout/layout_inline.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

namespace {

using Result = NGPhysicalFragmentWithOffset;

// Traverse the subtree of |container|, and collect the fragments satisfying
// |filter| into the |results| vector. Guarantees to call |filter.AddOnEnter()|
// for all fragments in preorder, and call |filter.RemoveOnExit()| on all
// fragments in postorder. A fragment is collected if |AddOnEnter()| returns
// true and |RemoveOnExit()| returns false on it.
template <typename Filter, size_t inline_capacity>
void CollectInlineFragments(const NGPhysicalContainerFragment& container,
                            NGPhysicalOffset offset_to_container_box,
                            Filter& filter,
                            Vector<Result, inline_capacity>* results) {
  DCHECK(container.IsInline() || container.IsLineBox() ||
         (container.IsBlockFlow() &&
          ToNGPhysicalBoxFragment(container).ChildrenInline()));
  for (const auto& child : container.Children()) {
    NGPhysicalOffset child_offset = child->Offset() + offset_to_container_box;

    if (filter.AddOnEnter(child.get())) {
      results->push_back(
          NGPhysicalFragmentWithOffset{child.get(), child_offset});
    }

    // Traverse descendants unless the fragment is laid out separately from the
    // inline layout algorithm.
    if (child->IsContainer() && !child->IsBlockLayoutRoot()) {
      CollectInlineFragments(ToNGPhysicalContainerFragment(*child),
                             child_offset, filter, results);
    }

    if (filter.RemoveOnExit(child.get())) {
      DCHECK(results->size());
      DCHECK_EQ(results->back().fragment, child.get());
      results->pop_back();
    }
  }
}

// The filter for CollectInlineFragments() collecting all fragments traversed.
class AddAllFilter {
 public:
  bool AddOnEnter(const NGPhysicalFragment*) const { return true; }
  bool RemoveOnExit(const NGPhysicalFragment*) const { return false; }
};

// The filter for CollectInlineFragments() collecting fragments generated from
// the given LayoutInline with supporting culled inline.
// Note: Since we apply culled inline per line, we have a fragment for
// LayoutInline in second line but not in first line in
// "t0803-c5502-imrgn-r-01-b-ag.html".
class LayoutInlineFilter {
 public:
  explicit LayoutInlineFilter(const LayoutInline& container) {
    CollectInclusiveDescendnats(container);
  }

  bool AddOnEnter(const NGPhysicalFragment* fragment) {
    if (fragment->IsLineBox())
      return false;
    return inclusive_descendants_.Contains(fragment->GetLayoutObject());
  }

  bool RemoveOnExit(const NGPhysicalFragment*) const { return false; }

 private:
  void CollectInclusiveDescendnats(const LayoutInline& container) {
    inclusive_descendants_.insert(&container);
    for (const LayoutObject* node = container.FirstChild(); node;
         node = node->NextSibling()) {
      if (node->IsFloatingOrOutOfFlowPositioned())
        continue;
      if (node->IsBox() || node->IsText()) {
        inclusive_descendants_.insert(node);
        continue;
      }
      if (!node->IsLayoutInline())
        continue;
      CollectInclusiveDescendnats(ToLayoutInline(*node));
    }
  }

  HashSet<const LayoutObject*> inclusive_descendants_;
};

// The filter for CollectInlineFragments() collecting fragments generated from
// the given LayoutObject.
class LayoutObjectFilter {
 public:
  explicit LayoutObjectFilter(const LayoutObject* layout_object)
      : layout_object_(layout_object) {
    DCHECK(layout_object);
  }

  bool AddOnEnter(const NGPhysicalFragment* fragment) const {
    return fragment->GetLayoutObject() == layout_object_;
  }
  bool RemoveOnExit(const NGPhysicalFragment*) const { return false; }

 private:
  const LayoutObject* layout_object_;
};

// The filter for CollectInlineFragments() collecting inclusive ancestors of the
// given fragment with the algorithm that, |fragment| is an ancestor of |target|
// if and only if both of the following are true:
// - |fragment| precedes |target| in preorder traversal
// - |fragment| succeeds |target| in postorder traversal
class InclusiveAncestorFilter {
 public:
  explicit InclusiveAncestorFilter(const NGPhysicalFragment& target)
      : target_(&target) {}

  bool AddOnEnter(const NGPhysicalFragment* fragment) {
    if (fragment == target_)
      has_entered_target_ = true;
    ancestors_precede_in_preorder_.push_back(!has_entered_target_);
    return true;
  }

  bool RemoveOnExit(const NGPhysicalFragment* fragment) {
    if (fragment != target_) {
      const bool precedes_in_preorder = ancestors_precede_in_preorder_.back();
      ancestors_precede_in_preorder_.pop_back();
      return !precedes_in_preorder || !has_exited_target_;
    }
    has_exited_target_ = true;
    ancestors_precede_in_preorder_.pop_back();
    return false;
  }

 private:
  const NGPhysicalFragment* target_;

  bool has_entered_target_ = false;
  bool has_exited_target_ = false;

  // For each currently entered but not-yet-exited fragment, stores a boolean of
  // whether it precedes |target_| in preorder.
  Vector<bool> ancestors_precede_in_preorder_;
};

}  // namespace

// static
Vector<Result, 1> NGInlineFragmentTraversal::SelfFragmentsOf(
    const NGPhysicalContainerFragment& container,
    const LayoutObject* layout_object) {
  if (layout_object->IsLayoutInline()) {
    LayoutInlineFilter filter(*ToLayoutInline(layout_object));
    Vector<Result, 1> results;
    CollectInlineFragments(container, {}, filter, &results);
    return results;
  }
  LayoutObjectFilter filter(layout_object);
  Vector<Result, 1> results;
  CollectInlineFragments(container, {}, filter, &results);
  return results;
}

// static
Vector<Result> NGInlineFragmentTraversal::DescendantsOf(
    const NGPhysicalContainerFragment& container) {
  AddAllFilter add_all;
  Vector<Result> results;
  CollectInlineFragments(container, {}, add_all, &results);
  return results;
}

// static
Vector<Result> NGInlineFragmentTraversal::InclusiveDescendantsOf(
    const NGPhysicalFragment& root) {
  Vector<Result> results =
      root.IsContainer() ? DescendantsOf(ToNGPhysicalContainerFragment(root))
                         : Vector<Result>();
  results.push_front(Result{&root, {}});
  return results;
}

// static
Vector<Result> NGInlineFragmentTraversal::InclusiveAncestorsOf(
    const NGPhysicalContainerFragment& container,
    const NGPhysicalFragment& target) {
  InclusiveAncestorFilter inclusive_ancestors_of(target);
  Vector<Result> results;
  CollectInlineFragments(container, {}, inclusive_ancestors_of, &results);
  std::reverse(results.begin(), results.end());
  return results;
}

// static
Vector<Result> NGInlineFragmentTraversal::AncestorsOf(
    const NGPhysicalContainerFragment& container,
    const NGPhysicalFragment& target) {
  Vector<Result> results = InclusiveAncestorsOf(container, target);
  DCHECK(results.size());
  DCHECK_EQ(results.front().fragment, &target);
  results.erase(results.begin());
  return results;
}

}  // namespace blink
