// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/heap/persistent_node.h"

#include "base/debug/alias.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/heap/process_heap.h"

namespace blink {

namespace {

class DummyGCBase final : public GarbageCollected<DummyGCBase> {
 public:
  void Trace(blink::Visitor* visitor) {}
};
}

PersistentRegion::~PersistentRegion() {
  PersistentNodeSlots* slots = slots_;
  while (slots) {
    PersistentNodeSlots* dead_slots = slots;
    slots = slots->next_;
    delete dead_slots;
  }
}

int PersistentRegion::NumberOfPersistents() {
  int persistent_count = 0;
  for (PersistentNodeSlots* slots = slots_; slots; slots = slots->next_) {
    for (int i = 0; i < PersistentNodeSlots::kSlotCount; ++i) {
      if (!slots->slot_[i].IsUnused())
        ++persistent_count;
    }
  }
#if DCHECK_IS_ON()
  DCHECK_EQ(persistent_count, persistent_count_);
#endif
  return persistent_count;
}

void PersistentRegion::EnsurePersistentNodeSlots(void* self,
                                                 TraceCallback trace) {
  DCHECK(!free_list_head_);
  PersistentNodeSlots* slots = new PersistentNodeSlots;
  for (int i = 0; i < PersistentNodeSlots::kSlotCount; ++i) {
    PersistentNode* node = &slots->slot_[i];
    node->SetFreeListNext(free_list_head_);
    free_list_head_ = node;
    DCHECK(node->IsUnused());
  }
  slots->next_ = slots_;
  slots_ = slots;
}

void PersistentRegion::ReleasePersistentNode(
    PersistentNode* persistent_node,
    ThreadState::PersistentClearCallback callback) {
  DCHECK(!persistent_node->IsUnused());
  // 'self' is in use, containing the persistent wrapper object.
  void* self = persistent_node->Self();
  if (callback) {
    (*callback)(self);
    DCHECK(persistent_node->IsUnused());
    return;
  }
  Persistent<DummyGCBase>* persistent =
      reinterpret_cast<Persistent<DummyGCBase>*>(self);
  persistent->Clear();
  DCHECK(persistent_node->IsUnused());
}

// This function traces all PersistentNodes. If we encounter
// a PersistentNodeSlot that contains only freed PersistentNodes,
// we delete the PersistentNodeSlot. This function rebuilds the free
// list of PersistentNodes.
void PersistentRegion::TracePersistentNodes(Visitor* visitor,
                                            ShouldTraceCallback should_trace) {
  size_t debug_marked_object_size = ProcessHeap::TotalMarkedObjectSize();
  base::debug::Alias(&debug_marked_object_size);

  free_list_head_ = nullptr;
  int persistent_count = 0;
  PersistentNodeSlots** prev_next = &slots_;
  PersistentNodeSlots* slots = slots_;
  while (slots) {
    PersistentNode* free_list_next = nullptr;
    PersistentNode* free_list_last = nullptr;
    int free_count = 0;
    for (int i = 0; i < PersistentNodeSlots::kSlotCount; ++i) {
      PersistentNode* node = &slots->slot_[i];
      if (node->IsUnused()) {
        if (!free_list_next)
          free_list_last = node;
        node->SetFreeListNext(free_list_next);
        free_list_next = node;
        ++free_count;
      } else {
        ++persistent_count;
        if (!should_trace(visitor, node))
          continue;
        node->TracePersistentNode(visitor);
        debug_marked_object_size = ProcessHeap::TotalMarkedObjectSize();
      }
    }
    if (free_count == PersistentNodeSlots::kSlotCount) {
      PersistentNodeSlots* dead_slots = slots;
      *prev_next = slots->next_;
      slots = slots->next_;
      delete dead_slots;
    } else {
      if (free_list_last) {
        DCHECK(free_list_next);
        DCHECK(!free_list_last->FreeListNext());
        free_list_last->SetFreeListNext(free_list_head_);
        free_list_head_ = free_list_next;
      }
      prev_next = &slots->next_;
      slots = slots->next_;
    }
  }
#if DCHECK_IS_ON()
  DCHECK_EQ(persistent_count, persistent_count_);
#endif
}

void PersistentRegion::PrepareForThreadStateTermination() {
  DCHECK(!IsMainThread());
  PersistentNodeSlots* slots = slots_;
  while (slots) {
    for (int i = 0; i < PersistentNodeSlots::kSlotCount; ++i) {
      PersistentNode* node = &slots->slot_[i];
      if (node->IsUnused())
        continue;
      // It is safe to cast to Persistent<DummyGCBase> because persistent heap
      // collections are banned in non-main threads.
      Persistent<DummyGCBase>* persistent =
          reinterpret_cast<Persistent<DummyGCBase>*>(node->Self());
      DCHECK(persistent);
      persistent->Clear();
      DCHECK(node->IsUnused());
    }
    slots = slots->next_;
  }
#if DCHECK_IS_ON()
  DCHECK_EQ(persistent_count_, 0);
#endif
}

bool CrossThreadPersistentRegion::ShouldTracePersistentNode(
    Visitor* visitor,
    PersistentNode* node) {
  CrossThreadPersistent<DummyGCBase>* persistent =
      reinterpret_cast<CrossThreadPersistent<DummyGCBase>*>(node->Self());
  DCHECK(persistent);
  DCHECK(!persistent->IsHashTableDeletedValue());
  Address raw_object = reinterpret_cast<Address>(persistent->Get());
  if (!raw_object)
    return false;
  return &visitor->Heap() == &ThreadState::FromObject(raw_object)->Heap();
}

void CrossThreadPersistentRegion::PrepareForThreadStateTermination(
    ThreadState* thread_state) {
  // For heaps belonging to a thread that's detaching, any cross-thread
  // persistents pointing into them needs to be disabled. Do that by clearing
  // out the underlying heap reference.
  RecursiveMutexLocker lock(ProcessHeap::CrossThreadPersistentMutex());

  PersistentNodeSlots* slots = persistent_region_->slots_;
  while (slots) {
    for (int i = 0; i < PersistentNodeSlots::kSlotCount; ++i) {
      if (slots->slot_[i].IsUnused())
        continue;

      // 'self' is in use, containing the cross-thread persistent wrapper
      // object.
      CrossThreadPersistent<DummyGCBase>* persistent =
          reinterpret_cast<CrossThreadPersistent<DummyGCBase>*>(
              slots->slot_[i].Self());
      DCHECK(persistent);
      void* raw_object = persistent->AtomicGet();
      if (!raw_object)
        continue;
      BasePage* page = PageFromObject(raw_object);
      DCHECK(page);
      if (page->Arena()->GetThreadState() == thread_state) {
        persistent->Clear();
        DCHECK(slots->slot_[i].IsUnused());
      }
    }
    slots = slots->next_;
  }
}

#if defined(ADDRESS_SANITIZER)
void CrossThreadPersistentRegion::UnpoisonCrossThreadPersistents() {
  RecursiveMutexLocker lock(ProcessHeap::CrossThreadPersistentMutex());
  int persistent_count = 0;
  for (PersistentNodeSlots* slots = persistent_region_->slots_; slots;
       slots = slots->next_) {
    for (int i = 0; i < PersistentNodeSlots::kSlotCount; ++i) {
      const PersistentNode& node = slots->slot_[i];
      if (!node.IsUnused()) {
        ASAN_UNPOISON_MEMORY_REGION(node.Self(),
                                    sizeof(CrossThreadPersistent<void*>));
        ++persistent_count;
      }
    }
  }
#if DCHECK_IS_ON()
  DCHECK_EQ(persistent_count, persistent_region_->persistent_count_);
#endif
}
#endif

}  // namespace blink
