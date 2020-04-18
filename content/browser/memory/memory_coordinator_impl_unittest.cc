// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/memory_coordinator_impl.h"

#include "base/memory/memory_coordinator_client_registry.h"
#include "base/memory/memory_coordinator_proxy.h"
#include "base/memory/memory_pressure_monitor.h"
#include "base/run_loop.h"
#include "base/test/multiprocess_test.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_mock_time_task_runner.h"
#include "content/browser/memory/memory_condition_observer.h"
#include "content/browser/memory/memory_monitor.h"
#include "content/public/common/content_features.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

void RunUntilIdle() {
  base::RunLoop loop;
  loop.RunUntilIdle();
}

// A mock ChildMemoryCoordinator, for testing interaction between MC and CMC.
class MockChildMemoryCoordinator : public mojom::ChildMemoryCoordinator {
 public:
  MockChildMemoryCoordinator()
      : state_(mojom::MemoryState::NORMAL),
        on_state_change_calls_(0),
        purge_memory_calls_(0) {}

  ~MockChildMemoryCoordinator() override {}

  void OnStateChange(mojom::MemoryState state) override {
    state_ = state;
    ++on_state_change_calls_;
  }

  void PurgeMemory() override { ++purge_memory_calls_; }

  mojom::MemoryState state() const { return state_; }
  int on_state_change_calls() const { return on_state_change_calls_; }
  int purge_memory_calls() const { return purge_memory_calls_; }

 private:
  mojom::MemoryState state_;
  int on_state_change_calls_;
  int purge_memory_calls_;
};

// A mock MemoryCoordinatorClient, for testing interaction between MC and
// clients.
class MockMemoryCoordinatorClient : public base::MemoryCoordinatorClient {
 public:
  void OnMemoryStateChange(base::MemoryState state) override {
    did_state_changed_ = true;
    state_ = state;
  }

  void OnPurgeMemory() override { ++purge_memory_calls_; }

  bool did_state_changed() const { return did_state_changed_; }
  base::MemoryState state() const { return state_; }
  int purge_memory_calls() const { return purge_memory_calls_; }

 private:
  bool did_state_changed_ = false;
  base::MemoryState state_ = base::MemoryState::NORMAL;
  int purge_memory_calls_ = 0;
};

class MockMemoryMonitor : public MemoryMonitor {
 public:
  MockMemoryMonitor() {}
  ~MockMemoryMonitor() override {}

  void SetFreeMemoryUntilCriticalMB(int free_memory) {
    free_memory_ = free_memory;
  }

  // MemoryMonitor implementation
  int GetFreeMemoryUntilCriticalMB() override { return free_memory_; }

 private:
  int free_memory_ = 0;

  DISALLOW_COPY_AND_ASSIGN(MockMemoryMonitor);
};

class TestMemoryCoordinatorDelegate : public MemoryCoordinatorDelegate {
 public:
  TestMemoryCoordinatorDelegate() {}
  ~TestMemoryCoordinatorDelegate() override {}

  void DiscardTab(bool skip_unload_handlers) override { ++discard_tab_count_; }

  int discard_tab_count() const { return discard_tab_count_; }

 private:
  int discard_tab_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestMemoryCoordinatorDelegate);
};

class MockMemoryCoordinatorPolicy : public MemoryCoordinatorImpl::Policy {
 public:
  explicit MockMemoryCoordinatorPolicy(MemoryCoordinatorImpl* coordinator)
      : coordinator_(coordinator) {
    DCHECK(coordinator_);
  }

  ~MockMemoryCoordinatorPolicy() override = default;

  void OnCriticalCondition() override {
    coordinator_->DiscardTab(false /* skip_unload_handlers */);
  }

  void OnConditionChanged(MemoryCondition prev, MemoryCondition next) override {
    EXPECT_NE(prev, next);
    last_condition_ = next;
  }

  MemoryCondition last_condition() const { return last_condition_; }

 private:
  MemoryCoordinatorImpl* coordinator_ = nullptr;
  MemoryCondition last_condition_ = MemoryCondition::NORMAL;

  DISALLOW_COPY_AND_ASSIGN(MockMemoryCoordinatorPolicy);
};

// A MemoryCoordinatorImpl that can be directly constructed.
class TestMemoryCoordinatorImpl : public MemoryCoordinatorImpl {
 public:
  // Mojo machinery for wrapping a mock ChildMemoryCoordinator.
  struct Child {
    Child(mojom::ChildMemoryCoordinatorPtr* cmc_ptr) : cmc_binding(&cmc) {
      cmc_binding.Bind(mojo::MakeRequest(cmc_ptr));
      RunUntilIdle();
    }

    MockChildMemoryCoordinator cmc;
    mojo::Binding<mojom::ChildMemoryCoordinator> cmc_binding;
  };

  TestMemoryCoordinatorImpl(
      scoped_refptr<base::TestMockTimeTaskRunner> task_runner)
      : MemoryCoordinatorImpl(task_runner,
                              std::make_unique<MockMemoryMonitor>()) {
    SetDelegateForTesting(std::make_unique<TestMemoryCoordinatorDelegate>());
    SetPolicyForTesting(std::make_unique<MockMemoryCoordinatorPolicy>(this));
    SetTickClockForTesting(task_runner->GetMockTickClock());
  }

  ~TestMemoryCoordinatorImpl() override {}

  using MemoryCoordinatorImpl::OnConnectionError;
  using MemoryCoordinatorImpl::children;

  MockChildMemoryCoordinator* CreateChildMemoryCoordinator(
      int process_id) {
    mojom::ChildMemoryCoordinatorPtr cmc_ptr;
    children_.push_back(std::unique_ptr<Child>(new Child(&cmc_ptr)));
    AddChildForTesting(process_id, std::move(cmc_ptr));
    render_process_hosts_[process_id] =
        std::make_unique<MockRenderProcessHost>(&browser_context_);
    return &children_.back()->cmc;
  }

  RenderProcessHost* GetRenderProcessHost(int render_process_id) override {
    return GetMockRenderProcessHost(render_process_id);
  }

  MockRenderProcessHost* GetMockRenderProcessHost(int render_process_id) {
    auto iter = render_process_hosts_.find(render_process_id);
    if (iter == render_process_hosts_.end())
      return nullptr;
    return iter->second.get();
  }

  TestMemoryCoordinatorDelegate* GetDelegate() {
    return static_cast<TestMemoryCoordinatorDelegate*>(delegate());
  }

  MockMemoryCoordinatorPolicy* GetPolicy() {
    return static_cast<MockMemoryCoordinatorPolicy*>(policy());
  }

  // Wrapper of MemoryCoordinator::SetMemoryState that also calls RunUntilIdle.
  bool SetChildMemoryState(
      int render_process_id, MemoryState memory_state) {
    bool result = MemoryCoordinatorImpl::SetChildMemoryState(
        render_process_id, memory_state);
    RunUntilIdle();
    return result;
  }

  TestBrowserContext browser_context_;
  std::vector<std::unique_ptr<Child>> children_;
  std::map<int, std::unique_ptr<MockRenderProcessHost>> render_process_hosts_;
};

}  // namespace

class MemoryCoordinatorImplTest : public base::MultiProcessTest {
 public:
  using MemoryState = base::MemoryState;

  void SetUp() override {
    base::MultiProcessTest::SetUp();
    scoped_feature_list_.InitAndEnableFeature(features::kMemoryCoordinator);

    task_runner_ = new base::TestMockTimeTaskRunner();
    thread_bundle_ = std::make_unique<TestBrowserThreadBundle>();
    coordinator_ = std::make_unique<TestMemoryCoordinatorImpl>(task_runner_);
  }

  MockMemoryMonitor* GetMockMemoryMonitor() {
    return static_cast<MockMemoryMonitor*>(coordinator_->memory_monitor());
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  std::unique_ptr<content::TestBrowserThreadBundle> thread_bundle_;
  std::unique_ptr<TestMemoryCoordinatorImpl> coordinator_;
};

TEST_F(MemoryCoordinatorImplTest, ChildRemovedOnConnectionError) {
  coordinator_->CreateChildMemoryCoordinator(1);
  ASSERT_EQ(1u, coordinator_->children().size());
  coordinator_->OnConnectionError(1);
  EXPECT_EQ(0u, coordinator_->children().size());
}

TEST_F(MemoryCoordinatorImplTest, SetMemoryStateFailsInvalidState) {
  auto* cmc1 = coordinator_->CreateChildMemoryCoordinator(1);

  EXPECT_FALSE(
      coordinator_->SetChildMemoryState(1, MemoryState::UNKNOWN));
  EXPECT_EQ(0, cmc1->on_state_change_calls());
}

TEST_F(MemoryCoordinatorImplTest, SetMemoryStateFailsInvalidRenderer) {
  auto* cmc1 = coordinator_->CreateChildMemoryCoordinator(1);

  EXPECT_FALSE(
      coordinator_->SetChildMemoryState(2, MemoryState::THROTTLED));
  EXPECT_EQ(0, cmc1->on_state_change_calls());
}

TEST_F(MemoryCoordinatorImplTest, SetMemoryStateNotDeliveredNop) {
  auto* cmc1 = coordinator_->CreateChildMemoryCoordinator(1);

  EXPECT_FALSE(
      coordinator_->SetChildMemoryState(2, MemoryState::NORMAL));
  EXPECT_EQ(0, cmc1->on_state_change_calls());
}

TEST_F(MemoryCoordinatorImplTest, SetMemoryStateDelivered) {
  auto* cmc1 = coordinator_->CreateChildMemoryCoordinator(1);
  auto* cmc2 = coordinator_->CreateChildMemoryCoordinator(2);

  EXPECT_TRUE(
      coordinator_->SetChildMemoryState(1, MemoryState::THROTTLED));
  EXPECT_EQ(1, cmc1->on_state_change_calls());
  EXPECT_EQ(mojom::MemoryState::THROTTLED, cmc1->state());
  EXPECT_EQ(0, cmc2->on_state_change_calls());
  EXPECT_EQ(mojom::MemoryState::NORMAL, cmc2->state());
}

TEST_F(MemoryCoordinatorImplTest, PurgeMemoryChild) {
  auto* child = coordinator_->CreateChildMemoryCoordinator(1);
  EXPECT_EQ(0, child->purge_memory_calls());
  child->PurgeMemory();
  RunUntilIdle();
  EXPECT_EQ(1, child->purge_memory_calls());
}

TEST_F(MemoryCoordinatorImplTest, SetChildMemoryState) {
  auto* cmc = coordinator_->CreateChildMemoryCoordinator(1);
  auto iter = coordinator_->children().find(1);
  auto* render_process_host = coordinator_->GetMockRenderProcessHost(1);
  ASSERT_TRUE(iter != coordinator_->children().end());
  ASSERT_TRUE(render_process_host);

  // Foreground
  iter->second.is_visible = true;
  render_process_host->set_is_process_backgrounded(false);
  EXPECT_TRUE(coordinator_->SetChildMemoryState(1, MemoryState::NORMAL));
  EXPECT_EQ(mojom::MemoryState::NORMAL, cmc->state());
  EXPECT_TRUE(
      coordinator_->SetChildMemoryState(1, MemoryState::THROTTLED));
  EXPECT_EQ(mojom::MemoryState::THROTTLED, cmc->state());
  EXPECT_TRUE(coordinator_->SetChildMemoryState(1, MemoryState::THROTTLED));
  EXPECT_EQ(mojom::MemoryState::THROTTLED, cmc->state());

  // Background
  iter->second.is_visible = false;
  render_process_host->set_is_process_backgrounded(true);
  EXPECT_TRUE(coordinator_->SetChildMemoryState(1, MemoryState::NORMAL));
#if defined(OS_ANDROID)
  EXPECT_EQ(mojom::MemoryState::THROTTLED, cmc->state());
#else
  EXPECT_EQ(mojom::MemoryState::NORMAL, cmc->state());
#endif
  EXPECT_TRUE(
      coordinator_->SetChildMemoryState(1, MemoryState::THROTTLED));
  EXPECT_EQ(mojom::MemoryState::THROTTLED, cmc->state());
}

TEST_F(MemoryCoordinatorImplTest, CalculateNextCondition) {
  auto* condition_observer = coordinator_->condition_observer_.get();

  // The default condition is NORMAL.
  EXPECT_EQ(MemoryCondition::NORMAL, coordinator_->GetMemoryCondition());

  // Transitions from NORMAL
  GetMockMemoryMonitor()->SetFreeMemoryUntilCriticalMB(1000);
  EXPECT_EQ(MemoryCondition::NORMAL,
            condition_observer->CalculateNextCondition());
  GetMockMemoryMonitor()->SetFreeMemoryUntilCriticalMB(0);
  EXPECT_EQ(MemoryCondition::CRITICAL,
            condition_observer->CalculateNextCondition());

  // Transitions from CRITICAL
  coordinator_->memory_condition_ = MemoryCondition::CRITICAL;
  EXPECT_EQ(MemoryCondition::CRITICAL, coordinator_->GetMemoryCondition());
  GetMockMemoryMonitor()->SetFreeMemoryUntilCriticalMB(1000);
  EXPECT_EQ(MemoryCondition::NORMAL,
            condition_observer->CalculateNextCondition());
}

TEST_F(MemoryCoordinatorImplTest, UpdateConditionIfNeeded) {
  auto* policy = coordinator_->GetPolicy();

  // The initial condition is NORMAL.
  EXPECT_EQ(MemoryCondition::NORMAL, coordinator_->GetMemoryCondition());
  coordinator_->UpdateConditionIfNeeded(MemoryCondition::NORMAL);
  RunUntilIdle();
  EXPECT_EQ(MemoryCondition::NORMAL, policy->last_condition());

  // Change to CRITICAL condition.
  coordinator_->UpdateConditionIfNeeded(MemoryCondition::CRITICAL);
  RunUntilIdle();
  EXPECT_EQ(MemoryCondition::CRITICAL, policy->last_condition());

  // Make sure that OnConditionChanged() won't get called when the condition is
  // unchanged. CHECK_NE() in MockMemoryCoordinatorPolicy ensures it.
  coordinator_->UpdateConditionIfNeeded(MemoryCondition::CRITICAL);
  RunUntilIdle();
  EXPECT_EQ(MemoryCondition::CRITICAL, policy->last_condition());
}

// TODO(bashi): Move ForceSetMemoryCondition?
TEST_F(MemoryCoordinatorImplTest, ForceSetMemoryCondition) {
  auto* condition_observer = coordinator_->condition_observer_.get();
  GetMockMemoryMonitor()->SetFreeMemoryUntilCriticalMB(1000);

  base::TimeDelta interval = base::TimeDelta::FromSeconds(5);
  condition_observer->monitoring_interval_ = interval;

  // Starts updating memory condition. The initial condition should be NORMAL
  // with above configuration.
  coordinator_->Start();
  task_runner_->RunUntilIdle();
  EXPECT_EQ(MemoryCondition::NORMAL, coordinator_->GetMemoryCondition());

  base::TimeDelta force_set_duration = interval * 3;
  coordinator_->ForceSetMemoryCondition(MemoryCondition::CRITICAL,
                                        force_set_duration);
  EXPECT_EQ(MemoryCondition::CRITICAL, coordinator_->GetMemoryCondition());

  // The condition should remain CRITICAL even after some monitoring period are
  // passed.
  task_runner_->FastForwardBy(interval * 2);
  task_runner_->RunUntilIdle();
  EXPECT_EQ(MemoryCondition::CRITICAL, coordinator_->GetMemoryCondition());

  // The condition should be updated after |force_set_duration| is passed.
  task_runner_->FastForwardBy(force_set_duration);
  task_runner_->RunUntilIdle();
  EXPECT_EQ(MemoryCondition::NORMAL, coordinator_->GetMemoryCondition());

  // Also make sure that the condition is updated based on free avaiable memory.
  GetMockMemoryMonitor()->SetFreeMemoryUntilCriticalMB(0);
  task_runner_->FastForwardBy(interval * 2);
  task_runner_->RunUntilIdle();
  EXPECT_EQ(MemoryCondition::CRITICAL, coordinator_->GetMemoryCondition());
}

TEST_F(MemoryCoordinatorImplTest, DiscardTab) {
  coordinator_->DiscardTab(false /* skip_unload_handlers */);
  EXPECT_EQ(1, coordinator_->GetDelegate()->discard_tab_count());
}

TEST_F(MemoryCoordinatorImplTest, DiscardTabUnderCritical) {
  auto* condition_observer = coordinator_->condition_observer_.get();
  GetMockMemoryMonitor()->SetFreeMemoryUntilCriticalMB(1000);

  base::TimeDelta interval = base::TimeDelta::FromSeconds(5);
  condition_observer->monitoring_interval_ = interval;

  auto* delegate = coordinator_->GetDelegate();

  coordinator_->Start();
  task_runner_->RunUntilIdle();
  EXPECT_EQ(MemoryCondition::NORMAL, coordinator_->GetMemoryCondition());
  EXPECT_EQ(0, delegate->discard_tab_count());

  // Enter CRITICAL condition. Tab discarding should start.
  GetMockMemoryMonitor()->SetFreeMemoryUntilCriticalMB(0);
  task_runner_->FastForwardBy(interval);
  EXPECT_EQ(1, delegate->discard_tab_count());
  task_runner_->FastForwardBy(interval);
  EXPECT_EQ(2, delegate->discard_tab_count());

  // Back to NORMAL. Tab discarding should stop.
  GetMockMemoryMonitor()->SetFreeMemoryUntilCriticalMB(1000);
  task_runner_->FastForwardBy(interval);
  EXPECT_EQ(2, delegate->discard_tab_count());
  task_runner_->FastForwardBy(interval);
  EXPECT_EQ(2, delegate->discard_tab_count());
}

#if defined(OS_ANDROID)
// TODO(jcivelli): Broken on Android. http://crbug.com/678665
#define MAYBE_GetStateForProcess DISABLED_GetStateForProcess
#else
#define MAYBE_GetStateForProcess GetStateForProcess
#endif
TEST_F(MemoryCoordinatorImplTest, MAYBE_GetStateForProcess) {
  EXPECT_EQ(base::MemoryState::UNKNOWN,
            coordinator_->GetStateForProcess(base::kNullProcessHandle));
  EXPECT_EQ(base::MemoryState::NORMAL,
            coordinator_->GetStateForProcess(base::GetCurrentProcessHandle()));

  coordinator_->CreateChildMemoryCoordinator(1);
  coordinator_->CreateChildMemoryCoordinator(2);
  base::Process process1 = SpawnChild("process1");
  base::ProcessHandle process1_handle = process1.Handle();
  base::Process process2 = SpawnChild("process2");
  base::ProcessHandle process2_handle = process2.Handle();

  coordinator_->GetMockRenderProcessHost(1)->SetProcess(std::move(process1));
  coordinator_->GetMockRenderProcessHost(2)->SetProcess(std::move(process2));

  EXPECT_EQ(base::MemoryState::NORMAL,
            coordinator_->GetStateForProcess(process1_handle));
  EXPECT_EQ(base::MemoryState::NORMAL,
            coordinator_->GetStateForProcess(process2_handle));

  EXPECT_TRUE(
      coordinator_->SetChildMemoryState(1, MemoryState::THROTTLED));
  EXPECT_EQ(base::MemoryState::THROTTLED,
            coordinator_->GetStateForProcess(process1_handle));
  EXPECT_EQ(base::MemoryState::NORMAL,
            coordinator_->GetStateForProcess(process2_handle));
}

}  // namespace content
