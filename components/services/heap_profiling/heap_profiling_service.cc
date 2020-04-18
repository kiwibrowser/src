// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/heap_profiling_service.h"

#include "base/logging.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "services/resource_coordinator/public/mojom/memory_instrumentation/memory_instrumentation.mojom.h"
#include "services/resource_coordinator/public/mojom/service_constants.mojom.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace heap_profiling {

HeapProfilingService::HeapProfilingService()
    : binding_(this), heap_profiler_binding_(this), weak_factory_(this) {}

HeapProfilingService::~HeapProfilingService() {}

std::unique_ptr<service_manager::Service>
HeapProfilingService::CreateService() {
  return std::make_unique<HeapProfilingService>();
}

void HeapProfilingService::OnStart() {
  registry_.AddInterface(
      base::Bind(&HeapProfilingService::OnProfilingServiceRequest,
                 base::Unretained(this)));
  registry_.AddInterface(base::Bind(
      &HeapProfilingService::OnHeapProfilerRequest, base::Unretained(this)));
}

void HeapProfilingService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void HeapProfilingService::OnProfilingServiceRequest(
    mojom::ProfilingServiceRequest request) {
  binding_.Bind(std::move(request));
}

void HeapProfilingService::OnHeapProfilerRequest(
    memory_instrumentation::mojom::HeapProfilerRequest request) {
  heap_profiler_binding_.Bind(std::move(request));
}

void HeapProfilingService::AddProfilingClient(
    base::ProcessId pid,
    mojom::ProfilingClientPtr client,
    mojo::ScopedHandle pipe_receiver,
    mojom::ProcessType process_type,
    mojom::ProfilingParamsPtr params) {
  if (params->sampling_rate == 0)
    params->sampling_rate = 1;
  connection_manager_.OnNewConnection(pid, std::move(client),
                                      std::move(pipe_receiver), process_type,
                                      std::move(params));
}

void HeapProfilingService::SetKeepSmallAllocations(
    bool keep_small_allocations) {
  keep_small_allocations_ = keep_small_allocations;
}

void HeapProfilingService::GetProfiledPids(GetProfiledPidsCallback callback) {
  std::move(callback).Run(connection_manager_.GetConnectionPids());
}

void HeapProfilingService::DumpProcessesForTracing(
    bool strip_path_from_mapped_files,
    DumpProcessesForTracingCallback callback) {
  if (!helper_) {
    context()->connector()->BindInterface(
        resource_coordinator::mojom::kServiceName, &helper_);
  }

  std::vector<base::ProcessId> pids =
      connection_manager_.GetConnectionPidsThatNeedVmRegions();
  if (pids.empty()) {
    connection_manager_.DumpProcessesForTracing(
        keep_small_allocations_, strip_path_from_mapped_files,
        std::move(callback), VmRegions());
  } else {
    // Need a memory map to make sense of the dump. The dump will be triggered
    // in the memory map global dump callback.
    helper_->GetVmRegionsForHeapProfiler(
        pids,
        base::BindOnce(&HeapProfilingService::
                           OnGetVmRegionsCompleteForDumpProcessesForTracing,
                       weak_factory_.GetWeakPtr(), strip_path_from_mapped_files,
                       std::move(callback)));
  }
}

void HeapProfilingService::OnGetVmRegionsCompleteForDumpProcessesForTracing(
    bool strip_path_from_mapped_files,
    DumpProcessesForTracingCallback callback,
    VmRegions vm_regions) {
  connection_manager_.DumpProcessesForTracing(
      keep_small_allocations_, strip_path_from_mapped_files,
      std::move(callback), std::move(vm_regions));
}

}  // namespace heap_profiling
