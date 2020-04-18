// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/agent_registry.h"

#include <memory>
#include <string>
#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "services/tracing/public/mojom/tracing.mojom.h"
#include "services/tracing/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tracing {

class AgentRegistryTest : public testing::Test {
 public:
  AgentRegistryTest() : service_ref_factory_(base::DoNothing()) {}

  void SetUp() override {
    message_loop_.reset(new base::MessageLoop());
    registry_.reset(new AgentRegistry(&service_ref_factory_));
  }

  void TearDown() override {
    registry_.reset();
    message_loop_.reset();
  }

  void RegisterAgent(mojom::AgentPtr agent,
                     const std::string& label,
                     mojom::TraceDataType type,
                     bool supports_explicit_clock_sync) {
    registry_->RegisterAgent(std::move(agent), label, type,
                             supports_explicit_clock_sync);
  }

  void RegisterAgent(mojom::AgentPtr agent) {
    registry_->RegisterAgent(std::move(agent), "label",
                             mojom::TraceDataType::ARRAY, false);
  }

  std::unique_ptr<AgentRegistry> registry_;
  service_manager::ServiceContextRefFactory service_ref_factory_;

 private:
  std::unique_ptr<base::MessageLoop> message_loop_;
};

TEST_F(AgentRegistryTest, RegisterAgent) {
  MockAgent agent1;
  RegisterAgent(agent1.CreateAgentPtr(), "TraceEvent",
                mojom::TraceDataType::ARRAY, false);
  size_t num_agents = 0;
  registry_->ForAllAgents([&num_agents](AgentRegistry::AgentEntry* entry) {
    num_agents++;
    EXPECT_EQ("TraceEvent", entry->label());
    EXPECT_EQ(mojom::TraceDataType::ARRAY, entry->type());
    EXPECT_FALSE(entry->supports_explicit_clock_sync());
  });
  EXPECT_EQ(1u, num_agents);

  MockAgent agent2;
  RegisterAgent(agent2.CreateAgentPtr(), "Power", mojom::TraceDataType::STRING,
                true);
  num_agents = 0;
  registry_->ForAllAgents([&num_agents](AgentRegistry::AgentEntry* entry) {
    num_agents++;
    // Properties of |agent1| is already verified.
    if (entry->label() == "TraceEvent")
      return;
    EXPECT_EQ("Power", entry->label());
    EXPECT_EQ(mojom::TraceDataType::STRING, entry->type());
    EXPECT_TRUE(entry->supports_explicit_clock_sync());
  });
  EXPECT_EQ(2u, num_agents);
}

TEST_F(AgentRegistryTest, UnregisterAgent) {
  base::RunLoop run_loop;
  MockAgent agent1;
  RegisterAgent(agent1.CreateAgentPtr());
  {
    MockAgent agent2;
    RegisterAgent(agent2.CreateAgentPtr());
    size_t num_agents = 0;
    registry_->ForAllAgents(
        [&num_agents](AgentRegistry::AgentEntry* entry) { num_agents++; });
    EXPECT_EQ(2u, num_agents);
  }
  run_loop.RunUntilIdle();

  // |agent2| is not alive anymore.
  size_t num_agents = 0;
  registry_->ForAllAgents(
      [&num_agents](AgentRegistry::AgentEntry* entry) { num_agents++; });
  EXPECT_EQ(1u, num_agents);
}

TEST_F(AgentRegistryTest, AgentInitialization) {
  size_t num_calls = 0;
  MockAgent agent1;
  RegisterAgent(agent1.CreateAgentPtr());
  registry_->SetAgentInitializationCallback(base::BindRepeating(
      [](size_t* num_calls, tracing::AgentRegistry::AgentEntry* entry) {
        (*num_calls)++;
      },
      base::Unretained(&num_calls)));
  // Since an agent was already registered, the callback should be run once.
  EXPECT_EQ(1u, num_calls);

  // The callback should be run on future agents, too.
  MockAgent agent2;
  RegisterAgent(agent2.CreateAgentPtr());
  EXPECT_EQ(2u, num_calls);

  // The callback should not be run on future agents if it is removed.
  registry_->RemoveAgentInitializationCallback();
  MockAgent agent3;
  RegisterAgent(agent3.CreateAgentPtr());
  EXPECT_EQ(2u, num_calls);
}

}  // namespace tracing
