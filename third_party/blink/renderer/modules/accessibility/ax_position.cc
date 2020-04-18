// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/accessibility/ax_position.h"

#include "third_party/blink/renderer/core/dom/ax_object_cache.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/editing/ephemeral_range.h"
#include "third_party/blink/renderer/core/editing/iterators/character_iterator.h"
#include "third_party/blink/renderer/core/editing/iterators/text_iterator.h"
#include "third_party/blink/renderer/core/editing/position.h"
#include "third_party/blink/renderer/core/editing/position_with_affinity.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object.h"
#include "third_party/blink/renderer/modules/accessibility/ax_object_cache_impl.h"

namespace blink {

// static
const AXPosition AXPosition::CreatePositionBeforeObject(const AXObject& child) {
  // If |child| is a text object, make behavior the same as
  // |CreateFirstPositionInObject| so that equality would hold.
  if (child.GetNode() && child.GetNode()->IsTextNode())
    return CreateFirstPositionInObject(child);

  const AXObject* parent = child.ParentObjectUnignored();
  DCHECK(parent);
  AXPosition position(*parent);
  position.text_offset_or_child_index_ = child.IndexInParent();
  DCHECK(position.IsValid());
  return position;
}

// static
const AXPosition AXPosition::CreatePositionAfterObject(const AXObject& child) {
  // If |child| is a text object, make behavior the same as
  // |CreateLastPositionInObject| so that equality would hold.
  if (child.GetNode() && child.GetNode()->IsTextNode())
    return CreateLastPositionInObject(child);

  const AXObject* parent = child.ParentObjectUnignored();
  DCHECK(parent);
  AXPosition position(*parent);
  position.text_offset_or_child_index_ = child.IndexInParent() + 1;
  DCHECK(position.IsValid());
  return position;
}

// static
const AXPosition AXPosition::CreateFirstPositionInObject(
    const AXObject& container) {
  if (container.GetNode() && container.GetNode()->IsTextNode()) {
    AXPosition position(container);
    position.text_offset_or_child_index_ = 0;
    DCHECK(position.IsValid());
    return position;
  }
  AXPosition position(container);
  position.text_offset_or_child_index_ = 0;
  DCHECK(position.IsValid());
  return position;
}

// static
const AXPosition AXPosition::CreateLastPositionInObject(
    const AXObject& container) {
  if (container.GetNode() && container.GetNode()->IsTextNode()) {
    AXPosition position(container);
    const auto first_position =
        Position::FirstPositionInNode(*container.GetNode());
    const auto last_position =
        Position::LastPositionInNode(*container.GetNode());
    position.text_offset_or_child_index_ =
        TextIterator::RangeLength(first_position, last_position);
    DCHECK(position.IsValid());
    return position;
  }
  AXPosition position(container);
  position.text_offset_or_child_index_ = container.ChildCount();
  DCHECK(position.IsValid());
  return position;
}

// static
const AXPosition AXPosition::CreatePositionInTextObject(
    const AXObject& container,
    int offset,
    TextAffinity affinity) {
  DCHECK(container.GetNode() && container.GetNode()->IsTextNode())
      << "Text positions should be anchored to a text node.";
  AXPosition position(container);
  position.text_offset_or_child_index_ = offset;
  position.affinity_ = affinity;
  DCHECK(position.IsValid());
  return position;
}

// static
const AXPosition AXPosition::FromPosition(const Position& position) {
  if (position.IsNull() || position.IsOrphan())
    return {};

  const Document* document = position.GetDocument();
  // Non orphan positions always have a document.
  DCHECK(document);

  AXObjectCache* ax_object_cache = document->ExistingAXObjectCache();
  if (!ax_object_cache)
    return {};

  auto* ax_object_cache_impl = static_cast<AXObjectCacheImpl*>(ax_object_cache);
  const Position& parent_anchored_position = position.ToOffsetInAnchor();
  const Node* anchor_node = parent_anchored_position.AnchorNode();
  DCHECK(anchor_node);
  const AXObject* container = ax_object_cache_impl->GetOrCreate(anchor_node);
  DCHECK(container);
  AXPosition ax_position(*container);
  if (anchor_node->IsTextNode()) {
    // Convert from a DOM offset that may have uncompressed white space to a
    // character offset.
    // TODO(ax-dev): Use LayoutNG offset mapping instead of |TextIterator|.
    const auto first_position = Position::FirstPositionInNode(*anchor_node);
    int offset =
        TextIterator::RangeLength(first_position, parent_anchored_position);
    ax_position.text_offset_or_child_index_ = offset;
    DCHECK(ax_position.IsValid());
    return ax_position;
  }

  const Node* node_after_position = position.ComputeNodeAfterPosition();
  if (!node_after_position) {
    ax_position.text_offset_or_child_index_ = container->ChildCount();
    DCHECK(ax_position.IsValid());
    return ax_position;
  }

  const AXObject* ax_child =
      ax_object_cache_impl->GetOrCreate(node_after_position);
  DCHECK(ax_child);
  if (ax_child->IsDescendantOf(*container)) {
    ax_position.text_offset_or_child_index_ = ax_child->IndexInParent();
    DCHECK(ax_position.IsValid());
    return ax_position;
  }
  return CreatePositionBeforeObject(*ax_child);
}

// Only for use by |AXSelection| to represent empty selection ranges.
AXPosition::AXPosition()
    : container_object_(nullptr),
      text_offset_or_child_index_(),
      affinity_(TextAffinity::kDownstream) {
#if DCHECK_IS_ON()
  dom_tree_version_ = 0;
  style_version_ = 0;
#endif
}

AXPosition::AXPosition(const AXObject& container)
    : container_object_(&container),
      text_offset_or_child_index_(),
      affinity_(TextAffinity::kDownstream) {
  const Document* document = container_object_->GetDocument();
  DCHECK(document);
#if DCHECK_IS_ON()
  dom_tree_version_ = document->DomTreeVersion();
  style_version_ = document->StyleVersion();
#endif
}

const AXObject* AXPosition::ObjectAfterPosition() const {
  if (!IsValid() || IsTextPosition())
    return nullptr;
  if (!container_object_->ChildCount())
    return container_object_->NextInTreeObject();
  if (container_object_->ChildCount() == ChildIndex())
    return container_object_->LastChild()->NextInTreeObject();
  return *(container_object_->Children().begin() + ChildIndex());
}

int AXPosition::ChildIndex() const {
  if (!IsTextPosition())
    return *text_offset_or_child_index_;
  NOTREACHED() << *this << " should not be a text position.";
  return 0;
}

int AXPosition::TextOffset() const {
  if (IsTextPosition())
    return *text_offset_or_child_index_;
  NOTREACHED() << *this << " should be a text position.";
  return 0;
}

bool AXPosition::IsValid() const {
  if (!container_object_ || container_object_->IsDetached())
    return false;
  if (!container_object_->GetNode() ||
      !container_object_->GetNode()->isConnected())
    return false;

  // We can't have both an object and a text anchored position, but we must have
  // at least one of them.
  DCHECK(text_offset_or_child_index_);
  if (text_offset_or_child_index_ &&
      !container_object_->GetNode()->IsTextNode()) {
    if (text_offset_or_child_index_ > container_object_->ChildCount())
      return false;
  }

  DCHECK(container_object_->GetNode()->GetDocument().IsActive());
  DCHECK(!container_object_->GetNode()->GetDocument().NeedsLayoutTreeUpdate());
#if DCHECK_IS_ON()
  DCHECK_EQ(container_object_->GetNode()->GetDocument().DomTreeVersion(),
            dom_tree_version_);
  DCHECK_EQ(container_object_->GetNode()->GetDocument().StyleVersion(),
            style_version_);
#endif  // DCHECK_IS_ON()
  return true;
}

bool AXPosition::IsTextPosition() const {
  return IsValid() && container_object_->GetNode()->IsTextNode();
}

const AXPosition AXPosition::CreateNextPosition() const {
  if (IsTextPosition()) {
    if (!ContainerObject()->NextInTreeObject())
      return {};
    return CreatePositionBeforeObject(*ContainerObject()->NextInTreeObject());
  }

  if (!ObjectAfterPosition())
    return {};
  return CreatePositionBeforeObject(*ObjectAfterPosition());
}

const AXPosition AXPosition::CreatePreviousPosition() const {
  if (!ContainerObject()->PreviousInTreeObject())
    return {};
  return CreatePositionAfterObject(*ContainerObject()->PreviousInTreeObject());
}

const PositionWithAffinity AXPosition::ToPositionWithAffinity(
    const AXPositionAdjustmentBehavior adjustment_behavior) const {
  if (!IsValid())
    return {};

  const Node* container_node = container_object_->GetNode();
  if (!container_node) {
    switch (adjustment_behavior) {
      case AXPositionAdjustmentBehavior::kMoveRight:
        CreateNextPosition().ToPositionWithAffinity(adjustment_behavior);
        break;
      case AXPositionAdjustmentBehavior::kMoveLeft:
        CreatePreviousPosition().ToPositionWithAffinity(adjustment_behavior);
        break;
    }
  }

  if (!IsTextPosition()) {
    if (ChildIndex() == container_object_->ChildCount()) {
      return PositionWithAffinity(Position::LastPositionInNode(*container_node),
                                  affinity_);
    }
    const AXObject* ax_child =
        *(container_object_->Children().begin() + ChildIndex());
    return PositionWithAffinity(
        Position::InParentBeforeNode(*(ax_child->GetNode())), affinity_);
  }

  // TODO(ax-dev): Use LayoutNG offset mapping instead of |TextIterator|.
  const auto first_position = Position::FirstPositionInNode(*container_node);
  const auto last_position = Position::LastPositionInNode(*container_node);
  CharacterIterator character_iterator(first_position, last_position);
  const EphemeralRange range = character_iterator.CalculateCharacterSubrange(
      0, *text_offset_or_child_index_);
  return PositionWithAffinity(range.EndPosition(), affinity_);
}

bool operator==(const AXPosition& a, const AXPosition& b) {
  DCHECK(a.IsValid() && b.IsValid());
  if (*a.ContainerObject() != *b.ContainerObject())
    return false;
  if (a.IsTextPosition() && b.IsTextPosition())
    return a.TextOffset() == b.TextOffset() && a.Affinity() == b.Affinity();
  if (!a.IsTextPosition() && !b.IsTextPosition())
    return a.ChildIndex() == b.ChildIndex();
  NOTREACHED() << "AXPosition objects having the same container object should "
                  "have the same type.";
  return false;
}

bool operator!=(const AXPosition& a, const AXPosition& b) {
  return !(a == b);
}

bool operator<(const AXPosition& a, const AXPosition& b) {
  DCHECK(a.IsValid() && b.IsValid());

  if (a.ContainerObject() == b.ContainerObject()) {
    if (a.IsTextPosition() && b.IsTextPosition())
      return a.TextOffset() < b.TextOffset();
    if (!a.IsTextPosition() && !b.IsTextPosition())
      return a.ChildIndex() < b.ChildIndex();
    NOTREACHED()
        << "AXPosition objects having the same container object should "
           "have the same type.";
    return false;
  }

  int index_in_ancestor1, index_in_ancestor2;
  const AXObject* ancestor =
      AXObject::LowestCommonAncestor(*a.ContainerObject(), *b.ContainerObject(),
                                     &index_in_ancestor1, &index_in_ancestor2);
  DCHECK_GE(index_in_ancestor1, -1);
  DCHECK_GE(index_in_ancestor2, -1);
  if (!ancestor)
    return false;
  if (ancestor == a.ContainerObject()) {
    DCHECK(!a.IsTextPosition());
    index_in_ancestor1 = a.ChildIndex();
  }
  if (ancestor == b.ContainerObject()) {
    DCHECK(!b.IsTextPosition());
    index_in_ancestor2 = b.ChildIndex();
  }
  return index_in_ancestor1 < index_in_ancestor2;
}

bool operator<=(const AXPosition& a, const AXPosition& b) {
  return a < b || a == b;
}

bool operator>(const AXPosition& a, const AXPosition& b) {
  DCHECK(a.IsValid() && b.IsValid());

  if (a.ContainerObject() == b.ContainerObject()) {
    if (a.IsTextPosition() && b.IsTextPosition())
      return a.TextOffset() > b.TextOffset();
    if (!a.IsTextPosition() && !b.IsTextPosition())
      return a.ChildIndex() > b.ChildIndex();
    NOTREACHED()
        << "AXPosition objects having the same container object should "
           "have the same type.";
    return false;
  }

  int index_in_ancestor1, index_in_ancestor2;
  const AXObject* ancestor =
      AXObject::LowestCommonAncestor(*a.ContainerObject(), *b.ContainerObject(),
                                     &index_in_ancestor1, &index_in_ancestor2);
  DCHECK_GE(index_in_ancestor1, -1);
  DCHECK_GE(index_in_ancestor2, -1);
  if (!ancestor)
    return false;
  if (ancestor == a.ContainerObject()) {
    DCHECK(!a.IsTextPosition());
    index_in_ancestor1 = a.ChildIndex();
  }
  if (ancestor == b.ContainerObject()) {
    DCHECK(!b.IsTextPosition());
    index_in_ancestor2 = b.ChildIndex();
  }
  return index_in_ancestor1 > index_in_ancestor2;
}

bool operator>=(const AXPosition& a, const AXPosition& b) {
  return a > b || a == b;
}

std::ostream& operator<<(std::ostream& ostream, const AXPosition& position) {
  if (!position.IsValid())
    return ostream << "Invalid AXPosition";
  if (position.IsTextPosition()) {
    return ostream << "AX text position in " << position.ContainerObject()
                   << ", " << position.TextOffset();
  }
  return ostream << "AX object anchored position in "
                 << position.ContainerObject() << ", " << position.ChildIndex();
}

}  // namespace blink
