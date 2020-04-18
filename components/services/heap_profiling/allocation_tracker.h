// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_ALLOCATION_TRACKER_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_ALLOCATION_TRACKER_H_

#include <map>
#include <unordered_map>

#include "base/callback.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/synchronization/lock.h"
#include "components/services/heap_profiling/allocation_event.h"
#include "components/services/heap_profiling/backtrace_storage.h"
#include "components/services/heap_profiling/receiver.h"

namespace heap_profiling {

// Tracks live allocations in one process. This is an analogue to memory-infra
// allocation register and needs to be merged/deduped.
//
// This class is not threadsafe except as noted.
class AllocationTracker : public Receiver {
 public:
  using CompleteCallback = base::OnceClosure;
  using ContextMap = std::map<std::string, int>;
  using AddressToStringMap = std::unordered_map<uint64_t, std::string>;

  // Callback for taking an asynchronous snapshot. The first parameter is a
  // success parameter. This class will always set it to true, but this is
  // useful for calling code that must handle failure.
  using SnapshotCallback = base::OnceCallback<
      void(bool, AllocationCountMap, ContextMap, AddressToStringMap)>;

  AllocationTracker(CompleteCallback complete_cb,
                    BacktraceStorage* backtrace_storage);
  ~AllocationTracker() override;

  void OnHeader(const StreamHeader& header) override;
  void OnAlloc(const AllocPacket& alloc_packet,
               std::vector<Address>&& bt,
               std::string&& context) override;
  void OnFree(const FreePacket& free_packet) override;
  void OnBarrier(const BarrierPacket& barrier_packet) override;
  void OnStringMapping(const StringMappingPacket& string_mapping_packet,
                       const std::string& str) override;
  void OnComplete() override;

  // Registers the given snapshot closure to be executed when the given barrier
  // ID is received from the process. This will only trigger for a barrier
  // received after registration.
  //
  // This function is threadsafe. The callback will be executed on the given
  // task runner.
  void SnapshotOnBarrier(
      uint32_t barrier_id,
      scoped_refptr<base::SequencedTaskRunner> callback_runner,
      SnapshotCallback callback);

 private:
  CompleteCallback complete_callback_;

  // The snapshot callbacks are threadsafe and are protected by the lock.
  base::Lock snapshot_lock_;
  using RunnerSnapshotCallbackPair =
      std::pair<scoped_refptr<base::SequencedTaskRunner>, SnapshotCallback>;
  std::map<uint32_t, RunnerSnapshotCallbackPair> registered_snapshot_callbacks_;

  BacktraceStorage* backtrace_storage_;

  AddressToStringMap mapped_strings_;

  // Need to track all live objects. Since the free information doesn't have
  // the metadata, we can't keep a map of counts indexed by just the metadata
  // (which is all the trace JSON needs), but need to keep an index by address.
  //
  // This could be a two-level index, where one set of metadata is kept and
  // addresses index into that. But a full copy of the metadata is about the
  // same size as the internal map node required for this second index, with
  // additional complexity.
  AllocationEventSet live_allocs_;

  // The context strings are atoms. Since there are O(100's) of these, we do
  // not bother to uniquify across all processes. This map contains the unique
  // context strings for the current process, mapped to the unique ID for that
  // context. This unique ID is stored in the allocation. This is optimized for
  // insertion. When querying, a reverse index will need to be generated
  ContextMap context_;
  int next_context_id_ = 1;  // 0 means no context.

  DISALLOW_COPY_AND_ASSIGN(AllocationTracker);
};

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_ALLOCATION_TRACKER_H_
