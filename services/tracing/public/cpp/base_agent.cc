// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/public/cpp/base_agent.h"

#include <utility>

#include "services/service_manager/public/cpp/connector.h"
#include "services/tracing/public/mojom/constants.mojom.h"

namespace tracing {

BaseAgent::BaseAgent(service_manager::Connector* connector,
                     const std::string& label,
                     mojom::TraceDataType type,
                     bool supports_explicit_clock_sync)
    : binding_(this) {
  // |connector| can be null in tests.
  if (!connector)
    return;
  tracing::mojom::AgentRegistryPtr agent_registry;
  connector->BindInterface(tracing::mojom::kServiceName, &agent_registry);

  tracing::mojom::AgentPtr agent;
  binding_.Bind(mojo::MakeRequest(&agent));
  agent_registry->RegisterAgent(std::move(agent), label, type,
                                supports_explicit_clock_sync);
}

BaseAgent::~BaseAgent() = default;

void BaseAgent::StartTracing(const std::string& config,
                             base::TimeTicks coordinator_time,
                             Agent::StartTracingCallback callback) {
  std::move(callback).Run(true /* success */);
}

void BaseAgent::StopAndFlush(tracing::mojom::RecorderPtr recorder) {}

void BaseAgent::RequestClockSyncMarker(
    const std::string& sync_id,
    Agent::RequestClockSyncMarkerCallback callback) {
  NOTREACHED() << "The agent claims to support explicit clock sync but does "
               << "not override BaseAgent::RequestClockSyncMarker()";
}

void BaseAgent::GetCategories(Agent::GetCategoriesCallback callback) {
  std::move(callback).Run("" /* categories */);
}

void BaseAgent::RequestBufferStatus(
    Agent::RequestBufferStatusCallback callback) {
  std::move(callback).Run(0 /* capacity */, 0 /* count */);
}

}  // namespace tracing
