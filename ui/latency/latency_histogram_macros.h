// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_LATENCY_LATENCY_HISTOGRAM_MACROS_H_
#define UI_LATENCY_LATENCY_HISTOGRAM_MACROS_H_

#include "base/metrics/histogram_functions.h"

// Check valid timing for start and end latency components.
#define CONFIRM_EVENT_TIMES_EXIST(start, end) \
  DCHECK(!start.first_event_time.is_null());  \
  DCHECK(!end.last_event_time.is_null());

// Event latency that is mostly under 5 seconds. We should only use 100 buckets
// when needed.
#define UMA_HISTOGRAM_INPUT_LATENCY_5_SECONDS_MAX_MICROSECONDS(name, start, \
                                                               end)         \
  CONFIRM_EVENT_TIMES_EXIST(start, end)                                     \
  base::UmaHistogramCustomCounts(                                           \
      name,                                                                 \
      std::max(                                                             \
          static_cast<int64_t>(0),                                          \
          (end.last_event_time - start.first_event_time).InMicroseconds()), \
      1, 5000000, 100);

// Event latency that is mostly under 1 second. We should only use 100 buckets
// when needed.
#define UMA_HISTOGRAM_INPUT_LATENCY_HIGH_RESOLUTION_MICROSECONDS(name, start, \
                                                                 end)         \
  CONFIRM_EVENT_TIMES_EXIST(start, end)                                       \
  base::UmaHistogramCustomCounts(                                             \
      name,                                                                   \
      std::max(                                                               \
          static_cast<int64_t>(0),                                            \
          (end.last_event_time - start.first_event_time).InMicroseconds()),   \
      1, 1000000, 100);

#define UMA_HISTOGRAM_INPUT_LATENCY_MILLISECONDS(name, start, end)          \
  CONFIRM_EVENT_TIMES_EXIST(start, end)                                     \
  base::UmaHistogramCustomCounts(                                           \
      name,                                                                 \
      std::max(                                                             \
          static_cast<int64_t>(0),                                          \
          (end.last_event_time - start.first_event_time).InMilliseconds()), \
      1, 1000, 50);

// Long touch/wheel scroll latency component that is mostly under 200ms.
#define UMA_HISTOGRAM_SCROLL_LATENCY_LONG_2(name, start, end)                 \
  CONFIRM_EVENT_TIMES_EXIST(start, end)                                       \
  base::Histogram::FactoryGet(name, 1000, 200000, 50,                         \
                              base::HistogramBase::kUmaTargetedHistogramFlag) \
      ->Add(std::max(                                                         \
          static_cast<int64_t>(0),                                            \
          (end.last_event_time - start.first_event_time).InMicroseconds()));

// Short touch/wheel scroll latency component that is mostly under 50ms.
#define UMA_HISTOGRAM_SCROLL_LATENCY_SHORT_2(name, start, end)                \
  CONFIRM_EVENT_TIMES_EXIST(start, end)                                       \
  base::Histogram::FactoryGet(name, 1, 50000, 50,                             \
                              base::HistogramBase::kUmaTargetedHistogramFlag) \
      ->Add(std::max(                                                         \
          static_cast<int64_t>(0),                                            \
          (end.last_event_time - start.first_event_time).InMicroseconds()));

#endif  // UI_LATENCY_LATENCY_HISTOGRAM_MACROS_H_
