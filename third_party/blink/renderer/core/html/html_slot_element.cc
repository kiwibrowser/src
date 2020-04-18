/*
 * Copyright (C) 2015 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
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

#include "third_party/blink/renderer/core/html/html_slot_element.h"

#include <array>
#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/dom/node_traversal.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/dom/slot_assignment.h"
#include "third_party/blink/renderer/core/dom/whitespace_attacher.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/assigned_nodes_options.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/bindings/microtask.h"

namespace blink {

using namespace HTMLNames;

namespace {
constexpr size_t kLCSTableSizeLimit = 16;
}

HTMLSlotElement* HTMLSlotElement::Create(Document& document) {
  return new HTMLSlotElement(document);
}

HTMLSlotElement* HTMLSlotElement::CreateUserAgentDefaultSlot(
    Document& document) {
  HTMLSlotElement* slot = new HTMLSlotElement(document);
  slot->setAttribute(nameAttr, UserAgentDefaultSlotName());
  return slot;
}

HTMLSlotElement* HTMLSlotElement::CreateUserAgentCustomAssignSlot(
    Document& document) {
  HTMLSlotElement* slot = new HTMLSlotElement(document);
  slot->setAttribute(nameAttr, UserAgentCustomAssignSlotName());
  return slot;
}

inline HTMLSlotElement::HTMLSlotElement(Document& document)
    : HTMLElement(slotTag, document) {
  UseCounter::Count(document, WebFeature::kHTMLSlotElement);
  SetHasCustomStyleCallbacks();
}

// static
AtomicString HTMLSlotElement::NormalizeSlotName(const AtomicString& name) {
  return (name.IsNull() || name.IsEmpty()) ? g_empty_atom : name;
}

// static
const AtomicString& HTMLSlotElement::UserAgentDefaultSlotName() {
  DEFINE_STATIC_LOCAL(const AtomicString, user_agent_default_slot_name,
                      ("user-agent-default-slot"));
  return user_agent_default_slot_name;
}

// static
const AtomicString& HTMLSlotElement::UserAgentCustomAssignSlotName() {
  DEFINE_STATIC_LOCAL(const AtomicString, user_agent_custom_assign_slot_name,
                      ("user-agent-custom-assign-slot"));
  return user_agent_custom_assign_slot_name;
}

const HeapVector<Member<Node>>& HTMLSlotElement::AssignedNodes() const {
  if (!SupportsAssignment()) {
    DCHECK(assigned_nodes_.IsEmpty());
    return assigned_nodes_;
  }
  if (RuntimeEnabledFeatures::IncrementalShadowDOMEnabled()) {
    ContainingShadowRoot()->GetSlotAssignment().RecalcAssignment();
    return assigned_nodes_;
  }

  DCHECK(!NeedsDistributionRecalc());
  return assigned_nodes_;
}

namespace {

HeapVector<Member<Node>> CollectFlattenedAssignedNodes(
    const HTMLSlotElement& slot) {
  DCHECK(RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  DCHECK(slot.SupportsAssignment());

  const HeapVector<Member<Node>>& assigned_nodes = slot.AssignedNodes();
  HeapVector<Member<Node>> nodes;
  if (assigned_nodes.IsEmpty()) {
    // Fallback contents.
    for (auto& child : NodeTraversal::ChildrenOf(slot)) {
      if (!child.IsSlotable())
        continue;
      if (auto* slot = ToHTMLSlotElementIfSupportsAssignmentOrNull(child))
        nodes.AppendVector(CollectFlattenedAssignedNodes(*slot));
      else
        nodes.push_back(child);
    }
  } else {
    for (auto& node : assigned_nodes) {
      DCHECK(node->IsSlotable());
      if (auto* slot = ToHTMLSlotElementIfSupportsAssignmentOrNull(*node))
        nodes.AppendVector(CollectFlattenedAssignedNodes(*slot));
      else
        nodes.push_back(node);
    }
  }
  return nodes;
}

}  // namespace

const HeapVector<Member<Node>> HTMLSlotElement::FlattenedAssignedNodes() {
  if (!SupportsAssignment()) {
    DCHECK(assigned_nodes_.IsEmpty());
    return assigned_nodes_;
  }
  if (RuntimeEnabledFeatures::IncrementalShadowDOMEnabled())
    return CollectFlattenedAssignedNodes(*this);
  UpdateDistributionForLegacyDistributedNodes();
  return GetDistributedNodes();
}

const HeapVector<Member<Node>> HTMLSlotElement::AssignedNodesForBinding(
    const AssignedNodesOptions& options) {
  if (options.hasFlatten() && options.flatten())
    return FlattenedAssignedNodes();
  if (RuntimeEnabledFeatures::IncrementalShadowDOMEnabled())
    return AssignedNodes();
  if (!SupportsAssignment()) {
    DCHECK(assigned_nodes_.IsEmpty());
    return assigned_nodes_;
  }
  UpdateDistributionForLegacyDistributedNodes();
  return assigned_nodes_;
}

const HeapVector<Member<Element>> HTMLSlotElement::AssignedElements() {
  HeapVector<Member<Element>> elements;
  for (auto& node : AssignedNodes()) {
    if (Element* element = ToElementOrNull(node))
      elements.push_back(element);
  }
  return elements;
}

const HeapVector<Member<Element>> HTMLSlotElement::AssignedElementsForBinding(
    const AssignedNodesOptions& options) {
  HeapVector<Member<Element>> elements;
  for (auto& node : AssignedNodesForBinding(options)) {
    if (Element* element = ToElementOrNull(node))
      elements.push_back(element);
  }
  return elements;
}

const HeapVector<Member<Node>>& HTMLSlotElement::GetDistributedNodes() {
  DCHECK(!RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  DCHECK(!NeedsDistributionRecalc());
  DCHECK(SupportsAssignment() || distributed_nodes_.IsEmpty());
  return distributed_nodes_;
}

void HTMLSlotElement::AppendAssignedNode(Node& host_child) {
  DCHECK(host_child.IsSlotable());
  assigned_nodes_.push_back(&host_child);
}

void HTMLSlotElement::RecalcDistributedNodes() {
  DCHECK(!RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  for (auto& node : assigned_nodes_) {
    DCHECK(node->IsSlotable());
    if (HTMLSlotElement* slot =
            ToHTMLSlotElementIfSupportsAssignmentOrNull(*node)) {
      AppendDistributedNodesFrom(*slot);
    } else {
      AppendDistributedNode(*node);
    }

    if (IsChildOfV1ShadowHost())
      ParentElementShadowRoot()->SetNeedsDistributionRecalc();
  }
}

void HTMLSlotElement::AppendDistributedNode(Node& node) {
  DCHECK(!RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  size_t size = distributed_nodes_.size();
  distributed_nodes_.push_back(&node);
  distributed_indices_.Set(&node, size);
}

void HTMLSlotElement::AppendDistributedNodesFrom(const HTMLSlotElement& other) {
  DCHECK(!RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  size_t index = distributed_nodes_.size();
  distributed_nodes_.AppendVector(other.distributed_nodes_);
  for (const auto& node : other.distributed_nodes_)
    distributed_indices_.Set(node.Get(), index++);
}

void HTMLSlotElement::ClearAssignedNodes() {
  DCHECK(RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  assigned_nodes_.clear();
}

void HTMLSlotElement::ClearAssignedNodesAndFlatTreeChildren() {
  DCHECK(RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  assigned_nodes_.clear();
  flat_tree_children_.clear();
}

void HTMLSlotElement::RecalcFlatTreeChildren() {
  DCHECK(RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  DCHECK(SupportsAssignment());

  HeapVector<Member<Node>> old_flat_tree_children;
  old_flat_tree_children.swap(flat_tree_children_);

  if (assigned_nodes_.IsEmpty()) {
    // Use children as fallback
    for (auto& child : NodeTraversal::ChildrenOf(*this))
      flat_tree_children_.push_back(child);
  } else {
    flat_tree_children_ = assigned_nodes_;
  }

  LazyReattachNodesIfNeeded(old_flat_tree_children, flat_tree_children_);
}

void HTMLSlotElement::ClearDistribution() {
  DCHECK(!RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  assigned_nodes_.clear();
  distributed_nodes_.clear();
  distributed_indices_.clear();
}

void HTMLSlotElement::SaveAndClearDistribution() {
  DCHECK(!RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  old_distributed_nodes_.swap(distributed_nodes_);
  ClearDistribution();
}

void HTMLSlotElement::DispatchSlotChangeEvent() {
  DCHECK(!IsInUserAgentShadowRoot());
  Event* event = Event::CreateBubble(EventTypeNames::slotchange);
  event->SetTarget(this);
  DispatchScopedEvent(event);
}

Node* HTMLSlotElement::AssignedNodeNextTo(const Node& node) const {
  DCHECK(SupportsAssignment());
  if (RuntimeEnabledFeatures::IncrementalShadowDOMEnabled())
    ContainingShadowRoot()->GetSlotAssignment().RecalcAssignment();
  else
    DCHECK(!NeedsDistributionRecalc());
  // TODO(crbug.com/776656): Use {node -> index} map to avoid O(N) lookup
  size_t index = assigned_nodes_.Find(&node);
  DCHECK(index != WTF::kNotFound);
  if (index + 1 == assigned_nodes_.size())
    return nullptr;
  return assigned_nodes_[index + 1].Get();
}

Node* HTMLSlotElement::AssignedNodePreviousTo(const Node& node) const {
  DCHECK(SupportsAssignment());
  if (RuntimeEnabledFeatures::IncrementalShadowDOMEnabled())
    ContainingShadowRoot()->GetSlotAssignment().RecalcAssignment();
  else
    DCHECK(!NeedsDistributionRecalc());
  // TODO(crbug.com/776656): Use {node -> index} map to avoid O(N) lookup
  size_t index = assigned_nodes_.Find(&node);
  DCHECK(index != WTF::kNotFound);
  if (index == 0)
    return nullptr;
  return assigned_nodes_[index - 1].Get();
}

Node* HTMLSlotElement::DistributedNodeNextTo(const Node& node) const {
  DCHECK(!RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  DCHECK(SupportsAssignment());
  const auto& it = distributed_indices_.find(&node);
  if (it == distributed_indices_.end())
    return nullptr;
  size_t index = it->value;
  if (index + 1 == distributed_nodes_.size())
    return nullptr;
  return distributed_nodes_[index + 1].Get();
}

Node* HTMLSlotElement::DistributedNodePreviousTo(const Node& node) const {
  DCHECK(!RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  DCHECK(SupportsAssignment());
  const auto& it = distributed_indices_.find(&node);
  if (it == distributed_indices_.end())
    return nullptr;
  size_t index = it->value;
  if (index == 0)
    return nullptr;
  return distributed_nodes_[index - 1].Get();
}

AtomicString HTMLSlotElement::GetName() const {
  return NormalizeSlotName(FastGetAttribute(nameAttr));
}

void HTMLSlotElement::AttachLayoutTree(AttachContext& context) {
  HTMLElement::AttachLayoutTree(context);

  if (SupportsAssignment()) {
    AttachContext children_context(context);

    for (auto& node : ChildrenInFlatTreeIfAssignmentIsSupported()) {
      if (node->NeedsAttach())
        node->AttachLayoutTree(children_context);
    }
    if (children_context.previous_in_flow)
      context.previous_in_flow = children_context.previous_in_flow;
  }
}

// TODO(hayato): Rename this function once we enable IncrementalShadowDOM
// by default because this function doesn't consider fallback elements in case
// of IncementalShadowDOM.
const HeapVector<Member<Node>>&
HTMLSlotElement::ChildrenInFlatTreeIfAssignmentIsSupported() {
  if (RuntimeEnabledFeatures::SlotInFlatTreeEnabled())
    return AssignedNodes();
  DCHECK(!NeedsDistributionRecalc());
  return distributed_nodes_;
}

void HTMLSlotElement::DetachLayoutTree(const AttachContext& context) {
  if (SupportsAssignment()) {
    const HeapVector<Member<Node>>& flat_tree_children =
        RuntimeEnabledFeatures::SlotInFlatTreeEnabled() ? assigned_nodes_
                                                        : distributed_nodes_;
    for (auto& node : flat_tree_children)
      node->LazyReattachIfAttached();
  }
  HTMLElement::DetachLayoutTree(context);
}

void HTMLSlotElement::RebuildDistributedChildrenLayoutTrees(
    WhitespaceAttacher& whitespace_attacher) {
  if (!SupportsAssignment())
    return;

  const HeapVector<Member<Node>>& flat_tree_children =
      ChildrenInFlatTreeIfAssignmentIsSupported();

  // This loop traverses the nodes from right to left for the same reason as the
  // one described in ContainerNode::RebuildChildrenLayoutTrees().
  for (auto it = flat_tree_children.rbegin(); it != flat_tree_children.rend();
       ++it) {
    RebuildLayoutTreeForChild(*it, whitespace_attacher);
  }
}

void HTMLSlotElement::AttributeChanged(
    const AttributeModificationParams& params) {
  if (params.name == nameAttr) {
    if (ShadowRoot* root = ContainingShadowRoot()) {
      if (root->IsV1() && params.old_value != params.new_value) {
        root->GetSlotAssignment().DidRenameSlot(
            NormalizeSlotName(params.old_value), *this);
      }
    }
  }
  HTMLElement::AttributeChanged(params);
}

Node::InsertionNotificationRequest HTMLSlotElement::InsertedInto(
    ContainerNode* insertion_point) {
  HTMLElement::InsertedInto(insertion_point);
  if (SupportsAssignment()) {
    ShadowRoot* root = ContainingShadowRoot();
    DCHECK(root);
    DCHECK(root->IsV1());
    if (root == insertion_point->ContainingShadowRoot()) {
      // This slot is inserted into the same tree of |insertion_point|
      root->DidAddSlot(*this);
    }
  }
  return kInsertionDone;
}

void HTMLSlotElement::RemovedFrom(ContainerNode* insertion_point) {
  // `removedFrom` is called after the node is removed from the tree.
  // That means:
  // 1. If this slot is still in a tree scope, it means the slot has been in a
  // shadow tree. An inclusive shadow-including ancestor of the shadow host was
  // originally removed from its parent.
  // 2. Or (this slot is not in a tree scope), this slot's inclusive
  // ancestor was orginally removed from its parent (== insertion point). This
  // slot and the originally removed node was in the same tree before removal.

  // For exmaple, given the following trees, (srN: = shadow root, sN: = slot)
  // a
  // |- b --sr1
  // |- c   |--d
  //           |- e-----sr2
  //              |- s1 |--f
  //                    |--s2

  // If we call 'e.remove()', then:
  // - For slot s1, s1.removedFrom(d) is called.
  // - For slot s2, s2.removedFrom(d) is called.

  // ContainingShadowRoot() is okay to use here because 1) It doesn't use
  // kIsInShadowTreeFlag flag, and 2) TreeScope has been alreay updated for the
  // slot.
  if (insertion_point->IsInV1ShadowTree() && !ContainingShadowRoot()) {
    // This slot was in a shadow tree and got disconnected from the shadow tree
    insertion_point->ContainingShadowRoot()->GetSlotAssignment().DidRemoveSlot(
        *this);
    if (RuntimeEnabledFeatures::IncrementalShadowDOMEnabled()) {
      ClearAssignedNodesAndFlatTreeChildren();
    } else {
      ClearDistribution();
    }
  }

  HTMLElement::RemovedFrom(insertion_point);
}

void HTMLSlotElement::WillRecalcStyle(StyleRecalcChange change) {
  if (RuntimeEnabledFeatures::SlotInFlatTreeEnabled())
    return;
  if (change < kIndependentInherit &&
      GetStyleChangeType() < kSubtreeStyleChange) {
    return;
  }
  for (auto& node : distributed_nodes_) {
    node->SetNeedsStyleRecalc(
        kLocalStyleChange,
        StyleChangeReasonForTracing::Create(
            StyleChangeReason::kPropagateInheritChangeToDistributedNodes));
  }
}

void HTMLSlotElement::DidRecalcStyle(StyleRecalcChange change) {
  if (!RuntimeEnabledFeatures::SlotInFlatTreeEnabled())
    return;
  if (change < kIndependentInherit)
    return;
  for (auto& node : assigned_nodes_) {
    node->SetNeedsStyleRecalc(
        kLocalStyleChange,
        StyleChangeReasonForTracing::Create(
            StyleChangeReason::kPropagateInheritChangeToDistributedNodes));
  }
}

void HTMLSlotElement::UpdateDistributedNodesWithFallback() {
  DCHECK(!RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());
  if (!distributed_nodes_.IsEmpty())
    return;
  for (auto& child : NodeTraversal::ChildrenOf(*this)) {
    if (!child.IsSlotable())
      continue;
    if (auto* slot = ToHTMLSlotElementOrNull(child))
      AppendDistributedNodesFrom(*slot);
    else
      AppendDistributedNode(child);
  }
}

void HTMLSlotElement::LazyReattachNodesByDynamicProgramming(
    const HeapVector<Member<Node>>& nodes1,
    const HeapVector<Member<Node>>& nodes2) {
  // Use dynamic programming to minimize the number of nodes being reattached.
  using LCSTable =
      std::array<std::array<size_t, kLCSTableSizeLimit>, kLCSTableSizeLimit>;
  using Backtrack = std::pair<size_t, size_t>;
  using BacktrackTable =
      std::array<std::array<Backtrack, kLCSTableSizeLimit>, kLCSTableSizeLimit>;

  DEFINE_STATIC_LOCAL(LCSTable*, lcs_table, (new LCSTable));
  DEFINE_STATIC_LOCAL(BacktrackTable*, backtrack_table, (new BacktrackTable));

  FillLongestCommonSubsequenceDynamicProgrammingTable(
      nodes1, nodes2, *lcs_table, *backtrack_table);

  size_t r = nodes1.size();
  size_t c = nodes2.size();
  while (r > 0 && c > 0) {
    Backtrack backtrack = (*backtrack_table)[r][c];
    if (backtrack == std::make_pair(r - 1, c - 1)) {
      DCHECK_EQ(nodes1[r - 1], nodes2[c - 1]);
    } else if (backtrack == std::make_pair(r - 1, c)) {
      nodes1[r - 1]->LazyReattachIfAttached();
    } else {
      DCHECK(backtrack == std::make_pair(r, c - 1));
      nodes2[c - 1]->LazyReattachIfAttached();
    }
    std::tie(r, c) = backtrack;
  }
  if (r > 0) {
    for (size_t i = 0; i < r; ++i)
      nodes1[i]->LazyReattachIfAttached();
  } else if (c > 0) {
    for (size_t i = 0; i < c; ++i)
      nodes2[i]->LazyReattachIfAttached();
  }
}

void HTMLSlotElement::LazyReattachDistributedNodesIfNeeded() {
  DCHECK(!RuntimeEnabledFeatures::IncrementalShadowDOMEnabled());

  LazyReattachNodesIfNeeded(old_distributed_nodes_, distributed_nodes_);
  old_distributed_nodes_.clear();
}

void HTMLSlotElement::LazyReattachNodesIfNeeded(
    const HeapVector<Member<Node>>& nodes1,
    const HeapVector<Member<Node>>& nodes2) {
  if (nodes1 == nodes2)
    return;
  probe::didPerformSlotDistribution(this);

  if (nodes1.size() + 1 > kLCSTableSizeLimit ||
      nodes2.size() + 1 > kLCSTableSizeLimit) {
    // Since DP takes O(N^2), we don't use DP if the size is larger than the
    // pre-defined limit.
    LazyReattachNodesNaive(nodes1, nodes2);
  } else {
    LazyReattachNodesByDynamicProgramming(nodes1, nodes2);
  }
}

void HTMLSlotElement::DidSlotChangeAfterRemovedFromShadowTree() {
  DCHECK(!ContainingShadowRoot());
  EnqueueSlotChangeEvent();
  CheckSlotChange(SlotChangeType::kSuppressSlotChangeEvent);
}

void HTMLSlotElement::DidSlotChangeAfterRenaming() {
  DCHECK(SupportsAssignment());
  EnqueueSlotChangeEvent();
  SetNeedsDistributionRecalcWillBeSetNeedsAssignmentRecalc();
  CheckSlotChange(SlotChangeType::kSuppressSlotChangeEvent);
}

void HTMLSlotElement::LazyReattachNodesNaive(
    const HeapVector<Member<Node>>& nodes1,
    const HeapVector<Member<Node>>& nodes2) {
  // TODO(hayato): Use some heuristic to avoid reattaching all nodes
  for (auto& node : nodes1)
    node->LazyReattachIfAttached();
  for (auto& node : nodes2)
    node->LazyReattachIfAttached();
}

void HTMLSlotElement::
    SetNeedsDistributionRecalcWillBeSetNeedsAssignmentRecalc() {
  if (RuntimeEnabledFeatures::IncrementalShadowDOMEnabled())
    ContainingShadowRoot()->GetSlotAssignment().SetNeedsAssignmentRecalc();
  else
    ContainingShadowRoot()->SetNeedsDistributionRecalc();
}

void HTMLSlotElement::DidSlotChange(SlotChangeType slot_change_type) {
  DCHECK(SupportsAssignment());
  if (slot_change_type == SlotChangeType::kSignalSlotChangeEvent)
    EnqueueSlotChangeEvent();
  SetNeedsDistributionRecalcWillBeSetNeedsAssignmentRecalc();
  // Check slotchange recursively since this slotchange may cause another
  // slotchange.
  CheckSlotChange(SlotChangeType::kSuppressSlotChangeEvent);
}

void HTMLSlotElement::CheckFallbackAfterInsertedIntoShadowTree() {
  DCHECK(SupportsAssignment());
  if (HasSlotableChild()) {
    // We use kSuppress here because a slotchange event shouldn't be
    // dispatched if a slot being inserted don't get any assigned
    // node, but has a slotable child, according to DOM Standard.
    DidSlotChange(SlotChangeType::kSuppressSlotChangeEvent);
  }
}

void HTMLSlotElement::CheckFallbackAfterRemovedFromShadowTree() {
  if (HasSlotableChild()) {
    // Since a slot was removed from a shadow tree,
    // we don't need to set dirty flag for a disconnected tree.
    // However, we need to call CheckSlotChange because we might need to set a
    // dirty flag for a shadow tree which a parent of the slot may host.
    CheckSlotChange(SlotChangeType::kSuppressSlotChangeEvent);
  }
}

bool HTMLSlotElement::HasSlotableChild() const {
  for (auto& child : NodeTraversal::ChildrenOf(*this)) {
    if (child.IsSlotable())
      return true;
  }
  return false;
}

void HTMLSlotElement::EnqueueSlotChangeEvent() {
  // TODO(kochi): This suppresses slotchange event on user-agent shadows,
  // but could be improved further by not running change detection logic
  // in SlotAssignment::Did{Add,Remove}SlotInternal etc., although naive
  // skipping turned out breaking fallback content handling.
  if (IsInUserAgentShadowRoot())
    return;
  if (slotchange_event_enqueued_)
    return;
  MutationObserver::EnqueueSlotChange(*this);
  slotchange_event_enqueued_ = true;
}

bool HTMLSlotElement::HasAssignedNodesSlow() const {
  ShadowRoot* root = ContainingShadowRoot();
  DCHECK(root);
  DCHECK(root->IsV1());
  SlotAssignment& assignment = root->GetSlotAssignment();
  if (assignment.FindSlotByName(GetName()) != this)
    return false;
  return assignment.FindHostChildBySlotName(GetName());
}

bool HTMLSlotElement::FindHostChildWithSameSlotName() const {
  ShadowRoot* root = ContainingShadowRoot();
  DCHECK(root);
  DCHECK(root->IsV1());
  SlotAssignment& assignment = root->GetSlotAssignment();
  return assignment.FindHostChildBySlotName(GetName());
}

int HTMLSlotElement::tabIndex() const {
  return Element::tabIndex();
}

void HTMLSlotElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(assigned_nodes_);
  visitor->Trace(flat_tree_children_);
  visitor->Trace(distributed_nodes_);
  visitor->Trace(old_distributed_nodes_);
  visitor->Trace(distributed_indices_);
  HTMLElement::Trace(visitor);
}

}  // namespace blink
