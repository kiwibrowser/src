// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/heap_profiling/public/cpp/controller.h"

#include "components/services/heap_profiling/public/cpp/sender_pipe.h"
#include "components/services/heap_profiling/public/cpp/settings.h"
#include "components/services/heap_profiling/public/mojom/constants.mojom.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/resource_coordinator/public/mojom/memory_instrumentation/memory_instrumentation.mojom.h"
#include "services/resource_coordinator/public/mojom/service_constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace heap_profiling {

Controller::Controller(std::unique_ptr<service_manager::Connector> connector,
                       mojom::StackMode stack_mode,
                       uint32_t sampling_rate)
    : connector_(std::move(connector)),
      sampling_rate_(sampling_rate),
      stack_mode_(stack_mode),
      weak_factory_(this) {
  DCHECK_NE(sampling_rate, 0u);

  // Start the Heap Profiling service.
  connector_->BindInterface(mojom::kServiceName, &heap_profiling_service_);

  // Pass state from command line flags to Heap Profiling service.
  SetKeepSmallAllocations(ShouldKeepSmallAllocations());

  // Grab a HeapProfiler InterfacePtr and pass that to memory instrumentation.
  memory_instrumentation::mojom::HeapProfilerPtr heap_profiler;
  connector_->BindInterface(mojom::kServiceName, &heap_profiler);

  memory_instrumentation::mojom::CoordinatorPtr coordinator;
  connector_->BindInterface(resource_coordinator::mojom::kServiceName,
                            &coordinator);
  coordinator->RegisterHeapProfiler(std::move(heap_profiler));
}

Controller::~Controller() = default;

void Controller::StartProfilingClient(mojom::ProfilingClientPtr client,
                                      base::ProcessId pid,
                                      mojom::ProcessType process_type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  SenderPipe::PipePair pipes;

  mojom::ProfilingParamsPtr params = mojom::ProfilingParams::New();
  params->sampling_rate = sampling_rate_;
  params->sender_pipe =
      mojo::WrapPlatformFile(pipes.PassSender().release().handle);
  params->stack_mode = stack_mode_;
  heap_profiling_service_->AddProfilingClient(
      pid, std::move(client),
      mojo::WrapPlatformFile(pipes.PassReceiver().release().handle),
      process_type, std::move(params));
}

void Controller::GetProfiledPids(GetProfiledPidsCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  heap_profiling_service_->GetProfiledPids(std::move(callback));
}

void Controller::SetKeepSmallAllocations(bool keep_small_allocations) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  heap_profiling_service_->SetKeepSmallAllocations(keep_small_allocations);
}

base::WeakPtr<Controller> Controller::GetWeakPtr() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return weak_factory_.GetWeakPtr();
}

service_manager::Connector* Controller::GetConnector() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return connector_.get();
}

}  // namespace heap_profiling
