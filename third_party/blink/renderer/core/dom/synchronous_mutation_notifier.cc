// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/synchronous_mutation_notifier.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/synchronous_mutation_observer.h"

namespace blink {

SynchronousMutationNotifier::SynchronousMutationNotifier() = default;

void SynchronousMutationNotifier::NotifyChangeChildren(
    const ContainerNode& container) {
  for (SynchronousMutationObserver* observer : observers_)
    observer->DidChangeChildren(container);
}

void SynchronousMutationNotifier::NotifyMergeTextNodes(
    const Text& node,
    const NodeWithIndex& node_to_be_removed_with_index,
    unsigned old_length) {
  for (SynchronousMutationObserver* observer : observers_)
    observer->DidMergeTextNodes(node, node_to_be_removed_with_index,
                                old_length);
}

void SynchronousMutationNotifier::NotifyMoveTreeToNewDocument(
    const Node& root) {
  for (SynchronousMutationObserver* observer : observers_)
    observer->DidMoveTreeToNewDocument(root);
}

void SynchronousMutationNotifier::NotifySplitTextNode(const Text& node) {
  for (SynchronousMutationObserver* observer : observers_)
    observer->DidSplitTextNode(node);
}

void SynchronousMutationNotifier::NotifyUpdateCharacterData(
    CharacterData* character_data,
    unsigned offset,
    unsigned old_length,
    unsigned new_length) {
  for (SynchronousMutationObserver* observer : observers_) {
    observer->DidUpdateCharacterData(character_data, offset, old_length,
                                     new_length);
  }
}

void SynchronousMutationNotifier::NotifyNodeChildrenWillBeRemoved(
    ContainerNode& container) {
  for (SynchronousMutationObserver* observer : observers_)
    observer->NodeChildrenWillBeRemoved(container);
}

void SynchronousMutationNotifier::NotifyNodeWillBeRemoved(Node& node) {
  for (SynchronousMutationObserver* observer : observers_)
    observer->NodeWillBeRemoved(node);
}

}  // namespace blink
