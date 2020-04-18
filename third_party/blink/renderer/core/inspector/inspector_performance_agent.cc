// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/inspector/inspector_performance_agent.h"

#include <utility>

#include "base/time/time_override.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/inspected_frames.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/paint/paint_timing.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/bindings/v8_per_isolate_data.h"
#include "third_party/blink/renderer/platform/instance_counters.h"
#include "third_party/blink/renderer/platform/wtf/dtoa/utils.h"

namespace blink {

using protocol::Response;

namespace {

static const char kPerformanceAgentEnabled[] = "PerformanceAgentEnabled";

constexpr bool isPlural(const char* str, int len) {
  return len > 1 && str[len - 2] == 's';
}

static constexpr const char* kInstanceCounterNames[] = {
#define INSTANCE_COUNTER_NAME(name) \
  (isPlural(#name, sizeof(#name)) ? #name : #name "s"),
    INSTANCE_COUNTERS_LIST(INSTANCE_COUNTER_NAME)
#undef INSTANCE_COUNTER_NAME
};

}  // namespace

InspectorPerformanceAgent::InspectorPerformanceAgent(
    InspectedFrames* inspected_frames)
    : inspected_frames_(inspected_frames) {}

InspectorPerformanceAgent::~InspectorPerformanceAgent() = default;

void InspectorPerformanceAgent::Restore() {
  if (state_->booleanProperty(kPerformanceAgentEnabled, false))
    enable();
}

protocol::Response InspectorPerformanceAgent::enable() {
  if (enabled_)
    return Response::OK();
  enabled_ = true;
  state_->setBoolean(kPerformanceAgentEnabled, true);
  instrumenting_agents_->addInspectorPerformanceAgent(this);
  Platform::Current()->CurrentThread()->AddTaskTimeObserver(this);
  layout_start_ticks_ = TimeTicks();
  recalc_style_start_ticks_ = TimeTicks();
  task_start_ticks_ = TimeTicks();
  script_start_ticks_ = TimeTicks();
  return Response::OK();
}

protocol::Response InspectorPerformanceAgent::disable() {
  if (!enabled_)
    return Response::OK();
  enabled_ = false;
  state_->setBoolean(kPerformanceAgentEnabled, false);
  instrumenting_agents_->removeInspectorPerformanceAgent(this);
  Platform::Current()->CurrentThread()->RemoveTaskTimeObserver(this);
  return Response::OK();
}

namespace {
void AppendMetric(protocol::Array<protocol::Performance::Metric>* container,
                  const String& name,
                  double value) {
  container->addItem(protocol::Performance::Metric::create()
                         .setName(name)
                         .setValue(value)
                         .build());
}
}  // namespace

Response InspectorPerformanceAgent::getMetrics(
    std::unique_ptr<protocol::Array<protocol::Performance::Metric>>*
        out_result) {
  if (!enabled_) {
    *out_result = protocol::Array<protocol::Performance::Metric>::create();
    return Response::OK();
  }

  std::unique_ptr<protocol::Array<protocol::Performance::Metric>> result =
      protocol::Array<protocol::Performance::Metric>::create();

  AppendMetric(result.get(), "Timestamp",
               TimeTicksInSeconds(CurrentTimeTicks()));

  // Renderer instance counters.
  for (size_t i = 0; i < ARRAY_SIZE(kInstanceCounterNames); ++i) {
    AppendMetric(result.get(), kInstanceCounterNames[i],
                 InstanceCounters::CounterValue(
                     static_cast<InstanceCounters::CounterType>(i)));
  }

  // Page performance metrics.
  TimeTicks now = base::subtle::TimeTicksNowIgnoringOverride();
  AppendMetric(result.get(), "LayoutCount", static_cast<double>(layout_count_));
  AppendMetric(result.get(), "RecalcStyleCount",
               static_cast<double>(recalc_style_count_));
  AppendMetric(result.get(), "LayoutDuration", layout_duration_.InSecondsF());
  AppendMetric(result.get(), "RecalcStyleDuration",
               recalc_style_duration_.InSecondsF());
  TimeDelta script_duration = script_duration_;
  if (!script_start_ticks_.is_null())
    script_duration += now - script_start_ticks_;
  AppendMetric(result.get(), "ScriptDuration", script_duration.InSecondsF());
  TimeDelta task_duration = task_duration_;
  if (!task_start_ticks_.is_null())
    task_duration += now - task_start_ticks_;
  AppendMetric(result.get(), "TaskDuration", task_duration.InSecondsF());

  v8::HeapStatistics heap_statistics;
  V8PerIsolateData::MainThreadIsolate()->GetHeapStatistics(&heap_statistics);
  AppendMetric(result.get(), "JSHeapUsedSize",
               heap_statistics.used_heap_size());
  AppendMetric(result.get(), "JSHeapTotalSize",
               heap_statistics.total_heap_size());

  // Performance timings.
  Document* document = inspected_frames_->Root()->GetDocument();
  if (document) {
    AppendMetric(result.get(), "FirstMeaningfulPaint",
                 TimeTicksInSeconds(
                     PaintTiming::From(*document).FirstMeaningfulPaint()));
    AppendMetric(
        result.get(), "DomContentLoaded",
        TimeTicksInSeconds(document->GetTiming().DomContentLoadedEventStart()));
    AppendMetric(
        result.get(), "NavigationStart",
        TimeTicksInSeconds(document->Loader()->GetTiming().NavigationStart()));
  }

  *out_result = std::move(result);
  return Response::OK();
}

void InspectorPerformanceAgent::ConsoleTimeStamp(const String& title) {
  if (!enabled_)
    return;
  std::unique_ptr<protocol::Array<protocol::Performance::Metric>> metrics;
  getMetrics(&metrics);
  GetFrontend()->metrics(std::move(metrics), title);
}

void InspectorPerformanceAgent::ScriptStarts() {
  if (!script_call_depth_++)
    script_start_ticks_ = base::subtle::TimeTicksNowIgnoringOverride();
}

void InspectorPerformanceAgent::ScriptEnds() {
  if (--script_call_depth_)
    return;
  script_duration_ +=
      base::subtle::TimeTicksNowIgnoringOverride() - script_start_ticks_;
  script_start_ticks_ = TimeTicks();
}

void InspectorPerformanceAgent::Will(const probe::CallFunction& probe) {
  ScriptStarts();
}

void InspectorPerformanceAgent::Did(const probe::CallFunction& probe) {
  ScriptEnds();
}

void InspectorPerformanceAgent::Will(const probe::ExecuteScript& probe) {
  ScriptStarts();
}

void InspectorPerformanceAgent::Did(const probe::ExecuteScript& probe) {
  ScriptEnds();
}

void InspectorPerformanceAgent::Will(const probe::RecalculateStyle& probe) {
  recalc_style_start_ticks_ = base::subtle::TimeTicksNowIgnoringOverride();
}

void InspectorPerformanceAgent::Did(const probe::RecalculateStyle& probe) {
  recalc_style_duration_ +=
      base::subtle::TimeTicksNowIgnoringOverride() - recalc_style_start_ticks_;
  recalc_style_count_++;
}

void InspectorPerformanceAgent::Will(const probe::UpdateLayout& probe) {
  if (!layout_depth_++)
    layout_start_ticks_ = base::subtle::TimeTicksNowIgnoringOverride();
}

void InspectorPerformanceAgent::Did(const probe::UpdateLayout& probe) {
  if (--layout_depth_)
    return;
  layout_duration_ +=
      base::subtle::TimeTicksNowIgnoringOverride() - layout_start_ticks_;
  layout_count_++;
}

void InspectorPerformanceAgent::WillProcessTask(double start_time) {
  task_start_ticks_ = TimeTicksFromSeconds(start_time);
}

void InspectorPerformanceAgent::DidProcessTask(double start_time,
                                               double end_time) {
  if (task_start_ticks_ == TimeTicksFromSeconds(start_time))
    task_duration_ += TimeDelta::FromSecondsD(end_time - start_time);
  task_start_ticks_ = TimeTicks();
}

void InspectorPerformanceAgent::Trace(blink::Visitor* visitor) {
  visitor->Trace(inspected_frames_);
  InspectorBaseAgent<protocol::Performance::Metainfo>::Trace(visitor);
}

}  // namespace blink
