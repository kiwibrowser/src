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

#include "third_party/blink/renderer/platform/heap/heap.h"

#include <algorithm>
#include <limits>
#include <memory>

#include "base/trace_event/process_memory_dump.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/heap/address_cache.h"
#include "third_party/blink/renderer/platform/heap/blink_gc_memory_dump_provider.h"
#include "third_party/blink/renderer/platform/heap/heap_compact.h"
#include "third_party/blink/renderer/platform/heap/heap_stats_collector.h"
#include "third_party/blink/renderer/platform/heap/marking_visitor.h"
#include "third_party/blink/renderer/platform/heap/page_memory.h"
#include "third_party/blink/renderer/platform/heap/page_pool.h"
#include "third_party/blink/renderer/platform/heap/safe_point.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/web_memory_allocator_dump.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/web_process_memory_dump.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/leak_annotations.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

HeapAllocHooks::AllocationHook* HeapAllocHooks::allocation_hook_ = nullptr;
HeapAllocHooks::FreeHook* HeapAllocHooks::free_hook_ = nullptr;

ThreadHeapStats::ThreadHeapStats()
    : allocated_space_(0),
      allocated_object_size_(0),
      object_size_at_last_gc_(0),
      marked_object_size_(0),
      marked_object_size_at_last_complete_sweep_(0),
      wrapper_count_(0),
      wrapper_count_at_last_gc_(0),
      collected_wrapper_count_(0),
      partition_alloc_size_at_last_gc_(
          WTF::Partitions::TotalSizeOfCommittedPages()),
      estimated_marking_time_per_byte_(0.0) {}

double ThreadHeapStats::EstimatedMarkingTime() {
  // Use 8 ms as initial estimated marking time.
  // 8 ms is long enough for low-end mobile devices to mark common
  // real-world object graphs.
  if (estimated_marking_time_per_byte_ == 0)
    return 0.008;

  // Assuming that the collection rate of this GC will be mostly equal to
  // the collection rate of the last GC, estimate the marking time of this GC.
  return estimated_marking_time_per_byte_ *
         (AllocatedObjectSize() + MarkedObjectSize());
}

void ThreadHeapStats::Reset() {
  object_size_at_last_gc_ = allocated_object_size_ + marked_object_size_;
  partition_alloc_size_at_last_gc_ =
      WTF::Partitions::TotalSizeOfCommittedPages();
  allocated_object_size_ = 0;
  marked_object_size_ = 0;
  wrapper_count_at_last_gc_ = wrapper_count_;
  collected_wrapper_count_ = 0;
}

void ThreadHeapStats::IncreaseAllocatedObjectSize(size_t delta) {
  allocated_object_size_ += delta;
  ProcessHeap::IncreaseTotalAllocatedObjectSize(delta);
}

void ThreadHeapStats::DecreaseAllocatedObjectSize(size_t delta) {
  allocated_object_size_ -= delta;
  ProcessHeap::DecreaseTotalAllocatedObjectSize(delta);
}

void ThreadHeapStats::IncreaseMarkedObjectSize(size_t delta) {
  marked_object_size_ += delta;
  ProcessHeap::IncreaseTotalMarkedObjectSize(delta);
}

void ThreadHeapStats::IncreaseAllocatedSpace(size_t delta) {
  allocated_space_ += delta;
  ProcessHeap::IncreaseTotalAllocatedSpace(delta);
}

void ThreadHeapStats::DecreaseAllocatedSpace(size_t delta) {
  allocated_space_ -= delta;
  ProcessHeap::DecreaseTotalAllocatedSpace(delta);
}

double ThreadHeapStats::LiveObjectRateSinceLastGC() const {
  if (ObjectSizeAtLastGC() > 0)
    return static_cast<double>(MarkedObjectSize()) / ObjectSizeAtLastGC();
  return 0.0;
}

ThreadHeap::ThreadHeap(ThreadState* thread_state)
    : thread_state_(thread_state),
      heap_stats_collector_(std::make_unique<ThreadHeapStatsCollector>()),
      region_tree_(std::make_unique<RegionTree>()),
      address_cache_(std::make_unique<AddressCache>()),
      free_page_pool_(std::make_unique<PagePool>()),
      marking_worklist_(nullptr),
      not_fully_constructed_worklist_(nullptr),
      weak_callback_worklist_(nullptr),
      vector_backing_arena_index_(BlinkGC::kVector1ArenaIndex),
      current_arena_ages_(0) {
  if (ThreadState::Current()->IsMainThread())
    main_thread_heap_ = this;

  for (int arena_index = 0; arena_index < BlinkGC::kLargeObjectArenaIndex;
       arena_index++)
    arenas_[arena_index] = new NormalPageArena(thread_state_, arena_index);
  arenas_[BlinkGC::kLargeObjectArenaIndex] =
      new LargeObjectArena(thread_state_, BlinkGC::kLargeObjectArenaIndex);

  likely_to_be_promptly_freed_ =
      std::make_unique<int[]>(kLikelyToBePromptlyFreedArraySize);
  ClearArenaAges();
}

ThreadHeap::~ThreadHeap() {
  for (int i = 0; i < BlinkGC::kNumberOfArenas; ++i)
    delete arenas_[i];
}

Address ThreadHeap::CheckAndMarkPointer(MarkingVisitor* visitor,
                                        Address address) {
  DCHECK(thread_state_->InAtomicMarkingPause());

#if !DCHECK_IS_ON()
  if (address_cache_->Lookup(address))
    return nullptr;
#endif

  if (BasePage* page = LookupPageForAddress(address)) {
#if DCHECK_IS_ON()
    DCHECK(page->Contains(address));
#endif
    DCHECK(!address_cache_->Lookup(address));
    DCHECK(&visitor->Heap() == &page->Arena()->GetThreadState()->Heap());
    visitor->ConservativelyMarkAddress(page, address);
    return address;
  }

#if !DCHECK_IS_ON()
  address_cache_->AddEntry(address);
#else
  if (!address_cache_->Lookup(address))
    address_cache_->AddEntry(address);
#endif
  return nullptr;
}

#if DCHECK_IS_ON()
// To support unit testing of the marking of off-heap root references
// into the heap, provide a checkAndMarkPointer() version with an
// extra notification argument.
Address ThreadHeap::CheckAndMarkPointer(
    MarkingVisitor* visitor,
    Address address,
    MarkedPointerCallbackForTesting callback) {
  DCHECK(thread_state_->InAtomicMarkingPause());

  if (BasePage* page = LookupPageForAddress(address)) {
    DCHECK(page->Contains(address));
    DCHECK(!address_cache_->Lookup(address));
    DCHECK(&visitor->Heap() == &page->Arena()->GetThreadState()->Heap());
    visitor->ConservativelyMarkAddress(page, address, callback);
    return address;
  }
  if (!address_cache_->Lookup(address))
    address_cache_->AddEntry(address);
  return nullptr;
}
#endif  // DCHECK_IS_ON()

void ThreadHeap::RegisterWeakTable(void* table,
                                   EphemeronCallback iteration_callback) {
  DCHECK(thread_state_->InAtomicMarkingPause());
#if DCHECK_IS_ON()
  auto result = ephemeron_callbacks_.insert(table, iteration_callback);
  DCHECK(result.is_new_entry ||
         result.stored_value->value == iteration_callback);
#else
  ephemeron_callbacks_.insert(table, iteration_callback);
#endif  // DCHECK_IS_ON()
}

void ThreadHeap::CommitCallbackStacks() {
  marking_worklist_.reset(new MarkingWorklist());
  not_fully_constructed_worklist_.reset(new NotFullyConstructedWorklist());
  weak_callback_worklist_.reset(new WeakCallbackWorklist());
  DCHECK(ephemeron_callbacks_.IsEmpty());
}

void ThreadHeap::DecommitCallbackStacks() {
  marking_worklist_.reset(nullptr);
  not_fully_constructed_worklist_.reset(nullptr);
  weak_callback_worklist_.reset(nullptr);
  ephemeron_callbacks_.clear();
}

HeapCompact* ThreadHeap::Compaction() {
  if (!compaction_)
    compaction_ = HeapCompact::Create();
  return compaction_.get();
}

void ThreadHeap::RegisterMovingObjectReference(MovableReference* slot) {
  DCHECK(slot);
  DCHECK(*slot);
  Compaction()->RegisterMovingObjectReference(slot);
}

void ThreadHeap::RegisterMovingObjectCallback(MovableReference reference,
                                              MovingObjectCallback callback,
                                              void* callback_data) {
  DCHECK(reference);
  Compaction()->RegisterMovingObjectCallback(reference, callback,
                                             callback_data);
}

void ThreadHeap::ProcessMarkingStack(Visitor* visitor) {
  bool complete = AdvanceMarkingStackProcessing(
      visitor, std::numeric_limits<double>::infinity());
  CHECK(complete);
}

void ThreadHeap::MarkNotFullyConstructedObjects(Visitor* visitor) {
  TRACE_EVENT0("blink_gc", "ThreadHeap::MarkNotFullyConstructedObjects");
  DCHECK(!thread_state_->IsIncrementalMarking());

  NotFullyConstructedItem item;
  while (
      not_fully_constructed_worklist_->Pop(WorklistTaskId::MainThread, &item)) {
    BasePage* const page = PageFromObject(item);
    reinterpret_cast<MarkingVisitor*>(visitor)->ConservativelyMarkAddress(
        page, reinterpret_cast<Address>(item));
  }
}

void ThreadHeap::InvokeEphemeronCallbacks(Visitor* visitor) {
  // Mark any strong pointers that have now become reachable in ephemeron maps.
  TRACE_EVENT0("blink_gc", "ThreadHeap::InvokeEphemeronCallbacks");

  // Avoid supporting a subtle scheme that allows insertion while iterating
  // by just creating temporary lists for iteration and sinking.
  WTF::HashMap<void*, EphemeronCallback> iteration_set;
  WTF::HashMap<void*, EphemeronCallback> final_set;

  bool found_new = false;
  do {
    iteration_set = std::move(ephemeron_callbacks_);
    ephemeron_callbacks_.clear();
    for (auto& tuple : iteration_set) {
      final_set.insert(tuple.key, tuple.value);
      tuple.value(visitor, tuple.key);
    }
    found_new = !ephemeron_callbacks_.IsEmpty();
  } while (found_new);
  ephemeron_callbacks_ = std::move(final_set);
}

bool ThreadHeap::AdvanceMarkingStackProcessing(Visitor* visitor,
                                               double deadline_seconds) {
  const size_t kDeadlineCheckInterval = 2500;
  size_t processed_callback_count = 0;
  // Ephemeron fixed point loop.
  do {
    {
      // Iteratively mark all objects that are reachable from the objects
      // currently pushed onto the marking stack.
      TRACE_EVENT0("blink_gc", "ThreadHeap::processMarkingStackSingleThreaded");
      MarkingItem item;
      while (marking_worklist_->Pop(WorklistTaskId::MainThread, &item)) {
        item.callback(visitor, item.object);
        processed_callback_count++;
        if (processed_callback_count % kDeadlineCheckInterval == 0) {
          if (deadline_seconds <= CurrentTimeTicksInSeconds()) {
            return false;
          }
        }
      }
    }

    InvokeEphemeronCallbacks(visitor);

    // Rerun loop if ephemeron processing queued more objects for tracing.
  } while (!marking_worklist_->IsGlobalEmpty());
  return true;
}

void ThreadHeap::WeakProcessing(Visitor* visitor) {
  TRACE_EVENT0("blink_gc", "ThreadHeap::WeakProcessing");
  double start_time = WTF::CurrentTimeTicksInMilliseconds();

  // Weak processing may access unmarked objects but are forbidden from
  // ressurecting them.
  ThreadState::ObjectResurrectionForbiddenScope object_resurrection_forbidden(
      ThreadState::Current());

  // Call weak callbacks on objects that may now be pointing to dead objects.
  CustomCallbackItem item;
  while (weak_callback_worklist_->Pop(WorklistTaskId::MainThread, &item)) {
    item.callback(visitor, item.object);
  }
  // Weak callbacks should not add any new objects for marking.
  DCHECK(marking_worklist_->IsGlobalEmpty());

  double time_for_weak_processing =
      WTF::CurrentTimeTicksInMilliseconds() - start_time;
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      CustomCountHistogram, weak_processing_time_histogram,
      ("BlinkGC.TimeForGlobalWeakProcessing", 1, 10 * 1000, 50));
  weak_processing_time_histogram.Count(time_for_weak_processing);
}

void ThreadHeap::VerifyMarking() {
  for (int i = 0; i < BlinkGC::kNumberOfArenas; ++i) {
    arenas_[i]->VerifyMarking();
  }
}

void ThreadHeap::ReportMemoryUsageHistogram() {
  static size_t supported_max_size_in_mb = 4 * 1024;
  static size_t observed_max_size_in_mb = 0;

  // We only report the memory in the main thread.
  if (!IsMainThread())
    return;
  // +1 is for rounding up the sizeInMB.
  size_t size_in_mb =
      ThreadState::Current()->Heap().HeapStats().AllocatedSpace() / 1024 /
          1024 +
      1;
  if (size_in_mb >= supported_max_size_in_mb)
    size_in_mb = supported_max_size_in_mb - 1;
  if (size_in_mb > observed_max_size_in_mb) {
    // Send a UseCounter only when we see the highest memory usage
    // we've ever seen.
    DEFINE_THREAD_SAFE_STATIC_LOCAL(
        EnumerationHistogram, commited_size_histogram,
        ("BlinkGC.CommittedSize", supported_max_size_in_mb));
    commited_size_histogram.Count(size_in_mb);
    observed_max_size_in_mb = size_in_mb;
  }
}

void ThreadHeap::ReportMemoryUsageForTracing() {
  bool gc_tracing_enabled;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(TRACE_DISABLED_BY_DEFAULT("blink_gc"),
                                     &gc_tracing_enabled);
  if (!gc_tracing_enabled)
    return;

  ThreadHeap& heap = ThreadState::Current()->Heap();
  // These values are divided by 1024 to avoid overflow in practical cases
  // (TRACE_COUNTER values are 32-bit ints).
  // They are capped to INT_MAX just in case.
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("blink_gc"),
                 "ThreadHeap::allocatedObjectSizeKB",
                 std::min(heap.HeapStats().AllocatedObjectSize() / 1024,
                          static_cast<size_t>(INT_MAX)));
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("blink_gc"),
                 "ThreadHeap::markedObjectSizeKB",
                 std::min(heap.HeapStats().MarkedObjectSize() / 1024,
                          static_cast<size_t>(INT_MAX)));
  TRACE_COUNTER1(
      TRACE_DISABLED_BY_DEFAULT("blink_gc"),
      "ThreadHeap::markedObjectSizeAtLastCompleteSweepKB",
      std::min(heap.HeapStats().MarkedObjectSizeAtLastCompleteSweep() / 1024,
               static_cast<size_t>(INT_MAX)));
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("blink_gc"),
                 "ThreadHeap::allocatedSpaceKB",
                 std::min(heap.HeapStats().AllocatedSpace() / 1024,
                          static_cast<size_t>(INT_MAX)));
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("blink_gc"),
                 "ThreadHeap::objectSizeAtLastGCKB",
                 std::min(heap.HeapStats().ObjectSizeAtLastGC() / 1024,
                          static_cast<size_t>(INT_MAX)));
  TRACE_COUNTER1(
      TRACE_DISABLED_BY_DEFAULT("blink_gc"), "ThreadHeap::wrapperCount",
      std::min(heap.HeapStats().WrapperCount(), static_cast<size_t>(INT_MAX)));
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("blink_gc"),
                 "ThreadHeap::wrapperCountAtLastGC",
                 std::min(heap.HeapStats().WrapperCountAtLastGC(),
                          static_cast<size_t>(INT_MAX)));
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("blink_gc"),
                 "ThreadHeap::collectedWrapperCount",
                 std::min(heap.HeapStats().CollectedWrapperCount(),
                          static_cast<size_t>(INT_MAX)));
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("blink_gc"),
                 "ThreadHeap::partitionAllocSizeAtLastGCKB",
                 std::min(heap.HeapStats().PartitionAllocSizeAtLastGC() / 1024,
                          static_cast<size_t>(INT_MAX)));
  TRACE_COUNTER1(TRACE_DISABLED_BY_DEFAULT("blink_gc"),
                 "Partitions::totalSizeOfCommittedPagesKB",
                 std::min(WTF::Partitions::TotalSizeOfCommittedPages() / 1024,
                          static_cast<size_t>(INT_MAX)));
}

size_t ThreadHeap::ObjectPayloadSizeForTesting() {
  ThreadState::AtomicPauseScope atomic_pause_scope(thread_state_);
  size_t object_payload_size = 0;
  thread_state_->SetGCPhase(ThreadState::GCPhase::kMarking);
  thread_state_->Heap().MakeConsistentForGC();
  thread_state_->Heap().PrepareForSweep();
  for (int i = 0; i < BlinkGC::kNumberOfArenas; ++i)
    object_payload_size += arenas_[i]->ObjectPayloadSizeForTesting();
  MakeConsistentForMutator();
  thread_state_->SetGCPhase(ThreadState::GCPhase::kSweeping);
  thread_state_->SetGCPhase(ThreadState::GCPhase::kNone);
  return object_payload_size;
}

void ThreadHeap::VisitPersistentRoots(Visitor* visitor) {
  DCHECK(thread_state_->InAtomicMarkingPause());
  TRACE_EVENT0("blink_gc", "ThreadHeap::visitPersistentRoots");
  thread_state_->VisitPersistents(visitor);
}

void ThreadHeap::VisitStackRoots(MarkingVisitor* visitor) {
  DCHECK(thread_state_->InAtomicMarkingPause());
  TRACE_EVENT0("blink_gc", "ThreadHeap::visitStackRoots");
  address_cache_->FlushIfDirty();
  address_cache_->EnableLookup();
  thread_state_->VisitStack(visitor);
  address_cache_->DisableLookup();
}

BasePage* ThreadHeap::LookupPageForAddress(Address address) {
  DCHECK(thread_state_->InAtomicMarkingPause());
  if (PageMemoryRegion* region = region_tree_->Lookup(address)) {
    return region->PageFromAddress(address);
  }
  return nullptr;
}

void ThreadHeap::ResetHeapCounters() {
  DCHECK(thread_state_->InAtomicMarkingPause());

  ThreadHeap::ReportMemoryUsageForTracing();

  ProcessHeap::DecreaseTotalAllocatedObjectSize(stats_.AllocatedObjectSize());
  ProcessHeap::DecreaseTotalMarkedObjectSize(stats_.MarkedObjectSize());

  stats_.Reset();
}

void ThreadHeap::MakeConsistentForGC() {
  DCHECK(thread_state_->InAtomicMarkingPause());
  TRACE_EVENT0("blink_gc", "ThreadHeap::MakeConsistentForGC");
  for (int i = 0; i < BlinkGC::kNumberOfArenas; ++i)
    arenas_[i]->MakeConsistentForGC();
}

void ThreadHeap::MakeConsistentForMutator() {
  DCHECK(thread_state_->InAtomicMarkingPause());
  for (int i = 0; i < BlinkGC::kNumberOfArenas; ++i)
    arenas_[i]->MakeConsistentForMutator();
}

void ThreadHeap::Compact() {
  if (!Compaction()->IsCompacting())
    return;

  // Compaction is done eagerly and before the mutator threads get
  // to run again. Doing it lazily is problematic, as the mutator's
  // references to live objects could suddenly be invalidated by
  // compaction of a page/heap. We do know all the references to
  // the relocating objects just after marking, but won't later.
  // (e.g., stack references could have been created, new objects
  // created which refer to old collection objects, and so on.)

  // Compact the hash table backing store arena first, it usually has
  // higher fragmentation and is larger.
  //
  // TODO: implement bail out wrt any overall deadline, not compacting
  // the remaining arenas if the time budget has been exceeded.
  Compaction()->StartThreadCompaction();
  for (int i = BlinkGC::kHashTableArenaIndex; i >= BlinkGC::kVector1ArenaIndex;
       --i)
    static_cast<NormalPageArena*>(arenas_[i])->SweepAndCompact();
  Compaction()->FinishThreadCompaction();
}

void ThreadHeap::PrepareForSweep() {
  DCHECK(thread_state_->InAtomicMarkingPause());
  DCHECK(thread_state_->CheckThread());
  for (int i = 0; i < BlinkGC::kNumberOfArenas; i++)
    arenas_[i]->PrepareForSweep();
}

void ThreadHeap::RemoveAllPages() {
  DCHECK(thread_state_->CheckThread());
  for (int i = 0; i < BlinkGC::kNumberOfArenas; ++i)
    arenas_[i]->RemoveAllPages();
}

void ThreadHeap::CompleteSweep() {
  static_assert(BlinkGC::kEagerSweepArenaIndex == 0,
                "Eagerly swept arenas must be processed first.");
  for (int i = 0; i < BlinkGC::kNumberOfArenas; i++)
    arenas_[i]->CompleteSweep();
}

void ThreadHeap::ClearArenaAges() {
  memset(arena_ages_, 0, sizeof(size_t) * BlinkGC::kNumberOfArenas);
  memset(likely_to_be_promptly_freed_.get(), 0,
         sizeof(int) * kLikelyToBePromptlyFreedArraySize);
  current_arena_ages_ = 0;
}

int ThreadHeap::ArenaIndexOfVectorArenaLeastRecentlyExpanded(
    int begin_arena_index,
    int end_arena_index) {
  size_t min_arena_age = arena_ages_[begin_arena_index];
  int arena_index_with_min_arena_age = begin_arena_index;
  for (int arena_index = begin_arena_index + 1; arena_index <= end_arena_index;
       arena_index++) {
    if (arena_ages_[arena_index] < min_arena_age) {
      min_arena_age = arena_ages_[arena_index];
      arena_index_with_min_arena_age = arena_index;
    }
  }
  DCHECK(IsVectorArenaIndex(arena_index_with_min_arena_age));
  return arena_index_with_min_arena_age;
}

BaseArena* ThreadHeap::ExpandedVectorBackingArena(size_t gc_info_index) {
  size_t entry_index = gc_info_index & kLikelyToBePromptlyFreedArrayMask;
  --likely_to_be_promptly_freed_[entry_index];
  int arena_index = vector_backing_arena_index_;
  arena_ages_[arena_index] = ++current_arena_ages_;
  vector_backing_arena_index_ = ArenaIndexOfVectorArenaLeastRecentlyExpanded(
      BlinkGC::kVector1ArenaIndex, BlinkGC::kVector4ArenaIndex);
  return arenas_[arena_index];
}

void ThreadHeap::AllocationPointAdjusted(int arena_index) {
  arena_ages_[arena_index] = ++current_arena_ages_;
  if (vector_backing_arena_index_ == arena_index) {
    vector_backing_arena_index_ = ArenaIndexOfVectorArenaLeastRecentlyExpanded(
        BlinkGC::kVector1ArenaIndex, BlinkGC::kVector4ArenaIndex);
  }
}

void ThreadHeap::PromptlyFreed(size_t gc_info_index) {
  DCHECK(thread_state_->CheckThread());
  size_t entry_index = gc_info_index & kLikelyToBePromptlyFreedArrayMask;
  // See the comment in vectorBackingArena() for why this is +3.
  likely_to_be_promptly_freed_[entry_index] += 3;
}

#if defined(ADDRESS_SANITIZER)
void ThreadHeap::PoisonAllHeaps() {
  RecursiveMutexLocker persistent_lock(
      ProcessHeap::CrossThreadPersistentMutex());
  // Poisoning all unmarked objects in the other arenas.
  for (int i = 1; i < BlinkGC::kNumberOfArenas; i++)
    arenas_[i]->PoisonArena();
  // CrossThreadPersistents in unmarked objects may be accessed from other
  // threads (e.g. in CrossThreadPersistentRegion::shouldTracePersistent) and
  // that would be fine.
  ProcessHeap::GetCrossThreadPersistentRegion()
      .UnpoisonCrossThreadPersistents();
}

void ThreadHeap::PoisonEagerArena() {
  RecursiveMutexLocker persistent_lock(
      ProcessHeap::CrossThreadPersistentMutex());
  arenas_[BlinkGC::kEagerSweepArenaIndex]->PoisonArena();
  // CrossThreadPersistents in unmarked objects may be accessed from other
  // threads (e.g. in CrossThreadPersistentRegion::shouldTracePersistent) and
  // that would be fine.
  ProcessHeap::GetCrossThreadPersistentRegion()
      .UnpoisonCrossThreadPersistents();
}
#endif

#if DCHECK_IS_ON()
BasePage* ThreadHeap::FindPageFromAddress(Address address) {
  for (int i = 0; i < BlinkGC::kNumberOfArenas; ++i) {
    if (BasePage* page = arenas_[i]->FindPageFromAddress(address))
      return page;
  }
  return nullptr;
}
#endif

void ThreadHeap::TakeSnapshot(SnapshotType type) {
  DCHECK(thread_state_->InAtomicMarkingPause());

  // 0 is used as index for freelist entries. Objects are indexed 1 to
  // gcInfoIndex.
  ThreadState::GCSnapshotInfo info(GCInfoTable::Get().GcInfoIndex() + 1);
  String thread_dump_name =
      String::Format("blink_gc/thread_%lu",
                     static_cast<unsigned long>(thread_state_->ThreadId()));
  const String heaps_dump_name = thread_dump_name + "/heaps";
  const String classes_dump_name = thread_dump_name + "/classes";

  int number_of_heaps_reported = 0;
#define SNAPSHOT_HEAP(ArenaType)                                          \
  {                                                                       \
    number_of_heaps_reported++;                                           \
    switch (type) {                                                       \
      case SnapshotType::kHeapSnapshot:                                   \
        arenas_[BlinkGC::k##ArenaType##ArenaIndex]->TakeSnapshot(         \
            heaps_dump_name + "/" #ArenaType, info);                      \
        break;                                                            \
      case SnapshotType::kFreelistSnapshot:                               \
        arenas_[BlinkGC::k##ArenaType##ArenaIndex]->TakeFreelistSnapshot( \
            heaps_dump_name + "/" #ArenaType);                            \
        break;                                                            \
      default:                                                            \
        NOTREACHED();                                                     \
    }                                                                     \
  }

  SNAPSHOT_HEAP(NormalPage1);
  SNAPSHOT_HEAP(NormalPage2);
  SNAPSHOT_HEAP(NormalPage3);
  SNAPSHOT_HEAP(NormalPage4);
  SNAPSHOT_HEAP(EagerSweep);
  SNAPSHOT_HEAP(Vector1);
  SNAPSHOT_HEAP(Vector2);
  SNAPSHOT_HEAP(Vector3);
  SNAPSHOT_HEAP(Vector4);
  SNAPSHOT_HEAP(InlineVector);
  SNAPSHOT_HEAP(HashTable);
  SNAPSHOT_HEAP(LargeObject);
  FOR_EACH_TYPED_ARENA(SNAPSHOT_HEAP);

  DCHECK_EQ(number_of_heaps_reported, BlinkGC::kNumberOfArenas);

#undef SNAPSHOT_HEAP

  if (type == SnapshotType::kFreelistSnapshot)
    return;

  size_t total_live_count = 0;
  size_t total_dead_count = 0;
  size_t total_live_size = 0;
  size_t total_dead_size = 0;
  for (size_t gc_info_index = 1;
       gc_info_index <= GCInfoTable::Get().GcInfoIndex(); ++gc_info_index) {
    total_live_count += info.live_count[gc_info_index];
    total_dead_count += info.dead_count[gc_info_index];
    total_live_size += info.live_size[gc_info_index];
    total_dead_size += info.dead_size[gc_info_index];
  }

  base::trace_event::MemoryAllocatorDump* thread_dump =
      BlinkGCMemoryDumpProvider::Instance()
          ->CreateMemoryAllocatorDumpForCurrentGC(thread_dump_name);
  thread_dump->AddScalar("live_count", "objects", total_live_count);
  thread_dump->AddScalar("dead_count", "objects", total_dead_count);
  thread_dump->AddScalar("live_size", "bytes", total_live_size);
  thread_dump->AddScalar("dead_size", "bytes", total_dead_size);

  base::trace_event::MemoryAllocatorDump* heaps_dump =
      BlinkGCMemoryDumpProvider::Instance()
          ->CreateMemoryAllocatorDumpForCurrentGC(heaps_dump_name);
  base::trace_event::MemoryAllocatorDump* classes_dump =
      BlinkGCMemoryDumpProvider::Instance()
          ->CreateMemoryAllocatorDumpForCurrentGC(classes_dump_name);
  BlinkGCMemoryDumpProvider::Instance()
      ->CurrentProcessMemoryDump()
      ->AddOwnershipEdge(classes_dump->guid(), heaps_dump->guid());
}

bool ThreadHeap::AdvanceLazySweep(double deadline_seconds) {
  for (int i = 0; i < BlinkGC::kNumberOfArenas; i++) {
    // lazySweepWithDeadline() won't check the deadline until it sweeps
    // 10 pages. So we give a small slack for safety.
    double slack = 0.001;
    double remaining_budget =
        deadline_seconds - slack - CurrentTimeTicksInSeconds();
    if (remaining_budget <= 0 ||
        !arenas_[i]->LazySweepWithDeadline(deadline_seconds)) {
      return false;
    }
  }
  return true;
}

void ThreadHeap::WriteBarrier(void* value) {
  DCHECK(thread_state_->IsIncrementalMarking());
  DCHECK(value);
  // '-1' is used to indicate deleted values.
  DCHECK_NE(value, reinterpret_cast<void*>(-1));

  BasePage* const page = PageFromObject(value);
  HeapObjectHeader* const header =
      page->IsLargeObjectPage()
          ? static_cast<LargeObjectPage*>(page)->GetHeapObjectHeader()
          : static_cast<NormalPage*>(page)->FindHeaderFromAddress(
                reinterpret_cast<Address>(const_cast<void*>(value)));
  if (header->IsMarked())
    return;

  // Mark and push trace callback.
  header->Mark();
  marking_worklist_->Push(
      WorklistTaskId::MainThread,
      {header->Payload(),
       GCInfoTable::Get().GCInfoFromIndex(header->GcInfoIndex())->trace_});
}

ThreadHeap* ThreadHeap::main_thread_heap_ = nullptr;

}  // namespace blink
