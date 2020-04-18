// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_LATENCY_LATENCY_TRACKER_H_
#define UI_LATENCY_LATENCY_TRACKER_H_

#include "base/macros.h"
#include "ui/latency/latency_info.h"

namespace ui {

// Utility class for tracking the latency of events. Relies on LatencyInfo
// components logged by content::RenderWidgetHostLatencyTracker.
class LatencyTracker {
 public:
  LatencyTracker();
  ~LatencyTracker() = default;

  // Terminates latency tracking for events that triggered rendering, also
  // performing relevant UMA latency reporting.
  // Called when GPU buffers swap completes.
  void OnGpuSwapBuffersCompleted(const std::vector<LatencyInfo>& latency_info);
  void OnGpuSwapBuffersCompleted(const LatencyInfo& latency);

  // Disables sampling of high volume metrics in unit tests.
  void DisableMetricSamplingForTesting();

 private:
  enum class InputMetricEvent {
    SCROLL_BEGIN_TOUCH = 0,
    SCROLL_UPDATE_TOUCH,
    SCROLL_BEGIN_WHEEL,
    SCROLL_UPDATE_WHEEL,

    INPUT_METRIC_EVENT_MAX = SCROLL_UPDATE_WHEEL
  };

  void ReportUkmScrollLatency(
      const InputMetricEvent& metric_event,
      const LatencyInfo::LatencyComponent& start_component,
      const LatencyInfo::LatencyComponent&
          time_to_scroll_update_swap_begin_component,
      const LatencyInfo::LatencyComponent& time_to_handled_component,
      bool is_main_thread,
      const ukm::SourceId ukm_source_id);

  void ComputeEndToEndLatencyHistograms(
      const LatencyInfo::LatencyComponent& gpu_swap_begin_component,
      const LatencyInfo::LatencyComponent& gpu_swap_end_component,
      const LatencyInfo& latency);

  typedef struct SamplingScheme {
    SamplingScheme() : interval_(1), last_sample_(0) {}
    SamplingScheme(int interval)
        : interval_(interval), last_sample_(rand() % interval) {}
    bool ShouldReport() {
      last_sample_++;
      last_sample_ %= interval_;
      return last_sample_ == 0;
    }

   private:
    int interval_;
    int last_sample_;
  } SamplingScheme;

  // Whether the sampling is needed for high volume metrics. This will be off
  // when we are in unit tests. This is a temporary field so we can come up with
  // a more permanent solution for crbug.com/739169.
  bool metric_sampling_ = true;

  // The i'th member of this array stores the sampling rate for the i'th
  // input metric event type. Initializing SamplingScheme with number X means
  // that from every X events one will be reported. Note that the first event
  // to report is also randomized.
  SamplingScheme sampling_scheme_
      [static_cast<int>(InputMetricEvent::INPUT_METRIC_EVENT_MAX) + 1] = {
          SamplingScheme(5),   // SCROLL_BEGIN_TOUCH
          SamplingScheme(50),  // SCROLL_UPDATE_TOUCH
          SamplingScheme(5),   // SCROLL_BEGIN_WHEEL
          SamplingScheme(2),   // SCROLL_UPDATE_WHEEL
  };

  DISALLOW_COPY_AND_ASSIGN(LatencyTracker);
};

}  // namespace latency

#endif  // UI_LATENCY_LATENCY_TRACKER_H_
