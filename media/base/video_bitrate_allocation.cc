// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "video_bitrate_allocation.h"

#include <limits>
#include <numeric>

#include "base/logging.h"

namespace media {

constexpr size_t VideoBitrateAllocation::kMaxSpatialLayers;
constexpr size_t VideoBitrateAllocation::kMaxTemporalLayers;

VideoBitrateAllocation::VideoBitrateAllocation() : bitrates_{} {}

bool VideoBitrateAllocation::SetBitrate(size_t spatial_index,
                                        size_t temporal_index,
                                        int bitrate_bps) {
  CHECK_LT(spatial_index, kMaxSpatialLayers);
  CHECK_LT(temporal_index, kMaxTemporalLayers);
  CHECK_GE(bitrate_bps, 0);

  if (GetSumBps() - bitrates_[spatial_index][temporal_index] >
      std::numeric_limits<int>::max() - bitrate_bps) {
    return false;  // Would cause overflow of the sum.
  }

  bitrates_[spatial_index][temporal_index] = bitrate_bps;
  return true;
}

int VideoBitrateAllocation::GetBitrateBps(size_t spatial_index,
                                          size_t temporal_index) const {
  CHECK_LT(spatial_index, kMaxSpatialLayers);
  CHECK_LT(temporal_index, kMaxTemporalLayers);
  return bitrates_[spatial_index][temporal_index];
}

int VideoBitrateAllocation::GetSumBps() const {
  int sum = 0;
  for (size_t spatial_index = 0; spatial_index < kMaxSpatialLayers;
       ++spatial_index) {
    for (size_t temporal_index = 0; temporal_index < kMaxTemporalLayers;
         ++temporal_index) {
      sum += bitrates_[spatial_index][temporal_index];
    }
  }
  return sum;
}

}  // namespace media
