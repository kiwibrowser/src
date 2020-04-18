/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_V0_INSERTION_POINT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_V0_INSERTION_POINT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/css/css_selector_list.h"
#include "third_party/blink/renderer/core/dom/distributed_nodes.h"
#include "third_party/blink/renderer/core/html/html_element.h"

namespace blink {

class CORE_EXPORT V0InsertionPoint : public HTMLElement {
 public:
  ~V0InsertionPoint() override;

  bool HasDistribution() const { return !distributed_nodes_.IsEmpty(); }
  void SetDistributedNodes(DistributedNodes&);
  void ClearDistribution() { distributed_nodes_.Clear(); }
  bool IsActive() const;
  bool CanBeActive() const;

  bool IsContentInsertionPoint() const;

  StaticNodeList* getDistributedNodes();

  virtual bool CanAffectSelector() const { return false; }

  void AttachLayoutTree(AttachContext&) override;
  void DetachLayoutTree(const AttachContext& = AttachContext()) override;
  void RebuildDistributedChildrenLayoutTrees(WhitespaceAttacher&);

  size_t DistributedNodesSize() const { return distributed_nodes_.size(); }
  Node* DistributedNodeAt(size_t index) const {
    return distributed_nodes_.at(index);
  }
  Node* FirstDistributedNode() const {
    return distributed_nodes_.IsEmpty() ? nullptr : distributed_nodes_.First();
  }
  Node* LastDistributedNode() const {
    return distributed_nodes_.IsEmpty() ? nullptr : distributed_nodes_.Last();
  }
  Node* DistributedNodeNextTo(const Node* node) const {
    return distributed_nodes_.NextTo(node);
  }
  Node* DistributedNodePreviousTo(const Node* node) const {
    return distributed_nodes_.PreviousTo(node);
  }

  void Trace(blink::Visitor*) override;

 protected:
  V0InsertionPoint(const QualifiedName&, Document&);
  bool LayoutObjectIsNeeded(const ComputedStyle&) const override;
  void ChildrenChanged(const ChildrenChange&) override;
  InsertionNotificationRequest InsertedInto(ContainerNode*) override;
  void RemovedFrom(ContainerNode*) override;
  void WillRecalcStyle(StyleRecalcChange) override;

 private:
  bool IsV0InsertionPoint() const =
      delete;  // This will catch anyone doing an unnecessary check.

  DistributedNodes distributed_nodes_;
  bool registered_with_shadow_root_;
};

using DestinationInsertionPoints = HeapVector<Member<V0InsertionPoint>, 1>;

DEFINE_ELEMENT_TYPE_CASTS(V0InsertionPoint, IsV0InsertionPoint());

inline bool IsActiveV0InsertionPoint(const Node& node) {
  return node.IsV0InsertionPoint() && ToV0InsertionPoint(node).IsActive();
}

inline ShadowRoot* ShadowRootWhereNodeCanBeDistributedForV0(const Node& node) {
  Node* parent = node.parentNode();
  if (!parent)
    return nullptr;
  if (IsActiveV0InsertionPoint(*parent))
    return node.ContainingShadowRoot();
  if (parent->IsElementNode())
    return ToElement(parent)->GetShadowRoot();
  return nullptr;
}

const V0InsertionPoint* ResolveReprojection(const Node*);

void CollectDestinationInsertionPoints(
    const Node&,
    HeapVector<Member<V0InsertionPoint>, 8>& results);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_V0_INSERTION_POINT_H_
