// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_TRACING_SERVICE_H_
#define SERVICES_TRACING_TRACING_SERVICE_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "services/tracing/agent_registry.h"
#include "services/tracing/coordinator.h"

namespace tracing {

class PerfettoTracingCoordinator;
class PerfettoService;

class TracingService : public service_manager::Service {
 public:
  TracingService();
  ~TracingService() override;

  // service_manager::Service:
  // Factory function for use as an embedded service.
  static std::unique_ptr<service_manager::Service> Create();

  // service_manager::Service:
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  service_manager::ServiceContextRefFactory* ref_factory() {
    return ref_factory_.get();
  }

 private:
  service_manager::BinderRegistryWithArgs<
      const service_manager::BindSourceInfo&>
      registry_;
  std::unique_ptr<tracing::AgentRegistry> tracing_agent_registry_;
  std::unique_ptr<Coordinator> tracing_coordinator_;
  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;

#if defined(OS_ANDROID) || defined(OS_LINUX) || defined(OS_MACOSX)
  std::unique_ptr<tracing::PerfettoService> perfetto_service_;
  std::unique_ptr<PerfettoTracingCoordinator> perfetto_tracing_coordinator_;
#endif

  // WeakPtrFactory members should always come last so WeakPtrs are destructed
  // before other members.
  base::WeakPtrFactory<TracingService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TracingService);
};

}  // namespace tracing

#endif  // SERVICES_TRACING_TRACING_SERVICE_H_
