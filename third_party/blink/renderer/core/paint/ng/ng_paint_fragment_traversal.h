// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGPaintFragmentTraversal_h
#define NGPaintFragmentTraversal_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_physical_offset.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class LayoutObject;
class NGPaintFragment;

// Used for return value of traversing fragment tree.
struct CORE_EXPORT NGPaintFragmentWithContainerOffset {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();
  NGPaintFragment* fragment;
  // Offset relative to container fragment
  NGPhysicalOffset container_offset;
};

// Represents an NGPaintFragment by its parent and its index in the parent's
// |Children()| vector.
struct NGPaintFragmentTraversalContext {
  STACK_ALLOCATED();

  static NGPaintFragmentTraversalContext Create(const NGPaintFragment*);

  bool IsNull() const { return !parent; }
  const NGPaintFragment* GetFragment() const;

  bool operator==(const NGPaintFragmentTraversalContext& other) const {
    return parent == other.parent && index == other.index;
  }

  const NGPaintFragment* parent = nullptr;
  unsigned index = 0;
};

// Utility class for traversing the paint fragment tree.
class CORE_EXPORT NGPaintFragmentTraversal {
  STATIC_ONLY(NGPaintFragmentTraversal);

 public:
  // Returns descendants without paint layer in preorder.
  static Vector<NGPaintFragmentWithContainerOffset> DescendantsOf(
      const NGPaintFragment&);

  // Returns inline descendants in preorder.
  static Vector<NGPaintFragmentWithContainerOffset> InlineDescendantsOf(
      const NGPaintFragment&);

  static Vector<NGPaintFragmentWithContainerOffset> SelfFragmentsOf(
      const NGPaintFragment&,
      const LayoutObject* target);

  // Returns the line box paint fragment of |line|. |line| itself must be the
  // paint fragment of a line box.
  static NGPaintFragment* PreviousLineOf(const NGPaintFragment& line);

  // Returns the previous/next inline leaf fragment (text or atomic inline)of
  // the passed fragment, which itself must be inline.
  static NGPaintFragmentTraversalContext PreviousInlineLeafOf(
      const NGPaintFragmentTraversalContext&);
  static NGPaintFragmentTraversalContext NextInlineLeafOf(
      const NGPaintFragmentTraversalContext&);

  // Variants of the above two skipping line break fragments.
  static NGPaintFragmentTraversalContext PreviousInlineLeafOfIgnoringLineBreak(
      const NGPaintFragmentTraversalContext&);
  static NGPaintFragmentTraversalContext NextInlineLeafOfIgnoringLineBreak(
      const NGPaintFragmentTraversalContext&);
};

}  // namespace blink

#endif  // NGPaintFragmentTraversal_h
