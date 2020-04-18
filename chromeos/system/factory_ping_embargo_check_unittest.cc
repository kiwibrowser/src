// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/system/factory_ping_embargo_check.h"

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "chromeos/system/fake_statistics_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {
namespace system {

namespace {

// Returns a string which can be put into the |kRlzEmbargoEndDateKey| VPD
// variable. If |days_offset| is 0, the return value represents the current day.
// If |days_offset| is positive, the return value represents |days_offset| days
// in the future. If |days_offset| is negative, the return value represents
// |days_offset| days in the past.
// For example, if the test runs on 2018-01-22 and |days_offset| is 3, the
// return value will be "2018-01-25". Similarly, if |days_offset| is -1, the
// return value will be "2018-01-21".
std::string GenerateEmbargoEndDate(int days_offset) {
  base::Time::Exploded exploded;
  const base::Time target_time =
      base::Time::Now() + base::TimeDelta::FromDays(days_offset);
  target_time.UTCExplode(&exploded);

  const std::string rlz_embargo_end_date_string = base::StringPrintf(
      "%04d-%02d-%02d", exploded.year, exploded.month, exploded.day_of_month);

  // Sanity check that base::Time::FromUTCString can read back the format used
  // here.
  base::Time reparsed_time;
  EXPECT_TRUE(base::Time::FromUTCString(rlz_embargo_end_date_string.c_str(),
                                        &reparsed_time));
  EXPECT_EQ(target_time.ToDeltaSinceWindowsEpoch().InMicroseconds() /
                base::Time::kMicrosecondsPerDay,
            reparsed_time.ToDeltaSinceWindowsEpoch().InMicroseconds() /
                base::Time::kMicrosecondsPerDay);

  return rlz_embargo_end_date_string;
}

}  // namespace

class FactoryPingEmbargoCheckTest : public ::testing::Test {
 protected:
  FakeStatisticsProvider statistics_provider_;
};

// No embargo end date in VPD.
TEST_F(FactoryPingEmbargoCheckTest, NoValue) {
  EXPECT_EQ(FactoryPingEmbargoState::kMissingOrMalformed,
            GetFactoryPingEmbargoState(&statistics_provider_));
}

// There is a malformed embargo end date in VPD.
TEST_F(FactoryPingEmbargoCheckTest, MalformedValue) {
  statistics_provider_.SetMachineStatistic(
      chromeos::system::kRlzEmbargoEndDateKey, "blabla");
  EXPECT_EQ(FactoryPingEmbargoState::kMissingOrMalformed,
            GetFactoryPingEmbargoState(&statistics_provider_));
}

// There is an embargo end date in VPD which is too far in the future to be
// plausible.
TEST_F(FactoryPingEmbargoCheckTest, InvalidValue) {
  statistics_provider_.SetMachineStatistic(
      chromeos::system::kRlzEmbargoEndDateKey,
      GenerateEmbargoEndDate(15 /* days_offset */));
  EXPECT_EQ(FactoryPingEmbargoState::kInvalid,
            GetFactoryPingEmbargoState(&statistics_provider_));
}

// The current time is before a (valid and plausible) embargo end date.
TEST_F(FactoryPingEmbargoCheckTest, EmbargoNotPassed) {
  statistics_provider_.SetMachineStatistic(
      chromeos::system::kRlzEmbargoEndDateKey,
      GenerateEmbargoEndDate(1 /* days_offset */));
  EXPECT_EQ(FactoryPingEmbargoState::kNotPassed,
            GetFactoryPingEmbargoState(&statistics_provider_));
}

// The current time is after a (valid and plausible) embargo end date.
TEST_F(FactoryPingEmbargoCheckTest, EmbargoPassed) {
  statistics_provider_.SetMachineStatistic(
      chromeos::system::kRlzEmbargoEndDateKey,
      GenerateEmbargoEndDate(-1 /* days_offset */));
  EXPECT_EQ(FactoryPingEmbargoState::kPassed,
            GetFactoryPingEmbargoState(&statistics_provider_));
}

}  // namespace system
}  // namespace chromeos
