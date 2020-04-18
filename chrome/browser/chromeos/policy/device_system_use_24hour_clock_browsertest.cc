// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/login_status.h"
#include "ash/shell.h"
#include "ash/system/date/date_view.h"
#include "ash/system/date/system_info_default_view.h"
#include "ash/system/date/tray_system_info.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_test_api.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/policy/device_policy_cros_browser_test.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/system/system_clock.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chromeos/chromeos_switches.h"
#include "components/policy/proto/chrome_device_policy.pb.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace em = enterprise_management;

namespace chromeos {

class SystemUse24HourClockPolicyTest
    : public policy::DevicePolicyCrosBrowserTest {
 public:
  SystemUse24HourClockPolicyTest() {
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kLoginManager);
    command_line->AppendSwitch(chromeos::switches::kForceLoginManagerInTests);
  }

  void SetUpInProcessBrowserTestFixture() override {
    InstallOwnerKey();
    MarkAsEnterpriseOwned();
    DevicePolicyCrosBrowserTest::SetUpInProcessBrowserTestFixture();
  }

  void TearDownOnMainThread() override {
    // If the login display is still showing, exit gracefully.
    if (LoginDisplayHost::default_host()) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::BindOnce(&chrome::AttemptExit));
      content::RunMessageLoop();
    }
  }

 protected:
  void RefreshPolicyAndWaitDeviceSettingsUpdated() {
    base::RunLoop run_loop;
    std::unique_ptr<CrosSettings::ObserverSubscription> observer =
        CrosSettings::Get()->AddSettingsObserver(
            kSystemUse24HourClock, run_loop.QuitWhenIdleClosure());

    RefreshDevicePolicy();
    run_loop.Run();
  }

  static bool SystemClockShouldUse24Hour() {
    return g_browser_process->platform_part()
        ->GetSystemClock()
        ->ShouldUse24HourClock();
  }

  static ash::TraySystemInfo* GetTraySystemInfo() {
    return ash::SystemTrayTestApi(ash::Shell::Get()->GetPrimarySystemTray())
        .tray_system_info();
  }

  static base::HourClockType TestGetPrimarySystemTrayTimeHourType() {
    const ash::TraySystemInfo* tray_system_info = GetTraySystemInfo();
    const ash::tray::TimeView* time_tray =
        tray_system_info->GetTimeTrayForTesting();

    return time_tray->GetHourTypeForTesting();
  }

  static bool TestPrimarySystemTrayHasDateDefaultView() {
    const ash::TraySystemInfo* tray_system_info = GetTraySystemInfo();
    const ash::SystemInfoDefaultView* system_info_default_view =
        tray_system_info->GetDefaultViewForTesting();
    return system_info_default_view != nullptr;
  }

  static void TestPrimarySystemTrayCreateDefaultView() {
    ash::TraySystemInfo* tray_system_info = GetTraySystemInfo();
    tray_system_info->CreateDefaultViewForTesting(
        ash::LoginStatus::NOT_LOGGED_IN);
  }

  static base::HourClockType TestGetPrimarySystemTrayDateHourType() {
    const ash::TraySystemInfo* tray_system_info = GetTraySystemInfo();
    const ash::SystemInfoDefaultView* system_info_default_view =
        tray_system_info->GetDefaultViewForTesting();

    return system_info_default_view->GetDateView()->GetHourTypeForTesting();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemUse24HourClockPolicyTest);
};

IN_PROC_BROWSER_TEST_F(SystemUse24HourClockPolicyTest, CheckUnset) {
  bool system_use_24hour_clock;
  EXPECT_FALSE(CrosSettings::Get()->GetBoolean(kSystemUse24HourClock,
                                               &system_use_24hour_clock));

  EXPECT_FALSE(SystemClockShouldUse24Hour());
  EXPECT_EQ(base::k12HourClock, TestGetPrimarySystemTrayTimeHourType());
  EXPECT_FALSE(TestPrimarySystemTrayHasDateDefaultView());

  TestPrimarySystemTrayCreateDefaultView();
  EXPECT_EQ(base::k12HourClock, TestGetPrimarySystemTrayDateHourType());
}

IN_PROC_BROWSER_TEST_F(SystemUse24HourClockPolicyTest, CheckTrue) {
  bool system_use_24hour_clock = true;
  EXPECT_FALSE(CrosSettings::Get()->GetBoolean(kSystemUse24HourClock,
                                               &system_use_24hour_clock));
  EXPECT_FALSE(TestPrimarySystemTrayHasDateDefaultView());

  EXPECT_FALSE(SystemClockShouldUse24Hour());
  EXPECT_EQ(base::k12HourClock, TestGetPrimarySystemTrayTimeHourType());
  TestPrimarySystemTrayCreateDefaultView();
  EXPECT_EQ(base::k12HourClock, TestGetPrimarySystemTrayDateHourType());

  em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
  proto.mutable_use_24hour_clock()->set_use_24hour_clock(true);
  RefreshPolicyAndWaitDeviceSettingsUpdated();

  system_use_24hour_clock = false;
  EXPECT_TRUE(CrosSettings::Get()->GetBoolean(kSystemUse24HourClock,
                                              &system_use_24hour_clock));
  EXPECT_TRUE(system_use_24hour_clock);
  EXPECT_TRUE(SystemClockShouldUse24Hour());
  EXPECT_EQ(base::k24HourClock, TestGetPrimarySystemTrayTimeHourType());

  EXPECT_TRUE(TestPrimarySystemTrayHasDateDefaultView());
  EXPECT_EQ(base::k24HourClock, TestGetPrimarySystemTrayDateHourType());
}

IN_PROC_BROWSER_TEST_F(SystemUse24HourClockPolicyTest, CheckFalse) {
  bool system_use_24hour_clock = true;
  EXPECT_FALSE(CrosSettings::Get()->GetBoolean(kSystemUse24HourClock,
                                               &system_use_24hour_clock));
  EXPECT_FALSE(TestPrimarySystemTrayHasDateDefaultView());

  EXPECT_FALSE(SystemClockShouldUse24Hour());
  EXPECT_EQ(base::k12HourClock, TestGetPrimarySystemTrayTimeHourType());
  TestPrimarySystemTrayCreateDefaultView();
  EXPECT_EQ(base::k12HourClock, TestGetPrimarySystemTrayDateHourType());

  em::ChromeDeviceSettingsProto& proto(device_policy()->payload());
  proto.mutable_use_24hour_clock()->set_use_24hour_clock(false);
  RefreshPolicyAndWaitDeviceSettingsUpdated();

  system_use_24hour_clock = true;
  EXPECT_TRUE(CrosSettings::Get()->GetBoolean(kSystemUse24HourClock,
                                              &system_use_24hour_clock));
  EXPECT_FALSE(system_use_24hour_clock);
  EXPECT_FALSE(SystemClockShouldUse24Hour());
  EXPECT_EQ(base::k12HourClock, TestGetPrimarySystemTrayTimeHourType());
  EXPECT_TRUE(TestPrimarySystemTrayHasDateDefaultView());
  EXPECT_EQ(base::k12HourClock, TestGetPrimarySystemTrayDateHourType());
}

}  // namespace chromeos
