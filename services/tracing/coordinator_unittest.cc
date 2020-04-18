// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/coordinator.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/data_pipe_drainer.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "services/tracing/public/mojom/tracing.mojom.h"
#include "services/tracing/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace tracing {

class CoordinatorTest : public testing::Test,
                        public mojo::DataPipeDrainer::Client {
 public:
  CoordinatorTest() : service_ref_factory_(base::DoNothing()) {}

  // testing::Test
  void SetUp() override {
    agent_registry_.reset(new AgentRegistry(&service_ref_factory_));
    coordinator_.reset(new Coordinator(&service_ref_factory_));
    output_ = "";
  }

  // testing::Test
  void TearDown() override {
    agents_.clear();
    coordinator_.reset();
    agent_registry_.reset();
  }

  // mojo::DataPipeDrainer::Client
  void OnDataAvailable(const void* data, size_t num_bytes) override {
    output_.append(static_cast<const char*>(data), num_bytes);
  }

  // mojo::DataPipeDrainer::Client
  void OnDataComplete() override { base::ResetAndReturn(&quit_closure_).Run(); }

  MockAgent* AddArrayAgent() {
    auto agent = std::make_unique<MockAgent>();
    agent_registry_->RegisterAgent(agent->CreateAgentPtr(), "traceEvents",
                                   mojom::TraceDataType::ARRAY, false);
    agents_.push_back(std::move(agent));
    return agents_.back().get();
  }

  MockAgent* AddObjectAgent() {
    auto agent = std::make_unique<MockAgent>();
    agent_registry_->RegisterAgent(agent->CreateAgentPtr(), "systemTraceEvents",
                                   mojom::TraceDataType::OBJECT, false);
    agents_.push_back(std::move(agent));
    return agents_.back().get();
  }

  MockAgent* AddStringAgent() {
    auto agent = std::make_unique<MockAgent>();
    agent_registry_->RegisterAgent(agent->CreateAgentPtr(), "battor",
                                   mojom::TraceDataType::STRING, false);
    agents_.push_back(std::move(agent));
    return agents_.back().get();
  }

  void StartTracing(std::string config,
                    bool expected_response,
                    bool stop_and_flush) {
    base::RepeatingClosure closure;
    if (stop_and_flush) {
      closure = base::BindRepeating(&CoordinatorTest::StopAndFlush,
                                    base::Unretained(this));
    }

    coordinator_->StartTracing(
        config,
        base::BindRepeating(
            [](bool expected, base::RepeatingClosure closure, bool actual) {
              EXPECT_EQ(expected, actual);
              if (!closure.is_null())
                closure.Run();
            },
            expected_response, closure));
  }

  void StartTracing(std::string config, bool expected_response) {
    StartTracing(config, expected_response, false);
  }

  void StopAndFlush() {
    mojo::DataPipe data_pipe;
    auto dummy_callback = [](base::Value metadata) {};
    coordinator_->StopAndFlush(std::move(data_pipe.producer_handle),
                               base::BindRepeating(dummy_callback));
    drainer_.reset(
        new mojo::DataPipeDrainer(this, std::move(data_pipe.consumer_handle)));
  }

  void IsTracing(bool expected_response) {
    coordinator_->IsTracing(base::BindRepeating(
        [](bool expected, bool actual) { EXPECT_EQ(expected, actual); },
        expected_response));
  }

  void RequestBufferUsage(float expected_usage, uint32_t expected_count) {
    coordinator_->RequestBufferUsage(base::BindRepeating(
        [](float expected_usage, uint32_t expected_count, bool success,
           float usage, uint32_t count) {
          EXPECT_TRUE(success);
          EXPECT_EQ(expected_usage, usage);
          EXPECT_EQ(expected_count, count);
        },
        expected_usage, expected_count));
  }

  void CheckDisconnectClosures(size_t num_agents) {
    // Verify that all disconnect closures are cleared up. This means that, for
    // each agent, either the tracing service is notified that the agent is
    // disconnected or the agent has answered to all requests.
    size_t count = 0;
    agent_registry_->ForAllAgents([&count](AgentRegistry::AgentEntry* entry) {
      count++;
      EXPECT_EQ(0u, entry->num_disconnect_closures_for_testing());
    });
    EXPECT_EQ(num_agents, count);
  }

  void GetCategories(bool expected_success,
                     std::set<std::string> expected_categories) {
    coordinator_->GetCategories(base::BindRepeating(
        [](bool expected_success, std::set<std::string> expected_categories,
           bool success, const std::string& categories) {
          EXPECT_EQ(expected_success, success);
          if (!success)
            return;
          std::vector<std::string> category_vector = base::SplitString(
              categories, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
          EXPECT_EQ(expected_categories.size(), category_vector.size());
          for (const auto& expected_category : expected_categories) {
            EXPECT_EQ(1, std::count(category_vector.begin(),
                                    category_vector.end(), expected_category));
          }
        },
        expected_success, expected_categories));
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<AgentRegistry> agent_registry_;
  std::unique_ptr<Coordinator> coordinator_;
  std::vector<std::unique_ptr<MockAgent>> agents_;
  std::unique_ptr<mojo::DataPipeDrainer> drainer_;
  base::RepeatingClosure quit_closure_;
  std::string output_;
  service_manager::ServiceContextRefFactory service_ref_factory_;
};

TEST_F(CoordinatorTest, StartTracingSimple) {
  base::RunLoop run_loop;
  auto* agent = AddArrayAgent();
  StartTracing("*", true);
  run_loop.RunUntilIdle();

  // The agent should have received exactly one call from the coordinator.
  EXPECT_EQ(1u, agent->call_stat().size());
  EXPECT_EQ("StartTracing", agent->call_stat()[0]);
}

TEST_F(CoordinatorTest, StartTracingTwoAgents) {
  base::RunLoop run_loop;
  auto* agent1 = AddArrayAgent();
  StartTracing("*", true);
  auto* agent2 = AddStringAgent();
  run_loop.RunUntilIdle();

  // Each agent should have received exactly one call from the coordinatr.
  EXPECT_EQ(1u, agent1->call_stat().size());
  EXPECT_EQ("StartTracing", agent1->call_stat()[0]);
  EXPECT_EQ(1u, agent2->call_stat().size());
  EXPECT_EQ("StartTracing", agent2->call_stat()[0]);
}

TEST_F(CoordinatorTest, StartTracingWithDifferentConfigs) {
  base::RunLoop run_loop;
  auto* agent = AddArrayAgent();
  StartTracing("config 1", true);
  // The 2nd |StartTracing| should return false.
  StartTracing("config 2", false);
  run_loop.RunUntilIdle();

  // The agent should have received exactly one call from the coordinator
  // because the 2nd |StartTracing| was aborted.
  EXPECT_EQ(1u, agent->call_stat().size());
  EXPECT_EQ("StartTracing", agent->call_stat()[0]);
}

TEST_F(CoordinatorTest, StartTracingWithSameConfigs) {
  base::RunLoop run_loop;
  auto* agent = AddArrayAgent();
  StartTracing("config", true);
  // The 2nd |StartTracing| should return true when we are not trying to change
  // the config.
  StartTracing("config", true);
  run_loop.RunUntilIdle();

  // The agent should have received exactly one call from the coordinator
  // because the 2nd |StartTracing| was a no-op.
  EXPECT_EQ(1u, agent->call_stat().size());
  EXPECT_EQ("StartTracing", agent->call_stat()[0]);
}

TEST_F(CoordinatorTest, StopAndFlushObjectAgent) {
  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();

  auto* agent = AddObjectAgent();
  agent->data_.push_back("\"content\":{\"a\":1}");
  agent->data_.push_back("\"name\":\"etw\"");

  StartTracing("config", true, true);
  if (!quit_closure_.is_null())
    run_loop.Run();

  EXPECT_EQ("{\"systemTraceEvents\":{\"content\":{\"a\":1},\"name\":\"etw\"}}",
            output_);

  // Each agent should have received exactly two calls.
  EXPECT_EQ(2u, agent->call_stat().size());
  EXPECT_EQ("StartTracing", agent->call_stat()[0]);
  EXPECT_EQ("StopAndFlush", agent->call_stat()[1]);
}

TEST_F(CoordinatorTest, StopAndFlushTwoArrayAgents) {
  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();

  auto* agent1 = AddArrayAgent();
  agent1->data_.push_back("e1");
  agent1->data_.push_back("e2");

  auto* agent2 = AddArrayAgent();
  agent2->data_.push_back("e3");
  agent2->data_.push_back("e4");

  StartTracing("config", true, true);
  if (!quit_closure_.is_null())
    run_loop.Run();

  // |output_| should be of the form {"traceEvents":[ei,ej,ek,el]}, where
  // ei,ej,ek,el is a permutation of e1,e2,e3,e4 such that e1 is before e2 and
  // e3 is before e4 since e1 and 2 come from the same agent and their order
  // should be preserved and, similarly, the order of e3 and e4 should be
  // preserved, too.
  EXPECT_TRUE(output_ == "{\"traceEvents\":[e1,e2,e3,e4]}" ||
              output_ == "{\"traceEvents\":[e1,e3,e2,e4]}" ||
              output_ == "{\"traceEvents\":[e1,e3,e4,e2]}" ||
              output_ == "{\"traceEvents\":[e3,e1,e2,e4]}" ||
              output_ == "{\"traceEvents\":[e3,e1,e4,e2]}" ||
              output_ == "{\"traceEvents\":[e3,e4,e1,e2]}");

  // Each agent should have received exactly two calls.
  EXPECT_EQ(2u, agent1->call_stat().size());
  EXPECT_EQ("StartTracing", agent1->call_stat()[0]);
  EXPECT_EQ("StopAndFlush", agent1->call_stat()[1]);

  EXPECT_EQ(2u, agent2->call_stat().size());
  EXPECT_EQ("StartTracing", agent2->call_stat()[0]);
  EXPECT_EQ("StopAndFlush", agent2->call_stat()[1]);
}

TEST_F(CoordinatorTest, StopAndFlushDifferentTypeAgents) {
  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();

  auto* agent1 = AddArrayAgent();
  agent1->data_.push_back("e1");
  agent1->data_.push_back("e2");

  auto* agent2 = AddStringAgent();
  agent2->data_.push_back("e3");
  agent2->data_.push_back("e4");

  StartTracing("config", true, true);
  if (!quit_closure_.is_null())
    run_loop.Run();

  EXPECT_TRUE(output_ == "{\"traceEvents\":[e1,e2],\"battor\":\"e3e4\"}" ||
              output_ == "{\"battor\":\"e3e4\",\"traceEvents\":[e1,e2]}");

  // Each agent should have received exactly two calls.
  EXPECT_EQ(2u, agent1->call_stat().size());
  EXPECT_EQ("StartTracing", agent1->call_stat()[0]);
  EXPECT_EQ("StopAndFlush", agent1->call_stat()[1]);

  EXPECT_EQ(2u, agent2->call_stat().size());
  EXPECT_EQ("StartTracing", agent2->call_stat()[0]);
  EXPECT_EQ("StopAndFlush", agent2->call_stat()[1]);
}

TEST_F(CoordinatorTest, StopAndFlushWithMetadata) {
  base::RunLoop run_loop;
  quit_closure_ = run_loop.QuitClosure();

  auto* agent = AddArrayAgent();
  agent->data_.push_back("event");
  agent->metadata_.SetString("key", "value");

  StartTracing("config", true, true);
  if (!quit_closure_.is_null())
    run_loop.Run();

  // Metadata is written at after trace data.
  EXPECT_EQ("{\"traceEvents\":[event],\"metadata\":{\"key\":\"value\"}}",
            output_);
  EXPECT_EQ(2u, agent->call_stat().size());
  EXPECT_EQ("StartTracing", agent->call_stat()[0]);
  EXPECT_EQ("StopAndFlush", agent->call_stat()[1]);
}

TEST_F(CoordinatorTest, IsTracing) {
  base::RunLoop run_loop;
  StartTracing("config", true);
  IsTracing(true);
  run_loop.RunUntilIdle();
}

TEST_F(CoordinatorTest, IsNotTracing) {
  base::RunLoop run_loop;
  IsTracing(false);
  run_loop.RunUntilIdle();
}

TEST_F(CoordinatorTest, RequestBufferUsage) {
  auto* agent1 = AddArrayAgent();
  agent1->trace_log_status_.event_capacity = 4;
  agent1->trace_log_status_.event_count = 1;
  RequestBufferUsage(0.25, 1);
  base::RunLoop().RunUntilIdle();
  CheckDisconnectClosures(1);

  auto* agent2 = AddArrayAgent();
  agent2->trace_log_status_.event_capacity = 8;
  agent2->trace_log_status_.event_count = 1;
  // The buffer usage of |agent2| is less than the buffer usage of |agent1| and
  // so the total buffer usage, i.e 0.25, does not change. But, the approximage
  // count will be increased from 1 to 2.
  RequestBufferUsage(0.25, 2);
  base::RunLoop().RunUntilIdle();
  CheckDisconnectClosures(2);

  base::RunLoop run_loop3;
  auto* agent3 = AddArrayAgent();
  agent3->trace_log_status_.event_capacity = 8;
  agent3->trace_log_status_.event_count = 4;
  // |agent3| has the worst buffer usage of 0.5.
  RequestBufferUsage(0.5, 6);
  base::RunLoop().RunUntilIdle();
  CheckDisconnectClosures(3);

  // At the end |agent1| receveis 3 calls, |agent2| receives 2 calls, and
  // |agent3| receives 1 call.
  EXPECT_EQ(3u, agent1->call_stat().size());
  EXPECT_EQ(2u, agent2->call_stat().size());
  EXPECT_EQ(1u, agent3->call_stat().size());
}

TEST_F(CoordinatorTest, GetCategoriesFail) {
  base::RunLoop run_loop;
  StartTracing("config", true);
  std::set<std::string> expected_categories;
  GetCategories(false, expected_categories);
  run_loop.RunUntilIdle();
}

TEST_F(CoordinatorTest, GetCategoriesSimple) {
  base::RunLoop run_loop;
  auto* agent = AddArrayAgent();
  agent->categories_ = "cat2,cat1";
  std::set<std::string> expected_categories;
  expected_categories.insert("cat1");
  expected_categories.insert("cat2");
  GetCategories(true, expected_categories);
  run_loop.RunUntilIdle();
}

TEST_F(CoordinatorTest, GetCategoriesFromTwoAgents) {
  base::RunLoop run_loop;
  auto* agent1 = AddArrayAgent();
  agent1->categories_ = "cat2,cat1";
  auto* agent2 = AddArrayAgent();
  agent2->categories_ = "cat3,cat2";
  std::set<std::string> expected_categories;
  expected_categories.insert("cat1");
  expected_categories.insert("cat2");
  expected_categories.insert("cat3");
  GetCategories(true, expected_categories);
  run_loop.RunUntilIdle();
}

}  // namespace tracing
