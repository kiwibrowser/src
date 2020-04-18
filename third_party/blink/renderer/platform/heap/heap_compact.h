// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_COMPACT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_COMPACT_H_

#include <memory>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/platform/heap/blink_gc.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/threading_primitives.h"

#include <bitset>
#include <utility>

// Compaction-specific debug switches:

// Set to 0 to prevent compaction GCs, disabling the heap compaction feature.
#define ENABLE_HEAP_COMPACTION 1

// Emit debug info during compaction.
#define DEBUG_HEAP_COMPACTION 0

// Emit stats on freelist occupancy.
// 0 - disabled, 1 - minimal, 2 - verbose.
#define DEBUG_HEAP_FREELIST 0

// Log the amount of time spent compacting.
#define DEBUG_LOG_HEAP_COMPACTION_RUNNING_TIME 0

// Compact during all idle + precise GCs; for debugging.
#define STRESS_TEST_HEAP_COMPACTION 0

namespace blink {

class NormalPageArena;
class BasePage;
class ThreadState;
class ThreadHeap;

class PLATFORM_EXPORT HeapCompact final {
 public:
  static std::unique_ptr<HeapCompact> Create() {
    return base::WrapUnique(new HeapCompact);
  }

  ~HeapCompact();

  // Determine if a GC for the given type and reason should also perform
  // additional heap compaction.
  //
  bool ShouldCompact(ThreadHeap*,
                     BlinkGC::StackState,
                     BlinkGC::MarkingType,
                     BlinkGC::GCReason);

  // Compaction should be performed as part of the ongoing GC, initialize
  // the heap compaction pass. Returns the appropriate visitor type to
  // use when running the marking phase.
  void Initialize(ThreadState*);

  // Returns true if the ongoing GC will perform compaction.
  bool IsCompacting() const { return do_compact_; }

  // Returns true if the ongoing GC will perform compaction for the given
  // heap arena.
  bool IsCompactingArena(int arena_index) const {
    return do_compact_ && (compactable_arenas_ & (0x1u << arena_index));
  }

  // Returns |true| if the ongoing GC may compact the given arena/sub-heap.
  static bool IsCompactableArena(int arena_index) {
    return arena_index >= BlinkGC::kVector1ArenaIndex &&
           arena_index <= BlinkGC::kHashTableArenaIndex;
  }

  // See |Heap::registerMovingObjectReference()| documentation.
  void RegisterMovingObjectReference(MovableReference* slot);

  // See |Heap::registerMovingObjectCallback()| documentation.
  void RegisterMovingObjectCallback(MovableReference,
                                    MovingObjectCallback,
                                    void* callback_data);

  // Thread signalling that a compaction pass is starting or has
  // completed.
  //
  // A thread participating in a heap GC will wait on the completion
  // of compaction across all threads. No thread can be allowed to
  // potentially access another thread's heap arenas while they're
  // still being compacted.
  void StartThreadCompaction();
  void FinishThreadCompaction();

  // Perform any relocation post-processing after having completed compacting
  // the given arena. The number of pages that were freed together with the
  // total size (in bytes) of freed heap storage, are passed in as arguments.
  void FinishedArenaCompaction(NormalPageArena*,
                               size_t freed_pages,
                               size_t freed_size);

  // Register the heap page as containing live objects that will all be
  // compacted. Registration happens as part of making the arenas ready
  // for a GC.
  void AddCompactingPage(BasePage*);

  // Notify heap compaction that object at |from| has been relocated to.. |to|.
  // (Called by the sweep compaction pass.)
  void Relocate(Address from, Address to);

  // For unit testing only: arrange for a compaction GC to be triggered
  // next time a non-conservative GC is run. Sets the compact-next flag
  // to the new value, returning old.
  static bool ScheduleCompactionGCForTesting(bool);

  // Test-only: verify that one or more of the vector arenas are
  // in the process of being compacted.
  bool IsCompactingVectorArenas() {
    for (int i = BlinkGC::kVector1ArenaIndex; i <= BlinkGC::kVector4ArenaIndex;
         ++i) {
      if (IsCompactingArena(i))
        return true;
    }
    return false;
  }

 private:
  class MovableObjectFixups;

  HeapCompact();

  // Sample the amount of fragmentation and heap memory currently residing
  // on the freelists of the arenas we're able to compact. The computed
  // numbers will be subsequently used to determine if a heap compaction
  // is on order (shouldCompact().)
  void UpdateHeapResidency(ThreadHeap*);

  // Parameters controlling when compaction should be done:

  // Number of GCs that must have passed since last compaction GC.
  static const int kGCCountSinceLastCompactionThreshold = 10;

  // Freelist size threshold that must be exceeded before compaction
  // should be considered.
  static const size_t kFreeListSizeThreshold = 512 * 1024;

  MovableObjectFixups& Fixups();

  std::unique_ptr<MovableObjectFixups> fixups_;

  // Set to |true| when a compacting sweep will go ahead.
  bool do_compact_;
  size_t gc_count_since_last_compaction_;

  // Last reported freelist size, across all compactable arenas.
  size_t free_list_size_;

  // If compacting, i'th heap arena will be compacted
  // if corresponding bit is set. Indexes are in
  // the range of BlinkGC::ArenaIndices.
  unsigned compactable_arenas_;

  // Stats, number of (complete) pages freed/decommitted +
  // bytes freed (which will include partial pages.)
  size_t freed_pages_;
  size_t freed_size_;

  double start_compaction_time_ms_;

  static bool force_compaction_gc_;
};

}  // namespace blink

// Logging macros activated by debug switches.

#define LOG_HEAP_COMPACTION_INTERNAL() DLOG(INFO)

#if DEBUG_HEAP_COMPACTION
#define LOG_HEAP_COMPACTION() LOG_HEAP_COMPACTION_INTERNAL()
#else
#define LOG_HEAP_COMPACTION() EAT_STREAM_PARAMETERS
#endif

#if DEBUG_HEAP_FREELIST
#define LOG_HEAP_FREELIST() LOG_HEAP_COMPACTION_INTERNAL()
#else
#define LOG_HEAP_FREELIST() EAT_STREAM_PARAMETERS
#endif

#if DEBUG_HEAP_FREELIST == 2
#define LOG_HEAP_FREELIST_VERBOSE() LOG_HEAP_COMPACTION_INTERNAL()
#else
#define LOG_HEAP_FREELIST_VERBOSE() EAT_STREAM_PARAMETERS
#endif

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_HEAP_COMPACT_H_
