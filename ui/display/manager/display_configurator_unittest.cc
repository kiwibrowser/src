// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/manager/display_configurator.h"

#include <stddef.h>
#include <stdint.h>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "chromeos/chromeos_switches.cc"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/manager/fake_display_snapshot.h"
#include "ui/display/manager/test/action_logger_util.h"
#include "ui/display/manager/test/test_native_display_delegate.h"
#include "ui/display/util/display_util.h"

namespace display {
namespace test {

namespace {

constexpr int64_t kDisplayIds[3] = {123, 456, 789};

std::unique_ptr<DisplayMode> MakeDisplayMode(int width,
                                             int height,
                                             bool is_interlaced,
                                             float refresh_rate) {
  return std::make_unique<DisplayMode>(gfx::Size(width, height), is_interlaced,
                                       refresh_rate);
}

enum CallbackResult {
  CALLBACK_FAILURE,
  CALLBACK_SUCCESS,
  CALLBACK_NOT_CALLED,
};

// Expected immediate configurations should be done without any delays.
constexpr base::TimeDelta kNoDelay = base::TimeDelta::FromMilliseconds(0);

// The expected configuration delay when resuming from suspend while in 2+
// display mode.
constexpr base::TimeDelta kLongDelay = base::TimeDelta::FromMilliseconds(
    DisplayConfigurator::kResumeConfigureMultiDisplayDelayMs);

class TestObserver : public DisplayConfigurator::Observer {
 public:
  explicit TestObserver(DisplayConfigurator* configurator)
      : configurator_(configurator) {
    Reset();
    configurator_->AddObserver(this);
  }
  ~TestObserver() override { configurator_->RemoveObserver(this); }

  int num_changes() const { return num_changes_; }
  int num_failures() const { return num_failures_; }
  const DisplayConfigurator::DisplayStateList& latest_outputs() const {
    return latest_outputs_;
  }
  MultipleDisplayState latest_failed_state() const {
    return latest_failed_state_;
  }

  void Reset() {
    num_changes_ = 0;
    num_failures_ = 0;
    latest_outputs_.clear();
    latest_failed_state_ = MULTIPLE_DISPLAY_STATE_INVALID;
  }

  // DisplayConfigurator::Observer overrides:
  void OnDisplayModeChanged(
      const DisplayConfigurator::DisplayStateList& outputs) override {
    num_changes_++;
    latest_outputs_ = outputs;
  }

  void OnDisplayModeChangeFailed(
      const DisplayConfigurator::DisplayStateList& outputs,
      MultipleDisplayState failed_new_state) override {
    num_failures_++;
    latest_failed_state_ = failed_new_state;
  }

 private:
  DisplayConfigurator* configurator_;  // Not owned.

  // Number of times that OnDisplayMode*() has been called.
  int num_changes_;
  int num_failures_;

  // Parameters most recently passed to OnDisplayMode*().
  DisplayConfigurator::DisplayStateList latest_outputs_;
  MultipleDisplayState latest_failed_state_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

class TestStateController : public DisplayConfigurator::StateController {
 public:
  TestStateController() : state_(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED) {}
  ~TestStateController() override {}

  void set_state(MultipleDisplayState state) { state_ = state; }

  // DisplayConfigurator::StateController overrides:
  MultipleDisplayState GetStateForDisplayIds(
      const DisplayConfigurator::DisplayStateList& outputs) override {
    return state_;
  }
  bool GetResolutionForDisplayId(int64_t display_id,
                                 gfx::Size* size) const override {
    return false;
  }

 private:
  MultipleDisplayState state_;

  DISALLOW_COPY_AND_ASSIGN(TestStateController);
};

class TestMirroringController
    : public DisplayConfigurator::SoftwareMirroringController {
 public:
  TestMirroringController() : software_mirroring_enabled_(false) {}
  ~TestMirroringController() override {}

  void SetSoftwareMirroring(bool enabled) override {
    software_mirroring_enabled_ = enabled;
  }

  bool SoftwareMirroringEnabled() const override {
    return software_mirroring_enabled_;
  }

  bool IsSoftwareMirroringEnforced() const override { return false; }

 private:
  bool software_mirroring_enabled_;

  DISALLOW_COPY_AND_ASSIGN(TestMirroringController);
};

// Abstracts waiting for the display configuration to be completed and getting
// the time it took to complete.
class ConfigurationWaiter {
 public:
  explicit ConfigurationWaiter(DisplayConfigurator::TestApi* test_api)
      : on_configured_callback_(base::Bind(&ConfigurationWaiter::OnConfigured,
                                           base::Unretained(this))),
        test_api_(test_api),
        callback_result_(CALLBACK_NOT_CALLED) {}

  ~ConfigurationWaiter() = default;

  const DisplayConfigurator::ConfigurationCallback& on_configuration_callback()
      const {
    return on_configured_callback_;
  }

  CallbackResult callback_result() const { return callback_result_; }

  void Reset() { callback_result_ = CALLBACK_NOT_CALLED; }

  // Simulates waiting for the next configuration. If an async task is pending,
  // runs it and returns base::TimeDelta(). Otherwise, triggers the
  // configuration timer and returns its delay. If the timer wasn't running,
  // returns base::TimeDelta::Max().
  base::TimeDelta Wait() WARN_UNUSED_RESULT {
    base::RunLoop().RunUntilIdle();
    if (callback_result_ != CALLBACK_NOT_CALLED)
      return base::TimeDelta();

    const base::TimeDelta delay = test_api_->GetConfigureDelay();
    if (!test_api_->TriggerConfigureTimeout())
      return base::TimeDelta::Max();

    return delay;
  }

 private:
  void OnConfigured(bool status) {
    CHECK_EQ(callback_result_, CALLBACK_NOT_CALLED);
    callback_result_ = status ? CALLBACK_SUCCESS : CALLBACK_FAILURE;
  }

  // Passed with configuration requests to run OnConfigured().
  const DisplayConfigurator::ConfigurationCallback on_configured_callback_;

  DisplayConfigurator::TestApi* test_api_;  // Not owned.

  // The status of the display configuration.
  CallbackResult callback_result_;

  DISALLOW_COPY_AND_ASSIGN(ConfigurationWaiter);
};

class DisplayConfiguratorTest : public testing::Test {
 public:
  DisplayConfiguratorTest()
      : small_mode_(gfx::Size(1366, 768), false, 60.0f),
        big_mode_(gfx::Size(2560, 1600), false, 60.0f),
        observer_(&configurator_),
        test_api_(&configurator_),
        config_waiter_(&test_api_),
        set_content_protection_status_(false),
        set_content_protection_call_count_(0),
        query_content_protection_response_success_(false),
        query_content_protection_response_link_mask_(0),
        query_content_protection_response_protection_mask_(0),
        query_content_protection_call_count_(0),
        display_control_result_(CALLBACK_NOT_CALLED) {}
  ~DisplayConfiguratorTest() override {}

  void SetUp() override {
    log_.reset(new ActionLogger());

    // Force system compositor mode to simulate on-device configurator behavior.
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        chromeos::switches::kForceSystemCompositorMode);

    native_display_delegate_ = new TestNativeDisplayDelegate(log_.get());
    configurator_.SetDelegateForTesting(
        std::unique_ptr<NativeDisplayDelegate>(native_display_delegate_));

    configurator_.set_state_controller(&state_controller_);
    configurator_.set_mirroring_controller(&mirroring_controller_);

    outputs_[0] = FakeDisplaySnapshot::Builder()
                      .SetId(kDisplayIds[0])
                      .SetNativeMode(small_mode_.Clone())
                      .SetCurrentMode(small_mode_.Clone())
                      .SetType(DISPLAY_CONNECTION_TYPE_INTERNAL)
                      .SetIsAspectPerservingScaling(true)
                      .Build();

    outputs_[1] = FakeDisplaySnapshot::Builder()
                      .SetId(kDisplayIds[1])
                      .SetNativeMode(big_mode_.Clone())
                      .SetCurrentMode(big_mode_.Clone())
                      .AddMode(small_mode_.Clone())
                      .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
                      .SetIsAspectPerservingScaling(true)
                      .Build();

    outputs_[2] = FakeDisplaySnapshot::Builder()
                      .SetId(kDisplayIds[2])
                      .SetNativeMode(small_mode_.Clone())
                      .SetCurrentMode(small_mode_.Clone())
                      .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
                      .SetIsAspectPerservingScaling(true)
                      .Build();

    UpdateOutputs(2, false);
  }

  void OnDisplayControlUpdated(bool status) {
    display_control_result_ = (status ? CALLBACK_SUCCESS : CALLBACK_FAILURE);
  }

  void SetContentProtectionCallback(bool status) {
    set_content_protection_status_ = status;
    set_content_protection_call_count_++;
  }

  void QueryContentProtectionCallback(bool success,
                                      uint32_t link_mask,
                                      uint32_t protection_mask) {
    query_content_protection_response_success_ = success;
    query_content_protection_response_link_mask_ = link_mask;
    query_content_protection_response_protection_mask_ = protection_mask;
    query_content_protection_call_count_++;
  }

  // Predefined modes that can be used by outputs.
  const DisplayMode small_mode_;
  const DisplayMode big_mode_;

 protected:
  // Configures |native_display_delegate_| to return the first |num_outputs|
  // entries from
  // |outputs_|. If |send_events| is true, also sends screen-change and
  // output-change events to |configurator_| and triggers the configure
  // timeout if one was scheduled.
  void UpdateOutputs(size_t num_outputs, bool send_events) {
    ASSERT_LE(num_outputs, arraysize(outputs_));
    std::vector<DisplaySnapshot*> outputs;
    for (size_t i = 0; i < num_outputs; ++i)
      outputs.push_back(outputs_[i].get());
    native_display_delegate_->set_outputs(outputs);

    if (send_events) {
      configurator_.OnConfigurationChanged();
      EXPECT_TRUE(test_api_.TriggerConfigureTimeout());
    }
  }

  void Init(bool panel_fitting_enabled) {
    configurator_.Init(nullptr, panel_fitting_enabled);
  }

  // Initializes |configurator_| with a single internal display.
  void InitWithSingleOutput() {
    UpdateOutputs(1, false);
    EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
    configurator_.Init(nullptr, false);

    EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
    configurator_.ForceInitialConfigure();
    EXPECT_EQ(
        JoinActions(
            kInit,
            GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
            nullptr),
        log_->GetActionsAndClear());
  }

  CallbackResult PopDisplayControlResult() {
    CallbackResult result = display_control_result_;
    display_control_result_ = CALLBACK_NOT_CALLED;
    return result;
  }

  base::MessageLoop message_loop_;
  TestStateController state_controller_;
  TestMirroringController mirroring_controller_;
  DisplayConfigurator configurator_;
  TestObserver observer_;
  std::unique_ptr<ActionLogger> log_;
  TestNativeDisplayDelegate* native_display_delegate_;  // not owned
  DisplayConfigurator::TestApi test_api_;
  ConfigurationWaiter config_waiter_;
  bool set_content_protection_status_;
  int set_content_protection_call_count_;
  bool query_content_protection_response_success_;
  uint32_t query_content_protection_response_link_mask_;
  uint32_t query_content_protection_response_protection_mask_;
  int query_content_protection_call_count_;

  std::unique_ptr<DisplaySnapshot> outputs_[3];

  CallbackResult display_control_result_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DisplayConfiguratorTest);
};

}  // namespace

TEST_F(DisplayConfiguratorTest, FindDisplayModeMatchingSize) {
  std::unique_ptr<DisplaySnapshot> output =
      FakeDisplaySnapshot::Builder()
          .SetId(kDisplayIds[0])
          .AddMode(MakeDisplayMode(1920, 1200, false, 60.0))
          .SetNativeMode(MakeDisplayMode(1920, 1200, false, 50.0))
          // Different rates.
          .AddMode(MakeDisplayMode(1920, 1080, false, 30.0))
          .AddMode(MakeDisplayMode(1920, 1080, false, 50.0))
          .AddMode(MakeDisplayMode(1920, 1080, false, 40.0))
          .AddMode(MakeDisplayMode(1920, 1080, false, 0.0))
          // Interlaced vs non-interlaced.
          .AddMode(MakeDisplayMode(1280, 720, true, 60.0))
          .AddMode(MakeDisplayMode(1280, 720, false, 40.0))
          // Interlaced only.
          .AddMode(MakeDisplayMode(1024, 768, true, 0.0))
          .AddMode(MakeDisplayMode(1024, 768, true, 40.0))
          .AddMode(MakeDisplayMode(1024, 768, true, 60.0))
          // Mixed.
          .AddMode(MakeDisplayMode(1024, 600, true, 60.0))
          .AddMode(MakeDisplayMode(1024, 600, false, 40.0))
          .AddMode(MakeDisplayMode(1024, 600, false, 50.0))
          // Just one interlaced mode.
          .AddMode(MakeDisplayMode(640, 480, true, 60.0))
          // Refresh rate not available.
          .AddMode(MakeDisplayMode(320, 200, false, 0.0))
          .Build();

  const std::vector<std::unique_ptr<const DisplayMode>>& modes =
      output->modes();

  // Should pick native over highest refresh rate.
  EXPECT_EQ(modes[1].get(), DisplayConfigurator::FindDisplayModeMatchingSize(
                                *output, gfx::Size(1920, 1200)));

  // Should pick highest refresh rate.
  EXPECT_EQ(modes[3].get(), DisplayConfigurator::FindDisplayModeMatchingSize(
                                *output, gfx::Size(1920, 1080)));

  // Should pick non-interlaced mode.
  EXPECT_EQ(modes[7].get(), DisplayConfigurator::FindDisplayModeMatchingSize(
                                *output, gfx::Size(1280, 720)));

  // Interlaced only. Should pick one with the highest refresh rate in
  // interlaced mode.
  EXPECT_EQ(modes[10].get(), DisplayConfigurator::FindDisplayModeMatchingSize(
                                 *output, gfx::Size(1024, 768)));

  // Mixed: Should pick one with the highest refresh rate in
  // interlaced mode.
  EXPECT_EQ(modes[13].get(), DisplayConfigurator::FindDisplayModeMatchingSize(
                                 *output, gfx::Size(1024, 600)));

  // Just one interlaced mode.
  EXPECT_EQ(modes[14].get(), DisplayConfigurator::FindDisplayModeMatchingSize(
                                 *output, gfx::Size(640, 480)));

  // Refresh rate not available.
  EXPECT_EQ(modes[15].get(), DisplayConfigurator::FindDisplayModeMatchingSize(
                                 *output, gfx::Size(320, 200)));

  // No mode found.
  EXPECT_EQ(nullptr, DisplayConfigurator::FindDisplayModeMatchingSize(
                         *output, gfx::Size(1440, 900)));
}

TEST_F(DisplayConfiguratorTest, ConnectSecondOutput) {
  InitWithSingleOutput();

  // Connect a second output and check that the configurator enters
  // extended mode.
  observer_.Reset();
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  UpdateOutputs(2, true);

  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());

  observer_.Reset();
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());

  // Disconnect the second output.
  observer_.Reset();
  UpdateOutputs(1, true);
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());

  // Get rid of shared modes to force software mirroring.
  outputs_[1] = FakeDisplaySnapshot::Builder()
                    .SetId(kDisplayIds[1])
                    .SetNativeMode(big_mode_.Clone())
                    .SetCurrentMode(big_mode_.Clone())
                    .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
                    .SetIsAspectPerservingScaling(true)
                    .Build();

  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  UpdateOutputs(2, true);
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());

  observer_.Reset();
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());
  EXPECT_TRUE(mirroring_controller_.SoftwareMirroringEnabled());

  EXPECT_EQ(1, observer_.num_changes());

  // Setting MULTIPLE_DISPLAY_STATE_DUAL_MIRROR should try to reconfigure.
  observer_.Reset();
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
  EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());

  // Set back to software mirror mode.
  observer_.Reset();
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());
  EXPECT_TRUE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());

  // Disconnect the second output.
  observer_.Reset();
  UpdateOutputs(1, true);
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());
}

TEST_F(DisplayConfiguratorTest, SetDisplayPower) {
  InitWithSingleOutput();

  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  observer_.Reset();
  UpdateOutputs(2, true);
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());

  // Turning off the internal display should switch the external display to
  // its native mode.
  observer_.Reset();
  config_waiter_.Reset();
  configurator_.SetDisplayPower(
      chromeos::DISPLAY_POWER_INTERNAL_OFF_EXTERNAL_ON,
      DisplayConfigurator::kSetDisplayPowerNoFlags,
      config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_SINGLE, configurator_.display_state());
  EXPECT_EQ(1, observer_.num_changes());

  // When all displays are turned off, the framebuffer should switch back
  // to the mirrored size.
  observer_.Reset();
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_OFF,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(JoinActions(
                GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
                GetCrtcAction(*outputs_[1], nullptr, gfx::Point(0, 0)).c_str(),
                nullptr),
            log_->GetActionsAndClear());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR, configurator_.display_state());
  EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());

  // Turn all displays on and check that mirroring is still used.
  observer_.Reset();
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR, configurator_.display_state());
  EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());

  // Get rid of shared modes to force software mirroring.
  outputs_[1] = FakeDisplaySnapshot::Builder()
                    .SetId(kDisplayIds[1])
                    .SetNativeMode(big_mode_.Clone())
                    .SetCurrentMode(big_mode_.Clone())
                    .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
                    .SetIsAspectPerservingScaling(true)
                    .Build();

  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  observer_.Reset();
  UpdateOutputs(2, true);

  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());
  EXPECT_TRUE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());

  // Turning off the internal display should switch the external display to
  // its native mode.
  observer_.Reset();
  config_waiter_.Reset();
  configurator_.SetDisplayPower(
      chromeos::DISPLAY_POWER_INTERNAL_OFF_EXTERNAL_ON,
      DisplayConfigurator::kSetDisplayPowerNoFlags,
      config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_SINGLE, configurator_.display_state());
  EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());

  // When all displays are turned off, the framebuffer should switch back
  // to the extended + software mirroring.
  observer_.Reset();
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_OFF,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], nullptr,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());
  EXPECT_TRUE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());

  // Turn all displays on and check that mirroring is still used.
  observer_.Reset();
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());
  EXPECT_TRUE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());
}

TEST_F(DisplayConfiguratorTest, SuspendAndResume) {
  InitWithSingleOutput();

  // Set the initial power state to on.
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());

  // No preparation is needed before suspending when the display is already
  // on.  The configurator should still reprobe on resume in case a display
  // was connected while suspended.
  config_waiter_.Reset();
  configurator_.SuspendDisplays(config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // No resume delay in single display mode.
  config_waiter_.Reset();
  configurator_.ResumeDisplays();
  // The timer should not be running.
  EXPECT_EQ(base::TimeDelta::Max(), config_waiter_.Wait());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // Now turn the display off before suspending and check that the
  // configurator turns it back on and syncs with the server.
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_OFF,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  config_waiter_.Reset();
  configurator_.SuspendDisplays(config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  config_waiter_.Reset();
  configurator_.ResumeDisplays();
  // The timer should not be running.
  EXPECT_EQ(base::TimeDelta::Max(), config_waiter_.Wait());
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  UpdateOutputs(2, true);
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_OFF,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR, configurator_.display_state());
  EXPECT_EQ(JoinActions(
                GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
                GetCrtcAction(*outputs_[1], nullptr, gfx::Point(0, 0)).c_str(),
                nullptr),
            log_->GetActionsAndClear());

  // No delay in suspend.
  config_waiter_.Reset();
  configurator_.SuspendDisplays(config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(chromeos::DISPLAY_POWER_ALL_OFF,
            configurator_.current_power_state());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR, configurator_.display_state());
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  // If a display is disconnected while suspended, the configurator should
  // pick up the change and only turn on the internal display. The should be
  // a longer configuration delay when we set the displays back to on.
  UpdateOutputs(1, false);
  config_waiter_.Reset();
  configurator_.ResumeDisplays();
  // Since we were in dual display mirror mode before suspend, the timer should
  // be running with kMinLongDelayMs.
  EXPECT_EQ(kLongDelay, test_api_.GetConfigureDelay());
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(CALLBACK_NOT_CALLED, config_waiter_.callback_result());
  EXPECT_EQ(kLongDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
}

TEST_F(DisplayConfiguratorTest, Headless) {
  UpdateOutputs(0, false);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
  Init(false);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
  configurator_.ForceInitialConfigure();
  EXPECT_EQ(JoinActions(kInit, nullptr), log_->GetActionsAndClear());

  // Not much should happen when the display power state is changed while
  // no displays are connected.
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_OFF,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  // Connect an external display and check that it's configured correctly.
  outputs_[0] = FakeDisplaySnapshot::Builder()
                    .SetId(kDisplayIds[0])
                    .SetNativeMode(big_mode_.Clone())
                    .SetCurrentMode(big_mode_.Clone())
                    .AddMode(small_mode_.Clone())
                    .SetType(DISPLAY_CONNECTION_TYPE_INTERNAL)
                    .SetIsAspectPerservingScaling(true)
                    .Build();

  UpdateOutputs(1, true);
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &big_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  UpdateOutputs(0, true);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
}

TEST_F(DisplayConfiguratorTest, StartWithTwoOutputs) {
  UpdateOutputs(2, false);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
  Init(false);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  configurator_.ForceInitialConfigure();
  EXPECT_EQ(
      JoinActions(
          kInit,
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
}

TEST_F(DisplayConfiguratorTest, InvalidMultipleDisplayStates) {
  UpdateOutputs(0, false);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
  Init(false);
  configurator_.ForceInitialConfigure();
  observer_.Reset();
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_HEADLESS);
  EXPECT_EQ(1, observer_.num_changes());
  EXPECT_EQ(0, observer_.num_failures());
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_SINGLE);
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  EXPECT_EQ(1, observer_.num_changes());
  EXPECT_EQ(3, observer_.num_failures());

  UpdateOutputs(1, true);
  observer_.Reset();
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_HEADLESS);
  EXPECT_EQ(0, observer_.num_changes());
  EXPECT_EQ(1, observer_.num_failures());
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_SINGLE);
  EXPECT_EQ(1, observer_.num_changes());
  EXPECT_EQ(1, observer_.num_failures());
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  EXPECT_EQ(1, observer_.num_changes());
  EXPECT_EQ(3, observer_.num_failures());

  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  UpdateOutputs(2, true);
  observer_.Reset();
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_HEADLESS);
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_SINGLE);
  EXPECT_EQ(0, observer_.num_changes());
  EXPECT_EQ(2, observer_.num_failures());
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  EXPECT_EQ(2, observer_.num_changes());
  EXPECT_EQ(2, observer_.num_failures());
}

TEST_F(DisplayConfiguratorTest, GetMultipleDisplayStateForMirroredDisplays) {
  UpdateOutputs(2, false);
  Init(false);
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  configurator_.ForceInitialConfigure();
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR, configurator_.display_state());
}

TEST_F(DisplayConfiguratorTest, UpdateCachedOutputsEvenAfterFailure) {
  InitWithSingleOutput();
  const DisplayConfigurator::DisplayStateList& cached =
      configurator_.cached_displays();
  ASSERT_EQ(static_cast<size_t>(1), cached.size());
  EXPECT_EQ(outputs_[0]->current_mode(), cached[0]->current_mode());

  // After connecting a second output, check that it shows up in
  // |cached_displays_| even if an invalid state is requested.
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_SINGLE);
  UpdateOutputs(2, true);
  ASSERT_EQ(static_cast<size_t>(2), cached.size());
  EXPECT_EQ(outputs_[0]->current_mode(), cached[0]->current_mode());
  EXPECT_EQ(outputs_[1]->current_mode(), cached[1]->current_mode());
}


TEST_F(DisplayConfiguratorTest, ContentProtection) {
  Init(false);
  configurator_.ForceInitialConfigure();
  EXPECT_NE(kNoActions, log_->GetActionsAndClear());

  uint64_t id = configurator_.RegisterContentProtectionClient();
  EXPECT_NE(0u, id);

  // One output.
  UpdateOutputs(1, true);
  EXPECT_NE(kNoActions, log_->GetActionsAndClear());
  configurator_.QueryContentProtectionStatus(
      id, outputs_[0]->display_id(),
      base::Bind(&DisplayConfiguratorTest::QueryContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(1, query_content_protection_call_count_);
  EXPECT_TRUE(query_content_protection_response_success_);
  EXPECT_EQ(static_cast<uint32_t>(DISPLAY_CONNECTION_TYPE_INTERNAL),
            query_content_protection_response_link_mask_);
  EXPECT_EQ(static_cast<uint32_t>(CONTENT_PROTECTION_METHOD_NONE),
            query_content_protection_response_protection_mask_);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  // Two outputs.
  UpdateOutputs(2, true);
  EXPECT_NE(kNoActions, log_->GetActionsAndClear());
  configurator_.QueryContentProtectionStatus(
      id, outputs_[1]->display_id(),
      base::Bind(&DisplayConfiguratorTest::QueryContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(2, query_content_protection_call_count_);
  EXPECT_TRUE(query_content_protection_response_success_);
  EXPECT_EQ(static_cast<uint32_t>(DISPLAY_CONNECTION_TYPE_HDMI),
            query_content_protection_response_link_mask_);
  EXPECT_EQ(static_cast<uint32_t>(CONTENT_PROTECTION_METHOD_NONE),
            query_content_protection_response_protection_mask_);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  configurator_.SetContentProtection(
      id, outputs_[1]->display_id(), CONTENT_PROTECTION_METHOD_HDCP,
      base::Bind(&DisplayConfiguratorTest::SetContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(1, set_content_protection_call_count_);
  EXPECT_TRUE(set_content_protection_status_);
  EXPECT_EQ(GetSetHDCPStateAction(*outputs_[1], HDCP_STATE_DESIRED),
            log_->GetActionsAndClear());

  // Enable protection.
  native_display_delegate_->set_hdcp_state(HDCP_STATE_ENABLED);
  configurator_.QueryContentProtectionStatus(
      id, outputs_[1]->display_id(),
      base::Bind(&DisplayConfiguratorTest::QueryContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(3, query_content_protection_call_count_);
  EXPECT_TRUE(query_content_protection_response_success_);
  EXPECT_EQ(static_cast<uint32_t>(DISPLAY_CONNECTION_TYPE_HDMI),
            query_content_protection_response_link_mask_);
  EXPECT_EQ(static_cast<uint32_t>(CONTENT_PROTECTION_METHOD_HDCP),
            query_content_protection_response_protection_mask_);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  // Protections should be disabled after unregister.
  configurator_.UnregisterContentProtectionClient(id);
  EXPECT_EQ(GetSetHDCPStateAction(*outputs_[1], HDCP_STATE_UNDESIRED),
            log_->GetActionsAndClear());
}

TEST_F(DisplayConfiguratorTest, DoNotConfigureWithSuspendedDisplays) {
  InitWithSingleOutput();

  // The DisplayConfigurator may occasionally receive OnConfigurationChanged()
  // after the displays have been suspended.  This event should be ignored since
  // the DisplayConfigurator will force a probe and reconfiguration of displays
  // at resume time.
  config_waiter_.Reset();
  configurator_.SuspendDisplays(config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // The configuration timer should not be started when the displays
  // are suspended.
  configurator_.OnConfigurationChanged();
  EXPECT_FALSE(test_api_.TriggerConfigureTimeout());
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  // Calls to SetDisplayPower should do nothing if the power state doesn't
  // change.
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_OFF,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  UpdateOutputs(2, false);
  configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // The DisplayConfigurator should do nothing at resume time if there is no
  // state change.
  config_waiter_.Reset();
  UpdateOutputs(1, false);
  configurator_.ResumeDisplays();
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  // If a configuration task is pending when the displays are suspended, that
  // task should not run either and the timer should be stopped. The displays
  // should be turned off by suspend.
  configurator_.OnConfigurationChanged();
  config_waiter_.Reset();
  configurator_.SuspendDisplays(config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
  EXPECT_FALSE(test_api_.TriggerConfigureTimeout());
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  config_waiter_.Reset();
  configurator_.ResumeDisplays();
  // The timer should not be running.
  EXPECT_EQ(base::TimeDelta::Max(), config_waiter_.Wait());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
}

TEST_F(DisplayConfiguratorTest, ContentProtectionTwoClients) {
  uint64_t client1 = configurator_.RegisterContentProtectionClient();
  uint64_t client2 = configurator_.RegisterContentProtectionClient();
  EXPECT_NE(client1, client2);

  Init(false);
  configurator_.ForceInitialConfigure();
  UpdateOutputs(2, true);
  EXPECT_NE(kNoActions, log_->GetActionsAndClear());

  // Clients never know state enableness for methods that they didn't request.
  configurator_.SetContentProtection(
      client1, outputs_[1]->display_id(), CONTENT_PROTECTION_METHOD_HDCP,
      base::Bind(&DisplayConfiguratorTest::SetContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(1, set_content_protection_call_count_);
  EXPECT_TRUE(set_content_protection_status_);
  EXPECT_EQ(GetSetHDCPStateAction(*outputs_[1], HDCP_STATE_DESIRED).c_str(),
            log_->GetActionsAndClear());
  native_display_delegate_->set_hdcp_state(HDCP_STATE_ENABLED);

  configurator_.QueryContentProtectionStatus(
      client1, outputs_[1]->display_id(),
      base::Bind(&DisplayConfiguratorTest::QueryContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(1, query_content_protection_call_count_);
  EXPECT_TRUE(query_content_protection_response_success_);
  EXPECT_EQ(static_cast<uint32_t>(DISPLAY_CONNECTION_TYPE_HDMI),
            query_content_protection_response_link_mask_);
  EXPECT_EQ(CONTENT_PROTECTION_METHOD_HDCP,
            query_content_protection_response_protection_mask_);

  configurator_.QueryContentProtectionStatus(
      client2, outputs_[1]->display_id(),
      base::Bind(&DisplayConfiguratorTest::QueryContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(2, query_content_protection_call_count_);
  EXPECT_TRUE(query_content_protection_response_success_);
  EXPECT_EQ(static_cast<uint32_t>(DISPLAY_CONNECTION_TYPE_HDMI),
            query_content_protection_response_link_mask_);
  EXPECT_EQ(CONTENT_PROTECTION_METHOD_NONE,
            query_content_protection_response_protection_mask_);

  // Protections will be disabled only if no more clients request them.
  configurator_.SetContentProtection(
      client2, outputs_[1]->display_id(), CONTENT_PROTECTION_METHOD_NONE,
      base::Bind(&DisplayConfiguratorTest::SetContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(2, set_content_protection_call_count_);
  EXPECT_TRUE(set_content_protection_status_);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  configurator_.SetContentProtection(
      client1, outputs_[1]->display_id(), CONTENT_PROTECTION_METHOD_NONE,
      base::Bind(&DisplayConfiguratorTest::SetContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(3, set_content_protection_call_count_);
  EXPECT_TRUE(set_content_protection_status_);
  EXPECT_EQ(GetSetHDCPStateAction(*outputs_[1], HDCP_STATE_UNDESIRED).c_str(),
            log_->GetActionsAndClear());
}

TEST_F(DisplayConfiguratorTest, ContentProtectionTwoClientsEnable) {
  uint64_t client1 = configurator_.RegisterContentProtectionClient();
  uint64_t client2 = configurator_.RegisterContentProtectionClient();
  EXPECT_NE(client1, client2);

  Init(false);
  configurator_.ForceInitialConfigure();
  UpdateOutputs(2, true);
  log_->GetActionsAndClear();

  // Only enable once if HDCP is enabling.
  configurator_.SetContentProtection(
      client1, outputs_[1]->display_id(), CONTENT_PROTECTION_METHOD_HDCP,
      base::Bind(&DisplayConfiguratorTest::SetContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(1, set_content_protection_call_count_);
  EXPECT_TRUE(set_content_protection_status_);
  native_display_delegate_->set_hdcp_state(HDCP_STATE_DESIRED);
  configurator_.SetContentProtection(
      client2, outputs_[1]->display_id(), CONTENT_PROTECTION_METHOD_HDCP,
      base::Bind(&DisplayConfiguratorTest::SetContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(2, set_content_protection_call_count_);
  EXPECT_TRUE(set_content_protection_status_);
  EXPECT_EQ(GetSetHDCPStateAction(*outputs_[1], HDCP_STATE_DESIRED).c_str(),
            log_->GetActionsAndClear());
  native_display_delegate_->set_hdcp_state(HDCP_STATE_ENABLED);

  // Don't enable again if HDCP is already active.
  configurator_.SetContentProtection(
      client1, outputs_[1]->display_id(), CONTENT_PROTECTION_METHOD_HDCP,
      base::Bind(&DisplayConfiguratorTest::SetContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(3, set_content_protection_call_count_);
  EXPECT_TRUE(set_content_protection_status_);
  configurator_.SetContentProtection(
      client2, outputs_[1]->display_id(), CONTENT_PROTECTION_METHOD_HDCP,
      base::Bind(&DisplayConfiguratorTest::SetContentProtectionCallback,
                 base::Unretained(this)));
  EXPECT_EQ(4, set_content_protection_call_count_);
  EXPECT_TRUE(set_content_protection_status_);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
}

TEST_F(DisplayConfiguratorTest, HandleConfigureCrtcFailure) {
  InitWithSingleOutput();

  std::vector<std::unique_ptr<const DisplayMode>> modes;
  // The first mode is the mode we are requesting DisplayConfigurator to choose.
  // The test will be setup so that this mode will fail and it will have to
  // choose the next best option.
  modes.push_back(MakeDisplayMode(2560, 1600, false, 60.0));
  modes.push_back(MakeDisplayMode(1024, 768, false, 60.0));
  modes.push_back(MakeDisplayMode(1280, 720, false, 60.0));
  modes.push_back(MakeDisplayMode(1920, 1080, false, 60.0));
  modes.push_back(MakeDisplayMode(1920, 1080, false, 40.0));

  outputs_[0] = FakeDisplaySnapshot::Builder()
                    .SetId(kDisplayIds[0])
                    .SetNativeMode(modes[0]->Clone())
                    .SetCurrentMode(modes[0]->Clone())
                    .AddMode(modes[1]->Clone())
                    .AddMode(modes[2]->Clone())
                    .AddMode(modes[3]->Clone())
                    .AddMode(modes[4]->Clone())
                    .SetType(DISPLAY_CONNECTION_TYPE_INTERNAL)
                    .SetIsAspectPerservingScaling(true)
                    .Build();

  // First test simply fails in MULTIPLE_DISPLAY_STATE_SINGLE mode. This is
  // probably unrealistic but we want to make sure any assumptions don't creep
  // in.
  native_display_delegate_->set_max_configurable_pixels(
      modes[2]->size().GetArea());
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_SINGLE);
  UpdateOutputs(1, true);

  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], modes[0].get(), gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[0], modes[3].get(), gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[0], modes[2].get(), gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  outputs_[1] = FakeDisplaySnapshot::Builder()
                    .SetId(kDisplayIds[1])
                    .SetNativeMode(modes[0]->Clone())
                    .SetCurrentMode(modes[0]->Clone())
                    .AddMode(modes[1]->Clone())
                    .AddMode(modes[2]->Clone())
                    .AddMode(modes[3]->Clone())
                    .AddMode(modes[4]->Clone())
                    .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
                    .SetIsAspectPerservingScaling(true)
                    .Build();

  // This test should attempt to configure a mirror mode that will not succeed
  // and should end up in extended mode.
  native_display_delegate_->set_max_configurable_pixels(
      modes[3]->size().GetArea());
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  UpdateOutputs(2, true);

  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], modes[0].get(), gfx::Point(0, 0)).c_str(),
          // Then attempt to configure crtc1 with the first mode.
          GetCrtcAction(*outputs_[1], modes[0].get(), gfx::Point(0, 0)).c_str(),
          // First mode tried is expected to fail and it will
          // retry wil the 4th mode in the list.
          GetCrtcAction(*outputs_[0], modes[3].get(), gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], modes[3].get(), gfx::Point(0, 0)).c_str(),
          // Since it was requested to go into mirror mode
          // and the configured modes were different, it
          // should now try and setup a valid configurable
          // extended mode.
          GetCrtcAction(*outputs_[0], modes[0].get(), gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], modes[0].get(),
                        gfx::Point(0, modes[0]->size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          GetCrtcAction(*outputs_[0], modes[3].get(), gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], modes[3].get(),
                        gfx::Point(0, modes[0]->size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());
}

// Tests that power state requests are saved after failed configuration attempts
// so they can be reused later: http://crosbug.com/p/31571
TEST_F(DisplayConfiguratorTest, SaveDisplayPowerStateOnConfigFailure) {
  // Start out with two displays in extended mode.
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  Init(false);
  configurator_.ForceInitialConfigure();
  log_->GetActionsAndClear();
  observer_.Reset();

  // Turn off the internal display, simulating docked mode.
  config_waiter_.Reset();
  configurator_.SetDisplayPower(
      chromeos::DISPLAY_POWER_INTERNAL_OFF_EXTERNAL_ON,
      DisplayConfigurator::kSetDisplayPowerNoFlags,
      config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(1, observer_.num_changes());
  EXPECT_EQ(0, observer_.num_failures());
  log_->GetActionsAndClear();

  // Make all subsequent configuration requests fail and try to turn the
  // internal display back on.
  config_waiter_.Reset();
  native_display_delegate_->set_max_configurable_pixels(1);
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_FAILURE, config_waiter_.callback_result());
  EXPECT_EQ(1, observer_.num_changes());
  EXPECT_EQ(1, observer_.num_failures());
  log_->GetActionsAndClear();

  // Simulate the external display getting disconnected and check that the
  // internal display is turned on (i.e. DISPLAY_POWER_ALL_ON is used) rather
  // than the earlier DISPLAY_POWER_INTERNAL_OFF_EXTERNAL_ON state.
  native_display_delegate_->set_max_configurable_pixels(0);
  UpdateOutputs(1, true);
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
}

// Tests that the SetDisplayPowerState() task posted by HandleResume() doesn't
// use a stale state if a new state is requested before it runs:
// http://crosbug.com/p/32393
TEST_F(DisplayConfiguratorTest, DontRestoreStalePowerStateAfterResume) {
  // Start out with two displays in mirrored mode.
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
  Init(false);
  configurator_.ForceInitialConfigure();
  log_->GetActionsAndClear();
  observer_.Reset();

  // Turn off the internal display, simulating docked mode.
  configurator_.SetDisplayPower(
      chromeos::DISPLAY_POWER_INTERNAL_OFF_EXTERNAL_ON,
      DisplayConfigurator::kSetDisplayPowerNoFlags,
      config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(1, observer_.num_changes());
  EXPECT_EQ(0, observer_.num_failures());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // Suspend and resume the system. Resuming should restore the previous power
  // state and force a probe. Suspend should turn off the displays since an
  // external monitor is connected.
  config_waiter_.Reset();
  configurator_.SuspendDisplays(config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(2, observer_.num_changes());
  EXPECT_EQ(JoinActions(
                GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
                GetCrtcAction(*outputs_[1], nullptr, gfx::Point(0, 0)).c_str(),
                nullptr),
            log_->GetActionsAndClear());

  // Before the task runs, exit docked mode.
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(3, observer_.num_changes());
  EXPECT_EQ(0, observer_.num_failures());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // Check that the display states are not changed after resuming.
  config_waiter_.Reset();
  // Since we are in dual display mode, a configuration task is scheduled after
  // kMinLongDelayMs delay.
  configurator_.ResumeDisplays();
  EXPECT_EQ(kLongDelay, test_api_.GetConfigureDelay());
  EXPECT_EQ(chromeos::DISPLAY_POWER_ALL_ON,
            configurator_.current_power_state());
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
  // Now trigger that delayed configuration.
  EXPECT_EQ(kLongDelay, config_waiter_.Wait());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
}

TEST_F(DisplayConfiguratorTest, ExternalControl) {
  InitWithSingleOutput();
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_SINGLE);

  // Set the initial power state and verify that it is restored when control is
  // taken.
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());

  configurator_.RelinquishControl(
      base::BindOnce(&DisplayConfiguratorTest::OnDisplayControlUpdated,
                     base::Unretained(this)));
  EXPECT_EQ(CALLBACK_SUCCESS, PopDisplayControlResult());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          kRelinquishDisplayControl, nullptr),
      log_->GetActionsAndClear());
  configurator_.TakeControl(
      base::BindOnce(&DisplayConfiguratorTest::OnDisplayControlUpdated,
                     base::Unretained(this)));
  EXPECT_EQ(CALLBACK_SUCCESS, PopDisplayControlResult());
  EXPECT_EQ(
      JoinActions(
          kTakeDisplayControl,
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());
}

TEST_F(DisplayConfiguratorTest,
       SetDisplayPowerWhilePendingConfigurationTaskRunning) {
  // Start out with two displays in extended mode.
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  Init(false);
  configurator_.ForceInitialConfigure();
  log_->GetActionsAndClear();
  observer_.Reset();

  native_display_delegate_->set_run_async(true);

  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_OFF,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(CALLBACK_NOT_CALLED, config_waiter_.callback_result());

  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());

  EXPECT_EQ(CALLBACK_NOT_CALLED, config_waiter_.callback_result());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(1, observer_.num_changes());
  EXPECT_EQ(0, observer_.num_failures());

  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], nullptr,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());

  config_waiter_.Reset();
  EXPECT_EQ(
      base::TimeDelta::FromMilliseconds(DisplayConfigurator::kConfigureDelayMs),
      config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_NOT_CALLED, config_waiter_.callback_result());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(2, observer_.num_changes());
  EXPECT_EQ(0, observer_.num_failures());

  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());
}

TEST_F(DisplayConfiguratorTest,
       SetDisplayPowerAfterFailedDisplayConfiguration) {
  // Start out with two displays in extended mode.
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  Init(false);
  configurator_.ForceInitialConfigure();
  log_->GetActionsAndClear();
  observer_.Reset();

  // Fail display configuration.
  native_display_delegate_->set_max_configurable_pixels(-1);

  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_OFF,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_FAILURE, config_waiter_.callback_result());
  EXPECT_EQ(0, observer_.num_changes());
  EXPECT_EQ(1, observer_.num_failures());

  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], nullptr,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // This configuration should trigger a display configuration since the
  // previous configuration failed.
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());

  EXPECT_EQ(0, observer_.num_changes());
  EXPECT_EQ(2, observer_.num_failures());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          GetCrtcAction(*outputs_[1], &small_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // Allow configuration to succeed.
  native_display_delegate_->set_max_configurable_pixels(0);

  // Validate that a configuration event has the proper power state (displays
  // should be on).
  configurator_.OnConfigurationChanged();
  EXPECT_TRUE(test_api_.TriggerConfigureTimeout());

  EXPECT_EQ(1, observer_.num_changes());
  EXPECT_EQ(2, observer_.num_failures());

  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());
}

TEST_F(DisplayConfiguratorTest, TestWithThreeDisplays) {
  // Start out with two displays in extended mode.
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  Init(false);
  configurator_.ForceInitialConfigure();
  log_->GetActionsAndClear();
  observer_.Reset();

  UpdateOutputs(3, true);
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);

  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          GetCrtcAction(
              *outputs_[2], &small_mode_,
              gfx::Point(0, small_mode_.size().height() +
                                big_mode_.size().height() +
                                2 * DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // Verify that turning the power off works.
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_OFF,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], nullptr,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          GetCrtcAction(
              *outputs_[2], nullptr,
              gfx::Point(0, small_mode_.size().height() +
                                big_mode_.size().height() +
                                2 * DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());

  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          GetCrtcAction(
              *outputs_[2], &small_mode_,
              gfx::Point(0, small_mode_.size().height() +
                                big_mode_.size().height() +
                                2 * DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // Disconnect the third output.
  observer_.Reset();
  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  UpdateOutputs(2, true);
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());
}

// Tests the suspend and resume behavior when in dual or multi display modes.
TEST_F(DisplayConfiguratorTest, SuspendResumeWithMultipleDisplays) {
  InitWithSingleOutput();

  // Set the initial power state and verify that it is restored on resume.
  config_waiter_.Reset();
  configurator_.SetDisplayPower(chromeos::DISPLAY_POWER_ALL_ON,
                                DisplayConfigurator::kSetDisplayPowerNoFlags,
                                config_waiter_.on_configuration_callback());

  state_controller_.set_state(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED);
  observer_.Reset();
  UpdateOutputs(2, true);
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());
  EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());
  EXPECT_EQ(1, observer_.num_changes());
  EXPECT_EQ(chromeos::DISPLAY_POWER_ALL_ON,
            configurator_.current_power_state());

  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // Suspending displays should result in an immediate configuration without
  // delays, even in dual display mode.
  config_waiter_.Reset();
  configurator_.SuspendDisplays(config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(chromeos::DISPLAY_POWER_ALL_OFF,
            configurator_.current_power_state());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], nullptr,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // Resuming from suspend with dual displays. Configuration should be done
  // after a long delay. Afterwards, we should still expect to be in a dual
  // display mode.
  config_waiter_.Reset();
  configurator_.ResumeDisplays();
  EXPECT_EQ(kLongDelay, config_waiter_.Wait());
  EXPECT_EQ(chromeos::DISPLAY_POWER_ALL_ON,
            configurator_.current_power_state());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], &big_mode_,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // Suspend displays and disconnect one of them while in suspend.
  config_waiter_.Reset();
  configurator_.SuspendDisplays(config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());
  EXPECT_EQ(chromeos::DISPLAY_POWER_ALL_OFF,
            configurator_.current_power_state());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], nullptr, gfx::Point(0, 0)).c_str(),
          GetCrtcAction(*outputs_[1], nullptr,
                        gfx::Point(0, small_mode_.size().height() +
                                          DisplayConfigurator::kVerticalGap))
              .c_str(),
          nullptr),
      log_->GetActionsAndClear());
  UpdateOutputs(1, false);
  EXPECT_EQ(kNoActions, log_->GetActionsAndClear());

  // Now resume, and expect that we'll still have a long delay since we were in
  // dual mode before suspend. The configurator should pick up the change and
  // detect that we are in single display mode now.
  config_waiter_.Reset();
  configurator_.ResumeDisplays();
  EXPECT_EQ(kLongDelay, config_waiter_.Wait());
  EXPECT_EQ(chromeos::DISPLAY_POWER_ALL_ON,
            configurator_.current_power_state());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_SINGLE, configurator_.display_state());
  EXPECT_EQ(
      JoinActions(
          GetCrtcAction(*outputs_[0], &small_mode_, gfx::Point(0, 0)).c_str(),
          nullptr),
      log_->GetActionsAndClear());

  // Verify that the above is the exact same behavior for 3+ displays.
  UpdateOutputs(3, true);
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());

  // Suspend.
  config_waiter_.Reset();
  configurator_.SuspendDisplays(config_waiter_.on_configuration_callback());
  EXPECT_EQ(kNoDelay, config_waiter_.Wait());
  EXPECT_EQ(CALLBACK_SUCCESS, config_waiter_.callback_result());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());
  EXPECT_EQ(chromeos::DISPLAY_POWER_ALL_OFF,
            configurator_.current_power_state());

  // Resume and expect the correct delay.
  config_waiter_.Reset();
  configurator_.ResumeDisplays();
  EXPECT_EQ(kLongDelay, config_waiter_.Wait());
  EXPECT_EQ(chromeos::DISPLAY_POWER_ALL_ON,
            configurator_.current_power_state());
  EXPECT_EQ(MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED,
            configurator_.display_state());
}

class DisplayConfiguratorMultiMirroringTest : public DisplayConfiguratorTest {
 public:
  DisplayConfiguratorMultiMirroringTest() = default;
  ~DisplayConfiguratorMultiMirroringTest() override = default;

  void SetUp() override {
    configurator_.set_is_multi_mirroring_enabled_for_test(true);
    DisplayConfiguratorTest::SetUp();
  }

  // Test that setting mirror mode with current outputs, all displays are set to
  // expected mirror mode.
  void TestHardwareMirrorModeExist(
      std::unique_ptr<DisplayMode> expected_mirror_mode) {
    UpdateOutputs(3, true);
    log_->GetActionsAndClear();
    observer_.Reset();
    configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
    EXPECT_EQ(
        JoinActions(GetCrtcAction(*outputs_[0], expected_mirror_mode.get(),
                                  gfx::Point(0, 0))
                        .c_str(),
                    GetCrtcAction(*outputs_[1], expected_mirror_mode.get(),
                                  gfx::Point(0, 0))
                        .c_str(),
                    GetCrtcAction(*outputs_[2], expected_mirror_mode.get(),
                                  gfx::Point(0, 0))
                        .c_str(),
                    nullptr),
        log_->GetActionsAndClear());
    EXPECT_FALSE(mirroring_controller_.SoftwareMirroringEnabled());
    EXPECT_EQ(1, observer_.num_changes());
  }

  // Test that setting mirror mode with current outputs, no matching mirror mode
  // is found.
  void TestHardwareMirrorModeNotExist() {
    UpdateOutputs(3, true);
    log_->GetActionsAndClear();
    observer_.Reset();
    configurator_.SetDisplayMode(MULTIPLE_DISPLAY_STATE_DUAL_MIRROR);
    EXPECT_EQ(kNoActions, log_->GetActionsAndClear());
    EXPECT_TRUE(mirroring_controller_.SoftwareMirroringEnabled());
    EXPECT_EQ(1, observer_.num_changes());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DisplayConfiguratorMultiMirroringTest);
};

TEST_F(DisplayConfiguratorMultiMirroringTest,
       FindMirrorModeWithInternalDisplay) {
  // Initialize with one internal display and two external displays.
  outputs_[0] = FakeDisplaySnapshot::Builder()
                    .SetId(kDisplayIds[0])
                    .SetType(DISPLAY_CONNECTION_TYPE_INTERNAL)
                    .SetNativeMode(MakeDisplayMode(1920, 1600, false, 60.0))
                    .AddMode(MakeDisplayMode(1920, 1600, false, 60.0))
                    .AddMode(MakeDisplayMode(1920, 1200, false, 60.0))
                    .AddMode(MakeDisplayMode(1920, 1080, true, 60.0))
                    .AddMode(MakeDisplayMode(1440, 900, true, 60.0))
                    .Build();
  outputs_[1] =
      FakeDisplaySnapshot::Builder()
          .SetId(kDisplayIds[1])
          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
          .SetNativeMode(MakeDisplayMode(1920, 1200, true, 60.0))
          .AddMode(MakeDisplayMode(1920, 1200, true, 60.0))  // same AR
          .AddMode(MakeDisplayMode(1920, 1080, true, 60.0))
          .AddMode(MakeDisplayMode(1680, 1050, false, 60.0))  // same AR
          .AddMode(MakeDisplayMode(1440, 900, true, 60.0))    // same AR
          .AddMode(MakeDisplayMode(500, 500, false, 60.0))
          .Build();
  outputs_[2] =
      FakeDisplaySnapshot::Builder()
          .SetId(kDisplayIds[2])
          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
          .SetNativeMode(MakeDisplayMode(1920, 1200, false, 60.0))
          .AddMode(MakeDisplayMode(1920, 1200, false, 60.0))  // same AR
          .AddMode(MakeDisplayMode(1920, 1080, true, 60.0))
          .AddMode(MakeDisplayMode(1680, 1050, false, 60.0))  // same AR
          .AddMode(MakeDisplayMode(1440, 900, true, 60.0))    // same AR
          .Build();

  // Find an exactly matching mirror mode while preserving aspect.
  TestHardwareMirrorModeExist(MakeDisplayMode(1440, 900, true, 60.0));

  // Find an exactly matching mirror mode while not preserving aspect.
  outputs_[2] =
      FakeDisplaySnapshot::Builder()
          .SetId(kDisplayIds[2])
          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
          .SetNativeMode(MakeDisplayMode(1920, 1200, false, 60.0))
          .AddMode(MakeDisplayMode(1920, 1200, false, 60.0))  // same AR
          .AddMode(MakeDisplayMode(1920, 1080, true, 60.0))
          .Build();
  TestHardwareMirrorModeExist(MakeDisplayMode(1920, 1080, true, 60.0));

  // Cannot find a matching mirror mode, so enable software mirroring.
  outputs_[2] =
      FakeDisplaySnapshot::Builder()
          .SetId(kDisplayIds[2])
          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
          .SetNativeMode(MakeDisplayMode(1920, 1200, false, 60.0))
          .AddMode(MakeDisplayMode(1920, 1200, false, 60.0))  // same AR
          .AddMode(MakeDisplayMode(500, 500, true, 60.0))
          .Build();
  TestHardwareMirrorModeNotExist();
}

TEST_F(DisplayConfiguratorMultiMirroringTest,
       FindMirrorModeWithoutInternalDisplay) {
  // Initialize with 3 external displays.
  outputs_[0] =
      FakeDisplaySnapshot::Builder()
          .SetId(kDisplayIds[1])
          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
          .SetNativeMode(MakeDisplayMode(1920, 1200, true, 60.0))
          .AddMode(MakeDisplayMode(1920, 1200, true, 60.0))  // same AR
          .AddMode(MakeDisplayMode(1920, 1080, false, 60.0))
          .AddMode(MakeDisplayMode(1680, 1050, true, 60.0))  // same AR
          .Build();
  outputs_[1] =
      FakeDisplaySnapshot::Builder()
          .SetId(kDisplayIds[2])
          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
          .SetNativeMode(MakeDisplayMode(1920, 1200, false, 60.0))
          .AddMode(MakeDisplayMode(1920, 1200, false, 60.0))  // same AR
          .AddMode(MakeDisplayMode(1920, 1080, false, 60.0))
          .AddMode(MakeDisplayMode(1680, 1050, true, 60.0))  // same AR
          .Build();
  outputs_[2] =
      FakeDisplaySnapshot::Builder()
          .SetId(kDisplayIds[2])
          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
          .SetNativeMode(MakeDisplayMode(1920, 1200, false, 60.0))
          .AddMode(MakeDisplayMode(1920, 1200, false, 60.0))  // same AR
          .AddMode(MakeDisplayMode(1920, 1080, false, 60.0))
          .AddMode(MakeDisplayMode(1680, 1050, true, 60.0))  // same AR
          .Build();

  // Find an exactly matching mirror mode while preserving aspect.
  TestHardwareMirrorModeExist(MakeDisplayMode(1680, 1050, true, 60.0));

  // Find an exactly matching mirror mode while not preserving aspect.
  outputs_[2] =
      FakeDisplaySnapshot::Builder()
          .SetId(kDisplayIds[0])
          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
          .SetNativeMode(MakeDisplayMode(1920, 1600, false, 60.0))
          .AddMode(MakeDisplayMode(1920, 1600, false, 60.0))  // same AR
          .AddMode(MakeDisplayMode(1920, 1200, false, 60.0))
          .AddMode(MakeDisplayMode(1920, 1080, false, 60.0))
          .Build();
  TestHardwareMirrorModeExist(MakeDisplayMode(1920, 1080, false, 60.0));

  // Cannot find a matching mirror mode, so enable software mirroring.
  outputs_[2] =
      FakeDisplaySnapshot::Builder()
          .SetId(kDisplayIds[0])
          .SetType(DISPLAY_CONNECTION_TYPE_HDMI)
          .SetNativeMode(MakeDisplayMode(1920, 1600, false, 60.0))
          .AddMode(MakeDisplayMode(1920, 1600, false, 60.0))  // same AR
          .AddMode(MakeDisplayMode(1920, 1200, false, 60.0))
          .Build();
  TestHardwareMirrorModeNotExist();
}

}  // namespace test
}  // namespace display
