// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/time.h"

#include <ctime>

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

using std::chrono::seconds;

namespace openscreen {
namespace platform {

#if __cplusplus < 202000L
// Before C++20, the standard does not guarantee that time_t is the number of
// seconds since the UNIX epoch. It also doesn't guarantee that one tick of
// time_t represents one second. However, this is so universally common, it's
// worth assuming the platform provides this.
TEST(TimeTest, TimeTMeetsTheCpp20Standard) {
  // Use the functions the standard *does* provide for calendar time
  // conversions to determine the value of time_t at the UNIX epoch.
  std::tm epoch_tm;
  epoch_tm.tm_sec = 0;
  epoch_tm.tm_min = 0;
  epoch_tm.tm_hour = 0;
  epoch_tm.tm_mday = 1;
  epoch_tm.tm_mon = 0;
  epoch_tm.tm_year = 70;  // 1970
  epoch_tm.tm_isdst = 0;
  // Note: std::mktime() assumes the translation is in the local time zone.
  const std::time_t new_year_1970_in_local_tz = std::mktime(&epoch_tm);
  // Purposely misinterpret |new_year_1970_in_local_tz| as UTC to provide
  // information as to how to offset |epoch_tm| such that std::mktime() will
  // be fooled into returning the value in terms of UTC.
  std::tm* const wrong_tm = std::gmtime(&new_year_1970_in_local_tz);
  epoch_tm.tm_sec += epoch_tm.tm_sec - wrong_tm->tm_sec;
  epoch_tm.tm_min += epoch_tm.tm_min - wrong_tm->tm_min;
  epoch_tm.tm_hour += epoch_tm.tm_hour - wrong_tm->tm_hour;
  epoch_tm.tm_mday += epoch_tm.tm_mday - wrong_tm->tm_mday;
  epoch_tm.tm_mon += epoch_tm.tm_mon - wrong_tm->tm_mon;
  epoch_tm.tm_year += epoch_tm.tm_year - wrong_tm->tm_year;

  const std::time_t epoch = std::mktime(&epoch_tm);
  EXPECT_EQ(seconds(0), seconds(epoch));

  ++epoch_tm.tm_sec;
  const std::time_t epoch_plus_one_second = std::mktime(&epoch_tm);
  EXPECT_EQ(seconds(1), seconds(epoch_plus_one_second));
}
#endif

}  // namespace platform
}  // namespace openscreen
