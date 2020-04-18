// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/night_light/night_light_controller.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <string>

#include "ash/display/cursor_window_controller.h"
#include "ash/display/window_tree_host_manager.h"
#include "ash/public/cpp/ash_pref_names.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/public/cpp/config.h"
#include "ash/public/cpp/session_types.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test_shell_delegate.h"
#include "base/bind.h"
#include "base/callback_forward.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/pattern.h"
#include "components/prefs/pref_service.h"
#include "ui/compositor/layer.h"
#include "ui/display/manager/display_change_observer.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/fake_display_snapshot.h"
#include "ui/display/manager/test/action_logger_util.h"
#include "ui/display/manager/test/test_native_display_delegate.h"

namespace ash {

namespace {

constexpr char kUser1Email[] = "user1@nightlight";
constexpr char kUser2Email[] = "user2@nightlight";

NightLightController* GetController() {
  return Shell::Get()->night_light_controller();
}

// Tests that the given display with |display_id| has the expected color matrix
// on its compositor that corresponds to the given |expected_temperature|.
void TestDisplayCompositorTemperature(int64_t display_id,
                                      float expected_temperature) {
  WindowTreeHostManager* wth_manager = Shell::Get()->window_tree_host_manager();
  aura::Window* root_window =
      wth_manager->GetRootWindowForDisplayId(display_id);
  DCHECK(root_window);
  aura::WindowTreeHost* host = root_window->GetHost();
  DCHECK(host);
  ui::Compositor* compositor = host->compositor();
  DCHECK(compositor);

  const SkMatrix44& matrix = compositor->display_color_matrix();
  const float blue_scale = matrix.get(2, 2);
  const float green_scale = matrix.get(1, 1);
  EXPECT_FLOAT_EQ(
      expected_temperature,
      NightLightController::TemperatureFromBlueColorScale(blue_scale));
  EXPECT_FLOAT_EQ(
      expected_temperature,
      NightLightController::TemperatureFromGreenColorScale(green_scale));
}

// Tests that the display color matrices of all compositors correctly correspond
// to the given |expected_temperature|.
void TestCompositorsTemperature(float expected_temperature) {
  for (int64_t display_id :
       Shell::Get()->display_manager()->GetCurrentDisplayIdList()) {
    TestDisplayCompositorTemperature(display_id, expected_temperature);
  }
}

class TestObserver : public NightLightController::Observer {
 public:
  TestObserver() { GetController()->AddObserver(this); }
  ~TestObserver() override { GetController()->RemoveObserver(this); }

  // ash::NightLightController::Observer:
  void OnNightLightEnabledChanged(bool enabled) override { status_ = enabled; }

  bool status() const { return status_; }

 private:
  bool status_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

constexpr double kFakePosition1_Latitude = 23.5;
constexpr double kFakePosition1_Longitude = 35.88;
constexpr int kFakePosition1_SunsetOffset = 20 * 60;
constexpr int kFakePosition1_SunriseOffset = 4 * 60;

constexpr double kFakePosition2_Latitude = 37.5;
constexpr double kFakePosition2_Longitude = -100.5;
constexpr int kFakePosition2_SunsetOffset = 17 * 60;
constexpr int kFakePosition2_SunriseOffset = 3 * 60;

class TestDelegate : public NightLightController::Delegate {
 public:
  TestDelegate() = default;
  ~TestDelegate() override = default;

  void SetFakeNow(TimeOfDay time) { fake_now_ = time.ToTimeToday(); }
  void SetFakeSunset(TimeOfDay time) { fake_sunset_ = time.ToTimeToday(); }
  void SetFakeSunrise(TimeOfDay time) { fake_sunrise_ = time.ToTimeToday(); }

  // ash::NightLightController::Delegate
  base::Time GetNow() const override { return fake_now_; }
  base::Time GetSunsetTime() const override { return fake_sunset_; }
  base::Time GetSunriseTime() const override { return fake_sunrise_; }
  void SetGeoposition(mojom::SimpleGeopositionPtr position) override {
    if (position.Equals(mojom::SimpleGeoposition::New(
            kFakePosition1_Latitude, kFakePosition1_Longitude))) {
      // Set sunset and sunrise times associated with fake position 1.
      SetFakeSunset(TimeOfDay(kFakePosition1_SunsetOffset));
      SetFakeSunrise(TimeOfDay(kFakePosition1_SunriseOffset));
    } else if (position.Equals(mojom::SimpleGeoposition::New(
                   kFakePosition2_Latitude, kFakePosition2_Longitude))) {
      // Set sunset and sunrise times associated with fake position 2.
      SetFakeSunset(TimeOfDay(kFakePosition2_SunsetOffset));
      SetFakeSunrise(TimeOfDay(kFakePosition2_SunriseOffset));
    }
  }

 private:
  base::Time fake_now_;
  base::Time fake_sunset_;
  base::Time fake_sunrise_;

  DISALLOW_COPY_AND_ASSIGN(TestDelegate);
};

class NightLightTest : public NoSessionAshTestBase {
 public:
  NightLightTest() = default;
  ~NightLightTest() override = default;

  PrefService* user1_pref_service() {
    return Shell::Get()->session_controller()->GetUserPrefServiceForUser(
        AccountId::FromUserEmail(kUser1Email));
  }

  PrefService* user2_pref_service() {
    return Shell::Get()->session_controller()->GetUserPrefServiceForUser(
        AccountId::FromUserEmail(kUser2Email));
  }

  TestDelegate* delegate() const { return delegate_; }

  // AshTestBase:
  void SetUp() override {
    // Explicitly enable the NightLight feature for the tests.
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        ash::switches::kAshEnableNightLight);

    NoSessionAshTestBase::SetUp();
    CreateTestUserSessions();

    // Simulate user 1 login.
    SwitchActiveUser(kUser1Email);

    delegate_ = new TestDelegate;
    GetController()->SetDelegateForTesting(base::WrapUnique(delegate_));
  }

  void CreateTestUserSessions() {
    GetSessionControllerClient()->Reset();
    GetSessionControllerClient()->AddUserSession(kUser1Email);
    GetSessionControllerClient()->AddUserSession(kUser2Email);
  }

  void SwitchActiveUser(const std::string& email) {
    GetSessionControllerClient()->SwitchActiveUser(
        AccountId::FromUserEmail(email));
  }

  void SetNightLightEnabled(bool enabled) {
    GetController()->SetEnabled(
        enabled, NightLightController::AnimationDuration::kShort);
  }

 private:
  TestDelegate* delegate_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(NightLightTest);
};

// Tests toggling NightLight on / off and makes sure the observer is updated and
// the layer temperatures are modified.
TEST_F(NightLightTest, TestToggle) {
  UpdateDisplay("800x600,800x600");

  TestObserver observer;
  NightLightController* controller = GetController();
  SetNightLightEnabled(false);
  ASSERT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  controller->Toggle();
  EXPECT_TRUE(controller->GetEnabled());
  EXPECT_TRUE(observer.status());
  TestCompositorsTemperature(GetController()->GetColorTemperature());
  controller->Toggle();
  EXPECT_FALSE(controller->GetEnabled());
  EXPECT_FALSE(observer.status());
  TestCompositorsTemperature(0.0f);
}

// Tests setting the temperature in various situations.
TEST_F(NightLightTest, TestSetTemperature) {
  UpdateDisplay("800x600,800x600");

  TestObserver observer;
  NightLightController* controller = GetController();
  SetNightLightEnabled(false);
  ASSERT_FALSE(controller->GetEnabled());

  // Setting the temperature while NightLight is disabled only changes the
  // color temperature field, but root layers temperatures are not affected, nor
  // the status of NightLight itself.
  const float temperature1 = 0.2f;
  controller->SetColorTemperature(temperature1);
  EXPECT_EQ(temperature1, controller->GetColorTemperature());
  TestCompositorsTemperature(0.0f);

  // When NightLight is enabled, temperature changes actually affect the root
  // layers temperatures.
  SetNightLightEnabled(true);
  ASSERT_TRUE(controller->GetEnabled());
  const float temperature2 = 0.7f;
  controller->SetColorTemperature(temperature2);
  EXPECT_EQ(temperature2, controller->GetColorTemperature());
  TestCompositorsTemperature(temperature2);

  // When NightLight is disabled, the value of the color temperature field
  // doesn't change, however the temperatures set on the root layers are all
  // 0.0f. Observers only receive an enabled status change notification; no
  // temperature change notification.
  SetNightLightEnabled(false);
  ASSERT_FALSE(controller->GetEnabled());
  EXPECT_FALSE(observer.status());
  EXPECT_EQ(temperature2, controller->GetColorTemperature());
  TestCompositorsTemperature(0.0f);

  // When re-enabled, the stored temperature is re-applied.
  SetNightLightEnabled(true);
  EXPECT_TRUE(observer.status());
  ASSERT_TRUE(controller->GetEnabled());
  TestCompositorsTemperature(temperature2);
}

TEST_F(NightLightTest, TestNightLightWithDisplayConfigurationChanges) {
  // Start with one display and enable NightLight.
  NightLightController* controller = GetController();
  SetNightLightEnabled(true);
  ASSERT_TRUE(controller->GetEnabled());
  const float temperature = 0.2f;
  controller->SetColorTemperature(temperature);
  EXPECT_EQ(temperature, controller->GetColorTemperature());
  TestCompositorsTemperature(temperature);

  // Add a new display, and expect that its compositor gets the already set from
  // before color temperature.
  display_manager()->AddRemoveDisplay();
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(2u, RootWindowController::root_window_controllers().size());
  TestCompositorsTemperature(temperature);

  // While we have the second display, enable mirror mode, the compositors
  // should still have the same temperature.
  display_manager()->SetMirrorMode(display::MirrorMode::kNormal, base::nullopt);
  EXPECT_TRUE(display_manager()->IsInMirrorMode());
  base::RunLoop().RunUntilIdle();
  TestCompositorsTemperature(temperature);

  // Exit mirror mode, temperature is still applied.
  display_manager()->SetMirrorMode(display::MirrorMode::kOff, base::nullopt);
  EXPECT_FALSE(display_manager()->IsInMirrorMode());
  base::RunLoop().RunUntilIdle();
  TestCompositorsTemperature(temperature);

  // Enter unified mode, temperature is still applied.
  display_manager()->SetUnifiedDesktopEnabled(true);
  EXPECT_TRUE(display_manager()->IsInUnifiedMode());
  base::RunLoop().RunUntilIdle();
  TestCompositorsTemperature(temperature);
  // The unified host should never have a temperature on its compositor.
  TestDisplayCompositorTemperature(display::kUnifiedDisplayId, 0.0f);

  // Exit unified mode, and remove the display, temperature should remain the
  // same.
  display_manager()->SetUnifiedDesktopEnabled(false);
  EXPECT_FALSE(display_manager()->IsInUnifiedMode());
  base::RunLoop().RunUntilIdle();
  TestCompositorsTemperature(temperature);

  display_manager()->AddRemoveDisplay();
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(1u, RootWindowController::root_window_controllers().size());
  TestCompositorsTemperature(temperature);
}

// Tests that switching users retrieves NightLight settings for the active
// user's prefs.
TEST_F(NightLightTest, TestUserSwitchAndSettingsPersistence) {
  // Test start with user1 logged in.
  NightLightController* controller = GetController();
  SetNightLightEnabled(true);
  EXPECT_TRUE(controller->GetEnabled());
  const float user1_temperature = 0.8f;
  controller->SetColorTemperature(user1_temperature);
  EXPECT_EQ(user1_temperature, controller->GetColorTemperature());
  TestCompositorsTemperature(user1_temperature);

  // Switch to user 2, and expect NightLight to be disabled.
  SwitchActiveUser(kUser2Email);
  EXPECT_FALSE(controller->GetEnabled());
  // Changing user_2's color temperature shouldn't affect user_1's settings.
  const float user2_temperature = 0.2f;
  user2_pref_service()->SetDouble(prefs::kNightLightTemperature,
                                  user2_temperature);
  EXPECT_EQ(user2_temperature, controller->GetColorTemperature());
  TestCompositorsTemperature(0.0f);
  EXPECT_EQ(user1_temperature,
            user1_pref_service()->GetDouble(prefs::kNightLightTemperature));

  // Switch back to user 1, to find NightLight is still enabled, and the same
  // user's color temperature are re-applied.
  SwitchActiveUser(kUser1Email);
  EXPECT_TRUE(controller->GetEnabled());
  EXPECT_EQ(user1_temperature, controller->GetColorTemperature());
  TestCompositorsTemperature(user1_temperature);
}

// Tests that changes from outside NightLightControlled to the NightLight
// Preferences are seen by the controlled and applied properly.
TEST_F(NightLightTest, TestOutsidePreferencesChangesAreApplied) {
  // Test start with user1 logged in.
  NightLightController* controller = GetController();
  user1_pref_service()->SetBoolean(prefs::kNightLightEnabled, true);
  EXPECT_TRUE(controller->GetEnabled());
  const float temperature1 = 0.65f;
  user1_pref_service()->SetDouble(prefs::kNightLightTemperature, temperature1);
  EXPECT_EQ(temperature1, controller->GetColorTemperature());
  TestCompositorsTemperature(temperature1);
  const float temperature2 = 0.23f;
  user1_pref_service()->SetDouble(prefs::kNightLightTemperature, temperature2);
  EXPECT_EQ(temperature2, controller->GetColorTemperature());
  TestCompositorsTemperature(temperature2);
  user1_pref_service()->SetBoolean(prefs::kNightLightEnabled, false);
  EXPECT_FALSE(controller->GetEnabled());
}

// Tests transitioning from kNone to kCustom and back to kNone schedule types.
TEST_F(NightLightTest, TestScheduleNoneToCustomTransition) {
  NightLightController* controller = GetController();
  // Now is 6:00 PM.
  delegate()->SetFakeNow(TimeOfDay(18 * 60));
  SetNightLightEnabled(false);
  controller->SetScheduleType(NightLightController::ScheduleType::kNone);
  // Start time is at 3:00 PM and end time is at 8:00 PM.
  controller->SetCustomStartTime(TimeOfDay(15 * 60));
  controller->SetCustomEndTime(TimeOfDay(20 * 60));

  //      15:00         18:00         20:00
  // <----- + ----------- + ----------- + ----->
  //        |             |             |
  //      start          now           end
  //
  // Even though "Now" is inside the NightLight interval, nothing should change,
  // since the schedule type is "none".
  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);

  // Now change the schedule type to custom, NightLight should turn on
  // immediately with a short animation duration, and the timer should be
  // running with a delay of exactly 2 hours scheduling the end.
  controller->SetScheduleType(NightLightController::ScheduleType::kCustom);
  EXPECT_TRUE(controller->GetEnabled());
  TestCompositorsTemperature(controller->GetColorTemperature());
  EXPECT_EQ(NightLightController::AnimationDuration::kShort,
            controller->last_animation_duration());
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(2),
            controller->timer().GetCurrentDelay());
  EXPECT_TRUE(controller->timer().user_task());

  // If the user changes the schedule type to "none", the NightLight status
  // should not change, but the timer should not be running.
  controller->SetScheduleType(NightLightController::ScheduleType::kNone);
  EXPECT_TRUE(controller->GetEnabled());
  TestCompositorsTemperature(controller->GetColorTemperature());
  EXPECT_FALSE(controller->timer().IsRunning());
}

// Tests what happens when the time now reaches the end of the NightLight
// interval when NightLight mode is on.
TEST_F(NightLightTest, TestCustomScheduleReachingEndTime) {
  NightLightController* controller = GetController();
  delegate()->SetFakeNow(TimeOfDay(18 * 60));
  controller->SetCustomStartTime(TimeOfDay(15 * 60));
  controller->SetCustomEndTime(TimeOfDay(20 * 60));
  controller->SetScheduleType(NightLightController::ScheduleType::kCustom);
  EXPECT_TRUE(controller->GetEnabled());
  TestCompositorsTemperature(controller->GetColorTemperature());

  // Simulate reaching the end time by triggering the timer's user task. Make
  // sure that NightLight ended with a long animation.
  //
  //      15:00                      20:00
  // <----- + ------------------------ + ----->
  //        |                          |
  //      start                    end & now
  //
  // Now is 8:00 PM.
  delegate()->SetFakeNow(TimeOfDay(20 * 60));
  controller->timer().user_task().Run();
  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  EXPECT_EQ(NightLightController::AnimationDuration::kLong,
            controller->last_animation_duration());
  // The timer should still be running, but now scheduling the start at 3:00 PM
  // tomorrow which is 19 hours from "now" (8:00 PM).
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(19),
            controller->timer().GetCurrentDelay());
}

// Tests that user toggles from the system menu or system settings override any
// status set by an automatic schedule.
TEST_F(NightLightTest, TestExplicitUserTogglesWhileScheduleIsActive) {
  // Start with the below custom schedule, where NightLight is off.
  //
  //      15:00               20:00          23:00
  // <----- + ----------------- + ------------ + ---->
  //        |                   |              |
  //      start                end            now
  //
  NightLightController* controller = GetController();
  delegate()->SetFakeNow(TimeOfDay(23 * 60));
  controller->SetCustomStartTime(TimeOfDay(15 * 60));
  controller->SetCustomEndTime(TimeOfDay(20 * 60));
  controller->SetScheduleType(NightLightController::ScheduleType::kCustom);
  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);

  // What happens if the user manually turns NightLight on while the schedule
  // type says it should be off?
  // User toggles either from the system menu or the System Settings toggle
  // button must override any automatic schedule, and should be performed with
  // the short animation duration.
  controller->Toggle();
  EXPECT_TRUE(controller->GetEnabled());
  TestCompositorsTemperature(controller->GetColorTemperature());
  EXPECT_EQ(NightLightController::AnimationDuration::kShort,
            controller->last_animation_duration());
  // The timer should still be running, but NightLight should automatically
  // turn off at 8:00 PM tomorrow, which is 21 hours from now (11:00 PM).
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(21),
            controller->timer().GetCurrentDelay());

  // Manually turning it back off should also be respected, and this time the
  // start is scheduled at 3:00 PM tomorrow after 19 hours from "now" (8:00 PM).
  controller->Toggle();
  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  EXPECT_EQ(NightLightController::AnimationDuration::kShort,
            controller->last_animation_duration());
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(16),
            controller->timer().GetCurrentDelay());
}

// Tests that changing the custom start and end times, in such a way that
// shouldn't change the current status, only updates the timer but doesn't
// change the status.
TEST_F(NightLightTest, TestChangingStartTimesThatDontChangeTheStatus) {
  //       16:00        18:00         22:00
  // <----- + ----------- + ----------- + ----->
  //        |             |             |
  //       now          start          end
  //
  NightLightController* controller = GetController();
  delegate()->SetFakeNow(TimeOfDay(16 * 60));  // 4:00 PM.
  SetNightLightEnabled(false);
  controller->SetScheduleType(NightLightController::ScheduleType::kNone);
  controller->SetCustomStartTime(TimeOfDay(18 * 60));  // 6:00 PM.
  controller->SetCustomEndTime(TimeOfDay(22 * 60));    // 10:00 PM.

  // Since now is outside the NightLight interval, changing the schedule type
  // to kCustom, shouldn't affect the status. Validate the timer is running with
  // a 2-hour delay.
  controller->SetScheduleType(NightLightController::ScheduleType::kCustom);
  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(2),
            controller->timer().GetCurrentDelay());

  // Change the start time in such a way that doesn't change the status, but
  // despite that, confirm that schedule has been updated.
  controller->SetCustomStartTime(TimeOfDay(19 * 60));  // 7:00 PM.
  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(3),
            controller->timer().GetCurrentDelay());

  // Changing the end time in a similar fashion to the above and expect no
  // change.
  controller->SetCustomEndTime(TimeOfDay(23 * 60));  // 11:00 PM.
  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(3),
            controller->timer().GetCurrentDelay());
}

// Tests the behavior of the sunset to sunrise automatic schedule type.
TEST_F(NightLightTest, TestSunsetSunrise) {
  //      16:00         18:00     20:00      22:00              5:00
  // <----- + ----------- + ------- + -------- + --------------- + ------->
  //        |             |         |          |                 |
  //       now      custom start  sunset   custom end         sunrise
  //
  NightLightController* controller = GetController();
  delegate()->SetFakeNow(TimeOfDay(16 * 60));     // 4:00 PM.
  delegate()->SetFakeSunset(TimeOfDay(20 * 60));  // 8:00 PM.
  delegate()->SetFakeSunrise(TimeOfDay(5 * 60));  // 5:00 AM.
  SetNightLightEnabled(false);
  controller->SetScheduleType(NightLightController::ScheduleType::kNone);
  controller->SetCustomStartTime(TimeOfDay(18 * 60));  // 6:00 PM.
  controller->SetCustomEndTime(TimeOfDay(22 * 60));    // 10:00 PM.

  // Custom times should have no effect when the schedule type is sunset to
  // sunrise.
  controller->SetScheduleType(
      NightLightController::ScheduleType::kSunsetToSunrise);
  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(4),
            controller->timer().GetCurrentDelay());

  // Simulate reaching sunset.
  delegate()->SetFakeNow(TimeOfDay(20 * 60));  // Now is 8:00 PM.
  controller->timer().user_task().Run();
  EXPECT_TRUE(controller->GetEnabled());
  TestCompositorsTemperature(controller->GetColorTemperature());
  EXPECT_EQ(NightLightController::AnimationDuration::kLong,
            controller->last_animation_duration());
  // Timer is running scheduling the end at sunrise.
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(9),
            controller->timer().GetCurrentDelay());

  // Simulate reaching sunrise.
  delegate()->SetFakeNow(TimeOfDay(5 * 60));  // Now is 5:00 AM.
  controller->timer().user_task().Run();
  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  EXPECT_EQ(NightLightController::AnimationDuration::kLong,
            controller->last_animation_duration());
  // Timer is running scheduling the start at the next sunset.
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(15),
            controller->timer().GetCurrentDelay());
}

// Tests the behavior of the sunset to sunrise automatic schedule type when the
// client sets the geoposition.
TEST_F(NightLightTest, TestSunsetSunriseGeoposition) {
  // Position 1 sunset and sunrise times.
  //
  //      16:00       20:00               4:00
  // <----- + --------- + ---------------- + ------->
  //        |           |                  |
  //       now        sunset            sunrise
  //
  NightLightController* controller = GetController();
  delegate()->SetFakeNow(TimeOfDay(16 * 60));  // 4:00 PM.
  controller->SetCurrentGeoposition(mojom::SimpleGeoposition::New(
      kFakePosition1_Latitude, kFakePosition1_Longitude));

  // Expect that timer is running and the start is scheduled after 4 hours.
  controller->SetScheduleType(
      NightLightController::ScheduleType::kSunsetToSunrise);
  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(4),
            controller->timer().GetCurrentDelay());

  // Simulate reaching sunset.
  delegate()->SetFakeNow(TimeOfDay(20 * 60));  // Now is 8:00 PM.
  controller->timer().user_task().Run();
  EXPECT_TRUE(controller->GetEnabled());
  TestCompositorsTemperature(controller->GetColorTemperature());
  EXPECT_EQ(NightLightController::AnimationDuration::kLong,
            controller->last_animation_duration());
  // Timer is running scheduling the end at sunrise.
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(8),
            controller->timer().GetCurrentDelay());

  // Now simulate user changing position.
  // Position 2 sunset and sunrise times.
  //
  //      17:00       20:00               3:00
  // <----- + --------- + ---------------- + ------->
  //        |           |                  |
  //      sunset       now               sunrise
  //
  controller->SetCurrentGeoposition(mojom::SimpleGeoposition::New(
      kFakePosition2_Latitude, kFakePosition2_Longitude));

  // Expect that the scheduled end delay has been updated, and the status hasn't
  // changed.
  EXPECT_TRUE(controller->GetEnabled());
  TestCompositorsTemperature(controller->GetColorTemperature());
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(7),
            controller->timer().GetCurrentDelay());

  // Simulate reaching sunrise.
  delegate()->SetFakeNow(TimeOfDay(3 * 60));  // Now is 5:00 AM.
  controller->timer().user_task().Run();
  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  EXPECT_EQ(NightLightController::AnimationDuration::kLong,
            controller->last_animation_duration());
  // Timer is running scheduling the start at the next sunset.
  EXPECT_TRUE(controller->timer().IsRunning());
  EXPECT_EQ(base::TimeDelta::FromHours(14),
            controller->timer().GetCurrentDelay());
}

// Tests that on device resume from sleep, the NightLight status is updated
// correctly if the time has changed meanwhile.
TEST_F(NightLightTest, TestCustomScheduleOnResume) {
  NightLightController* controller = GetController();
  // Now is 4:00 PM.
  delegate()->SetFakeNow(TimeOfDay(16 * 60));
  SetNightLightEnabled(false);
  // Start time is at 6:00 PM and end time is at 10:00 PM. NightLight should be
  // off.
  //      16:00         18:00         22:00
  // <----- + ----------- + ----------- + ----->
  //        |             |             |
  //       now          start          end
  //
  controller->SetColorTemperature(0.4f);
  controller->SetCustomStartTime(TimeOfDay(18 * 60));
  controller->SetCustomEndTime(TimeOfDay(22 * 60));
  controller->SetScheduleType(NightLightController::ScheduleType::kCustom);

  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  EXPECT_TRUE(controller->timer().IsRunning());
  // NightLight should start in 2 hours.
  EXPECT_EQ(base::TimeDelta::FromHours(2),
            controller->timer().GetCurrentDelay());

  // Now simulate that the device was suspended for 3 hours, and the time now
  // is 7:00 PM when the devices was resumed. Expect that NightLight turns on.
  delegate()->SetFakeNow(TimeOfDay(19 * 60));
  controller->SuspendDone(base::TimeDelta::Max());

  EXPECT_TRUE(controller->GetEnabled());
  TestCompositorsTemperature(0.4f);
  EXPECT_TRUE(controller->timer().IsRunning());
  // NightLight should end in 3 hours.
  EXPECT_EQ(base::TimeDelta::FromHours(3),
            controller->timer().GetCurrentDelay());
}

// The following tests ensure that the NightLight schedule is correctly
// refreshed when the start and end times are inverted (i.e. the "start time" as
// a time of day today is in the future with respect to the "end time" also as a
// time of day today).
//
// Case 1: "Now" is less than both "end" and "start".
TEST_F(NightLightTest, TestCustomScheduleInvertedStartAndEndTimesCase1) {
  NightLightController* controller = GetController();
  // Now is 4:00 AM.
  delegate()->SetFakeNow(TimeOfDay(4 * 60));
  SetNightLightEnabled(false);
  // Start time is at 9:00 PM and end time is at 6:00 AM. "Now" is less than
  // both. NightLight should be on.
  //       4:00          6:00         21:00
  // <----- + ----------- + ----------- + ----->
  //        |             |             |
  //       now           end          start
  //
  controller->SetColorTemperature(0.4f);
  controller->SetCustomStartTime(TimeOfDay(21 * 60));
  controller->SetCustomEndTime(TimeOfDay(6 * 60));
  controller->SetScheduleType(NightLightController::ScheduleType::kCustom);

  EXPECT_TRUE(controller->GetEnabled());
  TestCompositorsTemperature(0.4f);
  EXPECT_TRUE(controller->timer().IsRunning());
  // NightLight should end in two hours.
  EXPECT_EQ(base::TimeDelta::FromHours(2),
            controller->timer().GetCurrentDelay());
}

// Case 2: "Now" is between "end" and "start".
TEST_F(NightLightTest, TestCustomScheduleInvertedStartAndEndTimesCase2) {
  NightLightController* controller = GetController();
  // Now is 6:00 AM.
  delegate()->SetFakeNow(TimeOfDay(6 * 60));
  SetNightLightEnabled(false);
  // Start time is at 9:00 PM and end time is at 4:00 AM. "Now" is between both.
  // NightLight should be off.
  //       4:00          6:00         21:00
  // <----- + ----------- + ----------- + ----->
  //        |             |             |
  //       end           now          start
  //
  controller->SetColorTemperature(0.4f);
  controller->SetCustomStartTime(TimeOfDay(21 * 60));
  controller->SetCustomEndTime(TimeOfDay(4 * 60));
  controller->SetScheduleType(NightLightController::ScheduleType::kCustom);

  EXPECT_FALSE(controller->GetEnabled());
  TestCompositorsTemperature(0.0f);
  EXPECT_TRUE(controller->timer().IsRunning());
  // NightLight should start in 15 hours.
  EXPECT_EQ(base::TimeDelta::FromHours(15),
            controller->timer().GetCurrentDelay());
}

// Case 3: "Now" is greater than both "start" and "end".
TEST_F(NightLightTest, TestCustomScheduleInvertedStartAndEndTimesCase3) {
  NightLightController* controller = GetController();
  // Now is 11:00 PM.
  delegate()->SetFakeNow(TimeOfDay(23 * 60));
  SetNightLightEnabled(false);
  // Start time is at 9:00 PM and end time is at 4:00 AM. "Now" is greater than
  // both. NightLight should be on.
  //       4:00         21:00         23:00
  // <----- + ----------- + ----------- + ----->
  //        |             |             |
  //       end          start          now
  //
  controller->SetColorTemperature(0.4f);
  controller->SetCustomStartTime(TimeOfDay(21 * 60));
  controller->SetCustomEndTime(TimeOfDay(4 * 60));
  controller->SetScheduleType(NightLightController::ScheduleType::kCustom);

  EXPECT_TRUE(controller->GetEnabled());
  TestCompositorsTemperature(0.4f);
  EXPECT_TRUE(controller->timer().IsRunning());
  // NightLight should end in 5 hours.
  EXPECT_EQ(base::TimeDelta::FromHours(5),
            controller->timer().GetCurrentDelay());
}

// Fixture for testing behavior of Night Light when displays support hardware
// CRTC matrices.
class NightLightCrtcTest : public NightLightTest {
 public:
  NightLightCrtcTest()
      : logger_(std::make_unique<display::test::ActionLogger>()) {}
  ~NightLightCrtcTest() override = default;

  static constexpr gfx::Size kDisplaySize{1024, 768};
  static constexpr int64_t kId1 = 123;
  static constexpr int64_t kId2 = 456;

  display::DisplayConfigurator* display_configurator() {
    return Shell::Get()->display_configurator();
  }

  // NightLightTest:
  void SetUp() override {
    NightLightTest::SetUp();

    native_display_delegate_ =
        new display::test::TestNativeDisplayDelegate(logger_.get());
    display_configurator()->SetDelegateForTesting(
        std::unique_ptr<display::NativeDisplayDelegate>(
            native_display_delegate_));
    display_change_observer_ = std::make_unique<display::DisplayChangeObserver>(
        display_configurator(), display_manager());
    test_api_ = std::make_unique<display::DisplayConfigurator::TestApi>(
        display_configurator());
  }

  void TearDown() override {
    // DisplayChangeObserver access InputDeviceManager in its destructor, so
    // destroy it first.
    display_change_observer_ = nullptr;
    NightLightTest::TearDown();
  }

  // Builds two displays snapshots into |owned_snapshots_| and return a list of
  // unowned pointers to them. |color_matrix_support| should contain exactly 2
  // boolean values that correspond to the color correction matrix support for
  // both displays.
  std::vector<display::DisplaySnapshot*> BuildAndGetDisplaySnapshots(
      const std::vector<bool>& color_matrix_support) {
    DCHECK_EQ(2u, color_matrix_support.size());
    owned_snapshots_.clear();
    owned_snapshots_.emplace_back(
        display::FakeDisplaySnapshot::Builder()
            .SetId(kId1)
            .SetNativeMode(kDisplaySize)
            .SetCurrentMode(kDisplaySize)
            .SetHasColorCorrectionMatrix(color_matrix_support[0])
            .SetType(display::DISPLAY_CONNECTION_TYPE_INTERNAL)
            .Build());
    owned_snapshots_.back()->set_origin({0, 0});
    owned_snapshots_.emplace_back(
        display::FakeDisplaySnapshot::Builder()
            .SetId(kId2)
            .SetNativeMode(kDisplaySize)
            .SetCurrentMode(kDisplaySize)
            .SetHasColorCorrectionMatrix(color_matrix_support[1])
            .Build());
    owned_snapshots_.back()->set_origin({1030, 0});
    std::vector<display::DisplaySnapshot*> outputs = {
        owned_snapshots_[0].get(), owned_snapshots_[1].get()};
    return outputs;
  }

  // Updates the display configurator and display manager with the given list of
  // display snapshots.
  void UpdateDisplays(const std::vector<display::DisplaySnapshot*>& outputs) {
    native_display_delegate_->set_outputs(outputs);
    display_configurator()->OnConfigurationChanged();
    EXPECT_TRUE(test_api_->TriggerConfigureTimeout());
    display_change_observer_->GetStateForDisplayIds(outputs);
    display_change_observer_->OnDisplayModeChanged(outputs);
  }

  // Returns true if the software cursor is turned on.
  bool IsCursorCompositingEnabled() const {
    return Shell::Get()
        ->window_tree_host_manager()
        ->cursor_window_controller()
        ->ShouldEnableCursorCompositing();
  }

  std::string GetLoggerActionsAndClear() {
    return logger_->GetActionsAndClear();
  }

  bool VerifyCrtcMatrix(int64_t display_id,
                        float temperature,
                        const std::string& logger_actions_string) const {
    constexpr float kRedScale = 1.0f;
    const float blue_scale =
        NightLightController::BlueColorScaleFromTemperature(temperature);
    const float green_scale =
        NightLightController::GreenColorScaleFromTemperature(temperature);
    std::stringstream pattern_stream;
    pattern_stream << "*set_color_correction(id=" << display_id
                   << ",ctm[0]=" << kRedScale << "*ctm[4]=" << green_scale
                   << "*ctm[8]=" << blue_scale << "*)*";
    return base::MatchPattern(logger_actions_string, pattern_stream.str());
  }

 private:
  std::unique_ptr<display::test::ActionLogger> logger_;
  // Not owned.
  display::test::TestNativeDisplayDelegate* native_display_delegate_;
  std::unique_ptr<display::DisplayChangeObserver> display_change_observer_;
  std::unique_ptr<display::DisplayConfigurator::TestApi> test_api_;

  std::vector<std::unique_ptr<display::DisplaySnapshot>> owned_snapshots_;

  DISALLOW_COPY_AND_ASSIGN(NightLightCrtcTest);
};

// static
constexpr gfx::Size NightLightCrtcTest::kDisplaySize;

// All displays support CRTC matrices.
TEST_F(NightLightCrtcTest, TestAllDisplaysSupportCrtcMatrix) {
  // Create two displays with both having support for CRTC matrices.
  std::vector<display::DisplaySnapshot*> outputs =
      BuildAndGetDisplaySnapshots({true, true});
  UpdateDisplays(outputs);

  EXPECT_EQ(2u, display_manager()->GetNumDisplays());
  ASSERT_EQ(2u, RootWindowController::root_window_controllers().size());

  // Turn on Night Light:
  NightLightController* controller = GetController();
  SetNightLightEnabled(true);
  float temperature = 0.2f;
  GetLoggerActionsAndClear();
  controller->SetColorTemperature(temperature);
  EXPECT_EQ(temperature, controller->GetColorTemperature());

  // Since both displays support CRTC matrices, no compositor matrix should be
  // set (i.e. compositor matrix is identity which corresponds to the zero
  // temperature).
  TestCompositorsTemperature(0.0f);
  // Hence software cursor should not be used.
  EXPECT_FALSE(IsCursorCompositingEnabled());
  // Verify correct matrix has been set on both crtcs.
  std::string logger_actions = GetLoggerActionsAndClear();
  EXPECT_TRUE(VerifyCrtcMatrix(kId1, temperature, logger_actions));
  EXPECT_TRUE(VerifyCrtcMatrix(kId2, temperature, logger_actions));

  // Setting a new temperature is applied.
  temperature = 0.65f;
  controller->SetColorTemperature(temperature);
  EXPECT_EQ(temperature, controller->GetColorTemperature());
  TestCompositorsTemperature(0.0f);
  EXPECT_FALSE(IsCursorCompositingEnabled());
  logger_actions = GetLoggerActionsAndClear();
  EXPECT_TRUE(VerifyCrtcMatrix(kId1, temperature, logger_actions));
  EXPECT_TRUE(VerifyCrtcMatrix(kId2, temperature, logger_actions));
}

// One display supports CRTC matrix and the other doesn't.
TEST_F(NightLightCrtcTest, TestMixedCrtcMatrixSupport) {
  std::vector<display::DisplaySnapshot*> outputs =
      BuildAndGetDisplaySnapshots({true, false});
  UpdateDisplays(outputs);

  EXPECT_EQ(2u, display_manager()->GetNumDisplays());
  ASSERT_EQ(2u, RootWindowController::root_window_controllers().size());

  // Turn on Night Light:
  NightLightController* controller = GetController();
  SetNightLightEnabled(true);
  const float temperature = 0.2f;
  GetLoggerActionsAndClear();
  controller->SetColorTemperature(temperature);
  EXPECT_EQ(temperature, controller->GetColorTemperature());

  // The first display supports CRTC matrix, so its compositor has identity
  // matrix.
  TestDisplayCompositorTemperature(kId1, 0.0f);
  // However, the second display doesn't support CRTC matrix, Night Light is
  // using the compositor matrix on this display.
  TestDisplayCompositorTemperature(kId2, temperature);
  // With mixed CTRC support, software cursor must be on.
  EXPECT_TRUE(IsCursorCompositingEnabled());
  // Verify correct matrix has been set on both crtcs.
  const std::string logger_actions = GetLoggerActionsAndClear();
  EXPECT_TRUE(VerifyCrtcMatrix(kId1, temperature, logger_actions));
  EXPECT_FALSE(VerifyCrtcMatrix(kId2, temperature, logger_actions));
}

// All displays don't support CRTC matrices.
TEST_F(NightLightCrtcTest, TestNoCrtcMatrixSupport) {
  std::vector<display::DisplaySnapshot*> outputs =
      BuildAndGetDisplaySnapshots({false, false});
  UpdateDisplays(outputs);

  EXPECT_EQ(2u, display_manager()->GetNumDisplays());
  ASSERT_EQ(2u, RootWindowController::root_window_controllers().size());

  // Turn on Night Light:
  NightLightController* controller = GetController();
  SetNightLightEnabled(true);
  const float temperature = 0.2f;
  GetLoggerActionsAndClear();
  controller->SetColorTemperature(temperature);
  EXPECT_EQ(temperature, controller->GetColorTemperature());

  // Compositor matrices are used on both displays.
  TestCompositorsTemperature(temperature);
  // With no CTRC support, software cursor must be on.
  EXPECT_TRUE(IsCursorCompositingEnabled());
  // No CRTC matrices have been set.
  const std::string logger_actions = GetLoggerActionsAndClear();
  EXPECT_FALSE(VerifyCrtcMatrix(kId1, temperature, logger_actions));
  EXPECT_FALSE(VerifyCrtcMatrix(kId2, temperature, logger_actions));
}

// Tests that switching CRTC matrix support on while Night Light is enabled
// doesn't result in the matrix being applied twice.
TEST_F(NightLightCrtcTest, TestNoDoubleNightLightEffect) {
  std::vector<display::DisplaySnapshot*> outputs =
      BuildAndGetDisplaySnapshots({false, false});
  UpdateDisplays(outputs);

  EXPECT_EQ(2u, display_manager()->GetNumDisplays());
  ASSERT_EQ(2u, RootWindowController::root_window_controllers().size());

  // Turn on Night Light:
  NightLightController* controller = GetController();
  SetNightLightEnabled(true);
  const float temperature = 0.2f;
  GetLoggerActionsAndClear();
  controller->SetColorTemperature(temperature);
  EXPECT_EQ(temperature, controller->GetColorTemperature());

  // Compositor matrices are used on both displays.
  TestCompositorsTemperature(temperature);
  // With no CTRC support, software cursor must be on.
  EXPECT_TRUE(IsCursorCompositingEnabled());
  // No CRTC matrices have been set.
  std::string logger_actions = GetLoggerActionsAndClear();
  EXPECT_FALSE(VerifyCrtcMatrix(kId1, temperature, logger_actions));
  EXPECT_FALSE(VerifyCrtcMatrix(kId2, temperature, logger_actions));

  // Simulate that the two displays suddenly became able to support CRTC matrix.
  // This shouldn't happen in practice, but we noticed multiple times on resume
  // from suspend, or after the display turned on after it was off as a result
  // of no user activity, we noticed that the display can get into a transient
  // state where it is wrongly believed to have no CTM matrix capability, then
  // later corrected. When this happened, we noticed that the Night Light effect
  // is applied twice; once via the CRTC CTM matrix, and another via the
  // compositor matrix. When this happens, we need to assert that the compositor
  // matrix is set to identity, and the cursor compositing is updated correctly.
  // TODO(afakhry): Investigate the root cause of this https://crbug.com/844067.
  std::vector<display::DisplaySnapshot*> outputs2 =
      BuildAndGetDisplaySnapshots({true, true});
  UpdateDisplays(outputs2);
  TestCompositorsTemperature(0.0f);
  EXPECT_FALSE(IsCursorCompositingEnabled());
  logger_actions = GetLoggerActionsAndClear();
  EXPECT_TRUE(VerifyCrtcMatrix(kId1, temperature, logger_actions));
  EXPECT_TRUE(VerifyCrtcMatrix(kId2, temperature, logger_actions));
}

}  // namespace

}  // namespace ash
