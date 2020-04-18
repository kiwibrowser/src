// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_SYNCHRONOUS_MUTATION_NOTIFIER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_SYNCHRONOUS_MUTATION_NOTIFIER_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/lifecycle_notifier.h"

namespace blink {

class CharacterData;
class ContainerNode;
class Document;
class Node;
class NodeWithIndex;
class SynchronousMutationObserver;
class Text;

class CORE_EXPORT SynchronousMutationNotifier
    : public LifecycleNotifier<Document, SynchronousMutationObserver> {
 public:
  // TODO(yosin): We will have |notifyXXX()| functions defined in
  // |SynchronousMutationObserver|.
  void NotifyChangeChildren(const ContainerNode&);
  void NotifyMergeTextNodes(const Text& merged_node,
                            const NodeWithIndex& node_to_be_removed_with_index,
                            unsigned old_length);
  void NotifyMoveTreeToNewDocument(const Node&);
  void NotifySplitTextNode(const Text&);
  void NotifyUpdateCharacterData(CharacterData*,
                                 unsigned offset,
                                 unsigned old_length,
                                 unsigned new_length);
  void NotifyNodeChildrenWillBeRemoved(ContainerNode&);
  void NotifyNodeWillBeRemoved(Node&);

 protected:
  SynchronousMutationNotifier();

 private:
  DISALLOW_COPY_AND_ASSIGN(SynchronousMutationNotifier);
};

}  // namespace dom

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_SYNCHRONOUS_MUTATION_NOTIFIER_H_
