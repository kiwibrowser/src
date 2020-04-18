/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/dom/distributed_nodes.h"

#include "third_party/blink/renderer/core/dom/v0_insertion_point.h"

namespace blink {

void DistributedNodes::Swap(DistributedNodes& other) {
  nodes_.swap(other.nodes_);
  indices_.swap(other.indices_);
}

void DistributedNodes::Append(Node* node) {
  DCHECK(node);
  DCHECK(node->CanParticipateInFlatTree());
  size_t size = nodes_.size();
  indices_.Set(node, size);
  nodes_.push_back(node);
}

size_t DistributedNodes::Find(const Node* node) const {
  HeapHashMap<Member<const Node>, size_t>::const_iterator it =
      indices_.find(node);
  if (it == indices_.end())
    return kNotFound;

  return it.Get()->value;
}

Node* DistributedNodes::NextTo(const Node* node) const {
  size_t index = Find(node);
  if (index == kNotFound || index + 1 == size())
    return nullptr;
  return at(index + 1);
}

Node* DistributedNodes::PreviousTo(const Node* node) const {
  size_t index = Find(node);
  if (index == kNotFound || !index)
    return nullptr;
  return at(index - 1);
}

void DistributedNodes::Trace(blink::Visitor* visitor) {
  visitor->Trace(nodes_);
  visitor->Trace(indices_);
}

}  // namespace blink
