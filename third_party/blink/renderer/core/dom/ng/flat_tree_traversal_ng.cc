/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/dom/ng/flat_tree_traversal_ng.h"

#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/html/html_shadow_element.h"
#include "third_party/blink/renderer/core/html/html_slot_element.h"

namespace blink {

bool CanBeDistributedToV0InsertionPoint(const Node& node) {
  return node.IsInV0ShadowTree() || node.IsChildOfV0ShadowHost();
}

Node* FlatTreeTraversalNg::TraverseChild(const Node& node,
                                         TraversalDirection direction) {
  if (auto* slot = ToHTMLSlotElementIfSupportsAssignmentOrNull(node)) {
    if (slot->AssignedNodes().IsEmpty()) {
      return direction == kTraversalDirectionForward ? slot->firstChild()
                                                     : slot->lastChild();
    }
    return direction == kTraversalDirectionForward ? slot->FirstAssignedNode()
                                                   : slot->LastAssignedNode();
  }

  Node* child;
  if (ShadowRoot* shadow_root = node.GetShadowRoot()) {
    child = direction == kTraversalDirectionForward ? shadow_root->firstChild()
                                                    : shadow_root->lastChild();
  } else {
    child = direction == kTraversalDirectionForward ? node.firstChild()
                                                    : node.lastChild();
  }

  if (!child)
    return nullptr;

  if (child->IsInV0ShadowTree()) {
    return V0ResolveDistributionStartingAt(*child, direction);
  }
  return child;
}

// This needs only for v0
// Node* FlatTreeTraversalNg::ResolveDistributionStartingAt(
//     const Node* node,
//     TraversalDirection direction) {
//   if (!node)
//     return nullptr;
//   for (const Node* sibling = node; sibling;
//        sibling = (direction == kTraversalDirectionForward
//                       ? sibling->nextSibling()
//                       : sibling->previousSibling())) {
//     if (node->IsInV0ShadowTree())
//       return V0ResolveDistributionStartingAt(*sibling, direction);
//     return const_cast<Node*>(sibling);
//   }
//   return nullptr;
// }

Node* FlatTreeTraversalNg::V0ResolveDistributionStartingAt(
    const Node& node,
    TraversalDirection direction) {
  DCHECK(!ToHTMLSlotElementIfSupportsAssignmentOrNull(node));
  for (const Node* sibling = &node; sibling;
       sibling = (direction == kTraversalDirectionForward
                      ? sibling->nextSibling()
                      : sibling->previousSibling())) {
    if (!IsActiveV0InsertionPoint(*sibling))
      return const_cast<Node*>(sibling);
    const V0InsertionPoint& insertion_point = ToV0InsertionPoint(*sibling);
    if (Node* found = (direction == kTraversalDirectionForward
                           ? insertion_point.FirstDistributedNode()
                           : insertion_point.LastDistributedNode()))
      return found;
    DCHECK(IsHTMLShadowElement(insertion_point) ||
           (IsHTMLContentElement(insertion_point) &&
            !insertion_point.HasChildren()));
  }
  return nullptr;
}

// TODO(hayato): This may return a wrong result for a node which is not in a
// document flat tree.  See FlatTreeTraversalNgTest's redistribution test for
// details.
Node* FlatTreeTraversalNg::TraverseSiblings(const Node& node,
                                            TraversalDirection direction) {
  if (node.IsChildOfV1ShadowHost())
    return TraverseSiblingsForV1HostChild(node, direction);

  if (ShadowRootWhereNodeCanBeDistributedForV0(node))
    return TraverseSiblingsForV0Distribution(node, direction);

  Node* sibling = direction == kTraversalDirectionForward
                      ? node.nextSibling()
                      : node.previousSibling();

  if (!node.IsInV0ShadowTree())
    return sibling;

  if (sibling) {
    if (Node* found = V0ResolveDistributionStartingAt(*sibling, direction))
      return found;
  }

  // // Slotted nodes are already handled in traverseSiblingsForV1HostChild()
  // // above, here is for fallback contents.
  // if (auto* slot = ToHTMLSlotElementOrNull(node.parentElement())) {
  //   if (slot->SupportsAssignment() && slot->AssignedNodes().IsEmpty())
  //     return TraverseSiblings(*slot, direction);
  // }
  return nullptr;
}

Node* FlatTreeTraversalNg::TraverseSiblingsForV1HostChild(
    const Node& node,
    TraversalDirection direction) {
  HTMLSlotElement* slot = node.AssignedSlot();
  if (!slot)
    return nullptr;
  return direction == kTraversalDirectionForward
             ? slot->AssignedNodeNextTo(node)
             : slot->AssignedNodePreviousTo(node);
}

Node* FlatTreeTraversalNg::TraverseSiblingsForV0Distribution(
    const Node& node,
    TraversalDirection direction) {
  const V0InsertionPoint* final_destination = ResolveReprojection(&node);
  if (!final_destination)
    return nullptr;
  if (Node* found = (direction == kTraversalDirectionForward
                         ? final_destination->DistributedNodeNextTo(&node)
                         : final_destination->DistributedNodePreviousTo(&node)))
    return found;
  return TraverseSiblings(*final_destination, direction);
}

ContainerNode* FlatTreeTraversalNg::TraverseParent(
    const Node& node,
    ParentTraversalDetails* details) {
  // TODO(hayato): Stop this hack for a pseudo element because a pseudo element
  // is not a child of its parentOrShadowHostNode() in a flat tree.
  if (node.IsPseudoElement())
    return node.ParentOrShadowHostNode();

  if (node.IsChildOfV1ShadowHost())
    return node.AssignedSlot();

  if (auto* parent_slot =
          ToHTMLSlotElementIfSupportsAssignmentOrNull(node.parentElement())) {
    if (!parent_slot->AssignedNodes().IsEmpty())
      return nullptr;
    return parent_slot;
  }

  if (CanBeDistributedToV0InsertionPoint(node))
    return TraverseParentForV0(node, details);

  DCHECK(!ShadowRootWhereNodeCanBeDistributedForV0(node));
  return TraverseParentOrHost(node);
}

ContainerNode* FlatTreeTraversalNg::TraverseParentForV0(
    const Node& node,
    ParentTraversalDetails* details) {
  if (ShadowRootWhereNodeCanBeDistributedForV0(node)) {
    if (const V0InsertionPoint* insertion_point = ResolveReprojection(&node)) {
      if (details)
        details->DidTraverseInsertionPoint(insertion_point);
      // The node is distributed. But the distribution was stopped at this
      // insertion point.
      if (ShadowRootWhereNodeCanBeDistributedForV0(*insertion_point))
        return nullptr;
      return TraverseParent(*insertion_point);
    }
    return nullptr;
  }
  ContainerNode* parent = TraverseParentOrHost(node);
  if (IsActiveV0InsertionPoint(*parent))
    return nullptr;
  return parent;
}

ContainerNode* FlatTreeTraversalNg::TraverseParentOrHost(const Node& node) {
  ContainerNode* parent = node.parentNode();
  if (!parent)
    return nullptr;
  if (!parent->IsShadowRoot())
    return parent;
  ShadowRoot* shadow_root = ToShadowRoot(parent);
  return &shadow_root->host();
}

Node* FlatTreeTraversalNg::ChildAt(const Node& node, unsigned index) {
  AssertPrecondition(node);
  Node* child = TraverseFirstChild(node);
  while (child && index--)
    child = NextSibling(*child);
  AssertPostcondition(child);
  return child;
}

Node* FlatTreeTraversalNg::NextSkippingChildren(const Node& node) {
  if (Node* next_sibling = TraverseNextSibling(node))
    return next_sibling;
  return TraverseNextAncestorSibling(node);
}

bool FlatTreeTraversalNg::ContainsIncludingPseudoElement(
    const ContainerNode& container,
    const Node& node) {
  AssertPrecondition(container);
  AssertPrecondition(node);
  // This can be slower than FlatTreeTraversalNg::contains() because we
  // can't early exit even when container doesn't have children.
  for (const Node* current = &node; current;
       current = TraverseParent(*current)) {
    if (current == &container)
      return true;
  }
  return false;
}

Node* FlatTreeTraversalNg::PreviousSkippingChildren(const Node& node) {
  if (Node* previous_sibling = TraversePreviousSibling(node))
    return previous_sibling;
  return TraversePreviousAncestorSibling(node);
}

Node* FlatTreeTraversalNg::PreviousAncestorSiblingPostOrder(
    const Node& current,
    const Node* stay_within) {
  DCHECK(!FlatTreeTraversalNg::PreviousSibling(current));
  for (Node* parent = FlatTreeTraversalNg::Parent(current); parent;
       parent = FlatTreeTraversalNg::Parent(*parent)) {
    if (parent == stay_within)
      return nullptr;
    if (Node* previous_sibling = FlatTreeTraversalNg::PreviousSibling(*parent))
      return previous_sibling;
  }
  return nullptr;
}

// TODO(yosin) We should consider introducing template class to share code
// between DOM tree traversal and flat tree tarversal.
Node* FlatTreeTraversalNg::PreviousPostOrder(const Node& current,
                                             const Node* stay_within) {
  AssertPrecondition(current);
  if (stay_within)
    AssertPrecondition(*stay_within);
  if (Node* last_child = TraverseLastChild(current)) {
    AssertPostcondition(last_child);
    return last_child;
  }
  if (current == stay_within)
    return nullptr;
  if (Node* previous_sibling = TraversePreviousSibling(current)) {
    AssertPostcondition(previous_sibling);
    return previous_sibling;
  }
  return PreviousAncestorSiblingPostOrder(current, stay_within);
}

bool FlatTreeTraversalNg::IsDescendantOf(const Node& node, const Node& other) {
  AssertPrecondition(node);
  AssertPrecondition(other);
  if (!HasChildren(other) || node.isConnected() != other.isConnected())
    return false;
  for (const ContainerNode* n = TraverseParent(node); n;
       n = TraverseParent(*n)) {
    if (n == other)
      return true;
  }
  return false;
}

Node* FlatTreeTraversalNg::CommonAncestor(const Node& node_a,
                                          const Node& node_b) {
  AssertPrecondition(node_a);
  AssertPrecondition(node_b);
  Node* result = node_a.CommonAncestor(node_b, [](const Node& node) {
    return FlatTreeTraversalNg::Parent(node);
  });
  AssertPostcondition(result);
  return result;
}

Node* FlatTreeTraversalNg::TraverseNextAncestorSibling(const Node& node) {
  DCHECK(!TraverseNextSibling(node));
  for (Node* parent = TraverseParent(node); parent;
       parent = TraverseParent(*parent)) {
    if (Node* next_sibling = TraverseNextSibling(*parent))
      return next_sibling;
  }
  return nullptr;
}

Node* FlatTreeTraversalNg::TraversePreviousAncestorSibling(const Node& node) {
  DCHECK(!TraversePreviousSibling(node));
  for (Node* parent = TraverseParent(node); parent;
       parent = TraverseParent(*parent)) {
    if (Node* previous_sibling = TraversePreviousSibling(*parent))
      return previous_sibling;
  }
  return nullptr;
}

unsigned FlatTreeTraversalNg::Index(const Node& node) {
  AssertPrecondition(node);
  unsigned count = 0;
  for (Node* runner = TraversePreviousSibling(node); runner;
       runner = PreviousSibling(*runner))
    ++count;
  return count;
}

unsigned FlatTreeTraversalNg::CountChildren(const Node& node) {
  AssertPrecondition(node);
  unsigned count = 0;
  for (Node* runner = TraverseFirstChild(node); runner;
       runner = TraverseNextSibling(*runner))
    ++count;
  return count;
}

Node* FlatTreeTraversalNg::LastWithin(const Node& node) {
  AssertPrecondition(node);
  Node* descendant = TraverseLastChild(node);
  for (Node* child = descendant; child; child = LastChild(*child))
    descendant = child;
  AssertPostcondition(descendant);
  return descendant;
}

Node& FlatTreeTraversalNg::LastWithinOrSelf(const Node& node) {
  AssertPrecondition(node);
  Node* last_descendant = LastWithin(node);
  Node& result = last_descendant ? *last_descendant : const_cast<Node&>(node);
  AssertPostcondition(&result);
  return result;
}

}  // namespace blink
