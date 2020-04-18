// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_media_route_provider_metrics.h"
#include "base/test/histogram_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::Bucket;
using testing::ElementsAre;

namespace media_router {

TEST(CastMediaRouteProviderMetricsTest, RecordAppAvailabilityResult) {
  base::HistogramTester tester;

  RecordAppAvailabilityResult(
      cast_channel::GetAppAvailabilityResult::kAvailable,
      base::TimeDelta::FromMilliseconds(111));
  RecordAppAvailabilityResult(
      cast_channel::GetAppAvailabilityResult::kUnavailable,
      base::TimeDelta::FromMilliseconds(222));
  tester.ExpectTimeBucketCount(kHistogramAppAvailabilitySuccess,
                               base::TimeDelta::FromMilliseconds(111), 1);
  tester.ExpectTimeBucketCount(kHistogramAppAvailabilitySuccess,
                               base::TimeDelta::FromMilliseconds(222), 1);

  RecordAppAvailabilityResult(cast_channel::GetAppAvailabilityResult::kUnknown,
                              base::TimeDelta::FromMilliseconds(333));
  tester.ExpectTimeBucketCount(kHistogramAppAvailabilityFailure,
                               base::TimeDelta::FromMilliseconds(333), 1);
}

}  // namespace media_router
