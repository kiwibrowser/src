// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/off_hours/device_off_hours_controller.h"

#include <string>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/test/simple_test_clock.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/chromeos/settings/device_settings_test_helper.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "chromeos/dbus/fake_system_clock_client.h"
#include "components/policy/proto/chrome_device_policy.pb.h"

namespace em = enterprise_management;

namespace policy {
namespace off_hours {

using base::TimeDelta;

namespace {

constexpr em::WeeklyTimeProto_DayOfWeek kWeekdays[] = {
    em::WeeklyTimeProto::DAY_OF_WEEK_UNSPECIFIED,
    em::WeeklyTimeProto::MONDAY,
    em::WeeklyTimeProto::TUESDAY,
    em::WeeklyTimeProto::WEDNESDAY,
    em::WeeklyTimeProto::THURSDAY,
    em::WeeklyTimeProto::FRIDAY,
    em::WeeklyTimeProto::SATURDAY,
    em::WeeklyTimeProto::SUNDAY};

constexpr TimeDelta kHour = TimeDelta::FromHours(1);
constexpr TimeDelta kDay = TimeDelta::FromDays(1);

const char kUtcTimezone[] = "UTC";

const int kDeviceAllowNewUsersPolicyTag = 3;
const int kDeviceGuestModeEnabledPolicyTag = 8;

struct OffHoursPolicy {
  std::string timezone;
  std::vector<OffHoursInterval> intervals;
  std::vector<int> ignored_policy_proto_tags;

  OffHoursPolicy(const std::string& timezone,
                 const std::vector<OffHoursInterval>& intervals,
                 const std::vector<int>& ignored_policy_proto_tags)
      : timezone(timezone),
        intervals(intervals),
        ignored_policy_proto_tags(ignored_policy_proto_tags) {}

  OffHoursPolicy(const std::string& timezone,
                 const std::vector<OffHoursInterval>& intervals)
      : timezone(timezone),
        intervals(intervals),
        ignored_policy_proto_tags({kDeviceAllowNewUsersPolicyTag,
                                   kDeviceGuestModeEnabledPolicyTag}) {}
};

em::DeviceOffHoursIntervalProto ConvertOffHoursIntervalToProto(
    const OffHoursInterval& off_hours_interval) {
  em::DeviceOffHoursIntervalProto interval_proto;
  em::WeeklyTimeProto* start = interval_proto.mutable_start();
  em::WeeklyTimeProto* end = interval_proto.mutable_end();
  start->set_day_of_week(kWeekdays[off_hours_interval.start().day_of_week()]);
  start->set_time(off_hours_interval.start().milliseconds());
  end->set_day_of_week(kWeekdays[off_hours_interval.end().day_of_week()]);
  end->set_time(off_hours_interval.end().milliseconds());
  return interval_proto;
}

void RemoveOffHoursPolicyFromProto(em::ChromeDeviceSettingsProto* proto) {
  proto->clear_device_off_hours();
}

void SetOffHoursPolicyToProto(em::ChromeDeviceSettingsProto* proto,
                              const OffHoursPolicy& off_hours_policy) {
  RemoveOffHoursPolicyFromProto(proto);
  auto* off_hours = proto->mutable_device_off_hours();
  for (auto interval : off_hours_policy.intervals) {
    auto interval_proto = ConvertOffHoursIntervalToProto(interval);
    auto* cur = off_hours->add_intervals();
    *cur = interval_proto;
  }
  off_hours->set_timezone(off_hours_policy.timezone);
  for (auto p : off_hours_policy.ignored_policy_proto_tags) {
    off_hours->add_ignored_policy_proto_tags(p);
  }
}

}  // namespace

class DeviceOffHoursControllerSimpleTest
    : public chromeos::DeviceSettingsTestBase {
 protected:
  DeviceOffHoursControllerSimpleTest()
      : fake_user_manager_(new chromeos::FakeChromeUserManager()),
        scoped_user_manager_(base::WrapUnique(fake_user_manager_)) {}

  void SetUp() override {
    chromeos::DeviceSettingsTestBase::SetUp();
    system_clock_client_ = new chromeos::FakeSystemClockClient();
    dbus_setter_->SetSystemClockClient(base::WrapUnique(system_clock_client_));
    power_manager_client_ = new chromeos::FakePowerManagerClient();
    dbus_setter_->SetPowerManagerClient(
        base::WrapUnique(power_manager_client_));

    device_settings_service_.SetDeviceOffHoursControllerForTesting(
        std::make_unique<policy::off_hours::DeviceOffHoursController>());
    device_off_hours_controller_ =
        device_settings_service_.device_off_hours_controller();
  }

  void UpdateDeviceSettings() {
    device_policy_.Build();
    session_manager_client_.set_device_policy(device_policy_.GetBlob());
    ReloadDeviceSettings();
  }

  // Return number of weekday from 1 to 7 in |input_time|. (1 = Monday etc.)
  int ExtractDayOfWeek(base::Time input_time) {
    base::Time::Exploded exploded;
    input_time.UTCExplode(&exploded);
    int current_day_of_week = exploded.day_of_week;
    if (current_day_of_week == 0)
      current_day_of_week = 7;
    return current_day_of_week;
  }

  // Return next day of week. |day_of_week| and return value are from 1 to 7. (1
  // = Monday etc.)
  int NextDayOfWeek(int day_of_week) { return day_of_week % 7 + 1; }

  chromeos::FakeSystemClockClient* system_clock_client() {
    return system_clock_client_;
  }

  chromeos::FakePowerManagerClient* power_manager() {
    return power_manager_client_;
  }

  policy::off_hours::DeviceOffHoursController* device_off_hours_controller() {
    return device_off_hours_controller_;
  }

  chromeos::FakeChromeUserManager* fake_user_manager() {
    return fake_user_manager_;
  }

 private:
  // The object is owned by DeviceSettingsTestBase class.
  chromeos::FakeSystemClockClient* system_clock_client_;

  // The object is owned by DeviceSettingsTestBase class.
  chromeos::FakePowerManagerClient* power_manager_client_;

  // The object is owned by DeviceSettingsService class.
  policy::off_hours::DeviceOffHoursController* device_off_hours_controller_;

  chromeos::FakeChromeUserManager* fake_user_manager_;
  user_manager::ScopedUserManager scoped_user_manager_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursControllerSimpleTest);
};

TEST_F(DeviceOffHoursControllerSimpleTest, CheckOffHoursUnset) {
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  RemoveOffHoursPolicyFromProto(&proto);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerSimpleTest, CheckOffHoursModeOff) {
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  int current_day_of_week = ExtractDayOfWeek(base::Time::Now());
  SetOffHoursPolicyToProto(
      &proto, OffHoursPolicy(
                  kUtcTimezone,
                  {OffHoursInterval(
                      WeeklyTime(NextDayOfWeek(current_day_of_week),
                                 TimeDelta::FromHours(10).InMilliseconds()),
                      WeeklyTime(NextDayOfWeek(current_day_of_week),
                                 TimeDelta::FromHours(15).InMilliseconds()))}));
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerSimpleTest, CheckOffHoursModeOn) {
  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  int current_day_of_week = ExtractDayOfWeek(base::Time::Now());
  SetOffHoursPolicyToProto(
      &proto, OffHoursPolicy(
                  kUtcTimezone,
                  {OffHoursInterval(
                      WeeklyTime(current_day_of_week, 0),
                      WeeklyTime(NextDayOfWeek(current_day_of_week),
                                 TimeDelta::FromHours(10).InMilliseconds()))}));
  UpdateDeviceSettings();
  EXPECT_TRUE(device_settings_service_.device_settings()
                  ->guest_mode_enabled()
                  .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerSimpleTest, NoNetworkSynchronization) {
  system_clock_client()->set_network_synchronized(false);
  system_clock_client()->NotifyObserversSystemClockUpdated();
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  UpdateDeviceSettings();
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
  int current_day_of_week = ExtractDayOfWeek(base::Time::Now());
  SetOffHoursPolicyToProto(
      &proto, OffHoursPolicy(
                  kUtcTimezone,
                  {OffHoursInterval(
                      WeeklyTime(current_day_of_week, 0),
                      WeeklyTime(NextDayOfWeek(current_day_of_week),
                                 TimeDelta::FromHours(10).InMilliseconds()))}));
  EXPECT_FALSE(device_settings_service_.device_settings()
                   ->guest_mode_enabled()
                   .guest_mode_enabled());
}

TEST_F(DeviceOffHoursControllerSimpleTest,
       IsCurrentSessionAllowedOnlyForOffHours) {
  EXPECT_FALSE(
      device_off_hours_controller()->IsCurrentSessionAllowedOnlyForOffHours());

  system_clock_client()->set_network_synchronized(true);
  system_clock_client()->NotifyObserversSystemClockUpdated();

  EXPECT_FALSE(
      device_off_hours_controller()->IsCurrentSessionAllowedOnlyForOffHours());

  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  proto.mutable_guest_mode_enabled()->set_guest_mode_enabled(false);
  int current_day_of_week = ExtractDayOfWeek(base::Time::Now());
  SetOffHoursPolicyToProto(
      &proto, OffHoursPolicy(
                  kUtcTimezone,
                  {OffHoursInterval(
                      WeeklyTime(current_day_of_week, 0),
                      WeeklyTime(NextDayOfWeek(current_day_of_week),
                                 TimeDelta::FromHours(10).InMilliseconds()))}));
  UpdateDeviceSettings();

  EXPECT_FALSE(
      device_off_hours_controller()->IsCurrentSessionAllowedOnlyForOffHours());

  fake_user_manager()->AddGuestUser();
  fake_user_manager()->LoginUser(fake_user_manager()->GetGuestAccountId());

  EXPECT_TRUE(
      device_off_hours_controller()->IsCurrentSessionAllowedOnlyForOffHours());
}

class DeviceOffHoursControllerFakeClockTest
    : public DeviceOffHoursControllerSimpleTest {
 protected:
  DeviceOffHoursControllerFakeClockTest() {}

  void SetUp() override {
    DeviceOffHoursControllerSimpleTest::SetUp();
    system_clock_client()->set_network_synchronized(true);
    system_clock_client()->NotifyObserversSystemClockUpdated();
    // Clocks are set to 1970-01-01 00:00:00 UTC, Thursday.
    test_clock_.SetNow(base::Time::UnixEpoch());
    test_tick_clock_.SetNowTicks(base::TimeTicks::UnixEpoch());
    device_off_hours_controller()->SetClockForTesting(&test_clock_,
                                                      &test_tick_clock_);
  }

  void AdvanceTestClock(TimeDelta duration) {
    test_clock_.Advance(duration);
    test_tick_clock_.Advance(duration);
  }

  base::Clock* clock() { return &test_clock_; }

 private:
  base::SimpleTestClock test_clock_;
  base::SimpleTestTickClock test_tick_clock_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursControllerFakeClockTest);
};

TEST_F(DeviceOffHoursControllerFakeClockTest, FakeClock) {
  EXPECT_FALSE(device_off_hours_controller()->is_off_hours_mode());
  int current_day_of_week = ExtractDayOfWeek(clock()->Now());
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  SetOffHoursPolicyToProto(
      &proto, OffHoursPolicy(
                  kUtcTimezone,
                  {OffHoursInterval(
                      WeeklyTime(current_day_of_week,
                                 TimeDelta::FromHours(14).InMilliseconds()),
                      WeeklyTime(current_day_of_week,
                                 TimeDelta::FromHours(15).InMilliseconds()))}));
  AdvanceTestClock(TimeDelta::FromHours(14));
  UpdateDeviceSettings();
  EXPECT_TRUE(device_off_hours_controller()->is_off_hours_mode());
  AdvanceTestClock(TimeDelta::FromHours(1));
  UpdateDeviceSettings();
  EXPECT_FALSE(device_off_hours_controller()->is_off_hours_mode());
}

TEST_F(DeviceOffHoursControllerFakeClockTest, CheckSendSuspendDone) {
  int current_day_of_week = ExtractDayOfWeek(clock()->Now());
  LOG(ERROR) << "day " << current_day_of_week;
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  SetOffHoursPolicyToProto(
      &proto,
      OffHoursPolicy(
          kUtcTimezone,
          {OffHoursInterval(WeeklyTime(NextDayOfWeek(current_day_of_week), 0),
                            WeeklyTime(NextDayOfWeek(current_day_of_week),
                                       kHour.InMilliseconds()))}));
  UpdateDeviceSettings();
  EXPECT_FALSE(device_off_hours_controller()->is_off_hours_mode());

  AdvanceTestClock(kDay);
  power_manager()->SendSuspendDone();
  EXPECT_TRUE(device_off_hours_controller()->is_off_hours_mode());

  AdvanceTestClock(kHour);
  power_manager()->SendSuspendDone();
  EXPECT_FALSE(device_off_hours_controller()->is_off_hours_mode());
}

class DeviceOffHoursControllerUpdateTest
    : public DeviceOffHoursControllerFakeClockTest,
      public testing::WithParamInterface<
          std::tuple<OffHoursPolicy, TimeDelta, bool>> {
 public:
  OffHoursPolicy off_hours_policy() const { return std::get<0>(GetParam()); }
  TimeDelta advance_clock() const { return std::get<1>(GetParam()); }
  bool is_off_hours_expected() const { return std::get<2>(GetParam()); }
};

TEST_P(DeviceOffHoursControllerUpdateTest, CheckUpdateOffHoursPolicy) {
  em::ChromeDeviceSettingsProto& proto(device_policy_.payload());
  SetOffHoursPolicyToProto(&proto, off_hours_policy());
  AdvanceTestClock(advance_clock());
  UpdateDeviceSettings();
  EXPECT_EQ(device_off_hours_controller()->is_off_hours_mode(),
            is_off_hours_expected());
}

INSTANTIATE_TEST_CASE_P(
    TestCases,
    DeviceOffHoursControllerUpdateTest,
    testing::Values(
        std::make_tuple(
            OffHoursPolicy(
                kUtcTimezone,
                {OffHoursInterval(
                    WeeklyTime(em::WeeklyTimeProto::THURSDAY,
                               TimeDelta::FromHours(1).InMilliseconds()),
                    WeeklyTime(em::WeeklyTimeProto::THURSDAY,
                               TimeDelta::FromHours(2).InMilliseconds()))}),
            kHour,
            true),
        std::make_tuple(
            OffHoursPolicy(
                kUtcTimezone,
                {OffHoursInterval(
                    WeeklyTime(em::WeeklyTimeProto::THURSDAY,
                               TimeDelta::FromHours(1).InMilliseconds()),
                    WeeklyTime(em::WeeklyTimeProto::THURSDAY,
                               TimeDelta::FromHours(2).InMilliseconds()))}),
            kHour * 2,
            false),
        std::make_tuple(
            OffHoursPolicy(
                kUtcTimezone,
                {OffHoursInterval(
                    WeeklyTime(em::WeeklyTimeProto::THURSDAY,
                               TimeDelta::FromHours(1).InMilliseconds()),
                    WeeklyTime(em::WeeklyTimeProto::THURSDAY,
                               TimeDelta::FromHours(2).InMilliseconds()))}),
            kHour * 1.5,
            true),
        std::make_tuple(
            OffHoursPolicy(
                kUtcTimezone,
                {OffHoursInterval(
                    WeeklyTime(em::WeeklyTimeProto::THURSDAY,
                               TimeDelta::FromHours(1).InMilliseconds()),
                    WeeklyTime(em::WeeklyTimeProto::THURSDAY,
                               TimeDelta::FromHours(2).InMilliseconds()))}),
            kHour * 3,
            false)));

}  // namespace off_hours
}  // namespace policy
