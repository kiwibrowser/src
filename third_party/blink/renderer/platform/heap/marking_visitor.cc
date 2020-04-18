// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/heap/marking_visitor.h"

#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"

namespace blink {

namespace {

ALWAYS_INLINE bool IsHashTableDeleteValue(const void* value) {
  return value == reinterpret_cast<void*>(-1);
}

}  // namespace

std::unique_ptr<MarkingVisitor> MarkingVisitor::Create(ThreadState* state,
                                                       MarkingMode mode) {
  return std::make_unique<MarkingVisitor>(state, mode);
}

MarkingVisitor::MarkingVisitor(ThreadState* state, MarkingMode marking_mode)
    : Visitor(state),
      marking_worklist_(Heap().GetMarkingWorklist(),
                        WorklistTaskId::MainThread),
      not_fully_constructed_worklist_(Heap().GetNotFullyConstructedWorklist(),
                                      WorklistTaskId::MainThread),
      weak_callback_worklist_(Heap().GetWeakCallbackWorklist(),
                              WorklistTaskId::MainThread),
      marking_mode_(marking_mode) {
  DCHECK(state->InAtomicMarkingPause());
#if DCHECK_IS_ON()
  DCHECK(state->CheckThread());
#endif  // DCHECK_IS_ON
}

MarkingVisitor::~MarkingVisitor() = default;

void MarkingVisitor::ConservativelyMarkAddress(BasePage* page,
                                               Address address) {
#if DCHECK_IS_ON()
  DCHECK(page->Contains(address));
#endif
  HeapObjectHeader* const header =
      page->IsLargeObjectPage()
          ? static_cast<LargeObjectPage*>(page)->GetHeapObjectHeader()
          : static_cast<NormalPage*>(page)->FindHeaderFromAddress(address);
  if (!header)
    return;
  ConservativelyMarkHeader(header);
}

#if DCHECK_IS_ON()
void MarkingVisitor::ConservativelyMarkAddress(
    BasePage* page,
    Address address,
    MarkedPointerCallbackForTesting callback) {
  DCHECK(page->Contains(address));
  HeapObjectHeader* const header =
      page->IsLargeObjectPage()
          ? static_cast<LargeObjectPage*>(page)->GetHeapObjectHeader()
          : static_cast<NormalPage*>(page)->FindHeaderFromAddress(address);
  if (!header)
    return;
  if (!callback(header))
    ConservativelyMarkHeader(header);
}
#endif  // DCHECK_IS_ON

namespace {

#if DCHECK_IS_ON()
bool IsUninitializedMemory(void* object_pointer, size_t object_size) {
  // Scan through the object's fields and check that they are all zero.
  Address* object_fields = reinterpret_cast<Address*>(object_pointer);
  for (size_t i = 0; i < object_size / sizeof(Address); ++i) {
    if (object_fields[i])
      return false;
  }
  return true;
}
#endif

}  // namespace

void MarkingVisitor::ConservativelyMarkHeader(HeapObjectHeader* header) {
  const GCInfo* gc_info =
      GCInfoTable::Get().GCInfoFromIndex(header->GcInfoIndex());
  if (gc_info->HasVTable() && !VTableInitialized(header->Payload())) {
    // We hit this branch when a GC strikes before GarbageCollected<>'s
    // constructor runs.
    //
    // class A : public GarbageCollected<A> { virtual void f() = 0; };
    // class B : public A {
    //   B() : A(foo()) { };
    // };
    //
    // If foo() allocates something and triggers a GC, the vtable of A
    // has not yet been initialized. In this case, we should mark the A
    // object without tracing any member of the A object.
    MarkHeaderNoTracing(header);
#if DCHECK_IS_ON()
    DCHECK(IsUninitializedMemory(header->Payload(), header->PayloadSize()));
#endif
  } else {
    MarkHeader(header, gc_info->trace_);
  }
}

void MarkingVisitor::RegisterWeakCallback(void* object, WeakCallback callback) {
  // We don't want to run weak processings when taking a snapshot.
  if (marking_mode_ == kSnapshotMarking)
    return;
  weak_callback_worklist_.Push({object, callback});
}

void MarkingVisitor::RegisterBackingStoreReference(void* slot) {
  if (marking_mode_ != kGlobalMarkingWithCompaction)
    return;
  Heap().RegisterMovingObjectReference(
      reinterpret_cast<MovableReference*>(slot));
}

void MarkingVisitor::RegisterBackingStoreCallback(void* backing_store,
                                                  MovingObjectCallback callback,
                                                  void* callback_data) {
  if (marking_mode_ != kGlobalMarkingWithCompaction)
    return;
  Heap().RegisterMovingObjectCallback(
      reinterpret_cast<MovableReference>(backing_store), callback,
      callback_data);
}

bool MarkingVisitor::RegisterWeakTable(const void* closure,
                                       EphemeronCallback iteration_callback) {
  Heap().RegisterWeakTable(const_cast<void*>(closure), iteration_callback);
  return true;
}

void MarkingVisitor::WriteBarrierSlow(void* value) {
  if (!value || IsHashTableDeleteValue(value))
    return;

  ThreadState* const thread_state = ThreadState::Current();
  if (!thread_state->IsIncrementalMarking())
    return;

  thread_state->Heap().WriteBarrier(value);
}

void MarkingVisitor::TraceMarkedBackingStoreSlow(void* value) {
  if (!value)
    return;

  ThreadState* const thread_state = ThreadState::Current();
  if (!thread_state->IsIncrementalMarking())
    return;

  // |value| is pointing to the start of a backing store.
  HeapObjectHeader* header = HeapObjectHeader::FromPayload(value);
  CHECK(header->IsMarked());
  DCHECK(thread_state->CurrentVisitor());
  // This check ensures that the visitor will not eagerly recurse into children
  // but rather push all blink::GarbageCollected objects and only eagerly trace
  // non-managed objects.
  DCHECK(!thread_state->Heap().GetStackFrameDepth().IsEnabled());
  // No weak handling for write barriers. Modifying weakly reachable objects
  // strongifies them for the current cycle.
  GCInfoTable::Get()
      .GCInfoFromIndex(header->GcInfoIndex())
      ->trace_(thread_state->CurrentVisitor(), value);
}

}  // namespace blink
