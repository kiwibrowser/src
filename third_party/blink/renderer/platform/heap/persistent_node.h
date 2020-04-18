// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_PERSISTENT_NODE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_PERSISTENT_NODE_H_

#include <memory>
#include "third_party/blink/renderer/platform/heap/process_heap.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/threading_primitives.h"

namespace blink {

class CrossThreadPersistentRegion;

class PersistentNode final {
  DISALLOW_NEW();

 public:
  PersistentNode() : self_(nullptr), trace_(nullptr) { DCHECK(IsUnused()); }

#if DCHECK_IS_ON()
  ~PersistentNode() {
    // If you hit this assert, it means that the thread finished
    // without clearing persistent handles that the thread created.
    // We don't enable the assert for the main thread because the
    // main thread finishes without clearing all persistent handles.
    DCHECK(IsMainThread() || IsUnused());
  }
#endif

  // It is dangerous to copy the PersistentNode because it breaks the
  // free list.
  PersistentNode& operator=(const PersistentNode& otherref) = delete;

  // Ideally the trace method should be virtual and automatically dispatch
  // to the most specific implementation. However having a virtual method
  // on PersistentNode leads to too eager template instantiation with MSVC
  // which leads to include cycles.
  // Instead we call the constructor with a TraceCallback which knows the
  // type of the most specific child and calls trace directly. See
  // TraceMethodDelegate in Visitor.h for how this is done.
  void TracePersistentNode(Visitor* visitor) {
    DCHECK(!IsUnused());
    DCHECK(trace_);
    trace_(visitor, self_);
  }

  void Initialize(void* self, TraceCallback trace) {
    DCHECK(IsUnused());
    self_ = self;
    trace_ = trace;
  }

  void SetFreeListNext(PersistentNode* node) {
    DCHECK(!node || node->IsUnused());
    self_ = node;
    trace_ = nullptr;
    DCHECK(IsUnused());
  }

  PersistentNode* FreeListNext() {
    DCHECK(IsUnused());
    PersistentNode* node = reinterpret_cast<PersistentNode*>(self_);
    DCHECK(!node || node->IsUnused());
    return node;
  }

  bool IsUnused() const { return !trace_; }

  void* Self() const { return self_; }

 private:
  // If this PersistentNode is in use:
  //   - m_self points to the corresponding Persistent handle.
  //   - m_trace points to the trace method.
  // If this PersistentNode is freed:
  //   - m_self points to the next freed PersistentNode.
  //   - m_trace is nullptr.
  void* self_;
  TraceCallback trace_;
};

struct PersistentNodeSlots final {
  USING_FAST_MALLOC(PersistentNodeSlots);

 private:
  static const int kSlotCount = 256;
  PersistentNodeSlots* next_;
  PersistentNode slot_[kSlotCount];
  friend class PersistentRegion;
  friend class CrossThreadPersistentRegion;
};

// PersistentRegion provides a region of PersistentNodes. PersistentRegion
// holds a linked list of PersistentNodeSlots, each of which stores
// a predefined number of PersistentNodes. You can call allocatePersistentNode/
// freePersistentNode to allocate/free a PersistentNode on the region.
class PLATFORM_EXPORT PersistentRegion final {
  USING_FAST_MALLOC(PersistentRegion);

 public:
  PersistentRegion()
      : free_list_head_(nullptr),
        slots_(nullptr)
#if DCHECK_IS_ON()
        ,
        persistent_count_(0)
#endif
  {
  }
  ~PersistentRegion();

  PersistentNode* AllocatePersistentNode(void* self, TraceCallback trace) {
#if DCHECK_IS_ON()
    ++persistent_count_;
#endif
    if (UNLIKELY(!free_list_head_))
      EnsurePersistentNodeSlots(self, trace);
    DCHECK(free_list_head_);
    PersistentNode* node = free_list_head_;
    free_list_head_ = free_list_head_->FreeListNext();
    node->Initialize(self, trace);
    DCHECK(!node->IsUnused());
    return node;
  }

  void FreePersistentNode(PersistentNode* persistent_node) {
#if DCHECK_IS_ON()
    DCHECK_GT(persistent_count_, 0);
#endif
    persistent_node->SetFreeListNext(free_list_head_);
    free_list_head_ = persistent_node;
#if DCHECK_IS_ON()
    --persistent_count_;
#endif
  }

  static bool ShouldTracePersistentNode(Visitor*, PersistentNode*) {
    return true;
  }

  void ReleasePersistentNode(PersistentNode*,
                             ThreadState::PersistentClearCallback);
  using ShouldTraceCallback = bool (*)(Visitor*, PersistentNode*);
  void TracePersistentNodes(
      Visitor*,
      ShouldTraceCallback = PersistentRegion::ShouldTracePersistentNode);
  int NumberOfPersistents();
  void PrepareForThreadStateTermination();

 private:
  friend CrossThreadPersistentRegion;

  void EnsurePersistentNodeSlots(void*, TraceCallback);

  PersistentNode* free_list_head_;
  PersistentNodeSlots* slots_;
#if DCHECK_IS_ON()
  int persistent_count_;
#endif
};

class CrossThreadPersistentRegion final {
  USING_FAST_MALLOC(CrossThreadPersistentRegion);

 public:
  CrossThreadPersistentRegion()
      : persistent_region_(std::make_unique<PersistentRegion>()) {}

  void AllocatePersistentNode(PersistentNode*& persistent_node,
                              void* self,
                              TraceCallback trace) {
    RecursiveMutexLocker lock(ProcessHeap::CrossThreadPersistentMutex());
    PersistentNode* node =
        persistent_region_->AllocatePersistentNode(self, trace);
    ReleaseStore(reinterpret_cast<void* volatile*>(&persistent_node), node);
  }

  void FreePersistentNode(PersistentNode*& persistent_node) {
    RecursiveMutexLocker lock(ProcessHeap::CrossThreadPersistentMutex());
    // When the thread that holds the heap object that the cross-thread
    // persistent shuts down, prepareForThreadStateTermination() will clear out
    // the associated CrossThreadPersistent<> and PersistentNode so as to avoid
    // unsafe access. This can overlap with a holder of the
    // CrossThreadPersistent<> also clearing the persistent and freeing the
    // PersistentNode.
    //
    // The lock ensures the updating is ordered, but by the time lock has been
    // acquired the PersistentNode reference may have been cleared out already;
    // check for this.
    if (!persistent_node)
      return;
    persistent_region_->FreePersistentNode(persistent_node);
    ReleaseStore(reinterpret_cast<void* volatile*>(&persistent_node), nullptr);
  }

  void TracePersistentNodes(Visitor* visitor) {
// If this assert triggers, you're tracing without being in a LockScope.
#if DCHECK_IS_ON()
    DCHECK(ProcessHeap::CrossThreadPersistentMutex().Locked());
#endif
    persistent_region_->TracePersistentNodes(
        visitor, CrossThreadPersistentRegion::ShouldTracePersistentNode);
  }

  void PrepareForThreadStateTermination(ThreadState*);

  NO_SANITIZE_ADDRESS
  static bool ShouldTracePersistentNode(Visitor*, PersistentNode*);

#if defined(ADDRESS_SANITIZER)
  void UnpoisonCrossThreadPersistents();
#endif

 private:

  // We don't make CrossThreadPersistentRegion inherit from PersistentRegion
  // because we don't want to virtualize performance-sensitive methods
  // such as PersistentRegion::allocate/freePersistentNode.
  std::unique_ptr<PersistentRegion> persistent_region_;

  // Recursive as prepareForThreadStateTermination() clears a PersistentNode's
  // associated Persistent<> -- it in turn freeing the PersistentNode. And both
  // CrossThreadPersistentRegion operations need a lock on the region before
  // mutating.
  RecursiveMutex mutex_;
};

}  // namespace blink

#endif
