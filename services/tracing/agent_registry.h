// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_AGENT_REGISTRY_H_
#define SERVICES_TRACING_AGENT_REGISTRY_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/identity.h"
#include "services/tracing/public/mojom/tracing.mojom.h"

namespace service_manager {
struct BindSourceInfo;
class ServiceContextRef;
class ServiceContextRefFactory;
}  // namespace service_manager

namespace tracing {

class AgentRegistry : public mojom::AgentRegistry {
 public:
  class AgentEntry : public base::SupportsWeakPtr<AgentEntry> {
   public:
    AgentEntry(std::unique_ptr<service_manager::ServiceContextRef> service_ref,
               size_t id,
               AgentRegistry* agent_registry,
               mojom::AgentPtr agent,
               const std::string& label,
               mojom::TraceDataType type,
               bool supports_explicit_clock_sync);
    ~AgentEntry();

    void AddDisconnectClosure(const void* closure_name,
                              base::OnceClosure closure);
    bool RemoveDisconnectClosure(const void* closure_name);
    bool HasDisconnectClosure(const void* closure_name);
    size_t num_disconnect_closures_for_testing() const {
      return closures_.size();
    }

    mojom::Agent* agent() const { return agent_.get(); }
    const std::string& label() const { return label_; }
    mojom::TraceDataType type() const { return type_; }
    bool supports_explicit_clock_sync() const {
      return supports_explicit_clock_sync_;
    }
    bool is_tracing() const { return is_tracing_; }
    void set_is_tracing(bool is_tracing) { is_tracing_ = is_tracing; }

   private:
    void OnConnectionError();

    const size_t id_;
    AgentRegistry* agent_registry_;
    mojom::AgentPtr agent_;
    const std::string label_;
    const mojom::TraceDataType type_;
    const bool supports_explicit_clock_sync_;
    std::map<const void*, base::OnceClosure> closures_;
    bool is_tracing_;
    std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

    DISALLOW_COPY_AND_ASSIGN(AgentEntry);
  };

  // A function to be run for every agent that registers itself.
  using AgentInitializationCallback =
      base::RepeatingCallback<void(AgentEntry*)>;

  static AgentRegistry* GetInstance();

  explicit AgentRegistry(
      service_manager::ServiceContextRefFactory* service_ref_factory);

  void BindAgentRegistryRequest(
      mojom::AgentRegistryRequest request,
      const service_manager::BindSourceInfo& source_info);
  void SetAgentInitializationCallback(
      const AgentInitializationCallback& callback);
  void RemoveAgentInitializationCallback();
  bool HasDisconnectClosure(const void* closure_name);

  template <typename FunctionType>
  void ForAllAgents(FunctionType function) {
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
    for (const auto& key_value : agents_) {
      function(key_value.second.get());
    }
  }

 private:
  friend std::default_delete<AgentRegistry>;
  friend class AgentRegistryTest;  // For testing.
  friend class CoordinatorTest;    // For testing.

  ~AgentRegistry() override;

  // mojom::AgentRegistry
  void RegisterAgent(mojom::AgentPtr agent,
                     const std::string& label,
                     mojom::TraceDataType type,
                     bool supports_explicit_clock_sync) override;

  void UnregisterAgent(size_t agent_id);

  mojo::BindingSet<mojom::AgentRegistry, service_manager::Identity> bindings_;
  size_t next_agent_id_ = 0;
  std::map<size_t, std::unique_ptr<AgentEntry>> agents_;
  AgentInitializationCallback agent_initialization_callback_;
  service_manager::ServiceContextRefFactory* service_ref_factory_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(AgentRegistry);
};

}  // namespace tracing

#endif  // SERVICES_TRACING_AGENT_REGISTRY_H_
