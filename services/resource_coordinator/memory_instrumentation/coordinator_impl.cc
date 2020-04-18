// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/memory_instrumentation/coordinator_impl.h"

#include <inttypes.h>
#include <stdio.h>

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "services/resource_coordinator/memory_instrumentation/queued_request_dispatcher.h"
#include "services/resource_coordinator/memory_instrumentation/switches.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/client_process_impl.h"
#include "services/resource_coordinator/public/mojom/memory_instrumentation/constants.mojom.h"
#include "services/resource_coordinator/public/mojom/memory_instrumentation/memory_instrumentation.mojom.h"
#include "services/service_manager/public/cpp/identity.h"

#if defined(OS_MACOSX) && !defined(OS_IOS)
#include "base/mac/mac_util.h"
#endif

using base::trace_event::MemoryDumpLevelOfDetail;
using base::trace_event::MemoryDumpType;

namespace memory_instrumentation {

namespace {

memory_instrumentation::CoordinatorImpl* g_coordinator_impl;

constexpr base::TimeDelta kHeapDumpTimeout = base::TimeDelta::FromSeconds(60);

// A wrapper classes that allows a string to be exported as JSON in a trace
// event.
class StringWrapper : public base::trace_event::ConvertableToTraceFormat {
 public:
  explicit StringWrapper(std::string json) : json_(std::move(json)) {}

  void AppendAsTraceFormat(std::string* out) const override {
    out->append(json_);
  }

  std::string json_;
};

}  // namespace


// static
CoordinatorImpl* CoordinatorImpl::GetInstance() {
  return g_coordinator_impl;
}

CoordinatorImpl::CoordinatorImpl(service_manager::Connector* connector)
    : next_dump_id_(0),
      client_process_timeout_(base::TimeDelta::FromSeconds(15)) {
  process_map_ = std::make_unique<ProcessMap>(connector);
  DCHECK(!g_coordinator_impl);
  g_coordinator_impl = this;
  base::trace_event::MemoryDumpManager::GetInstance()->set_tracing_process_id(
      mojom::kServiceTracingProcessId);

  tracing_observer_ = std::make_unique<TracingObserver>(
      base::trace_event::TraceLog::GetInstance(), nullptr);
}

CoordinatorImpl::~CoordinatorImpl() {
  g_coordinator_impl = nullptr;
}

base::ProcessId CoordinatorImpl::GetProcessIdForClientIdentity(
    service_manager::Identity identity) const {
  DCHECK(identity.IsValid());
  return process_map_->GetProcessId(identity);
}

service_manager::Identity CoordinatorImpl::GetClientIdentityForCurrentRequest()
    const {
  return bindings_.dispatch_context();
}

void CoordinatorImpl::BindCoordinatorRequest(
    mojom::CoordinatorRequest request,
    const service_manager::BindSourceInfo& source_info) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  bindings_.AddBinding(this, std::move(request), source_info.identity);
}

void CoordinatorImpl::BindHeapProfilerHelperRequest(
    mojom::HeapProfilerHelperRequest request,
    const service_manager::BindSourceInfo& source_info) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  bindings_heap_profiler_helper_.AddBinding(this, std::move(request),
                                            source_info.identity);
}

void CoordinatorImpl::RequestGlobalMemoryDump(
    MemoryDumpType dump_type,
    MemoryDumpLevelOfDetail level_of_detail,
    const std::vector<std::string>& allocator_dump_names,
    RequestGlobalMemoryDumpCallback callback) {
  // This merely strips out the |dump_guid| argument.
  auto adapter = [](RequestGlobalMemoryDumpCallback callback, bool success,
                    uint64_t, mojom::GlobalMemoryDumpPtr global_memory_dump) {
    std::move(callback).Run(success, std::move(global_memory_dump));
  };

  QueuedRequest::Args args(dump_type, level_of_detail, allocator_dump_names,
                           false /* add_to_trace */, base::kNullProcessId);
  RequestGlobalMemoryDumpInternal(args,
                                  base::BindOnce(adapter, std::move(callback)));
}

void CoordinatorImpl::RequestGlobalMemoryDumpForPid(
    base::ProcessId pid,
    const std::vector<std::string>& allocator_dump_names,
    RequestGlobalMemoryDumpForPidCallback callback) {
  // Error out early if process id is null to avoid confusing with global
  // dump for all processes case when pid is kNullProcessId.
  if (pid == base::kNullProcessId) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  // This merely strips out the |dump_guid| argument; this is not relevant
  // as we are not adding to trace.
  auto adapter = [](RequestGlobalMemoryDumpForPidCallback callback,
                    bool success, uint64_t,
                    mojom::GlobalMemoryDumpPtr global_memory_dump) {
    std::move(callback).Run(success, std::move(global_memory_dump));
  };

  QueuedRequest::Args args(
      base::trace_event::MemoryDumpType::SUMMARY_ONLY,
      base::trace_event::MemoryDumpLevelOfDetail::BACKGROUND,
      allocator_dump_names, false /* add_to_trace */, pid);
  RequestGlobalMemoryDumpInternal(args,
                                  base::BindOnce(adapter, std::move(callback)));
}

void CoordinatorImpl::RequestGlobalMemoryDumpAndAppendToTrace(
    MemoryDumpType dump_type,
    MemoryDumpLevelOfDetail level_of_detail,
    RequestGlobalMemoryDumpAndAppendToTraceCallback callback) {
  // This merely strips out the |dump_ptr| argument.
  auto adapter = [](RequestGlobalMemoryDumpAndAppendToTraceCallback callback,
                    bool success, uint64_t dump_guid,
                    mojom::GlobalMemoryDumpPtr) {
    std::move(callback).Run(success, dump_guid);
  };

  QueuedRequest::Args args(dump_type, level_of_detail, {},
                           true /* add_to_trace */, base::kNullProcessId);
  RequestGlobalMemoryDumpInternal(args,
                                  base::BindOnce(adapter, std::move(callback)));
}

void CoordinatorImpl::RegisterHeapProfiler(
    mojom::HeapProfilerPtr heap_profiler) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  heap_profiler_ = std::move(heap_profiler);
}

void CoordinatorImpl::GetVmRegionsForHeapProfiler(
    const std::vector<base::ProcessId>& pids,
    GetVmRegionsForHeapProfilerCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  uint64_t dump_guid = ++next_dump_id_;
  std::unique_ptr<QueuedVmRegionRequest> request =
      std::make_unique<QueuedVmRegionRequest>(dump_guid, std::move(callback));
  in_progress_vm_region_requests_[dump_guid] = std::move(request);

  std::vector<QueuedRequestDispatcher::ClientInfo> clients;
  for (const auto& kv : clients_) {
    auto client_identity = kv.second->identity;
    const base::ProcessId pid = GetProcessIdForClientIdentity(client_identity);
    clients.emplace_back(kv.second->client.get(), pid, kv.second->process_type);
  }

  QueuedVmRegionRequest* request_ptr =
      in_progress_vm_region_requests_[dump_guid].get();
  auto os_callback =
      base::BindRepeating(&CoordinatorImpl::OnOSMemoryDumpForVMRegions,
                          base::Unretained(this), dump_guid);
  QueuedRequestDispatcher::SetUpAndDispatchVmRegionRequest(request_ptr, clients,
                                                           pids, os_callback);
  FinalizeVmRegionDumpIfAllManagersReplied(dump_guid);
}

void CoordinatorImpl::RegisterClientProcess(
    mojom::ClientProcessPtr client_process_ptr,
    mojom::ProcessType process_type) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  mojom::ClientProcess* client_process = client_process_ptr.get();
  client_process_ptr.set_connection_error_handler(
      base::BindOnce(&CoordinatorImpl::UnregisterClientProcess,
                     base::Unretained(this), client_process));
  auto identity = GetClientIdentityForCurrentRequest();
  auto client_info = std::make_unique<ClientInfo>(
      std::move(identity), std::move(client_process_ptr), process_type);
  auto iterator_and_inserted =
      clients_.emplace(client_process, std::move(client_info));
  DCHECK(iterator_and_inserted.second);
}

void CoordinatorImpl::UnregisterClientProcess(
    mojom::ClientProcess* client_process) {
  QueuedRequest* request = GetCurrentRequest();
  if (request != nullptr) {
    // Check if we are waiting for an ack from this client process.
    auto it = request->pending_responses.begin();
    while (it != request->pending_responses.end()) {
      // The calls to On*MemoryDumpResponse below, if executed, will delete the
      // element under the iterator which invalidates it. To avoid this we
      // increment the iterator in advance while keeping a reference to the
      // current element.
      std::set<QueuedRequest::PendingResponse>::iterator current = it++;
      if (current->client != client_process)
        continue;
      RemovePendingResponse(client_process, current->type);
      request->failed_memory_dump_count++;
    }
    FinalizeGlobalMemoryDumpIfAllManagersReplied();
  }

  for (auto& pair : in_progress_vm_region_requests_) {
    QueuedVmRegionRequest* request = pair.second.get();
    auto it = request->pending_responses.begin();
    while (it != request->pending_responses.end()) {
      auto current = it++;
      if (*current == client_process) {
        request->pending_responses.erase(current);
      }
    }
  }

  // Try to finalize all outstanding vm region requests.
  for (auto& pair : in_progress_vm_region_requests_) {
    // PostTask to avoid re-entrancy or modification of data-structure during
    // iteration.
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &CoordinatorImpl::FinalizeVmRegionDumpIfAllManagersReplied,
            base::Unretained(this), pair.second->dump_guid));
  }

  size_t num_deleted = clients_.erase(client_process);
  DCHECK(num_deleted == 1);
}

void CoordinatorImpl::RequestGlobalMemoryDumpInternal(
    const QueuedRequest::Args& args,
    RequestGlobalMemoryDumpInternalCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  UMA_HISTOGRAM_COUNTS_1000("Memory.Experimental.Debug.GlobalDumpQueueLength",
                            queued_memory_dump_requests_.size());

  bool another_dump_is_queued = !queued_memory_dump_requests_.empty();

  // If this is a periodic or peak memory dump request and there already is
  // another request in the queue with the same level of detail, there's no
  // point in enqueuing this request.
  if (another_dump_is_queued &&
      (args.dump_type == MemoryDumpType::PERIODIC_INTERVAL ||
       args.dump_type == MemoryDumpType::PEAK_MEMORY_USAGE)) {
    for (const auto& request : queued_memory_dump_requests_) {
      if (request.args.level_of_detail == args.level_of_detail) {
        VLOG(1) << "RequestGlobalMemoryDump("
                << base::trace_event::MemoryDumpTypeToString(args.dump_type)
                << ") skipped because another dump request with the same "
                   "level of detail ("
                << base::trace_event::MemoryDumpLevelOfDetailToString(
                       args.level_of_detail)
                << ") is already in the queue";
        std::move(callback).Run(false /* success */, 0 /* dump_guid */,
                                nullptr /* global_memory_dump */);
        return;
      }
    }
  }

  queued_memory_dump_requests_.emplace_back(args, ++next_dump_id_,
                                            std::move(callback));

  // If another dump is already in queued, this dump will automatically be
  // scheduled when the other dump finishes.
  if (another_dump_is_queued)
    return;

  PerformNextQueuedGlobalMemoryDump();
}

void CoordinatorImpl::OnQueuedRequestTimedOut(uint64_t dump_guid) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  QueuedRequest* request = GetCurrentRequest();

  // TODO(lalitm): add metrics for how often this happens.

  // Only consider the current request timed out if we fired off this
  // delayed callback in association with this request.
  if (!request || request->dump_guid != dump_guid)
    return;

  // Fail all remaining dumps being waited upon and clear the vector.
  request->failed_memory_dump_count += request->pending_responses.size();
  request->pending_responses.clear();

  // Callback the consumer of the service.
  FinalizeGlobalMemoryDumpIfAllManagersReplied();
}

void CoordinatorImpl::OnHeapDumpTimeOut(uint64_t dump_guid) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  QueuedRequest* request = GetCurrentRequest();

  // TODO(lalitm): add metrics for how often this happens.

  // Only consider the current request timed out if we fired off this
  // delayed callback in association with this request.
  if (!request || request->dump_guid != dump_guid)
    return;

  // Fail all remaining dumps being waited upon and clear the vector.
  if (request->heap_dump_in_progress) {
    request->heap_dump_in_progress = false;
    FinalizeGlobalMemoryDumpIfAllManagersReplied();
  }
}

void CoordinatorImpl::PerformNextQueuedGlobalMemoryDump() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  QueuedRequest* request = GetCurrentRequest();

  if (request == nullptr)
    return;

  std::vector<QueuedRequestDispatcher::ClientInfo> clients;
  for (const auto& kv : clients_) {
    auto client_identity = kv.second->identity;
    const base::ProcessId pid = GetProcessIdForClientIdentity(client_identity);
    if (pid == base::kNullProcessId) {
      VLOG(1) << "Couldn't find a PID for client \"" << client_identity.name()
              << "." << client_identity.instance() << "\"";
      continue;
    }
    clients.emplace_back(kv.second->client.get(), pid, kv.second->process_type);
  }

  auto chrome_callback = base::Bind(
      &CoordinatorImpl::OnChromeMemoryDumpResponse, base::Unretained(this));
  auto os_callback = base::Bind(&CoordinatorImpl::OnOSMemoryDumpResponse,
                                base::Unretained(this), request->dump_guid);
  QueuedRequestDispatcher::SetUpAndDispatch(request, clients, chrome_callback,
                                            os_callback);

  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&CoordinatorImpl::OnQueuedRequestTimedOut,
                     base::Unretained(this), request->dump_guid),
      client_process_timeout_);

  if (request->args.add_to_trace && heap_profiler_) {
    request->heap_dump_in_progress = true;

    // |IsArgumentFilterEnabled| is the round-about way of asking to anonymize
    // the trace. The only way that PII gets leaked is if the full path is
    // emitted for mapped files. Passing |strip_path_from_mapped_files|
    // is all that is necessary to anonymize the trace.
    bool strip_path_from_mapped_files =
        base::trace_event::TraceLog::GetInstance()
            ->GetCurrentTraceConfig()
            .IsArgumentFilterEnabled();
    heap_profiler_->DumpProcessesForTracing(
        strip_path_from_mapped_files,
            base::BindRepeating(&CoordinatorImpl::OnDumpProcessesForTracing,
                           base::Unretained(this), request->dump_guid));

    base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce(&CoordinatorImpl::OnHeapDumpTimeOut,
                       base::Unretained(this), request->dump_guid),
        kHeapDumpTimeout);
  }

  // Run the callback in case there are no client processes registered.
  FinalizeGlobalMemoryDumpIfAllManagersReplied();
}

QueuedRequest* CoordinatorImpl::GetCurrentRequest() {
  if (queued_memory_dump_requests_.empty()) {
    return nullptr;
  }
  return &queued_memory_dump_requests_.front();
}

void CoordinatorImpl::OnChromeMemoryDumpResponse(
    mojom::ClientProcess* client,
    bool success,
    uint64_t dump_guid,
    std::unique_ptr<base::trace_event::ProcessMemoryDump> chrome_memory_dump) {
  using ResponseType = QueuedRequest::PendingResponse::Type;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  QueuedRequest* request = GetCurrentRequest();
  if (request == nullptr || request->dump_guid != dump_guid) {
    return;
  }

  RemovePendingResponse(client, ResponseType::kChromeDump);

  if (!clients_.count(client)) {
    VLOG(1) << "Received a memory dump response from an unregistered client";
    return;
  }
  auto* response = &request->responses[client];
  response->chrome_dump = std::move(chrome_memory_dump);

  if (!success) {
    request->failed_memory_dump_count++;
    VLOG(1) << "RequestGlobalMemoryDump() FAIL: NACK from client process";
  }

  FinalizeGlobalMemoryDumpIfAllManagersReplied();
}

void CoordinatorImpl::OnOSMemoryDumpResponse(uint64_t dump_guid,
                                             mojom::ClientProcess* client,
                                             bool success,
                                             OSMemDumpMap os_dumps) {
  using ResponseType = QueuedRequest::PendingResponse::Type;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  QueuedRequest* request = GetCurrentRequest();
  if (request == nullptr || request->dump_guid != dump_guid) {
    return;
  }

  RemovePendingResponse(client, ResponseType::kOSDump);

  if (!clients_.count(client)) {
    VLOG(1) << "Received a memory dump response from an unregistered client";
    return;
  }

  request->responses[client].os_dumps = std::move(os_dumps);

  if (!success) {
    request->failed_memory_dump_count++;
    VLOG(1) << "RequestGlobalMemoryDump() FAIL: NACK from client process";
  }

  FinalizeGlobalMemoryDumpIfAllManagersReplied();
}

void CoordinatorImpl::OnOSMemoryDumpForVMRegions(uint64_t dump_guid,
                                                 mojom::ClientProcess* client,
                                                 bool success,
                                                 OSMemDumpMap os_dumps) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto request_it = in_progress_vm_region_requests_.find(dump_guid);
  DCHECK(request_it != in_progress_vm_region_requests_.end());

  QueuedVmRegionRequest* request = request_it->second.get();
  auto it = request->pending_responses.find(client);
  DCHECK(it != request->pending_responses.end());
  request->pending_responses.erase(it);
  request->responses[client].os_dumps = std::move(os_dumps);

  FinalizeVmRegionDumpIfAllManagersReplied(request->dump_guid);
}

void CoordinatorImpl::FinalizeVmRegionDumpIfAllManagersReplied(
    uint64_t dump_guid) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  auto it = in_progress_vm_region_requests_.find(dump_guid);
  if (it == in_progress_vm_region_requests_.end())
    return;

  if (!it->second->pending_responses.empty())
    return;

  QueuedRequestDispatcher::VmRegions results =
      QueuedRequestDispatcher::FinalizeVmRegionRequest(it->second.get());
  std::move(it->second->callback).Run(std::move(results));
  in_progress_vm_region_requests_.erase(it);
}

void CoordinatorImpl::OnDumpProcessesForTracing(
    uint64_t dump_guid,
    std::vector<mojom::SharedBufferWithSizePtr> buffers) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  QueuedRequest* request = GetCurrentRequest();
  if (!request || request->dump_guid != dump_guid) {
    return;
  }

  request->heap_dump_in_progress = false;

  for (auto& buffer_ptr : buffers) {
    mojo::ScopedSharedBufferHandle& buffer = buffer_ptr->buffer;
    uint32_t size = buffer_ptr->size;

    if (!buffer->is_valid())
      continue;

    mojo::ScopedSharedBufferMapping mapping = buffer->Map(size);
    if (!mapping) {
      DLOG(ERROR) << "Failed to map buffer";
      continue;
    }

    const char* char_buffer = static_cast<const char*>(mapping.get());
    std::string json(char_buffer, char_buffer + size);

    const int kTraceEventNumArgs = 1;
    const char* const kTraceEventArgNames[] = {"dumps"};
    const unsigned char kTraceEventArgTypes[] = {TRACE_VALUE_TYPE_CONVERTABLE};
    std::unique_ptr<base::trace_event::ConvertableToTraceFormat> wrapper(
        new StringWrapper(std::move(json)));

    // Using the same id merges all of the heap dumps into a single detailed
    // dump node in the UI.
    TRACE_EVENT_API_ADD_TRACE_EVENT_WITH_PROCESS_ID(
        TRACE_EVENT_PHASE_MEMORY_DUMP,
        base::trace_event::TraceLog::GetCategoryGroupEnabled(
            base::trace_event::MemoryDumpManager::kTraceCategory),
        "periodic_interval", trace_event_internal::kGlobalScope, dump_guid,
        buffer_ptr->pid, kTraceEventNumArgs, kTraceEventArgNames,
        kTraceEventArgTypes, nullptr /* arg_values */, &wrapper,
        TRACE_EVENT_FLAG_HAS_ID);
  }

  FinalizeGlobalMemoryDumpIfAllManagersReplied();
}

void CoordinatorImpl::RemovePendingResponse(
    mojom::ClientProcess* client,
    QueuedRequest::PendingResponse::Type type) {
  QueuedRequest* request = GetCurrentRequest();
  if (request == nullptr) {
    NOTREACHED() << "No current dump request.";
    return;
  }
  auto it = request->pending_responses.find({client, type});
  if (it == request->pending_responses.end()) {
    VLOG(1) << "Unexpected memory dump response";
    return;
  }
  request->pending_responses.erase(it);
}

void CoordinatorImpl::FinalizeGlobalMemoryDumpIfAllManagersReplied() {
  TRACE_EVENT0(base::trace_event::MemoryDumpManager::kTraceCategory,
               "GlobalMemoryDump.Computation");
  DCHECK(!queued_memory_dump_requests_.empty());

  QueuedRequest* request = &queued_memory_dump_requests_.front();
  if (!request->dump_in_progress || request->pending_responses.size() > 0 ||
      request->heap_dump_in_progress) {
    return;
  }

  QueuedRequestDispatcher::Finalize(request, tracing_observer_.get());

  queued_memory_dump_requests_.pop_front();
  request = nullptr;

  // Schedule the next queued dump (if applicable).
  if (!queued_memory_dump_requests_.empty()) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&CoordinatorImpl::PerformNextQueuedGlobalMemoryDump,
                       base::Unretained(this)));
  }
}

CoordinatorImpl::ClientInfo::ClientInfo(
    const service_manager::Identity& identity,
    mojom::ClientProcessPtr client,
    mojom::ProcessType process_type)
    : identity(identity),
      client(std::move(client)),
      process_type(process_type) {}
CoordinatorImpl::ClientInfo::~ClientInfo() {}

}  // namespace memory_instrumentation
