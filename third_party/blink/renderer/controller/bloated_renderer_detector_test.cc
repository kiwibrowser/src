// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/controller/bloated_renderer_detector.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class BloatedRendererDetectorTest : public testing::Test {
 public:
  static double GetMockLargeUptime() {
    int large_uptime = BloatedRendererDetector::kMinimumUptimeInMinutes + 1;
    return WTF::TimeTicksInSeconds(
        WTF::TimeTicksFromSeconds(large_uptime * 60));
  }

  static double GetMockSmallUptime() {
    int small_uptime = BloatedRendererDetector::kMinimumUptimeInMinutes - 1;
    return WTF::TimeTicksInSeconds(
        WTF::TimeTicksFromSeconds(small_uptime * 60));
  }
};

TEST_F(BloatedRendererDetectorTest, ForwardToBrowser) {
  BloatedRendererDetector detector(WTF::TimeTicksFromSeconds(0));
  WTF::TimeFunction original_time_function =
      WTF::SetTimeFunctionsForTesting(GetMockLargeUptime);
  EXPECT_EQ(NearV8HeapLimitHandling::kForwardedToBrowser,
            detector.OnNearV8HeapLimitOnMainThreadImpl());
  WTF::SetTimeFunctionsForTesting(original_time_function);
}

TEST_F(BloatedRendererDetectorTest, SmallUptime) {
  BloatedRendererDetector detector(WTF::TimeTicksFromSeconds(0));
  WTF::TimeFunction original_time_function =
      SetTimeFunctionsForTesting(GetMockSmallUptime);
  EXPECT_EQ(NearV8HeapLimitHandling::kIgnoredDueToSmallUptime,
            detector.OnNearV8HeapLimitOnMainThreadImpl());
  WTF::SetTimeFunctionsForTesting(original_time_function);
}

}  // namespace blink
