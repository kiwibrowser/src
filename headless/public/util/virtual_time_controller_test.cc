// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/virtual_time_controller.h"

#include <memory>
#include <tuple>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/strings/stringprintf.h"
#include "base/test/test_simple_task_runner.h"
#include "headless/public/internal/headless_devtools_client_impl.h"
#include "headless/public/util/testing/mock_devtools_agent_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace headless {

using testing::ElementsAre;
using testing::Mock;
using testing::Return;
using testing::_;

class VirtualTimeControllerTest : public ::testing::Test {
 protected:
  VirtualTimeControllerTest() {
    task_runner_ = base::MakeRefCounted<base::TestSimpleTaskRunner>();
    client_.SetTaskRunnerForTests(task_runner_);
    mock_host_ = base::MakeRefCounted<MockDevToolsAgentHost>();

    EXPECT_CALL(*mock_host_, AttachClient(&client_));
    client_.AttachToHost(mock_host_.get());
    controller_ = std::make_unique<VirtualTimeController>(&client_, 0);
  }

  ~VirtualTimeControllerTest() override = default;

  // TODO(alexclarke): This is a common pattern add a helper.
  class AdditionalVirtualTimeBudget
      : public VirtualTimeController::RepeatingTask,
        public VirtualTimeController::Observer {
   public:
    AdditionalVirtualTimeBudget(VirtualTimeController* virtual_time_controller,
                                VirtualTimeControllerTest* test,
                                base::TimeDelta budget)
        : RepeatingTask(StartPolicy::START_IMMEDIATELY, 0),
          virtual_time_controller_(virtual_time_controller),
          test_(test) {
      virtual_time_controller_->ScheduleRepeatingTask(this, budget);
      virtual_time_controller_->AddObserver(this);
      virtual_time_controller_->StartVirtualTime();
    }

    ~AdditionalVirtualTimeBudget() override {
      virtual_time_controller_->RemoveObserver(this);
      virtual_time_controller_->CancelRepeatingTask(this);
    }

    // headless::VirtualTimeController::RepeatingTask implementation:
    void IntervalElapsed(
        base::TimeDelta virtual_time,
        base::OnceCallback<void(ContinuePolicy)> continue_callback) override {
      std::move(continue_callback).Run(ContinuePolicy::NOT_REQUIRED);
    }

    // headless::VirtualTimeController::Observer:
    void VirtualTimeStarted(base::TimeDelta virtual_time_offset) override {
      EXPECT_FALSE(test_->set_up_complete_);
      test_->set_up_complete_ = true;
    }

    void VirtualTimeStopped(base::TimeDelta virtual_time_offset) override {
      EXPECT_FALSE(test_->budget_expired_);
      test_->budget_expired_ = true;
      delete this;
    }

   private:
    headless::VirtualTimeController* const virtual_time_controller_;
    VirtualTimeControllerTest* test_;
  };

  void GrantVirtualTimeBudget(int budget_ms) {
    ASSERT_FALSE(set_up_complete_);
    ASSERT_FALSE(budget_expired_);

    // AdditionalVirtualTimeBudget will self delete
    new AdditionalVirtualTimeBudget(
        controller_.get(), this, base::TimeDelta::FromMilliseconds(budget_ms));

    EXPECT_FALSE(set_up_complete_);
    EXPECT_FALSE(budget_expired_);
  }

  void SendVirtualTimeBudgetExpiredEvent() {
    client_.DispatchProtocolMessage(
        mock_host_.get(),
        "{\"method\":\"Emulation.virtualTimeBudgetExpired\",\"params\":{}}");
    // Events are dispatched asynchronously.
    task_runner_->RunPendingTasks();
  }

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  scoped_refptr<MockDevToolsAgentHost> mock_host_;
  HeadlessDevToolsClientImpl client_;
  std::unique_ptr<VirtualTimeController> controller_;

  bool set_up_complete_ = false;
  bool budget_expired_ = false;
};

TEST_F(VirtualTimeControllerTest, DoesNotAdvanceTimeWithoutTasks) {
  controller_ = std::make_unique<VirtualTimeController>(&client_, 1000);

  EXPECT_CALL(*mock_host_, DispatchProtocolMessage(&client_, _)).Times(0);

  controller_->StartVirtualTime();
}

TEST_F(VirtualTimeControllerTest, MaxVirtualTimeTaskStarvationCount) {
  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":0,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":5000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  GrantVirtualTimeBudget(5000);

  client_.DispatchProtocolMessage(mock_host_.get(),
                                  "{\"id\":0,\"result\":{\"virtualTimeBase\":1."
                                  "0,\"virtualTimeTicksBase\":1.0}}");
  task_runner_->RunPendingTasks();

  EXPECT_TRUE(set_up_complete_);
  EXPECT_FALSE(budget_expired_);

  SendVirtualTimeBudgetExpiredEvent();

  EXPECT_TRUE(budget_expired_);
}

namespace {
class MockTask : public VirtualTimeController::RepeatingTask {
 public:
  MockTask() : RepeatingTask(StartPolicy::START_IMMEDIATELY, 0) {}

  MockTask(StartPolicy start_policy, int priority)
      : RepeatingTask(start_policy, priority) {}

  ~MockTask() override { EXPECT_TRUE(!expected_virtual_time_offset_); }

  // GMock doesn't support move only types
  void IntervalElapsed(
      base::TimeDelta virtual_time_offset,
      base::OnceCallback<void(ContinuePolicy)> continue_callback) override {
    EXPECT_EQ(*expected_virtual_time_offset_, virtual_time_offset);
    expected_virtual_time_offset_ = base::nullopt;
    std::move(continue_callback).Run(policy_);
    interval_elapsed_ = true;
  }

  void ExpectCallOnceWithOffsetAndReturn(base::TimeDelta virtual_time_offset,
                                         ContinuePolicy policy) {
    expected_virtual_time_offset_ = virtual_time_offset;
    policy_ = policy;
  }

 private:
  base::Optional<base::TimeDelta> expected_virtual_time_offset_;
  ContinuePolicy policy_ = ContinuePolicy::NOT_REQUIRED;
  bool interval_elapsed_ = false;
};

class MockDeferrer : public VirtualTimeController::ResumeDeferrer {
 public:
  // GMock doesn't support move only types
  void DeferResume(base::OnceCallback<void()> continue_callback) override {
    continue_callback_ = std::move(continue_callback);
  }

  base::OnceCallback<void()> continue_callback_;
};

class MockObserver : public VirtualTimeController::Observer {
 public:
  MOCK_METHOD1(VirtualTimeStarted, void(base::TimeDelta virtual_time_offset));

  MOCK_METHOD1(VirtualTimeStopped, void(base::TimeDelta virtual_time_offset));
};

ACTION_TEMPLATE(RunClosure,
                HAS_1_TEMPLATE_PARAMS(int, k),
                AND_0_VALUE_PARAMS()) {
  std::get<k>(args).Run();
}

ACTION_P(RunClosure, closure) {
  closure.Run();
};
}  // namespace

TEST_F(VirtualTimeControllerTest, InterleavesTasksWithVirtualTime) {
  MockTask task;
  MockObserver observer;
  controller_->AddObserver(&observer);
  controller_->ScheduleRepeatingTask(&task,
                                     base::TimeDelta::FromMilliseconds(1000));

  EXPECT_CALL(observer,
              VirtualTimeStarted(base::TimeDelta::FromMilliseconds(0)));
  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":0,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  GrantVirtualTimeBudget(3000);

  EXPECT_FALSE(set_up_complete_);
  EXPECT_FALSE(budget_expired_);

  client_.DispatchProtocolMessage(mock_host_.get(),
                                  "{\"id\":0,\"result\":{\"virtualTimeBase\":1."
                                  "0,\"virtualTimeTicksBase\":1.0}}");
  task_runner_->RunPendingTasks();

  EXPECT_TRUE(set_up_complete_);
  EXPECT_FALSE(budget_expired_);

  // We check that set_up_complete_callback is only run once, so reset it here.
  set_up_complete_ = false;

  for (int i = 1; i < 3; i++) {
    task.ExpectCallOnceWithOffsetAndReturn(
        base::TimeDelta::FromMilliseconds(1000 * i),
        MockTask::ContinuePolicy::NOT_REQUIRED);

    EXPECT_CALL(
        *mock_host_,
        DispatchProtocolMessage(
            &client_,
            base::StringPrintf(
                "{\"id\":%d,\"method\":\"Emulation.setVirtualTimePolicy\","
                "\"params\":{\"budget\":1000.0,"
                "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                "\"pauseIfNetworkFetchesPending\","
                "\"waitForNavigation\":false}}",
                i * 2)))
        .WillOnce(Return(true));

    SendVirtualTimeBudgetExpiredEvent();

    EXPECT_FALSE(set_up_complete_);
    EXPECT_FALSE(budget_expired_);

    client_.DispatchProtocolMessage(
        mock_host_.get(),
        base::StringPrintf("{\"id\":%d,\"result\":{\"virtualTimeBase\":1.0,"
                           "\"virtualTimeTicksBase\":1.0}}",
                           i * 2));

    EXPECT_FALSE(set_up_complete_);
    EXPECT_FALSE(budget_expired_);
  }

  task.ExpectCallOnceWithOffsetAndReturn(
      base::TimeDelta::FromMilliseconds(3000),
      MockTask::ContinuePolicy::NOT_REQUIRED);
  EXPECT_CALL(observer,
              VirtualTimeStopped(base::TimeDelta::FromMilliseconds(3000)));
  SendVirtualTimeBudgetExpiredEvent();

  EXPECT_FALSE(set_up_complete_);
  EXPECT_TRUE(budget_expired_);
}

TEST_F(VirtualTimeControllerTest, CanceledTask) {
  MockTask task;
  controller_->ScheduleRepeatingTask(&task,
                                     base::TimeDelta::FromMilliseconds(1000));

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":0,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  GrantVirtualTimeBudget(5000);

  EXPECT_FALSE(set_up_complete_);
  EXPECT_FALSE(budget_expired_);

  client_.DispatchProtocolMessage(mock_host_.get(),
                                  "{\"id\":0,\"result\":{\"virtualTimeBase\":1."
                                  "0,\"virtualTimeTicksBase\":1.0}}");
  task_runner_->RunPendingTasks();

  EXPECT_TRUE(set_up_complete_);
  EXPECT_FALSE(budget_expired_);

  // We check that set_up_complete_callback is only run once, so reset it here.
  set_up_complete_ = false;

  task.ExpectCallOnceWithOffsetAndReturn(
      base::TimeDelta::FromMilliseconds(1000),
      MockTask::ContinuePolicy::NOT_REQUIRED);

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":2,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  SendVirtualTimeBudgetExpiredEvent();

  EXPECT_FALSE(set_up_complete_);
  EXPECT_FALSE(budget_expired_);

  client_.DispatchProtocolMessage(
      mock_host_.get(),
      base::StringPrintf("{\"id\":2,\"result\":{\"virtualTimeBase\":1.0,"
                         "\"virtualTimeTicksBase\":1.0}}"));

  EXPECT_FALSE(set_up_complete_);
  EXPECT_FALSE(budget_expired_);

  controller_->CancelRepeatingTask(&task);

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":4,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":3000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  SendVirtualTimeBudgetExpiredEvent();

  EXPECT_FALSE(set_up_complete_);
  EXPECT_FALSE(budget_expired_);

  client_.DispatchProtocolMessage(
      mock_host_.get(),
      base::StringPrintf("{\"id\":4,\"result\":{\"virtualTimeBase\":1.0,"
                         "\"virtualTimeTicksBase\":1.0}}"));

  EXPECT_FALSE(set_up_complete_);
  EXPECT_FALSE(budget_expired_);

  SendVirtualTimeBudgetExpiredEvent();

  EXPECT_FALSE(set_up_complete_);
  EXPECT_TRUE(budget_expired_);
}

TEST_F(VirtualTimeControllerTest, MultipleTasks) {
  MockTask task1;
  MockTask task2;
  controller_->ScheduleRepeatingTask(&task1,
                                     base::TimeDelta::FromMilliseconds(1000));
  controller_->ScheduleRepeatingTask(&task2,
                                     base::TimeDelta::FromMilliseconds(1000));

  // We should only get one call to Emulation.setVirtualTimePolicy despite
  // having two tasks.
  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":0,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  GrantVirtualTimeBudget(2000);
  EXPECT_FALSE(set_up_complete_);
  EXPECT_FALSE(budget_expired_);

  client_.DispatchProtocolMessage(
      mock_host_.get(),
      base::StringPrintf("{\"id\":0,\"result\":{\"virtualTimeBase\":1.0,"
                         "\"virtualTimeTicksBase\":1.0}}"));

  task_runner_->RunPendingTasks();
  EXPECT_TRUE(set_up_complete_);
  EXPECT_FALSE(budget_expired_);
}

TEST_F(VirtualTimeControllerTest, StartPolicy) {
  MockTask task1(MockTask::StartPolicy::START_IMMEDIATELY, 0);
  MockTask task2(MockTask::StartPolicy::START_IMMEDIATELY, 0);
  MockTask task3(MockTask::StartPolicy::WAIT_FOR_NAVIGATION, 0);
  controller_->ScheduleRepeatingTask(&task1,
                                     base::TimeDelta::FromMilliseconds(1000));
  controller_->ScheduleRepeatingTask(&task2,
                                     base::TimeDelta::FromMilliseconds(1000));
  controller_->ScheduleRepeatingTask(&task3,
                                     base::TimeDelta::FromMilliseconds(1000));

  // Despite only one task asking for it we should get waitForNavigation:true
  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":0,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":true}}"))
      .WillOnce(Return(true));

  GrantVirtualTimeBudget(2000);
}

TEST_F(VirtualTimeControllerTest, DeferStartAndResume) {
  MockDeferrer deferrer;
  controller_->SetResumeDeferrer(&deferrer);

  MockTask task1(MockTask::StartPolicy::START_IMMEDIATELY, 0);
  controller_->ScheduleRepeatingTask(&task1,
                                     base::TimeDelta::FromMilliseconds(1000));
  EXPECT_FALSE(deferrer.continue_callback_);

  // Shouldn't see the devtools command until the deferrer's callback has run.
  EXPECT_CALL(*mock_host_, DispatchProtocolMessage(&client_, _)).Times(0);
  GrantVirtualTimeBudget(2000);
  EXPECT_TRUE(deferrer.continue_callback_);

  Mock::VerifyAndClearExpectations(mock_host_.get());

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":0,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  std::move(deferrer.continue_callback_).Run();

  client_.DispatchProtocolMessage(mock_host_.get(),
                                  "{\"id\":0,\"result\":{\"virtualTimeBase\":1."
                                  "0,\"virtualTimeTicksBase\":1.0}}");
  EXPECT_FALSE(deferrer.continue_callback_);

  task1.ExpectCallOnceWithOffsetAndReturn(
      base::TimeDelta::FromMilliseconds(1000),
      MockTask::ContinuePolicy::NOT_REQUIRED);

  // Even after executing task1, virtual time shouldn't resume until the
  // deferrer's callback has run.
  EXPECT_CALL(*mock_host_, DispatchProtocolMessage(&client_, _)).Times(0);

  SendVirtualTimeBudgetExpiredEvent();
  EXPECT_TRUE(deferrer.continue_callback_);

  Mock::VerifyAndClearExpectations(mock_host_.get());

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":2,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  std::move(deferrer.continue_callback_).Run();
}

class VirtualTimeTask : public VirtualTimeController::RepeatingTask,
                        public VirtualTimeController::Observer {
 public:
  using Task = base::RepeatingCallback<void(base::TimeDelta virtual_time)>;

  VirtualTimeTask(VirtualTimeController* controller,
                  Task virtual_time_started_task,
                  Task interval_elapsed_task,
                  Task virtual_time_stopped_task,
                  int priority = 0)
      : RepeatingTask(StartPolicy::START_IMMEDIATELY, priority),
        controller_(controller),
        virtual_time_started_task_(virtual_time_started_task),
        interval_elapsed_task_(interval_elapsed_task),
        virtual_time_stopped_task_(virtual_time_stopped_task) {
    controller_->AddObserver(this);
  }

  ~VirtualTimeTask() override { controller_->RemoveObserver(this); }

  // VirtualTimeController::RepeatingTask:
  void IntervalElapsed(
      base::TimeDelta virtual_time,
      base::OnceCallback<void(ContinuePolicy)> continue_callback) override {
    std::move(continue_callback).Run(ContinuePolicy::NOT_REQUIRED);
    interval_elapsed_task_.Run(virtual_time);
  }

  // VirtualTimeController::Observer:
  void VirtualTimeStarted(base::TimeDelta virtual_time) override {
    virtual_time_started_task_.Run(virtual_time);
  }

  void VirtualTimeStopped(base::TimeDelta virtual_time) override {
    virtual_time_stopped_task_.Run(virtual_time);
  };

  VirtualTimeController* controller_;  // NOT OWNED
  Task virtual_time_started_task_;
  Task interval_elapsed_task_;
  Task virtual_time_stopped_task_;
};

TEST_F(VirtualTimeControllerTest, ReentrantTask) {
#if defined(__clang__)
  std::vector<std::string> log;
  VirtualTimeTask task_b(
      controller_.get(),
      base::BindRepeating([](base::TimeDelta virtual_time) {}),
      base::BindRepeating(
          [](std::vector<std::string>* log, VirtualTimeController* controller,
             VirtualTimeTask* task_b, base::TimeDelta virtual_time) {
            log->push_back(base::StringPrintf(
                "B: interval elapsed @ %d",
                static_cast<int>(virtual_time.InMilliseconds())));
            controller->CancelRepeatingTask(task_b);
          },
          &log, controller_.get(), &task_b),
      base::BindRepeating(
          [](std::vector<std::string>* log, base::TimeDelta virtual_time) {
            log->push_back(base::StringPrintf(
                "B: virtual time stopped @ %d",
                static_cast<int>(virtual_time.InMilliseconds())));
          },
          &log));

  VirtualTimeTask task_a(
      controller_.get(),
      base::BindRepeating(
          [](std::vector<std::string>* log, base::TimeDelta virtual_time) {
            log->push_back(base::StringPrintf(
                "Virtual time started @ %d",
                static_cast<int>(virtual_time.InMilliseconds())));
          },
          &log),
      base::BindRepeating(
          [](std::vector<std::string>* log, VirtualTimeController* controller,
             VirtualTimeTask* task_a, VirtualTimeTask* task_b,
             base::TimeDelta virtual_time) {
            log->push_back(base::StringPrintf(
                "A: interval elapsed @ %d",
                static_cast<int>(virtual_time.InMilliseconds())));
            controller->CancelRepeatingTask(task_a);
            controller->ScheduleRepeatingTask(
                task_b, base::TimeDelta::FromMilliseconds(1500));
          },
          &log, controller_.get(), &task_a, &task_b),
      base::BindRepeating(
          [](std::vector<std::string>* log, base::TimeDelta virtual_time) {
            log->push_back(base::StringPrintf(
                "A: virtual time stopped @ %d",
                static_cast<int>(virtual_time.InMilliseconds())));
          },
          &log));

  controller_->ScheduleRepeatingTask(&task_a,
                                     base::TimeDelta::FromMilliseconds(1000));

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":0,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  GrantVirtualTimeBudget(6000);
  client_.DispatchProtocolMessage(
      mock_host_.get(),
      base::StringPrintf("{\"id\":0,\"result\":{\"virtualTimeBase\":1.0,"
                         "\"virtualTimeTicksBase\":1.0}}"));

  Mock::VerifyAndClearExpectations(&mock_host_);

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":2,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1500.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  SendVirtualTimeBudgetExpiredEvent();
  client_.DispatchProtocolMessage(
      mock_host_.get(),
      base::StringPrintf("{\"id\":2,\"result\":{\"virtualTimeBase\":1.0,"
                         "\"virtualTimeTicksBase\":1.0}}"));
  Mock::VerifyAndClearExpectations(&mock_host_);

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":4,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":3500.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));
  SendVirtualTimeBudgetExpiredEvent();
  client_.DispatchProtocolMessage(
      mock_host_.get(),
      base::StringPrintf("{\"id\":4,\"result\":{\"virtualTimeBase\":1.0,"
                         "\"virtualTimeTicksBase\":1.0}}"));

  EXPECT_THAT(
      log, ElementsAre("Virtual time started @ 0", "A: interval elapsed @ 1000",
                       "B: interval elapsed @ 2500"));
#endif
}

TEST_F(VirtualTimeControllerTest, Priority) {
  std::vector<std::string> log;
  VirtualTimeTask task_a(
      controller_.get(),
      base::BindRepeating([](base::TimeDelta virtual_time) {}),
      base::BindRepeating(
          [](std::vector<std::string>* log, base::TimeDelta virtual_time) {
            log->push_back(base::StringPrintf(
                "A: interval elapsed @ %d",
                static_cast<int>(virtual_time.InMilliseconds())));
          },
          &log),
      base::BindRepeating([](base::TimeDelta virtual_time) {}), 30);

  VirtualTimeTask task_b(
      controller_.get(),
      base::BindRepeating([](base::TimeDelta virtual_time) {}),
      base::BindRepeating(
          [](std::vector<std::string>* log, base::TimeDelta virtual_time) {
            log->push_back(base::StringPrintf(
                "B: interval elapsed @ %d",
                static_cast<int>(virtual_time.InMilliseconds())));
          },
          &log),
      base::BindRepeating([](base::TimeDelta virtual_time) {}), 20);

  VirtualTimeTask task_c(
      controller_.get(),
      base::BindRepeating([](base::TimeDelta virtual_time) {}),
      base::BindRepeating(
          [](std::vector<std::string>* log, base::TimeDelta virtual_time) {
            log->push_back(base::StringPrintf(
                "C: interval elapsed @ %d",
                static_cast<int>(virtual_time.InMilliseconds())));
          },
          &log),
      base::BindRepeating([](base::TimeDelta virtual_time) {}), 10);

  controller_->ScheduleRepeatingTask(&task_a,
                                     base::TimeDelta::FromMilliseconds(1000));

  controller_->ScheduleRepeatingTask(&task_b,
                                     base::TimeDelta::FromMilliseconds(1000));

  controller_->ScheduleRepeatingTask(&task_c,
                                     base::TimeDelta::FromMilliseconds(1000));

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":0,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  GrantVirtualTimeBudget(2000);
  client_.DispatchProtocolMessage(
      mock_host_.get(),
      base::StringPrintf("{\"id\":0,\"result\":{\"virtualTimeBase\":1.0,"
                         "\"virtualTimeTicksBase\":1.0}}"));

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":2,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  SendVirtualTimeBudgetExpiredEvent();

  EXPECT_THAT(log, ElementsAre("C: interval elapsed @ 1000",
                               "B: interval elapsed @ 1000",
                               "A: interval elapsed @ 1000"));
}

TEST_F(VirtualTimeControllerTest, ContinuePolicyContinueMoreTimeNeeded) {
  MockTask task;
  MockObserver observer;
  controller_->AddObserver(&observer);
  controller_->ScheduleRepeatingTask(&task,
                                     base::TimeDelta::FromMilliseconds(1000));

  EXPECT_CALL(observer,
              VirtualTimeStarted(base::TimeDelta::FromMilliseconds(0)));

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":0,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  controller_->StartVirtualTime();

  client_.DispatchProtocolMessage(mock_host_.get(),
                                  "{\"id\":0,\"result\":{\"virtualTimeBase\":1."
                                  "0,\"virtualTimeTicksBase\":1.0}}");

  for (int i = 1; i < 4; i++) {
    task.ExpectCallOnceWithOffsetAndReturn(
        base::TimeDelta::FromMilliseconds(1000 * i),
        MockTask::ContinuePolicy::CONTINUE_MORE_TIME_NEEDED);

    EXPECT_CALL(
        *mock_host_,
        DispatchProtocolMessage(
            &client_,
            base::StringPrintf(
                "{\"id\":%d,\"method\":\"Emulation.setVirtualTimePolicy\","
                "\"params\":{\"budget\":1000.0,"
                "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                "\"pauseIfNetworkFetchesPending\","
                "\"waitForNavigation\":false}}",
                i * 2)))
        .WillOnce(Return(true));

    SendVirtualTimeBudgetExpiredEvent();

    client_.DispatchProtocolMessage(
        mock_host_.get(),
        base::StringPrintf("{\"id\":%d,\"result\":{\"virtualTimeBase\":1.0,"
                           "\"virtualTimeTicksBase\":1.0}}",
                           i * 2));
  }

  task.ExpectCallOnceWithOffsetAndReturn(
      base::TimeDelta::FromMilliseconds(4000),
      MockTask::ContinuePolicy::NOT_REQUIRED);

  EXPECT_CALL(observer,
              VirtualTimeStopped(base::TimeDelta::FromMilliseconds(4000)));

  SendVirtualTimeBudgetExpiredEvent();
}

TEST_F(VirtualTimeControllerTest, ContinuePolicyStopAndRestart) {
  MockTask task1;
  MockTask task2;
  MockTask task3;
  MockObserver observer;
  controller_->AddObserver(&observer);
  controller_->ScheduleRepeatingTask(&task1,
                                     base::TimeDelta::FromMilliseconds(1000));
  controller_->ScheduleRepeatingTask(&task2,
                                     base::TimeDelta::FromMilliseconds(1000));
  controller_->ScheduleRepeatingTask(&task3,
                                     base::TimeDelta::FromMilliseconds(4000));

  EXPECT_CALL(observer,
              VirtualTimeStarted(base::TimeDelta::FromMilliseconds(0)));

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":0,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  controller_->StartVirtualTime();

  client_.DispatchProtocolMessage(mock_host_.get(),
                                  "{\"id\":0,\"result\":{\"virtualTimeBase\":1."
                                  "0,\"virtualTimeTicksBase\":1.0}}");

  for (int i = 1; i < 4; i++) {
    task1.ExpectCallOnceWithOffsetAndReturn(
        base::TimeDelta::FromMilliseconds(1000 * i),
        MockTask::ContinuePolicy::CONTINUE_MORE_TIME_NEEDED);
    task2.ExpectCallOnceWithOffsetAndReturn(
        base::TimeDelta::FromMilliseconds(1000 * i),
        MockTask::ContinuePolicy::CONTINUE_MORE_TIME_NEEDED);

    EXPECT_CALL(
        *mock_host_,
        DispatchProtocolMessage(
            &client_,
            base::StringPrintf(
                "{\"id\":%d,\"method\":\"Emulation.setVirtualTimePolicy\","
                "\"params\":{\"budget\":1000.0,"
                "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                "\"pauseIfNetworkFetchesPending\","
                "\"waitForNavigation\":false}}",
                i * 2)))
        .WillOnce(Return(true));

    SendVirtualTimeBudgetExpiredEvent();

    client_.DispatchProtocolMessage(
        mock_host_.get(),
        base::StringPrintf("{\"id\":%d,\"result\":{\"virtualTimeBase\":1.0,"
                           "\"virtualTimeTicksBase\":1.0}}",
                           i * 2));
  }

  task1.ExpectCallOnceWithOffsetAndReturn(
      base::TimeDelta::FromMilliseconds(4000),
      MockTask::ContinuePolicy::CONTINUE_MORE_TIME_NEEDED);
  // STOP should take precedence over CONTINUE_MORE_TIME_NEEDED.
  task2.ExpectCallOnceWithOffsetAndReturn(
      base::TimeDelta::FromMilliseconds(4000), MockTask::ContinuePolicy::STOP);
  task3.ExpectCallOnceWithOffsetAndReturn(
      base::TimeDelta::FromMilliseconds(4000),
      MockTask::ContinuePolicy::CONTINUE_MORE_TIME_NEEDED);

  EXPECT_CALL(observer,
              VirtualTimeStopped(base::TimeDelta::FromMilliseconds(4000)));

  SendVirtualTimeBudgetExpiredEvent();

  // If we start again, no task should block initially.
  EXPECT_CALL(observer,
              VirtualTimeStarted(base::TimeDelta::FromMilliseconds(4000)));

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":8,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  controller_->StartVirtualTime();

  client_.DispatchProtocolMessage(mock_host_.get(),
                                  "{\"id\":8,\"result\":{\"virtualTimeBase\":1."
                                  "0,\"virtualTimeTicksBase\":1.0}}");

  task1.ExpectCallOnceWithOffsetAndReturn(
      base::TimeDelta::FromMilliseconds(5000),
      MockTask::ContinuePolicy::CONTINUE_MORE_TIME_NEEDED);
  task2.ExpectCallOnceWithOffsetAndReturn(
      base::TimeDelta::FromMilliseconds(5000),
      MockTask::ContinuePolicy::CONTINUE_MORE_TIME_NEEDED);

  EXPECT_CALL(*mock_host_,
              DispatchProtocolMessage(
                  &client_,
                  "{\"id\":10,\"method\":\"Emulation.setVirtualTimePolicy\","
                  "\"params\":{\"budget\":1000.0,"
                  "\"maxVirtualTimeTaskStarvationCount\":0,\"policy\":"
                  "\"pauseIfNetworkFetchesPending\","
                  "\"waitForNavigation\":false}}"))
      .WillOnce(Return(true));

  SendVirtualTimeBudgetExpiredEvent();
}
}  // namespace headless
