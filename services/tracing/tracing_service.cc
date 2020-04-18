// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/tracing_service.h"

#include <utility>

#include "base/timer/timer.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/tracing/agent_registry.h"
#include "services/tracing/coordinator.h"
#include "services/tracing/public/cpp/tracing_features.h"

#if defined(OS_ANDROID) || defined(OS_LINUX) || defined(OS_MACOSX)
#include "services/tracing/perfetto/perfetto_service.h"
#include "services/tracing/perfetto/perfetto_tracing_coordinator.h"
#endif

namespace tracing {

std::unique_ptr<service_manager::Service> TracingService::Create() {
  return std::make_unique<TracingService>();
}

TracingService::TracingService() : weak_factory_(this) {}

TracingService::~TracingService() {
#if defined(OS_ANDROID) || defined(OS_LINUX) || defined(OS_MACOSX)
  if (perfetto_tracing_coordinator_) {
    PerfettoTracingCoordinator::DestroyOnSequence(
        std::move(perfetto_tracing_coordinator_));
  }

  if (perfetto_service_) {
    PerfettoService::DestroyOnSequence(std::move(perfetto_service_));
  }
#endif
}

void TracingService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      context()->CreateQuitClosure()));

  tracing_agent_registry_ = std::make_unique<AgentRegistry>(ref_factory_.get());
  registry_.AddInterface(
      base::BindRepeating(&AgentRegistry::BindAgentRegistryRequest,
                          base::Unretained(tracing_agent_registry_.get())));

  if (TracingUsesPerfettoBackend()) {
#if defined(OS_ANDROID) || defined(OS_LINUX) || defined(OS_MACOSX)
    perfetto_service_ = std::make_unique<tracing::PerfettoService>();
    registry_.AddInterface(
        base::BindRepeating(&tracing::PerfettoService::BindRequest,
                            base::Unretained(perfetto_service_.get())));

    auto perfetto_coordinator = std::make_unique<PerfettoTracingCoordinator>();
    registry_.AddInterface(
        base::BindRepeating(&PerfettoTracingCoordinator::BindCoordinatorRequest,
                            base::Unretained(perfetto_coordinator.get())));
    perfetto_tracing_coordinator_ = std::move(perfetto_coordinator);
#else
    LOG(FATAL) << "Perfetto is not yet available for this platform.";
#endif
  } else {
    auto tracing_coordinator =
        std::make_unique<Coordinator>(ref_factory_.get());
    registry_.AddInterface(
        base::BindRepeating(&Coordinator::BindCoordinatorRequest,
                            base::Unretained(tracing_coordinator.get())));
    tracing_coordinator_ = std::move(tracing_coordinator);
  }
}

void TracingService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe),
                          source_info);
}

}  // namespace tracing
