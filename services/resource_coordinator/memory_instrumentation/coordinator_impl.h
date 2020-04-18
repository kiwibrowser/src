// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_COORDINATOR_IMPL_H_
#define SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_COORDINATOR_IMPL_H_

#include <list>
#include <map>
#include <set>
#include <unordered_map>

#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/resource_coordinator/memory_instrumentation/process_map.h"
#include "services/resource_coordinator/memory_instrumentation/queued_request.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/coordinator.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/tracing_observer.h"
#include "services/resource_coordinator/public/mojom/memory_instrumentation/memory_instrumentation.mojom.h"
#include "services/service_manager/public/cpp/identity.h"

namespace service_manager {
struct BindSourceInfo;
class Connector;
}

namespace memory_instrumentation {

// Memory instrumentation service. It serves two purposes:
// - Handles a registry of the processes that have a memory instrumentation
//   client library instance (../../public/cpp/memory).
// - Provides global (i.e. for all processes) memory snapshots on demand.
//   Global snapshots are obtained by requesting in-process snapshots from each
//   registered client and aggregating them.
class CoordinatorImpl : public Coordinator,
                        public mojom::Coordinator,
                        public mojom::HeapProfilerHelper {
 public:
  // The getter of the unique instance.
  static CoordinatorImpl* GetInstance();

  CoordinatorImpl(service_manager::Connector* connector);

  // Binds a client library to this coordinator instance.
  void BindCoordinatorRequest(
      mojom::CoordinatorRequest,
      const service_manager::BindSourceInfo& source_info) override;

  void BindHeapProfilerHelperRequest(
      mojom::HeapProfilerHelperRequest request,
      const service_manager::BindSourceInfo& source_info);

  // mojom::Coordinator implementation.
  void RegisterClientProcess(mojom::ClientProcessPtr,
                             mojom::ProcessType) override;
  void UnregisterClientProcess(mojom::ClientProcess*);
  void RequestGlobalMemoryDump(
      base::trace_event::MemoryDumpType,
      base::trace_event::MemoryDumpLevelOfDetail,
      const std::vector<std::string>& allocator_dump_names,
      RequestGlobalMemoryDumpCallback) override;
  void RequestGlobalMemoryDumpForPid(
      base::ProcessId,
      const std::vector<std::string>& allocator_dump_names,
      RequestGlobalMemoryDumpForPidCallback) override;
  void RequestGlobalMemoryDumpAndAppendToTrace(
      base::trace_event::MemoryDumpType,
      base::trace_event::MemoryDumpLevelOfDetail,
      RequestGlobalMemoryDumpAndAppendToTraceCallback) override;
  void RegisterHeapProfiler(mojom::HeapProfilerPtr heap_profiler) override;

  // mojom::HeapProfilerHelper implementation.
  void GetVmRegionsForHeapProfiler(
      const std::vector<base::ProcessId>& pids,
      GetVmRegionsForHeapProfilerCallback) override;

 protected:
  // virtual for testing.
  virtual service_manager::Identity GetClientIdentityForCurrentRequest() const;
  // virtual for testing.
  virtual base::ProcessId GetProcessIdForClientIdentity(
      service_manager::Identity identity) const;
  ~CoordinatorImpl() override;

 private:
  using OSMemDumpMap = base::flat_map<base::ProcessId, mojom::RawOSMemDumpPtr>;
  using RequestGlobalMemoryDumpInternalCallback =
      base::OnceCallback<void(bool, uint64_t, mojom::GlobalMemoryDumpPtr)>;
  friend std::default_delete<CoordinatorImpl>;  // For testing
  friend class CoordinatorImplTest;             // For testing

  // Holds the identity and remote reference of registered clients.
  struct ClientInfo {
    ClientInfo(const service_manager::Identity&,
               mojom::ClientProcessPtr,
               mojom::ProcessType);
    ~ClientInfo();

    const service_manager::Identity identity;
    const mojom::ClientProcessPtr client;
    const mojom::ProcessType process_type;
  };

  void RequestGlobalMemoryDumpInternal(
      const QueuedRequest::Args& args,
      RequestGlobalMemoryDumpInternalCallback callback);

  // Callback of RequestChromeMemoryDump.
  void OnChromeMemoryDumpResponse(
      mojom::ClientProcess*,
      bool success,
      uint64_t dump_guid,
      std::unique_ptr<base::trace_event::ProcessMemoryDump> chrome_memory_dump);

  // Callback of RequestOSMemoryDump.
  void OnOSMemoryDumpResponse(uint64_t dump_guid,
                              mojom::ClientProcess*,
                              bool success,
                              OSMemDumpMap);

  // Callback of RequestOSMemoryDumpForVmRegions.
  void OnOSMemoryDumpForVMRegions(uint64_t dump_guid,
                                  mojom::ClientProcess* client,
                                  bool success,
                                  OSMemDumpMap);

  void FinalizeVmRegionDumpIfAllManagersReplied(uint64_t dump_guid);

  // Callback of DumpProcessesForTracing.
  void OnDumpProcessesForTracing(
      uint64_t dump_guid,
      std::vector<mojom::SharedBufferWithSizePtr> buffers);

  void RemovePendingResponse(mojom::ClientProcess*,
                             QueuedRequest::PendingResponse::Type);

  void OnQueuedRequestTimedOut(uint64_t dump_guid);
  void OnHeapDumpTimeOut(uint64_t dump_guid);

  void PerformNextQueuedGlobalMemoryDump();
  void FinalizeGlobalMemoryDumpIfAllManagersReplied();
  QueuedRequest* GetCurrentRequest();

  void set_client_process_timeout(base::TimeDelta client_process_timeout) {
    client_process_timeout_ = client_process_timeout;
  }

  // Map of registered client processes.
  std::map<mojom::ClientProcess*, std::unique_ptr<ClientInfo>> clients_;

  // Outstanding dump requests, enqueued via RequestGlobalMemoryDump().
  std::list<QueuedRequest> queued_memory_dump_requests_;

  // Outstanding vm region requests, enqueued via GetVmRegionsForHeapProfiler().
  // This is kept in a separate list from |queued_memory_dump_requests_| for two
  // reasons:
  //   1) The profiling service is queried during a memory dump request, but the
  //   profiling service in turn needs to query for vm regions. Keeping this in
  //   the same list as |queued_memory_dump_requests_| would require this class
  //   to support concurrent requests.
  //
  //   2) Vm region requests are only ever requested by the profiling service,
  //   which uses the HeapProfilerHelper interface. Keeping the requests
  //   separate means we can avoid littering the RequestGlobalMemoryDump
  //   interface with flags intended for HeapProfilerHelper. This was already
  //   technically possible before, but required some additional plumbing. Now
  //   the separation is much cleaner.
  //
  // Unlike queued_memory_dump_requests_, all requests are executed in parallel.
  // The key is a |dump_guid|.
  std::map<uint64_t, std::unique_ptr<QueuedVmRegionRequest>>
      in_progress_vm_region_requests_;

  // There may be extant callbacks in |queued_memory_dump_requests_|. The
  // bindings_ must be closed before destroying the un-run callbacks.
  mojo::BindingSet<mojom::Coordinator, service_manager::Identity> bindings_;

  // There may be extant callbacks in |queued_memory_dump_requests_|. The
  // bindings_ must be closed before destroying the un-run callbacks.
  mojo::BindingSet<mojom::HeapProfilerHelper, service_manager::Identity>
      bindings_heap_profiler_helper_;

  // Maintains a map of service_manager::Identity -> pid for registered clients.
  std::unique_ptr<ProcessMap> process_map_;

  // Dump IDs are unique across both heap dump and memory dump requests.
  uint64_t next_dump_id_;
  std::unique_ptr<TracingObserver> tracing_observer_;

  // Timeout for registered client processes to respond to dump requests.
  base::TimeDelta client_process_timeout_;

  // When not null, can be queried for heap dumps.
  mojom::HeapProfilerPtr heap_profiler_;

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(CoordinatorImpl);
};

}  // namespace memory_instrumentation
#endif  // SERVICES_RESOURCE_COORDINATOR_MEMORY_INSTRUMENTATION_COORDINATOR_IMPL_H_
