// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/compositor_controller.h"

#include <memory>

#include "base/base64.h"
#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/test/test_simple_task_runner.h"
#include "headless/public/internal/headless_devtools_client_impl.h"
#include "headless/public/util/testing/mock_devtools_agent_host.h"
#include "headless/public/util/virtual_time_controller.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace headless {

namespace {
static constexpr base::TimeDelta kAnimationFrameInterval =
    base::TimeDelta::FromMilliseconds(16);
}  // namespace

using testing::_;
using testing::Return;

class TestVirtualTimeController : public VirtualTimeController {
 public:
  TestVirtualTimeController(HeadlessDevToolsClient* devtools_client)
      : VirtualTimeController(devtools_client) {}
  ~TestVirtualTimeController() override = default;

  MOCK_METHOD0(StartVirtualTime, void());
  MOCK_METHOD2(ScheduleRepeatingTask,
               void(RepeatingTask* task, base::TimeDelta interval));
  MOCK_METHOD1(CancelRepeatingTask, void(RepeatingTask* task));
  MOCK_METHOD1(AddObserver, void(Observer* observer));
  MOCK_METHOD1(RemoveObserver, void(Observer* observer));
  MOCK_METHOD1(SetResumeDeferrer, void(ResumeDeferrer* deferrer));

  MOCK_CONST_METHOD0(GetVirtualTimeBase, base::TimeTicks());
  MOCK_CONST_METHOD0(GetCurrentVirtualTimeOffset, base::TimeDelta());
};

class CompositorControllerTest : public ::testing::Test {
 protected:
  CompositorControllerTest(bool update_display_for_animations = true) {
    task_runner_ = base::MakeRefCounted<base::TestSimpleTaskRunner>();
    client_.SetTaskRunnerForTests(task_runner_);
    mock_host_ = base::MakeRefCounted<MockDevToolsAgentHost>();

    EXPECT_CALL(*mock_host_, AttachClient(&client_));
    client_.AttachToHost(mock_host_.get());
    virtual_time_controller_ =
        std::make_unique<TestVirtualTimeController>(&client_);
    EXPECT_CALL(*virtual_time_controller_,
                ScheduleRepeatingTask(_, kAnimationFrameInterval))
        .WillOnce(testing::SaveArg<0>(&task_));
    EXPECT_CALL(*virtual_time_controller_, SetResumeDeferrer(_))
        .WillOnce(testing::SaveArg<0>(&deferrer_));
    ExpectHeadlessExperimentalEnable();
    controller_ = std::make_unique<CompositorController>(
        task_runner_, &client_, virtual_time_controller_.get(),
        kAnimationFrameInterval, update_display_for_animations);
    EXPECT_NE(nullptr, task_);
  }

  ~CompositorControllerTest() override {
    EXPECT_CALL(*virtual_time_controller_, CancelRepeatingTask(_));
    EXPECT_CALL(*virtual_time_controller_, SetResumeDeferrer(_));
  }

  void ExpectHeadlessExperimentalEnable() {
    last_command_id_ += 2;
    EXPECT_CALL(*mock_host_,
                DispatchProtocolMessage(
                    &client_,
                    base::StringPrintf(
                        "{\"id\":%d,\"method\":\"HeadlessExperimental.enable\","
                        "\"params\":{}}",
                        last_command_id_)))
        .WillOnce(Return(true));
  }

  void ExpectVirtualTime(double base, double offset) {
    auto base_time_ticks =
        base::TimeTicks() + base::TimeDelta::FromMillisecondsD(base);
    auto offset_delta = base::TimeDelta::FromMilliseconds(offset);
    EXPECT_CALL(*virtual_time_controller_, GetVirtualTimeBase())
        .WillOnce(Return(base_time_ticks));
    EXPECT_CALL(*virtual_time_controller_, GetCurrentVirtualTimeOffset())
        .WillOnce(Return(offset_delta));

    // Next BeginFrame's time should be the virtual time provided it has
    // progressed.
    base::TimeTicks virtual_time = base_time_ticks + offset_delta;
    if (virtual_time > next_begin_frame_time_)
      next_begin_frame_time_ = virtual_time;
  }

  void ExpectBeginFrame(bool no_display_updates = false,
                        std::unique_ptr<headless_experimental::ScreenshotParams>
                            screenshot_params = nullptr) {
    last_command_id_ += 2;
    base::DictionaryValue params;
    auto builder = std::move(
        headless_experimental::BeginFrameParams::Builder()
            .SetFrameTimeTicks(
                (next_begin_frame_time_ - base::TimeTicks()).InMillisecondsF())
            .SetInterval(kAnimationFrameInterval.InMillisecondsF())
            .SetNoDisplayUpdates(no_display_updates));
    if (screenshot_params)
      builder.SetScreenshot(std::move(screenshot_params));
    // Subsequent BeginFrames should have a later timestamp.
    next_begin_frame_time_ += base::TimeDelta::FromMicroseconds(1);

    std::string params_json;
    auto params_value = builder.Build()->Serialize();
    base::JSONWriter::Write(*params_value, &params_json);

    EXPECT_CALL(
        *mock_host_,
        DispatchProtocolMessage(
            &client_,
            base::StringPrintf(
                "{\"id\":%d,\"method\":\"HeadlessExperimental.beginFrame\","
                "\"params\":%s}",
                last_command_id_, params_json.c_str())))
        .WillOnce(Return(true));
  }

  void SendBeginFrameReply(bool has_damage,
                           const std::string& screenshot_data) {
    auto result = headless_experimental::BeginFrameResult::Builder()
                      .SetHasDamage(has_damage)
                      .Build();
    if (screenshot_data.length())
      result->SetScreenshotData(screenshot_data);
    std::string result_json;
    auto result_value = result->Serialize();
    base::JSONWriter::Write(*result_value, &result_json);

    client_.DispatchProtocolMessage(
        mock_host_.get(),
        base::StringPrintf("{\"id\":%d,\"result\":%s}", last_command_id_,
                           result_json.c_str()));
    task_runner_->RunPendingTasks();
  }

  void SendNeedsBeginFramesEvent(bool needs_begin_frames) {
    client_.DispatchProtocolMessage(
        mock_host_.get(),
        base::StringPrintf("{\"method\":\"HeadlessExperimental."
                           "needsBeginFramesChanged\",\"params\":{"
                           "\"needsBeginFrames\":%s}}",
                           needs_begin_frames ? "true" : "false"));
    // Events are dispatched asynchronously.
    task_runner_->RunPendingTasks();
  }

  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  scoped_refptr<MockDevToolsAgentHost> mock_host_;
  HeadlessDevToolsClientImpl client_;
  std::unique_ptr<TestVirtualTimeController> virtual_time_controller_;
  std::unique_ptr<CompositorController> controller_;
  int last_command_id_ = -2;
  TestVirtualTimeController::RepeatingTask* task_ = nullptr;
  TestVirtualTimeController::Observer* observer_ = nullptr;
  TestVirtualTimeController::ResumeDeferrer* deferrer_ = nullptr;
  base::TimeTicks next_begin_frame_time_ =
      base::TimeTicks() + base::TimeDelta::FromMicroseconds(1);
};

TEST_F(CompositorControllerTest, CaptureScreenshot) {
  bool done = false;
  controller_->CaptureScreenshot(
      headless_experimental::ScreenshotParamsFormat::PNG, 100,
      base::BindRepeating(
          [](bool* done, const std::string& screenshot_data) {
            *done = true;
            EXPECT_EQ("test", screenshot_data);
          },
          &done));

  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectVirtualTime(0, 0);
  ExpectBeginFrame(
      false, headless_experimental::ScreenshotParams::Builder()
                 .SetFormat(headless_experimental::ScreenshotParamsFormat::PNG)
                 .SetQuality(100)
                 .Build());
  task_runner_->RunPendingTasks();

  std::string base64;
  base::Base64Encode("test", &base64);
  SendBeginFrameReply(true, base64);
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(done);
}

TEST_F(CompositorControllerTest, SendsAnimationFrames) {
  base::Optional<VirtualTimeController::RepeatingTask::ContinuePolicy>
      continue_policy;
  auto continue_callback = base::BindRepeating(
      [](base::Optional<VirtualTimeController::RepeatingTask::ContinuePolicy>*
             continue_policy,
         VirtualTimeController::RepeatingTask::ContinuePolicy policy) {
        *continue_policy = policy;
      },
      &continue_policy);

  // Doesn't send BeginFrames before virtual time started.
  SendNeedsBeginFramesEvent(true);
  EXPECT_FALSE(task_runner_->HasPendingTask());

  bool can_continue = false;
  auto defer_callback = base::BindRepeating(
      [](bool* can_continue) { *can_continue = true; }, &can_continue);

  // Sends a BeginFrame at start of interval.
  deferrer_->DeferResume(defer_callback);
  EXPECT_TRUE(task_runner_->HasPendingTask());

  ExpectVirtualTime(1000, 0);
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();
  EXPECT_FALSE(can_continue);

  // Lets virtual time continue after BeginFrame was completed.
  SendBeginFrameReply(false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(can_continue);
  can_continue = false;

  // Sends a BeginFrame after interval elapsed, but only just before virtual
  // time resumes.
  task_->IntervalElapsed(kAnimationFrameInterval, continue_callback);
  EXPECT_EQ(VirtualTimeController::RepeatingTask::ContinuePolicy::NOT_REQUIRED,
            *continue_policy);
  continue_policy = base::nullopt;
  EXPECT_FALSE(task_runner_->HasPendingTask());

  deferrer_->DeferResume(defer_callback);
  EXPECT_TRUE(task_runner_->HasPendingTask());

  ExpectVirtualTime(1000, kAnimationFrameInterval.InMillisecondsF());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();
  EXPECT_FALSE(can_continue);

  // Lets virtual time continue after BeginFrame was completed.
  SendBeginFrameReply(false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(can_continue);
  can_continue = false;

  // Doesn't send a BeginFrame if another task pauses and resumes virtual time.
  deferrer_->DeferResume(defer_callback);
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(can_continue);
  can_continue = false;

  // Sends another BeginFrame after next animation interval elapsed.
  task_->IntervalElapsed(kAnimationFrameInterval, continue_callback);
  EXPECT_EQ(VirtualTimeController::RepeatingTask::ContinuePolicy::NOT_REQUIRED,
            *continue_policy);
  continue_policy = base::nullopt;
  EXPECT_FALSE(task_runner_->HasPendingTask());

  deferrer_->DeferResume(defer_callback);
  EXPECT_TRUE(task_runner_->HasPendingTask());

  ExpectVirtualTime(1000, kAnimationFrameInterval.InMillisecondsF() * 2);
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();
  EXPECT_FALSE(can_continue);

  // Lets virtual time continue after BeginFrame was completed.
  SendBeginFrameReply(false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(can_continue);
  can_continue = false;
}

TEST_F(CompositorControllerTest, WaitUntilIdle) {
  bool idle = false;
  auto idle_callback =
      base::BindRepeating([](bool* idle) { *idle = true; }, &idle);

  SendNeedsBeginFramesEvent(true);
  EXPECT_FALSE(task_runner_->HasPendingTask());

  // WaitUntilIdle executes callback immediately if no BeginFrame is active.
  controller_->WaitUntilIdle(idle_callback);
  EXPECT_TRUE(idle);
  idle = false;

  // Send a BeginFrame.
  task_->IntervalElapsed(
      kAnimationFrameInterval,
      base::BindRepeating(
          [](VirtualTimeController::RepeatingTask::ContinuePolicy) {}));
  EXPECT_FALSE(task_runner_->HasPendingTask());

  bool can_continue = false;
  auto defer_callback = base::BindRepeating(
      [](bool* can_continue) { *can_continue = true; }, &can_continue);

  deferrer_->DeferResume(defer_callback);
  EXPECT_TRUE(task_runner_->HasPendingTask());

  ExpectVirtualTime(1000, kAnimationFrameInterval.InMillisecondsF());
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();
  EXPECT_FALSE(can_continue);

  // WaitUntilIdle only executes callback after BeginFrame was completed.
  controller_->WaitUntilIdle(idle_callback);
  EXPECT_FALSE(idle);

  SendBeginFrameReply(false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(idle);
  idle = false;
  EXPECT_TRUE(can_continue);
  can_continue = false;
}

class CompositorControllerNoDisplayUpdateTest
    : public CompositorControllerTest {
 protected:
  CompositorControllerNoDisplayUpdateTest() : CompositorControllerTest(false) {}
};

TEST_F(CompositorControllerNoDisplayUpdateTest,
       SkipsDisplayUpdateOnlyForAnimationFrames) {
  base::Optional<VirtualTimeController::RepeatingTask::ContinuePolicy>
      continue_policy;
  auto continue_callback = base::BindRepeating(
      [](base::Optional<VirtualTimeController::RepeatingTask::ContinuePolicy>*
             continue_policy,
         VirtualTimeController::RepeatingTask::ContinuePolicy policy) {
        *continue_policy = policy;
      },
      &continue_policy);

  SendNeedsBeginFramesEvent(true);
  EXPECT_FALSE(task_runner_->HasPendingTask());

  bool can_continue = false;
  auto defer_callback = base::BindRepeating(
      [](bool* can_continue) { *can_continue = true; }, &can_continue);

  // Initial animation-BeginFrame always updates display (see comment in
  // compositor_controller.cc).
  deferrer_->DeferResume(defer_callback);
  EXPECT_TRUE(task_runner_->HasPendingTask());

  ExpectVirtualTime(1000, 0);
  ExpectBeginFrame();
  task_runner_->RunPendingTasks();
  EXPECT_FALSE(can_continue);

  // Lets virtual time continue after BeginFrame was completed.
  SendBeginFrameReply(false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(can_continue);
  can_continue = false;

  // Sends an animation BeginFrame without display update after interval
  // elapsed.
  task_->IntervalElapsed(kAnimationFrameInterval, continue_callback);
  EXPECT_EQ(VirtualTimeController::RepeatingTask::ContinuePolicy::NOT_REQUIRED,
            *continue_policy);
  continue_policy = base::nullopt;
  EXPECT_FALSE(task_runner_->HasPendingTask());

  deferrer_->DeferResume(defer_callback);
  EXPECT_TRUE(task_runner_->HasPendingTask());

  ExpectVirtualTime(1000, kAnimationFrameInterval.InMillisecondsF());
  ExpectBeginFrame(true);
  task_runner_->RunPendingTasks();
  EXPECT_FALSE(can_continue);

  // Lets virtual time continue after BeginFrame was completed.
  SendBeginFrameReply(false, std::string());
  EXPECT_FALSE(task_runner_->HasPendingTask());
  EXPECT_TRUE(can_continue);
  can_continue = false;

  // Screenshots update display.
  task_->IntervalElapsed(kAnimationFrameInterval, continue_callback);
  EXPECT_EQ(VirtualTimeController::RepeatingTask::ContinuePolicy::NOT_REQUIRED,
            *continue_policy);
  continue_policy = base::nullopt;
  EXPECT_FALSE(task_runner_->HasPendingTask());

  controller_->CaptureScreenshot(
      headless_experimental::ScreenshotParamsFormat::PNG, 100,
      base::BindRepeating([](const std::string&) {}));

  EXPECT_TRUE(task_runner_->HasPendingTask());
  ExpectVirtualTime(1000, kAnimationFrameInterval.InMillisecondsF() * 2);
  ExpectBeginFrame(
      false, headless_experimental::ScreenshotParams::Builder()
                 .SetFormat(headless_experimental::ScreenshotParamsFormat::PNG)
                 .SetQuality(100)
                 .Build());
  task_runner_->RunPendingTasks();
}

}  // namespace headless
