// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <set>

#include "base/logging.h"
#include "media/base/video_bitrate_allocation.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

TEST(VideoBitrateAllocationTest, SetAndGet) {
  int sum = 0;
  int layer_rate = 0;
  VideoBitrateAllocation allocation;
  for (size_t spatial_index = 0;
       spatial_index < VideoBitrateAllocation::kMaxSpatialLayers;
       ++spatial_index) {
    for (size_t temporal_index = 0;
         temporal_index < VideoBitrateAllocation::kMaxTemporalLayers;
         ++temporal_index) {
      sum += layer_rate;
      EXPECT_TRUE(
          allocation.SetBitrate(spatial_index, temporal_index, layer_rate++));
    }
  }
  EXPECT_EQ(sum, allocation.GetSumBps());

  layer_rate = 0;
  for (size_t spatial_index = 0;
       spatial_index < VideoBitrateAllocation::kMaxSpatialLayers;
       ++spatial_index) {
    for (size_t temporal_index = 0;
         temporal_index < VideoBitrateAllocation::kMaxTemporalLayers;
         ++temporal_index) {
      EXPECT_EQ(allocation.GetBitrateBps(spatial_index, temporal_index),
                layer_rate++);
    }
  }
}

TEST(VideoBitrateAllocationTest, CanSetMaxValue) {
  VideoBitrateAllocation allocation;
  // Single cell containing max value.
  EXPECT_TRUE(allocation.SetBitrate(0, 0, std::numeric_limits<int>::max()));
  // Setting to 0 is OK. Doesn't increase sum.
  EXPECT_TRUE(allocation.SetBitrate(0, 1, 0));
  // Adding 1 will overflow.
  EXPECT_FALSE(allocation.SetBitrate(0, 2, 1));

  EXPECT_EQ(std::numeric_limits<int>::max(), allocation.GetSumBps());
}

TEST(VideoBitrateAllocationTest, ValidatesSumWhenOverwriting) {
  VideoBitrateAllocation allocation;
  // Fill up to max sum.
  EXPECT_TRUE(allocation.SetBitrate(0, 0, std::numeric_limits<int>::max() - 2));
  EXPECT_TRUE(allocation.SetBitrate(0, 1, 2));
  // This will overflow, old value kept.
  EXPECT_FALSE(allocation.SetBitrate(0, 1, 3));
  EXPECT_EQ(allocation.GetBitrateBps(0, 1), 2);
  // OK only since we subtract the previous 2.
  EXPECT_TRUE(allocation.SetBitrate(0, 1, 1));
  EXPECT_EQ(allocation.GetBitrateBps(0, 1), 1);

  EXPECT_EQ(std::numeric_limits<int>::max() - 1, allocation.GetSumBps());
}

}  // namespace media
