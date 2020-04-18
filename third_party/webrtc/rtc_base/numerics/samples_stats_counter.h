/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef RTC_BASE_NUMERICS_SAMPLES_STATS_COUNTER_H_
#define RTC_BASE_NUMERICS_SAMPLES_STATS_COUNTER_H_

#include <limits>
#include <vector>

#include "rtc_base/checks.h"

namespace webrtc {

class SamplesStatsCounter {
 public:
  SamplesStatsCounter();
  ~SamplesStatsCounter();
  SamplesStatsCounter(SamplesStatsCounter&);
  SamplesStatsCounter(SamplesStatsCounter&&);

  // Adds sample to the stats in amortized O(1) time.
  void AddSample(double value);

  // Returns if there are any values in O(1) time.
  bool IsEmpty() const { return samples_.empty(); }

  // Returns min in O(1) time. This function may not be called if there are no
  // samples.
  double GetMin() const {
    RTC_DCHECK(!IsEmpty());
    return min_;
  }
  // Returns max in O(1) time. This function may not be called if there are no
  // samples.
  double GetMax() const {
    RTC_DCHECK(!IsEmpty());
    return max_;
  }
  // Returns average in O(1) time. This function may not be called if there are
  // no samples.
  double GetAverage() const {
    RTC_DCHECK(!IsEmpty());
    return sum_ / samples_.size();
  }
  // Returns percentile in O(nlogn) on first call and in O(1) after, if no
  // additions were done. This function may not be called if there are no
  // samples.
  //
  // |percentile| has to be in [0; 1]. 0 percentile is the min in the array and
  // 1 percentile is the max in the array.
  double GetPercentile(double percentile);

 private:
  std::vector<double> samples_;
  double min_ = std::numeric_limits<double>::max();
  double max_ = std::numeric_limits<double>::min();
  double sum_ = 0;
  bool sorted_ = false;
};

}  // namespace webrtc

#endif  // RTC_BASE_NUMERICS_SAMPLES_STATS_COUNTER_H_
