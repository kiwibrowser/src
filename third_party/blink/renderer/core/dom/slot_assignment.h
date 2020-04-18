// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_SLOT_ASSIGNMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_SLOT_ASSIGNMENT_H_

#include "third_party/blink/renderer/core/dom/tree_ordered_map.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_hash.h"

namespace blink {

class HTMLSlotElement;
class Node;
class ShadowRoot;

class SlotAssignment final : public GarbageCollected<SlotAssignment> {
 public:
  static SlotAssignment* Create(ShadowRoot& owner) {
    return new SlotAssignment(owner);
  }

  // Relevant DOM Standard: https://dom.spec.whatwg.org/#find-a-slot
  HTMLSlotElement* FindSlot(const Node&) const;
  HTMLSlotElement* FindSlotByName(const AtomicString& slot_name) const;

  // DOM Standaard defines these two procedures:
  // 1. https://dom.spec.whatwg.org/#assign-a-slot
  //    void assignSlot(const Node& slottable);
  // 2. https://dom.spec.whatwg.org/#assign-slotables
  //    void assignSlotables(HTMLSlotElement&);
  // As an optimization, Blink does not implement these literally.
  // Instead, provide alternative, HTMLSlotElement::hasAssignedNodesSlow()
  // so that slotchange can be detected.

  const HeapVector<Member<HTMLSlotElement>>& Slots();

  void DidAddSlot(HTMLSlotElement&);
  void DidRemoveSlot(HTMLSlotElement&);
  void DidRenameSlot(const AtomicString& old_name, HTMLSlotElement&);
  void DidChangeHostChildSlotName(const AtomicString& old_value,
                                  const AtomicString& new_value);

  bool FindHostChildBySlotName(const AtomicString& slot_name) const;

  void Trace(blink::Visitor*);

  // For Incremental Shadow DOM
  bool NeedsAssignmentRecalc() const { return needs_assignment_recalc_; }
  void SetNeedsAssignmentRecalc();
  void RecalcAssignment();

  // For Non-Incremental Shadow DOM
  void RecalcDistribution();

 private:
  explicit SlotAssignment(ShadowRoot& owner);

  enum class SlotMutationType {
    kRemoved,
    kRenamed,
  };

  HTMLSlotElement* FindSlotInUserAgentShadow(const Node&) const;

  void CollectSlots();
  HTMLSlotElement* GetCachedFirstSlotWithoutAccessingNodeTree(
      const AtomicString& slot_name);

  void DidAddSlotInternal(HTMLSlotElement&);
  void DidRemoveSlotInternal(HTMLSlotElement&,
                             const AtomicString& slot_name,
                             SlotMutationType);

  // For Non-Incremental Shadow DOM
  void RecalcAssignmentForDistribution();

  HeapVector<Member<HTMLSlotElement>> slots_;
  Member<TreeOrderedMap> slot_map_;
  WeakMember<ShadowRoot> owner_;
  unsigned needs_collect_slots_ : 1;
  unsigned needs_assignment_recalc_ : 1;  // For Incremental Shadow DOM
  unsigned slot_count_ : 30;
};

}  // namespace blink

#endif  // HTMLSlotAssignment_h
