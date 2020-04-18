// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/allocation_tracker.h"

#include "base/callback.h"
#include "base/json/string_escape.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/services/heap_profiling/backtrace_storage.h"

namespace heap_profiling {

AllocationTracker::AllocationTracker(CompleteCallback complete_cb,
                                     BacktraceStorage* backtrace_storage)
    : complete_callback_(std::move(complete_cb)),
      backtrace_storage_(backtrace_storage) {}

AllocationTracker::~AllocationTracker() {
  for (auto& pair : registered_snapshot_callbacks_) {
    RunnerSnapshotCallbackPair& rsc_pair = pair.second;
    rsc_pair.first->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(rsc_pair.second), false, AllocationCountMap(),
                       ContextMap(), AddressToStringMap()));
  }

  std::vector<const Backtrace*> to_free;
  to_free.reserve(live_allocs_.size());
  for (const auto& cur : live_allocs_)
    to_free.push_back(cur.backtrace());
  backtrace_storage_->Free(to_free);
}

void AllocationTracker::OnHeader(const StreamHeader& header) {}

void AllocationTracker::OnAlloc(const AllocPacket& alloc_packet,
                                std::vector<Address>&& bt,
                                std::string&& context) {
  // Compute the context ID for this allocation, 0 means no context.
  int context_id = 0;
  if (!context.empty()) {
    // Escape the strings before saving them, to simplify exporting a heap dump.
    std::string escaped_context;
    base::EscapeJSONString(context, false /* put_in_quotes */,
                           &escaped_context);

    auto inserted_record =
        context_.emplace(std::piecewise_construct,
                         std::forward_as_tuple(std::move(escaped_context)),
                         std::forward_as_tuple(next_context_id_));
    context_id = inserted_record.first->second;
    if (inserted_record.second)
      next_context_id_++;
  }

  const Backtrace* backtrace = backtrace_storage_->Insert(std::move(bt));
  live_allocs_.emplace(alloc_packet.allocator, Address(alloc_packet.address),
                       alloc_packet.size, backtrace, context_id);
}

void AllocationTracker::OnFree(const FreePacket& free_packet) {
  AllocationEvent find_me(Address(free_packet.address));
  auto found = live_allocs_.find(find_me);
  if (found != live_allocs_.end()) {
    backtrace_storage_->Free(found->backtrace());
    live_allocs_.erase(found);
  }
}

void AllocationTracker::OnBarrier(const BarrierPacket& barrier_packet) {
  RunnerSnapshotCallbackPair pair;
  {
    base::AutoLock lock(snapshot_lock_);
    auto found = registered_snapshot_callbacks_.find(barrier_packet.barrier_id);
    if (found == registered_snapshot_callbacks_.end()) {
      DLOG(WARNING) << "Unexpected barrier";
      return;
    }
    pair = std::move(found->second);
    registered_snapshot_callbacks_.erase(found);
  }

  // Execute the callback outside of the lock. The arguments here must be
  // copied.
  pair.first->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](SnapshotCallback cb, AllocationCountMap counts, ContextMap context,
             AddressToStringMap mapped_strings) {
            std::move(cb).Run(true, std::move(counts), std::move(context),
                              mapped_strings);
          },
          std::move(pair.second), AllocationEventSetToCountMap(live_allocs_),
          context_, mapped_strings_));
}

void AllocationTracker::OnStringMapping(
    const StringMappingPacket& string_mapping_packet,
    const std::string& str) {
  std::string dest;

  // Escape the strings before saving them, to simplify exporting a heap dump.
  base::EscapeJSONString(str, false /* put_in_quotes */, &dest);
  mapped_strings_[string_mapping_packet.address] = std::move(dest);
}

void AllocationTracker::OnComplete() {
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                std::move(complete_callback_));
}

void AllocationTracker::SnapshotOnBarrier(
    uint32_t barrier_id,
    scoped_refptr<base::SequencedTaskRunner> callback_runner,
    SnapshotCallback callback) {
  base::AutoLock lock(snapshot_lock_);
  registered_snapshot_callbacks_[barrier_id] =
      std::make_pair(std::move(callback_runner), std::move(callback));
}

}  // namespace heap_profiling
