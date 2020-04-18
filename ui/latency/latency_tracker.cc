// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/latency/latency_tracker.h"

#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/trace_event/trace_event.h"
#include "services/metrics/public/cpp/ukm_entry_builder.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "ui/latency/latency_histogram_macros.h"

// Impose some restrictions for tests etc, but also be lenient since some of the
// data come from untrusted sources.
#define DCHECK_AND_RETURN_ON_FAIL(x) \
  DCHECK(x);                         \
  if (!(x))                          \
    return;

namespace ui {
namespace {

std::string LatencySourceEventTypeToInputModalityString(
    ui::SourceEventType type) {
  switch (type) {
    case ui::SourceEventType::WHEEL:
      return "Wheel";
    case ui::SourceEventType::MOUSE:
      return "Mouse";
    case ui::SourceEventType::TOUCH:
    case ui::SourceEventType::INERTIAL:
      return "Touch";
    case ui::SourceEventType::KEY_PRESS:
      return "KeyPress";
    default:
      return "";
  }
}

bool IsInertialScroll(const LatencyInfo& latency) {
  return latency.source_event_type() == ui::SourceEventType::INERTIAL;
}

// This UMA metric tracks the time from when the original wheel event is created
// to when the scroll gesture results in final frame swap. All scroll events are
// included in this metric.
void RecordUmaEventLatencyScrollWheelTimeToScrollUpdateSwapBegin2Histogram(
    const ui::LatencyInfo::LatencyComponent& start,
    const ui::LatencyInfo::LatencyComponent& end) {
  CONFIRM_EVENT_TIMES_EXIST(start, end);
  UMA_HISTOGRAM_CUSTOM_COUNTS(
      "Event.Latency.Scroll.Wheel.TimeToScrollUpdateSwapBegin2",
      std::max(static_cast<int64_t>(0),
               (end.last_event_time - start.first_event_time).InMicroseconds()),
      1, 1000000, 100);
}

}  // namespace

LatencyTracker::LatencyTracker() = default;

void LatencyTracker::OnGpuSwapBuffersCompleted(
    const std::vector<ui::LatencyInfo>& latency_info) {
  for (const auto& latency : latency_info)
    OnGpuSwapBuffersCompleted(latency);
}

void LatencyTracker::OnGpuSwapBuffersCompleted(const LatencyInfo& latency) {
  LatencyInfo::LatencyComponent gpu_swap_end_component;
  if (!latency.FindLatency(
          ui::INPUT_EVENT_LATENCY_TERMINATED_FRAME_SWAP_COMPONENT, 0,
          &gpu_swap_end_component)) {
    return;
  }

  LatencyInfo::LatencyComponent gpu_swap_begin_component;
  bool found_component = latency.FindLatency(
      ui::INPUT_EVENT_GPU_SWAP_BUFFER_COMPONENT, 0, &gpu_swap_begin_component);
  DCHECK_AND_RETURN_ON_FAIL(found_component);

  LatencyInfo::LatencyComponent tab_switch_component;
  if (latency.FindLatency(ui::TAB_SHOW_COMPONENT, &tab_switch_component)) {
    base::TimeDelta delta =
        gpu_swap_end_component.event_time - tab_switch_component.event_time;
    for (size_t i = 0; i < tab_switch_component.event_count; i++) {
      UMA_HISTOGRAM_TIMES("MPArch.RWH_TabSwitchPaintDuration", delta);
      TRACE_EVENT_ASYNC_END0("latency", "TabSwitching::Latency",
                             latency.trace_id());
    }
  }

  if (!latency.FindLatency(ui::INPUT_EVENT_LATENCY_BEGIN_RWH_COMPONENT,
                           nullptr)) {
    return;
  }

  ui::SourceEventType source_event_type = latency.source_event_type();
  if (source_event_type == ui::SourceEventType::WHEEL ||
      source_event_type == ui::SourceEventType::MOUSE ||
      source_event_type == ui::SourceEventType::TOUCH ||
      source_event_type == ui::SourceEventType::INERTIAL ||
      source_event_type == ui::SourceEventType::KEY_PRESS) {
    ComputeEndToEndLatencyHistograms(gpu_swap_begin_component,
                                     gpu_swap_end_component, latency);
  }
}

void LatencyTracker::DisableMetricSamplingForTesting() {
  metric_sampling_ = false;
}

void LatencyTracker::ReportUkmScrollLatency(
    const InputMetricEvent& metric_event,
    const LatencyInfo::LatencyComponent& start_component,
    const LatencyInfo::LatencyComponent&
        time_to_scroll_update_swap_begin_component,
    const LatencyInfo::LatencyComponent& time_to_handled_component,
    bool is_main_thread,
    const ukm::SourceId ukm_source_id) {
  CONFIRM_EVENT_TIMES_EXIST(start_component,
                            time_to_scroll_update_swap_begin_component)
  CONFIRM_EVENT_TIMES_EXIST(start_component, time_to_handled_component)

  // Only report a subset of this metric as the volume is too high.
  if (metric_sampling_ &&
      !sampling_scheme_[static_cast<int>(metric_event)].ShouldReport())
    return;

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (ukm_source_id == ukm::kInvalidSourceId || !ukm_recorder)
    return;

  std::string event_name = "";
  switch (metric_event) {
    case InputMetricEvent::SCROLL_BEGIN_TOUCH:
      event_name = "Event.ScrollBegin.Touch";
      break;
    case InputMetricEvent::SCROLL_UPDATE_TOUCH:
      event_name = "Event.ScrollUpdate.Touch";
      break;
    case InputMetricEvent::SCROLL_BEGIN_WHEEL:
      event_name = "Event.ScrollBegin.Wheel";
      break;
    case InputMetricEvent::SCROLL_UPDATE_WHEEL:
      event_name = "Event.ScrollUpdate.Wheel";
      break;
  }

  ukm::UkmEntryBuilder builder(ukm_source_id, event_name.c_str());
  builder.SetMetric(
      "TimeToScrollUpdateSwapBegin",
      std::max(static_cast<int64_t>(0),
               (time_to_scroll_update_swap_begin_component.last_event_time -
                start_component.first_event_time)
                   .InMicroseconds()));
  builder.SetMetric("TimeToHandled",
                    std::max(static_cast<int64_t>(0),
                             (time_to_handled_component.last_event_time -
                              start_component.first_event_time)
                                 .InMicroseconds()));
  builder.SetMetric("IsMainThread", is_main_thread);
  builder.Record(ukm_recorder);
}

void LatencyTracker::ComputeEndToEndLatencyHistograms(
    const ui::LatencyInfo::LatencyComponent& gpu_swap_begin_component,
    const ui::LatencyInfo::LatencyComponent& gpu_swap_end_component,
    const ui::LatencyInfo& latency) {
  DCHECK_AND_RETURN_ON_FAIL(!latency.coalesced());

  LatencyInfo::LatencyComponent original_component;
  std::string scroll_name = "Uninitialized";

  const std::string input_modality =
      LatencySourceEventTypeToInputModalityString(latency.source_event_type());

  if (latency.FindLatency(
          ui::INPUT_EVENT_LATENCY_FIRST_SCROLL_UPDATE_ORIGINAL_COMPONENT,
          &original_component)) {
    DCHECK(input_modality == "Wheel" || input_modality == "Touch");

    // For inertial scrolling we don't separate the first event from the rest of
    // them.
    scroll_name = IsInertialScroll(latency) ? "ScrollInertial" : "ScrollBegin";

    // This UMA metric tracks the performance of overall scrolling as a high
    // level metric.
    UMA_HISTOGRAM_INPUT_LATENCY_5_SECONDS_MAX_MICROSECONDS(
        "Event.Latency.ScrollBegin.TimeToScrollUpdateSwapBegin2",
        original_component, gpu_swap_begin_component);

    // This UMA metric tracks the time between the final frame swap for the
    // first scroll event in a sequence and the original timestamp of that
    // scroll event's underlying touch/wheel event.
    UMA_HISTOGRAM_INPUT_LATENCY_5_SECONDS_MAX_MICROSECONDS(
        "Event.Latency." + scroll_name + "." + input_modality +
            ".TimeToScrollUpdateSwapBegin4",
        original_component, gpu_swap_begin_component);

    // This is the same metric as above. But due to a change in rebucketing,
    // UMA pipeline cannot process this for the chirp alerts. Hence adding a
    // newer version the this metric above. TODO(nzolghadr): Remove it in a
    // future milesone like M70.
    UMA_HISTOGRAM_INPUT_LATENCY_HIGH_RESOLUTION_MICROSECONDS(
        "Event.Latency." + scroll_name + "." + input_modality +
            ".TimeToScrollUpdateSwapBegin2",
        original_component, gpu_swap_begin_component);

    if (input_modality == "Wheel") {
      RecordUmaEventLatencyScrollWheelTimeToScrollUpdateSwapBegin2Histogram(
          original_component, gpu_swap_begin_component);
    }

  } else if (latency.FindLatency(
                 ui::INPUT_EVENT_LATENCY_SCROLL_UPDATE_ORIGINAL_COMPONENT,
                 &original_component)) {
    DCHECK(input_modality == "Wheel" || input_modality == "Touch");

    // For inertial scrolling we don't separate the first event from the rest of
    // them.
    scroll_name = IsInertialScroll(latency) ? "ScrollInertial" : "ScrollUpdate";

    // This UMA metric tracks the performance of overall scrolling as a high
    // level metric.
    UMA_HISTOGRAM_INPUT_LATENCY_5_SECONDS_MAX_MICROSECONDS(
        "Event.Latency.ScrollUpdate.TimeToScrollUpdateSwapBegin2",
        original_component, gpu_swap_begin_component);

    // This UMA metric tracks the time from when the original touch/wheel event
    // is created to when the scroll gesture results in final frame swap.
    // First scroll events are excluded from this metric.
    UMA_HISTOGRAM_INPUT_LATENCY_5_SECONDS_MAX_MICROSECONDS(
        "Event.Latency." + scroll_name + "." + input_modality +
            ".TimeToScrollUpdateSwapBegin4",
        original_component, gpu_swap_begin_component);

    // This is the same metric as above. But due to a change in rebucketing,
    // UMA pipeline cannot process this for the chirp alerts. Hence adding a
    // newer version the this metric above. TODO(nzolghadr): Remove it in a
    // future milesone like M70.
    UMA_HISTOGRAM_INPUT_LATENCY_HIGH_RESOLUTION_MICROSECONDS(
        "Event.Latency." + scroll_name + "." + input_modality +
            ".TimeToScrollUpdateSwapBegin2",
        original_component, gpu_swap_begin_component);

    if (input_modality == "Wheel") {
      RecordUmaEventLatencyScrollWheelTimeToScrollUpdateSwapBegin2Histogram(
          original_component, gpu_swap_begin_component);
    }

  } else if (latency.FindLatency(ui::INPUT_EVENT_LATENCY_ORIGINAL_COMPONENT, 0,
                                 &original_component)) {
    if (latency.source_event_type() == SourceEventType::KEY_PRESS) {
      UMA_HISTOGRAM_INPUT_LATENCY_HIGH_RESOLUTION_MICROSECONDS(
          "Event.Latency.EndToEnd.KeyPress", original_component,
          gpu_swap_begin_component);
    } else if (latency.source_event_type() == SourceEventType::MOUSE) {
      UMA_HISTOGRAM_INPUT_LATENCY_HIGH_RESOLUTION_MICROSECONDS(
          "Event.Latency.EndToEnd.Mouse", original_component,
          gpu_swap_begin_component);
    }
    return;
  } else {
    // No original component found.
    return;
  }

  // Record scroll latency metrics.
  DCHECK(scroll_name == "ScrollBegin" || scroll_name == "ScrollUpdate" ||
         (IsInertialScroll(latency) && scroll_name == "ScrollInertial"));
  LatencyInfo::LatencyComponent rendering_scheduled_component;
  bool rendering_scheduled_on_main = latency.FindLatency(
      ui::INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_MAIN_COMPONENT, 0,
      &rendering_scheduled_component);
  if (!rendering_scheduled_on_main) {
    bool found_component = latency.FindLatency(
        ui::INPUT_EVENT_LATENCY_RENDERING_SCHEDULED_IMPL_COMPONENT, 0,
        &rendering_scheduled_component);
    DCHECK_AND_RETURN_ON_FAIL(found_component);
  }

  // Inertial scrolls are excluded from Ukm metrics.
  if ((input_modality == "Touch" && !IsInertialScroll(latency)) ||
      input_modality == "Wheel") {
    InputMetricEvent input_metric_event;
    if (scroll_name == "ScrollBegin") {
      input_metric_event = input_modality == "Touch"
                               ? InputMetricEvent::SCROLL_BEGIN_TOUCH
                               : InputMetricEvent::SCROLL_BEGIN_WHEEL;
    } else {
      DCHECK_EQ(scroll_name, "ScrollUpdate");
      input_metric_event = input_modality == "Touch"
                               ? InputMetricEvent::SCROLL_UPDATE_TOUCH
                               : InputMetricEvent::SCROLL_UPDATE_WHEEL;
    }
    ReportUkmScrollLatency(
        input_metric_event, original_component, gpu_swap_begin_component,
        rendering_scheduled_component, rendering_scheduled_on_main,
        latency.ukm_source_id());
  }

  const std::string thread_name = rendering_scheduled_on_main ? "Main" : "Impl";

  UMA_HISTOGRAM_SCROLL_LATENCY_LONG_2(
      "Event.Latency." + scroll_name + "." + input_modality +
          ".TimeToHandled2_" + thread_name,
      original_component, rendering_scheduled_component);

  if (input_modality == "Wheel") {
    UMA_HISTOGRAM_SCROLL_LATENCY_LONG_2(
        "Event.Latency.Scroll.Wheel.TimeToHandled2_" + thread_name,
        original_component, rendering_scheduled_component);
  }

  LatencyInfo::LatencyComponent renderer_swap_component;
  bool found_component =
      latency.FindLatency(ui::INPUT_EVENT_LATENCY_RENDERER_SWAP_COMPONENT, 0,
                          &renderer_swap_component);
  DCHECK_AND_RETURN_ON_FAIL(found_component);

  UMA_HISTOGRAM_SCROLL_LATENCY_LONG_2(
      "Event.Latency." + scroll_name + "." + input_modality +
          ".HandledToRendererSwap2_" + thread_name,
      rendering_scheduled_component, renderer_swap_component);

  LatencyInfo::LatencyComponent browser_received_swap_component;
  found_component =
      latency.FindLatency(ui::DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT, 0,
                          &browser_received_swap_component);
  DCHECK_AND_RETURN_ON_FAIL(found_component);

  UMA_HISTOGRAM_SCROLL_LATENCY_SHORT_2(
      "Event.Latency." + scroll_name + "." + input_modality +
          ".RendererSwapToBrowserNotified2",
      renderer_swap_component, browser_received_swap_component);

  UMA_HISTOGRAM_SCROLL_LATENCY_LONG_2(
      "Event.Latency." + scroll_name + "." + input_modality +
          ".BrowserNotifiedToBeforeGpuSwap2",
      browser_received_swap_component, gpu_swap_begin_component);

  UMA_HISTOGRAM_SCROLL_LATENCY_SHORT_2(
      "Event.Latency." + scroll_name + "." + input_modality + ".GpuSwap2",
      gpu_swap_begin_component, gpu_swap_end_component);
}

}  // namespace ui
