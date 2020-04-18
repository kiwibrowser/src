// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/agent_registry.h"

#include <string>
#include <utility>

#include "base/callback_forward.h"
#include "base/logging.h"
#include "base/threading/thread_checker.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace {
tracing::AgentRegistry* g_agent_registry;
}

namespace tracing {

AgentRegistry::AgentEntry::AgentEntry(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref,
    size_t id,
    AgentRegistry* agent_registry,
    mojom::AgentPtr agent,
    const std::string& label,
    mojom::TraceDataType type,
    bool supports_explicit_clock_sync)
    : id_(id),
      agent_registry_(agent_registry),
      agent_(std::move(agent)),
      label_(label),
      type_(type),
      supports_explicit_clock_sync_(supports_explicit_clock_sync),
      is_tracing_(false),
      service_ref_(std::move(service_ref)) {
  DCHECK(!label.empty());
  agent_.set_connection_error_handler(base::BindRepeating(
      &AgentRegistry::AgentEntry::OnConnectionError, AsWeakPtr()));
}

AgentRegistry::AgentEntry::~AgentEntry() = default;

void AgentRegistry::AgentEntry::AddDisconnectClosure(
    const void* closure_name,
    base::OnceClosure closure) {
  DCHECK_EQ(0u, closures_.count(closure_name));
  closures_[closure_name] = std::move(closure);
}

bool AgentRegistry::AgentEntry::RemoveDisconnectClosure(
    const void* closure_name) {
  return closures_.erase(closure_name) > 0;
}

bool AgentRegistry::AgentEntry::HasDisconnectClosure(const void* closure_name) {
  return closures_.count(closure_name) > 0;
}

void AgentRegistry::AgentEntry::OnConnectionError() {
  // Run disconnect closures if there is any. We should mark |key_value.second|
  // as movable so that the version of |Run| that takes an rvalue reference is
  // selected not the version that takes a const reference. The former is for
  // once callbacks and the latter is for repeating callbacks.
  while (!closures_.empty()) {
    auto iterator = closures_.begin();
    auto callback = std::move(iterator->second);
    const size_t closures_size = closures_.size();
    std::move(callback).Run();
    // Verify that the callback has removed itself.
    DCHECK_EQ(1u, closures_size - closures_.size());
  }
  agent_registry_->UnregisterAgent(id_);
}

// static
AgentRegistry* AgentRegistry::GetInstance() {
  return g_agent_registry;
}

AgentRegistry::AgentRegistry(
    service_manager::ServiceContextRefFactory* service_ref_factory)
    : service_ref_factory_(service_ref_factory) {
  DCHECK(!g_agent_registry);
  g_agent_registry = this;
}

AgentRegistry::~AgentRegistry() {
  // For testing only.
  g_agent_registry = nullptr;
}

void AgentRegistry::BindAgentRegistryRequest(
    mojom::AgentRegistryRequest request,
    const service_manager::BindSourceInfo& source_info) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  bindings_.AddBinding(this, std::move(request), source_info.identity);
}

void AgentRegistry::SetAgentInitializationCallback(
    const AgentInitializationCallback& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  agent_initialization_callback_ = callback;
  ForAllAgents([this](AgentEntry* agent_entry) {
    agent_initialization_callback_.Run(agent_entry);
  });
}

void AgentRegistry::RemoveAgentInitializationCallback() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  agent_initialization_callback_.Reset();
}

bool AgentRegistry::HasDisconnectClosure(const void* closure_name) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  for (const auto& key_value : agents_) {
    if (key_value.second->HasDisconnectClosure(closure_name))
      return true;
  }
  return false;
}

void AgentRegistry::RegisterAgent(mojom::AgentPtr agent,
                                  const std::string& label,
                                  mojom::TraceDataType type,
                                  bool supports_explicit_clock_sync) {
  auto id = next_agent_id_++;
  auto entry = std::make_unique<AgentEntry>(service_ref_factory_->CreateRef(),
                                            id, this, std::move(agent), label,
                                            type, supports_explicit_clock_sync);
  if (!agent_initialization_callback_.is_null())
    agent_initialization_callback_.Run(entry.get());
  auto result = agents_.insert(std::make_pair(id, std::move(entry)));
  DCHECK(result.second);
}

void AgentRegistry::UnregisterAgent(size_t agent_id) {
  size_t num_deleted = agents_.erase(agent_id);
  DCHECK_EQ(1u, num_deleted);
}

}  // namespace tracing
