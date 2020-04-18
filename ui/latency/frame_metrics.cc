// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/latency/frame_metrics.h"

#include <cmath>
#include <limits>
#include <vector>

#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"

namespace ui {

namespace {

// How often to report results.
// This needs to be short enough to avoid overflow in the accumulators.
constexpr base::TimeDelta kDefaultReportPeriod =
    base::TimeDelta::FromMinutes(1);

// Gives the histogram for skips the highest precision just above a
// skipped:produced ratio of 1.
constexpr int64_t kFixedPointMultiplierSkips =
    frame_metrics::kFixedPointMultiplier;

// Gives latency a precision of 1 microsecond in both the histogram and
// the fixed point values.
constexpr int64_t kFixedPointMultiplierLatency = 1;

// This is used to weigh each latency sample by a constant value since
// we don't weigh it by the frame duration like other metrics.
// A larger weight improves precision in the fixed point accumulators, but we
// don't want to make it so big that it causes overflow before we start a new
// reporting period.
constexpr uint32_t kLatencySampleWeight = 1u << 10;
constexpr uint32_t kMaxFramesBeforeOverflowPossible =
    std::numeric_limits<uint32_t>::max() / kLatencySampleWeight;

// Gives the histogram for latency speed the highest precision just above a
// (latency delta : frame delta) ratio of 1.
constexpr int64_t kFixedPointMultiplierLatencySpeed =
    frame_metrics::kFixedPointMultiplier;

// Gives the histogram for latency acceleration the highest precision just
// above a (latency speed delta : frame delta) of 1/1024.
// A value ~1k was chosen since frame deltas are on the order of microseconds.
// Use 1024 instead of 1000 since powers of 2 let the compiler optimize integer
// multiplies with shifts if it wants.
// TODO(brianderson): Fine tune these values. http://crbug.com/837434
constexpr int64_t kFixedPointMultiplierLatencyAcceleration =
    frame_metrics::kFixedPointMultiplier * 1024;

// Converts a ratio to a fixed point value.
// Each threshold is offset by 0.5 to filter out jitter/inaccuracies.
constexpr uint32_t RatioThreshold(double fraction) {
  return static_cast<uint32_t>((fraction + 0.5) *
                               frame_metrics::kFixedPointMultiplier);
}

// Converts frequency as a floating point value into a fixed point value
// representing microseconds of latency.
// The result is scaled by 110% to allow for slack in cases the actual refresh
// period is slightly longer (common) or if there is some jitter in the
// timestamp sampling.
constexpr uint32_t LatencyThreshold(double Hz) {
  return static_cast<uint32_t>((1.1 / Hz) *
                               base::TimeTicks::kMicrosecondsPerSecond);
}

// The skip thresholds are selected to track each time more than 0, 1, 2, or 4
// frames were skipped at once.
constexpr std::initializer_list<uint32_t> kSkipThresholds = {
    RatioThreshold(0), RatioThreshold(1), RatioThreshold(2), RatioThreshold(4),
};

// The latency thresholds are selected based on common display frequencies.
// We often begin a frames on a vsync which would result in whole vsync periods
// of latency. However, in case begin frames are offset slightly from the vsync,
// which is common on Android, the frequency goes all the way to 240Hz.
constexpr std::initializer_list<uint32_t> kLatencyThresholds = {
    LatencyThreshold(240),  //  4.17 ms * 110% =  4.58 ms
    LatencyThreshold(120),  //  8.33 ms * 110% =  9.17 ms
    LatencyThreshold(60),   // 16.67 ms * 110% = 18.33 ms
    LatencyThreshold(30),   // 33.33 ms * 110% = 36.67 ms
};

// The latency speed thresholds are chosen to track each frame where the
// latency was constant (0) or when there was a jump of 1, 2, or 4 frame
// periods.
constexpr std::initializer_list<uint32_t> kLatencySpeedThresholds = {
    RatioThreshold(0), RatioThreshold(1), RatioThreshold(2), RatioThreshold(4),
};

// The latency acceleration thresholds here are tentative.
// TODO(brianderson): Fine tune these values. http://crbug.com/837434
constexpr std::initializer_list<uint32_t> kLatencyAccelerationThresholds = {
    RatioThreshold(0), RatioThreshold(1), RatioThreshold(2), RatioThreshold(4),
};

const char kTraceCategories[] = "gpu,benchmark";

// uint32_t should be plenty of range for real world values, but clip individual
// entries to make sure no single value dominates and also to avoid overflow
// in the accumulators and the fixed point math.
// This also makes sure overflowing values saturate instead of wrapping around
// and skewing our results.
// TODO(brianderson): Report warning if clipping occurred.
uint32_t CapValue(int64_t value) {
  return static_cast<uint32_t>(std::min<int64_t>(
      std::llabs(value), std::numeric_limits<uint32_t>::max()));
}

uint32_t CapDuration(const base::TimeDelta duration) {
  constexpr base::TimeDelta kDurationCap = base::TimeDelta::FromMinutes(1);
  return std::min(duration, kDurationCap).InMicroseconds();
}

}  // namespace

namespace frame_metrics {

// Converts result to fraction of frames skipped.
// The internal skip values are (skipped:produced). This transform converts
// the result to (skipped:total), which is:
//  a) Easier to interpret as a human, and
//  b) In the same units as latency speed, which may help us create a unified
//    smoothness metric in the future.
// The internal representation uses (skipped:produced) to:
//  a) Allow RMS, SMR, StdDev, etc to be performed on values that increase
//    linearly (rather than asymptotically to 1) with the amount of jank, and
//  b) Give us better precision where it's important when stored as a fixed
//    point number and in histogram buckets.
double SkipClient::TransformResult(double result) const {
  // Avoid divide by zero.
  if (result < 1e-32)
    return 0;
  return 1.0 / (1.0 + (kFixedPointMultiplierSkips / result));
}

// Converts result to seconds.
double LatencyClient::TransformResult(double result) const {
  return result / (base::TimeTicks::kMicrosecondsPerSecond *
                   kFixedPointMultiplierLatency);
}

// Converts result to s/s. ie: fraction of frames traveled.
double LatencySpeedClient::TransformResult(double result) const {
  return result / kFixedPointMultiplierLatencySpeed;
}

// Converts result to (s/s^2).
// ie: change in fraction of frames traveled per second.
double LatencyAccelerationClient::TransformResult(double result) const {
  return (result * base::TimeTicks::kMicrosecondsPerSecond) /
         kFixedPointMultiplierLatencyAcceleration;
}

}  // namespace frame_metrics

FrameMetrics::FrameMetrics(const FrameMetricsSettings& settings,
                           const char* source_name)
    : settings_(settings),
      source_name_(source_name),
      shared_skip_client_(settings_.max_window_size),
      shared_latency_client_(settings_.max_window_size),
      frame_skips_analyzer_(&skip_client_,
                            &shared_skip_client_,
                            kSkipThresholds,
                            std::make_unique<frame_metrics::RatioHistogram>()),
      latency_analyzer_(&latency_client_,
                        &shared_latency_client_,
                        kLatencyThresholds,
                        std::make_unique<frame_metrics::VSyncHistogram>()),
      latency_speed_analyzer_(
          &latency_speed_client_,
          &shared_latency_client_,
          kLatencySpeedThresholds,
          std::make_unique<frame_metrics::RatioHistogram>()),
      latency_acceleration_analyzer_(
          &latency_acceleration_client_,
          &shared_latency_client_,
          kLatencyAccelerationThresholds,
          std::make_unique<frame_metrics::RatioHistogram>()) {}

FrameMetrics::~FrameMetrics() = default;

base::TimeDelta FrameMetrics::ReportPeriod() {
  return kDefaultReportPeriod;
}

void FrameMetrics::AddFrameProduced(base::TimeTicks source_timestamp,
                                    base::TimeDelta amount_produced,
                                    base::TimeDelta amount_skipped) {
  DCHECK_GE(amount_skipped, base::TimeDelta());
  DCHECK_GT(amount_produced, base::TimeDelta());
  base::TimeDelta source_timestamp_delta;
  if (!skip_timestamp_queue_.empty()) {
    source_timestamp_delta = source_timestamp - skip_timestamp_queue_.back();
    DCHECK_GT(source_timestamp_delta, base::TimeDelta());
  }

  // Periodically report all metrics and reset the accumulators.
  // Do this before adding any samples to avoid overflow before it might happen.
  time_since_start_of_report_period_ += source_timestamp_delta;
  frames_produced_since_start_of_report_period_++;
  if (time_since_start_of_report_period_ > ReportPeriod() ||
      frames_produced_since_start_of_report_period_ >
          kMaxFramesBeforeOverflowPossible) {
    StartNewReportPeriod();
  }

  if (skip_timestamp_queue_.size() >= settings_.max_window_size) {
    skip_timestamp_queue_.pop_front();
  }
  skip_timestamp_queue_.push_back(source_timestamp);

  shared_skip_client_.window_begin = skip_timestamp_queue_.front();
  shared_skip_client_.window_end = source_timestamp;

  int64_t skipped_to_produced_ratio =
      (amount_skipped * kFixedPointMultiplierSkips) / amount_produced;
  DCHECK_GE(skipped_to_produced_ratio, 0);
  frame_skips_analyzer_.AddSample(CapValue(skipped_to_produced_ratio),
                                  CapDuration(amount_produced));

  bool tracing_enabled = 0;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(kTraceCategories, &tracing_enabled);
  if (tracing_enabled)
    TraceProducedStats();
}

void FrameMetrics::AddFrameDisplayed(base::TimeTicks source_timestamp,
                                     base::TimeTicks display_timestamp) {
  // Frame timestamps shouldn't go back in time, but check and drop them just
  // in case. Much of the code assumes a positive and non-zero delta.
  if (source_timestamp <= source_timestamp_prev_) {
    // TODO(brianderson): Flag a warning.
    return;
  }

  base::TimeDelta latency = display_timestamp - source_timestamp;

  if (latency_timestamp_queue_.size() >= settings_.max_window_size) {
    latency_timestamp_queue_.pop_front();
  }
  latency_timestamp_queue_.push_back(source_timestamp);

  shared_latency_client_.window_begin = latency_timestamp_queue_.front();
  shared_latency_client_.window_end = source_timestamp;

  // TODO(brianderson): Handle negative latency better.
  // For now, reporting the magnitude of the latency will reflect
  // how far off the ideal display time the frame was, but it won't indicate
  // in which direction. This might be important for sources like video, where
  // a frame might be displayed a little bit earlier than its ideal display
  // time.
  int64_t latency_value =
      latency.InMicroseconds() * kFixedPointMultiplierLatency;
  latency_analyzer_.AddSample(CapValue(latency_value), kLatencySampleWeight);

  // Only calculate velocity if there's enough history.
  if (latencies_added_ >= 1) {
    base::TimeDelta latency_delta = latency - latency_prev_;
    base::TimeDelta source_duration = source_timestamp - source_timestamp_prev_;
    int64_t latency_velocity =
        (latency_delta * kFixedPointMultiplierLatencySpeed) / source_duration;

    // This should be plenty of range for real world values, but clip
    // entries to avoid overflow in the accumulators just in case.
    latency_speed_analyzer_.AddSample(CapValue(latency_velocity),
                                      CapDuration(source_duration));

    // Only calculate acceleration if there's enough history.
    if (latencies_added_ >= 2) {
      base::TimeDelta source_duration_average =
          (source_duration + source_duration_prev_) / 2;
      int64_t latency_acceleration =
          (((latency_delta * kFixedPointMultiplierLatencyAcceleration) /
            source_duration) -
           ((latency_delta_prev_ * kFixedPointMultiplierLatencyAcceleration) /
            source_duration_prev_)) /
          source_duration_average.InMicroseconds();
      latency_acceleration_analyzer_.AddSample(
          CapValue(latency_acceleration), CapDuration(source_duration_average));
    }

    // Update history.
    source_duration_prev_ = source_duration;
    latency_delta_prev_ = latency_delta;
  }

  // Update history.
  source_timestamp_prev_ = source_timestamp;
  latency_prev_ = latency;
  latencies_added_++;

  bool tracing_enabled = 0;
  TRACE_EVENT_CATEGORY_GROUP_ENABLED(kTraceCategories, &tracing_enabled);
  if (tracing_enabled)
    TraceDisplayedStats();
}

void FrameMetrics::Reset() {
  TRACE_EVENT0(kTraceCategories, "FrameMetrics::Reset");

  skip_timestamp_queue_.clear();
  latency_timestamp_queue_.clear();

  time_since_start_of_report_period_ = base::TimeDelta();

  latencies_added_ = 0;
  source_timestamp_prev_ = base::TimeTicks();
  latency_prev_ = base::TimeDelta();
  source_duration_prev_ = base::TimeDelta();
  latency_delta_prev_ = base::TimeDelta();

  frame_skips_analyzer_.Reset();
  latency_analyzer_.Reset();
  latency_speed_analyzer_.Reset();
  latency_acceleration_analyzer_.Reset();
}

// Reset analyzers, but don't reset resent latency history so we can get
// latency speed and acceleration values immediately.
// TODO(brianderson): Once we support UKM reporting, store the frame skips
//   result and defer it's reporting until the latency numbers are also
//   available. Reporting everything at this point would put some frames in
//   different reporting periods, which could skew the results.
void FrameMetrics::StartNewReportPeriod() {
  TRACE_EVENT0(kTraceCategories, "FrameMetrics::StartNewReportPeriod");

  time_since_start_of_report_period_ = base::TimeDelta();
  frames_produced_since_start_of_report_period_ = 0;

  frame_skips_analyzer_.StartNewReportPeriod();
  latency_analyzer_.StartNewReportPeriod();
  latency_speed_analyzer_.StartNewReportPeriod();
  latency_acceleration_analyzer_.StartNewReportPeriod();
}

void FrameMetrics::TraceProducedStats() {
  TRACE_EVENT1(kTraceCategories, "FrameProduced", "Skips",
               frame_skips_analyzer_.AsValue());
}

void FrameMetrics::TraceDisplayedStats() {
  TRACE_EVENT0(kTraceCategories, "FrameDisplayed");
  TRACE_EVENT_INSTANT1(kTraceCategories, "FrameDisplayed",
                       TRACE_EVENT_SCOPE_THREAD, "Latency",
                       latency_analyzer_.AsValue());
  TRACE_EVENT_INSTANT1(kTraceCategories, "FrameDisplayed",
                       TRACE_EVENT_SCOPE_THREAD, "LatencySpeed",
                       latency_speed_analyzer_.AsValue());
  TRACE_EVENT_INSTANT1(kTraceCategories, "FrameDisplayed",
                       TRACE_EVENT_SCOPE_THREAD, "LatencyAcceleration",
                       latency_acceleration_analyzer_.AsValue());
}

}  // namespace ui
