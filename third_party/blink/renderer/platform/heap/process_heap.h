// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_PROCESS_HEAP_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_PROCESS_HEAP_H_

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/atomics.h"
#include "third_party/blink/renderer/platform/wtf/threading_primitives.h"

namespace blink {

class CrossThreadPersistentRegion;

class PLATFORM_EXPORT ProcessHeap {
  STATIC_ONLY(ProcessHeap);

 public:
  static void Init();

  static CrossThreadPersistentRegion& GetCrossThreadPersistentRegion();
  static CrossThreadPersistentRegion& GetCrossThreadWeakPersistentRegion();

  // Access to the CrossThreadPersistentRegion from multiple threads has to be
  // prevented as allocation, freeing, and iteration of nodes may otherwise
  // cause data races.
  //
  // Examples include:
  // - Iteration of strong cross-thread Persistents.
  // - Iteration and processing of weak cross-thread Persistents. The lock
  //   needs to span both operations as iteration of weak persistents only
  //   registers memory regions that are then processed afterwards.
  //
  // Recursive as PrepareForThreadStateTermination() clears a PersistentNode's
  // associated Persistent<> -- it in turn freeing the PersistentNode. And both
  // CrossThreadPersistentRegion operations need a lock on the region before
  // mutating.
  static RecursiveMutex& CrossThreadPersistentMutex();

  static void IncreaseTotalAllocatedObjectSize(size_t delta) {
    AtomicAdd(&total_allocated_object_size_, static_cast<long>(delta));
  }
  static void DecreaseTotalAllocatedObjectSize(size_t delta) {
    AtomicSubtract(&total_allocated_object_size_, static_cast<long>(delta));
  }
  static size_t TotalAllocatedObjectSize() {
    return AcquireLoad(&total_allocated_object_size_);
  }
  static void IncreaseTotalMarkedObjectSize(size_t delta) {
    AtomicAdd(&total_marked_object_size_, static_cast<long>(delta));
  }
  static void DecreaseTotalMarkedObjectSize(size_t delta) {
    AtomicSubtract(&total_marked_object_size_, static_cast<long>(delta));
  }
  static size_t TotalMarkedObjectSize() {
    return AcquireLoad(&total_marked_object_size_);
  }
  static void IncreaseTotalAllocatedSpace(size_t delta) {
    AtomicAdd(&total_allocated_space_, static_cast<long>(delta));
  }
  static void DecreaseTotalAllocatedSpace(size_t delta) {
    AtomicSubtract(&total_allocated_space_, static_cast<long>(delta));
  }
  static size_t TotalAllocatedSpace() {
    return AcquireLoad(&total_allocated_space_);
  }
  static void ResetHeapCounters();

 private:
  static size_t total_allocated_space_;
  static size_t total_allocated_object_size_;
  static size_t total_marked_object_size_;

  friend class ThreadState;
};

}  // namespace blink

#endif
