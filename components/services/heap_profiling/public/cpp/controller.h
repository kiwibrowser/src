// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_CONTROLLER_H_
#define COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_CONTROLLER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "components/services/heap_profiling/public/mojom/heap_profiling_client.mojom.h"
#include "components/services/heap_profiling/public/mojom/heap_profiling_service.mojom.h"

namespace service_manager {
class Connector;
}

namespace heap_profiling {

// This class is responsible for
//   * Starting the Heap Profiling Service
//   * Hooking up clients to the service
//   * Getting information about profiled clients
//
// This class is sequence-affine. The public non-getter methods must be called
// from a single sequence. Getters return const members set during the
// constructor, so they can be called from any sequence.
//
// This class must be used from a privileged process, as it needs to be able to
// create OS pipes.
class Controller {
 public:
  // |connector| is used to connect to other services.
  // |stack_mode| describes the type of metadata to record for each allocation.
  // A |sampling_rate| of 1 indicates that all allocations should be recorded.
  // A |sampling_rate| greater than 1 describes the Poisson Process sampling
  // interval. If |sampling_rate| is N, then on average, an allocation will be
  // recorded every N bytes of allocated objects.
  //
  // Note: The name |sampling_rate| is a bit confusing. A higher sampling rate
  // causes there to be fewer samples taken. This probably should have been
  // named |sampling_interval|.
  Controller(std::unique_ptr<service_manager::Connector> connector,
             mojom::StackMode stack_mode,
             uint32_t sampling_rate);
  ~Controller();

  // Starts Heap Profiling for the client.
  void StartProfilingClient(mojom::ProfilingClientPtr client,
                            base::ProcessId pid,
                            mojom::ProcessType);

  uint32_t sampling_rate() const { return sampling_rate_; }

  // Forwards to HeapProfilingService.
  using GetProfiledPidsCallback =
      base::OnceCallback<void(const std::vector<base::ProcessId>&)>;
  void GetProfiledPids(GetProfiledPidsCallback callback);
  void SetKeepSmallAllocations(bool keep_small_allocations);

  // Careful! WeakPtrs are also sequence-affine.
  // This method must be called from the same sequence the instance is bound to.
  base::WeakPtr<Controller> GetWeakPtr();

  service_manager::Connector* GetConnector();

 private:
  std::unique_ptr<service_manager::Connector> connector_;
  mojom::ProfilingServicePtr heap_profiling_service_;

  // The same sampling rate and stack mode is used for each client.
  const uint32_t sampling_rate_ = 1;
  const mojom::StackMode stack_mode_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<Controller> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(Controller);
};

}  // namespace heap_profiling

#endif  // COMPONENTS_SERVICES_HEAP_PROFILING_PUBLIC_CPP_CONTROLLER_H_
