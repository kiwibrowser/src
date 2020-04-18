// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_QUEUED_REQUEST_H_
#define SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_QUEUED_REQUEST_H_

#include <map>
#include <memory>
#include <set>

#include "base/containers/flat_map.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/coordinator.h"
#include "services/resource_coordinator/public/mojom/memory_instrumentation/memory_instrumentation.mojom.h"

using base::trace_event::MemoryDumpLevelOfDetail;
using base::trace_event::MemoryDumpType;

namespace memory_instrumentation {

using OSMemDumpMap =
    base::flat_map<base::ProcessId,
                   memory_instrumentation::mojom::RawOSMemDumpPtr>;

// Holds data for pending requests enqueued via RequestGlobalMemoryDump().
struct QueuedRequest {
  using RequestGlobalMemoryDumpInternalCallback = base::OnceCallback<
      void(bool, uint64_t, memory_instrumentation::mojom::GlobalMemoryDumpPtr)>;

  struct Args {
    Args(MemoryDumpType dump_type,
         MemoryDumpLevelOfDetail level_of_detail,
         const std::vector<std::string>& allocator_dump_names,
         bool add_to_trace,
         base::ProcessId pid);
    Args(const Args&);
    ~Args();

    const MemoryDumpType dump_type;
    const MemoryDumpLevelOfDetail level_of_detail;
    const std::vector<std::string> allocator_dump_names;
    const bool add_to_trace;
    const base::ProcessId pid;
  };

  struct PendingResponse {
    enum Type {
      kChromeDump,
      kOSDump,
    };
    PendingResponse(const mojom::ClientProcess* client, const Type type);

    bool operator<(const PendingResponse& other) const;

    const mojom::ClientProcess* client;
    const Type type;
  };

  struct Response {
    Response();
    ~Response();

    base::ProcessId process_id = base::kNullProcessId;
    mojom::ProcessType process_type = mojom::ProcessType::OTHER;
    std::unique_ptr<base::trace_event::ProcessMemoryDump> chrome_dump;
    OSMemDumpMap os_dumps;
  };

  QueuedRequest(const Args& args,
                uint64_t dump_guid,
                RequestGlobalMemoryDumpInternalCallback callback);
  ~QueuedRequest();

  base::trace_event::MemoryDumpRequestArgs GetRequestArgs();

  mojom::MemoryMapOption memory_map_option() const {
    return args.level_of_detail ==
                   base::trace_event::MemoryDumpLevelOfDetail::DETAILED
               ? mojom::MemoryMapOption::FULL
               : mojom::MemoryMapOption::NONE;
  }

  bool should_return_summaries() const {
    return args.dump_type == base::trace_event::MemoryDumpType::SUMMARY_ONLY;
  }

  const Args args;
  const uint64_t dump_guid;
  RequestGlobalMemoryDumpInternalCallback callback;

  // When a dump, requested via RequestGlobalMemoryDump(), is in progress this
  // set contains a |PendingResponse| for each |RequestChromeMemoryDump| and
  // |RequestOSMemoryDump| call that has not yet replied or been canceled (due
  // to the client disconnecting).
  std::set<PendingResponse> pending_responses;
  std::map<mojom::ClientProcess*, Response> responses;
  int failed_memory_dump_count = 0;
  bool dump_in_progress = false;

  // This field is set to |true| before a heap dump is requested, and set to
  // |false| after the heap dump has been added to the trace.
  bool heap_dump_in_progress = false;

  // The time we started handling the request (does not including queuing
  // time).
  base::Time start_time;
};

// Holds data for pending requests enqueued via GetVmRegionsForHeapProfiler().
struct QueuedVmRegionRequest {
  QueuedVmRegionRequest(
      uint64_t dump_guid,
      mojom::HeapProfilerHelper::GetVmRegionsForHeapProfilerCallback callback);
  ~QueuedVmRegionRequest();
  const uint64_t dump_guid;
  mojom::HeapProfilerHelper::GetVmRegionsForHeapProfilerCallback callback;

  struct Response {
    Response();
    ~Response();

    base::ProcessId process_id = base::kNullProcessId;
    OSMemDumpMap os_dumps;
  };

  std::set<mojom::ClientProcess*> pending_responses;
  std::map<mojom::ClientProcess*, Response> responses;
};

}  // namespace memory_instrumentation

#endif  // SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_QUEUED_REQUEST_H_
