// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_test_base.h"

#include "ash/public/cpp/ash_switches.h"
#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shell.h"
#include "ash/shell_test_api.h"
#include "ash/system/power/power_button_controller.h"
#include "ash/system/power/power_button_controller_test_api.h"
#include "ash/wm/lock_state_controller.h"
#include "ash/wm/lock_state_controller_test_api.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/test/simple_test_tick_clock.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "ui/events/event.h"
#include "ui/events/test/event_generator.h"

namespace ash {

PowerButtonTestBase::PowerButtonTestBase() = default;

PowerButtonTestBase::~PowerButtonTestBase() = default;

void PowerButtonTestBase::SetUp() {
  // This also initializes DBusThreadManager.
  std::unique_ptr<chromeos::DBusThreadManagerSetter> dbus_setter =
      chromeos::DBusThreadManager::GetSetterForTesting();
  power_manager_client_ = new chromeos::FakePowerManagerClient();
  dbus_setter->SetPowerManagerClient(base::WrapUnique(power_manager_client_));
  session_manager_client_ = new chromeos::FakeSessionManagerClient;
  dbus_setter->SetSessionManagerClient(
      base::WrapUnique(session_manager_client_));
  AshTestBase::SetUp();

  lock_state_controller_ = Shell::Get()->lock_state_controller();
  lock_state_test_api_ =
      std::make_unique<LockStateControllerTestApi>(lock_state_controller_);
}

void PowerButtonTestBase::TearDown() {
  AshTestBase::TearDown();
  chromeos::DBusThreadManager::Shutdown();
}

void PowerButtonTestBase::ResetPowerButtonController() {
  ShellTestApi().ResetPowerButtonControllerForTest();
  power_button_test_api_ = nullptr;
  InitPowerButtonControllerMembers(
      chromeos::PowerManagerClient::TabletMode::UNSUPPORTED);
}

void PowerButtonTestBase::InitPowerButtonControllerMembers(
    chromeos::PowerManagerClient::TabletMode initial_tablet_mode_switch_state) {
  power_button_controller_ = Shell::Get()->power_button_controller();
  power_button_test_api_ =
      std::make_unique<PowerButtonControllerTestApi>(power_button_controller_);
  power_button_test_api_->SetTickClock(&tick_clock_);

  if (initial_tablet_mode_switch_state !=
      chromeos::PowerManagerClient::TabletMode::UNSUPPORTED) {
    SetTabletModeSwitchState(initial_tablet_mode_switch_state);
  } else {
    screenshot_controller_ = nullptr;
  }
}

void PowerButtonTestBase::SetTabletModeSwitchState(
    chromeos::PowerManagerClient::TabletMode tablet_mode_switch_state) {
  power_button_controller_->OnGetSwitchStates(
      chromeos::PowerManagerClient::SwitchStates{
          chromeos::PowerManagerClient::LidState::OPEN,
          tablet_mode_switch_state});

  screenshot_controller_ = power_button_test_api_->GetScreenshotController();
}

void PowerButtonTestBase::PressPowerButton() {
  power_button_controller_->PowerButtonEventReceived(true,
                                                     tick_clock_.NowTicks());
}

void PowerButtonTestBase::ReleasePowerButton() {
  power_button_controller_->PowerButtonEventReceived(false,
                                                     tick_clock_.NowTicks());
}

void PowerButtonTestBase::PressKey(ui::KeyboardCode key_code) {
  GetEventGenerator().PressKey(key_code, ui::EF_NONE);
}

void PowerButtonTestBase::ReleaseKey(ui::KeyboardCode key_code) {
  GetEventGenerator().ReleaseKey(key_code, ui::EF_NONE);
}

void PowerButtonTestBase::GenerateMouseMoveEvent() {
  GetEventGenerator().MoveMouseTo(10, 10);
}

void PowerButtonTestBase::Initialize(
    PowerButtonController::ButtonType button_type,
    LoginStatus status) {
  power_button_test_api_->SetPowerButtonType(button_type);
  if (status == LoginStatus::NOT_LOGGED_IN)
    ClearLogin();
  else
    CreateUserSessions(1);

  if (status == LoginStatus::GUEST)
    SetCanLockScreen(false);
}

void PowerButtonTestBase::LockScreen() {
  lock_state_controller_->OnLockStateChanged(true);
  Shell::Get()->session_controller()->LockScreenAndFlushForTest();
}

void PowerButtonTestBase::UnlockScreen() {
  lock_state_controller_->OnLockStateChanged(false);
  GetSessionControllerClient()->UnlockScreen();
}

void PowerButtonTestBase::EnableTabletMode(bool enable) {
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(enable);
}

void PowerButtonTestBase::AdvanceClockToAvoidIgnoring() {
  tick_clock_.Advance(PowerButtonController::kIgnoreRepeatedButtonUpDelay +
                      base::TimeDelta::FromMilliseconds(1));
}

}  // namespace ash
