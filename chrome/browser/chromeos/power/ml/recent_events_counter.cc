// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/recent_events_counter.h"

#include "base/logging.h"

namespace chromeos {
namespace power {
namespace ml {

RecentEventsCounter::RecentEventsCounter(base::TimeDelta duration,
                                         int num_buckets)
    : duration_(duration), num_buckets_(num_buckets) {
  DCHECK_GT(num_buckets_, 0);
  bucket_duration_ = duration_ / num_buckets_;
  event_count_.resize(num_buckets_, 0);
}

RecentEventsCounter::~RecentEventsCounter() = default;

void RecentEventsCounter::Log(base::TimeDelta timestamp) {
  if (timestamp < first_bucket_time_) {
    // This event is too old to log.
    return;
  }
  if (timestamp > latest_) {
    latest_ = timestamp;
  }
  int bucket_index = GetBucketIndex(timestamp);
  if (timestamp < first_bucket_time_ + duration_) {
    // The event is within the current time window so increment the bucket.
    event_count_[bucket_index]++;
    return;
  }

  // The event is later than the current window for the existing data.
  event_count_[bucket_index] = 1;

  for (int i = first_bucket_index_; i != bucket_index;
       i = (i + 1) % num_buckets_) {
    event_count_[i] = 0;
  }
  first_bucket_index_ = (bucket_index + 1) % num_buckets_;

  int num_cycles = floor(timestamp / duration_);
  base::TimeDelta cycle_start = num_cycles * duration_;
  int extra_buckets = floor((timestamp - cycle_start) / bucket_duration_);
  first_bucket_time_ = cycle_start + extra_buckets * bucket_duration_ +
                       bucket_duration_ - duration_;
}

int RecentEventsCounter::GetTotal(base::TimeDelta now) {
  DCHECK_GE(now, latest_);
  if (now >= first_bucket_time_ + 2 * duration_) {
    return 0;
  }
  int total = 0;
  base::TimeDelta start =
      std::max(first_bucket_time_, now - duration_ + bucket_duration_);
  base::TimeDelta end =
      std::min(now + bucket_duration_, first_bucket_time_ + duration_);
  int end_index = GetBucketIndex(end);
  for (int i = GetBucketIndex(start); i != end_index;
       i = (i + 1) % num_buckets_) {
    total += event_count_[i];
  }
  return total;
}

int RecentEventsCounter::GetBucketIndex(base::TimeDelta timestamp) const {
  DCHECK_GE(timestamp, base::TimeDelta());

  int num_cycles = floor(timestamp / duration_);
  base::TimeDelta cycle_start = num_cycles * duration_;
  int index = floor((timestamp - cycle_start) / bucket_duration_);
  if (index >= num_buckets_) {
    return num_buckets_ - 1;
  }
  DCHECK_GE(index, 0);
  return index;
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
