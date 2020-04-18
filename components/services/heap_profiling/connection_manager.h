// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_CONNECTION_MANAGER_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_CONNECTION_MANAGER_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/process/process_handle.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/services/heap_profiling/allocation_event.h"
#include "components/services/heap_profiling/allocation_tracker.h"
#include "components/services/heap_profiling/backtrace_storage.h"
#include "components/services/heap_profiling/public/mojom/heap_profiling_service.mojom.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "services/resource_coordinator/public/mojom/memory_instrumentation/memory_instrumentation.mojom.h"

namespace base {

class SequencedTaskRunner;

}  // namespace base

namespace heap_profiling {

using VmRegions =
    base::flat_map<base::ProcessId,
                   std::vector<memory_instrumentation::mojom::VmRegionPtr>>;

// Manages all connections and logging for each process. Pipes are supplied by
// the pipe server and this class will connect them to a parser and logger.
//
// Note |backtrace_storage| must outlive ConnectionManager.
//
// This object is constructed on the UI thread, but the rest of the usage
// (including deletion) is on the IO thread.
class ConnectionManager {
  using DumpProcessesForTracingCallback = memory_instrumentation::mojom::
      HeapProfiler::DumpProcessesForTracingCallback;

 public:
  ConnectionManager();
  ~ConnectionManager();

  // Shared types for the dump-type-specific args structures.
  struct DumpArgs {
    DumpArgs();
    DumpArgs(DumpArgs&&) noexcept;
    ~DumpArgs();

   private:
    friend ConnectionManager;

    // This lock keeps the backtrace atoms alive throughout the dumping
    // process. It will be initialized by DumpProcess.
    BacktraceStorage::Lock backtrace_storage_lock;

    DISALLOW_COPY_AND_ASSIGN(DumpArgs);
  };

  // Dumping is asynchronous so will not be complete when this function
  // returns. The dump is complete when the callback provided in the args is
  // fired.
  void DumpProcessesForTracing(bool keep_small_allocations,
                               bool strip_path_from_mapped_files,
                               DumpProcessesForTracingCallback callback,
                               VmRegions vm_regions);

  void OnNewConnection(base::ProcessId pid,
                       mojom::ProfilingClientPtr client,
                       mojo::ScopedHandle receiver_pipe_end,
                       mojom::ProcessType process_type,
                       mojom::ProfilingParamsPtr params);

  std::vector<base::ProcessId> GetConnectionPids();
  std::vector<base::ProcessId> GetConnectionPidsThatNeedVmRegions();

 private:
  struct Connection;
  struct DumpProcessesForTracingTracking;

  void DoDumpOneProcessForTracing(
      scoped_refptr<DumpProcessesForTracingTracking> tracking,
      base::ProcessId pid,
      mojom::ProcessType process_type,
      bool keep_small_allocations,
      bool strip_path_from_mapped_files,
      uint32_t sampling_rate,
      bool success,
      AllocationCountMap counts,
      AllocationTracker::ContextMap context,
      AllocationTracker::AddressToStringMap mapped_strings);

  // Notification that a connection is complete. Unlike OnNewConnection which
  // is signaled by the pipe server, this is signaled by the allocation tracker
  // to ensure that the pipeline for this process has been flushed of all
  // messages.
  void OnConnectionComplete(base::ProcessId pid);

  // Reports the ProcessTypes of the processes being profiled.
  void ReportMetrics();

  // These thunks post the request back to the given thread.
  static void OnConnectionCompleteThunk(
      scoped_refptr<base::SequencedTaskRunner> main_loop,
      base::WeakPtr<ConnectionManager> connection_manager,
      base::ProcessId process_id);

  BacktraceStorage backtrace_storage_;

  // Next ID to use for a barrier request. This is incremented for each use
  // to ensure barrier IDs are unique.
  uint32_t next_barrier_id_ = 1;

  // The next ID to use when exporting a heap dump.
  size_t next_id_ = 1;

  // Maps process ID to the connection information for it.
  base::flat_map<base::ProcessId, std::unique_ptr<Connection>> connections_;
  base::Lock connections_lock_;

  // Every 24-hours, reports the types of profiled processes.
  base::RepeatingTimer metrics_timer_;

  // To avoid deadlock, synchronous calls to the browser are made on a dedicated
  // thread that does nothing else. Both the IO thread and connection-specific
  // threads could potentially be processing messages from the browser process,
  // which in turn could be blocked on sending more messages over the pipe.
  base::Thread blocking_thread_;

  // Must be last.
  base::WeakPtrFactory<ConnectionManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ConnectionManager);
};

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_CONNECTION_MANAGER_H_
