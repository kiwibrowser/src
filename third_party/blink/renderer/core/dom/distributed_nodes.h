/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DISTRIBUTED_NODES_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DISTRIBUTED_NODES_H_

#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class DistributedNodes final {
  DISALLOW_NEW();

 public:
  DistributedNodes() = default;

  Node* First() const { return nodes_.front(); }
  Node* Last() const { return nodes_.back(); }
  Node* at(size_t index) const { return nodes_.at(index); }

  size_t size() const { return nodes_.size(); }
  bool IsEmpty() const { return nodes_.IsEmpty(); }

  void Append(Node*);
  void Clear() {
    nodes_.clear();
    indices_.clear();
  }
  void ShrinkToFit() { nodes_.ShrinkToFit(); }

  bool Contains(const Node* node) const { return indices_.Contains(node); }
  size_t Find(const Node*) const;
  Node* NextTo(const Node*) const;
  Node* PreviousTo(const Node*) const;

  void Swap(DistributedNodes& other);

  void Trace(blink::Visitor*);

 private:
  HeapVector<Member<Node>> nodes_;
  HeapHashMap<Member<const Node>, size_t> indices_;
};

}  // namespace blink

#endif
