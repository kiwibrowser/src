// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/performance_monitor.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/scheduled_action.h"
#include "third_party/blink/renderer/bindings/core/v8/script_event_listener.h"
#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/core/CoreProbeSink.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event_listener.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/frame.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/parser/html_document_parser.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {
constexpr auto kLongTaskSubTaskThreshold = TimeDelta::FromMilliseconds(12);
}  // namespace

void PerformanceMonitor::BypassLongCompileThresholdOnceForTesting() {
  bypass_long_compile_threshold_ = true;
};

// static
double PerformanceMonitor::Threshold(ExecutionContext* context,
                                     Violation violation) {
  PerformanceMonitor* monitor =
      PerformanceMonitor::InstrumentingMonitor(context);
  return monitor ? monitor->thresholds_[violation] : 0;
}

// static
void PerformanceMonitor::ReportGenericViolation(
    ExecutionContext* context,
    Violation violation,
    const String& text,
    double time,
    std::unique_ptr<SourceLocation> location) {
  PerformanceMonitor* monitor =
      PerformanceMonitor::InstrumentingMonitor(context);
  if (!monitor)
    return;
  monitor->InnerReportGenericViolation(context, violation, text, time,
                                       std::move(location));
}

// static
PerformanceMonitor* PerformanceMonitor::Monitor(
    const ExecutionContext* context) {
  if (!context || !context->IsDocument())
    return nullptr;
  LocalFrame* frame = ToDocument(context)->GetFrame();
  if (!frame)
    return nullptr;
  return frame->GetPerformanceMonitor();
}

// static
PerformanceMonitor* PerformanceMonitor::InstrumentingMonitor(
    const ExecutionContext* context) {
  PerformanceMonitor* monitor = PerformanceMonitor::Monitor(context);
  return monitor && monitor->enabled_ ? monitor : nullptr;
}

PerformanceMonitor::PerformanceMonitor(LocalFrame* local_root)
    : local_root_(local_root) {
  std::fill(std::begin(thresholds_), std::end(thresholds_), 0);
  Platform::Current()->CurrentThread()->AddTaskTimeObserver(this);
  local_root_->GetProbeSink()->addPerformanceMonitor(this);
}

PerformanceMonitor::~PerformanceMonitor() {
  DCHECK(!local_root_);
}

void PerformanceMonitor::Subscribe(Violation violation,
                                   double threshold,
                                   Client* client) {
  DCHECK(violation < kAfterLast);
  ClientThresholds* client_thresholds = subscriptions_.at(violation);
  if (!client_thresholds) {
    client_thresholds = new ClientThresholds();
    subscriptions_.Set(violation, client_thresholds);
  }
  client_thresholds->Set(client, threshold);
  UpdateInstrumentation();
}

void PerformanceMonitor::UnsubscribeAll(Client* client) {
  for (const auto& it : subscriptions_)
    it.value->erase(client);
  UpdateInstrumentation();
}

void PerformanceMonitor::Shutdown() {
  if (!local_root_)
    return;
  subscriptions_.clear();
  UpdateInstrumentation();
  Platform::Current()->CurrentThread()->RemoveTaskTimeObserver(this);
  local_root_->GetProbeSink()->removePerformanceMonitor(this);
  local_root_ = nullptr;
}

void PerformanceMonitor::UpdateInstrumentation() {
  std::fill(std::begin(thresholds_), std::end(thresholds_), 0);

  for (const auto& it : subscriptions_) {
    Violation violation = static_cast<Violation>(it.key);
    ClientThresholds* client_thresholds = it.value;
    for (const auto& client_threshold : *client_thresholds) {
      if (!thresholds_[violation] ||
          thresholds_[violation] > client_threshold.value)
        thresholds_[violation] = client_threshold.value;
    }
  }

  enabled_ = std::count(std::begin(thresholds_), std::end(thresholds_), 0) <
             static_cast<int>(kAfterLast);
}

void PerformanceMonitor::WillExecuteScript(ExecutionContext* context) {
  // Heuristic for minimal frame context attribution: note the frame context
  // for each script execution. When a long task is encountered,
  // if there is only one frame context involved, then report it.
  // Otherwise don't report frame context.
  // NOTE: This heuristic is imperfect and will be improved in V2 API.
  // In V2, timing of script execution along with style & layout updates will be
  // accounted for detailed and more accurate attribution.
  ++script_depth_;
  UpdateTaskAttribution(context);
}

void PerformanceMonitor::DidExecuteScript() {
  --script_depth_;
}

void PerformanceMonitor::UpdateTaskAttribution(ExecutionContext* context) {
  // If |context| is not a document, unable to attribute a frame context.
  if (!context || !context->IsDocument())
    return;

  UpdateTaskShouldBeReported(ToDocument(context)->GetFrame());
  if (!task_execution_context_)
    task_execution_context_ = context;
  else if (task_execution_context_ != context)
    task_has_multiple_contexts_ = true;
}

void PerformanceMonitor::UpdateTaskShouldBeReported(LocalFrame* frame) {
  if (frame && local_root_ == &(frame->LocalFrameRoot()))
    task_should_be_reported_ = true;
}

void PerformanceMonitor::Will(const probe::RecalculateStyle& probe) {
  UpdateTaskShouldBeReported(probe.document ? probe.document->GetFrame()
                                            : nullptr);
  if (enabled_ && thresholds_[kLongLayout] && script_depth_)
    probe.CaptureStartTime();
}

void PerformanceMonitor::Did(const probe::RecalculateStyle& probe) {
  if (enabled_ && script_depth_ && thresholds_[kLongLayout])
    per_task_style_and_layout_time_ += probe.Duration();
}

void PerformanceMonitor::Will(const probe::UpdateLayout& probe) {
  UpdateTaskShouldBeReported(probe.document ? probe.document->GetFrame()
                                            : nullptr);
  ++layout_depth_;
  if (!enabled_)
    return;
  if (layout_depth_ > 1 || !script_depth_ || !thresholds_[kLongLayout])
    return;

  probe.CaptureStartTime();
}

void PerformanceMonitor::Did(const probe::UpdateLayout& probe) {
  --layout_depth_;
  if (!enabled_)
    return;
  if (thresholds_[kLongLayout] && script_depth_ && !layout_depth_)
    per_task_style_and_layout_time_ += probe.Duration();
}

void PerformanceMonitor::Will(const probe::ExecuteScript& probe) {
  WillExecuteScript(probe.context);

  probe.CaptureStartTime();
}

void PerformanceMonitor::Did(const probe::ExecuteScript& probe) {
  DidExecuteScript();

  if (!enabled_ || !thresholds_[kLongTask])
    return;

  if (probe.Duration() <= kLongTaskSubTaskThreshold)
    return;
  std::unique_ptr<SubTaskAttribution> sub_task_attribution =
      SubTaskAttribution::Create(String("script-run"),
                                 probe.context->Url().GetString(),
                                 probe.CaptureStartTime(), probe.Duration());
  sub_task_attributions_.push_back(std::move(sub_task_attribution));
}

void PerformanceMonitor::Will(const probe::CallFunction& probe) {
  WillExecuteScript(probe.context);
  if (user_callback_)
    probe.CaptureStartTime();
}

void PerformanceMonitor::Did(const probe::CallFunction& probe) {
  DidExecuteScript();
  if (!enabled_ || !user_callback_)
    return;

  // Working around Oilpan - probes are STACK_ALLOCATED.
  const probe::UserCallback* user_callback =
      static_cast<const probe::UserCallback*>(user_callback_);
  Violation handler_type =
      user_callback->recurring ? kRecurringHandler : kHandler;
  double threshold = thresholds_[handler_type];
  double duration = probe.Duration().InSecondsF();
  if (!threshold || duration < threshold)
    return;

  String name = user_callback->name ? String(user_callback->name)
                                    : String(user_callback->atomicName);
  String text = String::Format("'%s' handler took %ldms", name.Utf8().data(),
                               lround(duration * 1000));
  InnerReportGenericViolation(probe.context, handler_type, text, duration,
                              SourceLocation::FromFunction(probe.function));
}

void PerformanceMonitor::Will(const probe::V8Compile& probe) {
  UpdateTaskAttribution(probe.context);
  if (!enabled_ || !thresholds_[kLongTask])
    return;

  v8_compile_start_time_ = probe.CaptureStartTime();
}

void PerformanceMonitor::Did(const probe::V8Compile& probe) {
  if (!enabled_ || !thresholds_[kLongTask])
    return;

  TimeDelta v8_compile_duration = probe.Duration();

  if (bypass_long_compile_threshold_) {
    bypass_long_compile_threshold_ = false;
  } else {
    if (v8_compile_duration <= kLongTaskSubTaskThreshold)
      return;
  }

  std::unique_ptr<SubTaskAttribution> sub_task_attribution =
      SubTaskAttribution::Create(
          String("script-compile"),
          String::Format("%s(%d, %d)", probe.file_name.Utf8().data(),
                         probe.line, probe.column),
          v8_compile_start_time_, v8_compile_duration);
  sub_task_attributions_.push_back(std::move(sub_task_attribution));
}

void PerformanceMonitor::Will(const probe::UserCallback& probe) {
  ++user_callback_depth_;
  UpdateTaskAttribution(probe.context);
  if (!enabled_ || user_callback_depth_ != 1 ||
      !thresholds_[probe.recurring ? kRecurringHandler : kHandler])
    return;

  DCHECK(!user_callback_);
  user_callback_ = &probe;
}

void PerformanceMonitor::Did(const probe::UserCallback& probe) {
  --user_callback_depth_;
  if (!user_callback_depth_)
    user_callback_ = nullptr;
  DCHECK(user_callback_ != &probe);
}

void PerformanceMonitor::DocumentWriteFetchScript(Document* document) {
  if (!enabled_)
    return;
  String text = "Parser was blocked due to document.write(<script>)";
  InnerReportGenericViolation(document, kBlockedParser, text, 0, nullptr);
}

void PerformanceMonitor::WillProcessTask(double start_time) {
  // Reset m_taskExecutionContext. We don't clear this in didProcessTask
  // as it is needed in ReportTaskTime which occurs after didProcessTask.
  task_execution_context_ = nullptr;
  task_has_multiple_contexts_ = false;
  task_should_be_reported_ = false;

  if (!enabled_)
    return;

  // Reset everything for regular and nested tasks.
  script_depth_ = 0;
  layout_depth_ = 0;
  per_task_style_and_layout_time_ = TimeDelta();
  user_callback_ = nullptr;
  v8_compile_start_time_ = TimeTicks();
  sub_task_attributions_.clear();
}

void PerformanceMonitor::DidProcessTask(double start_time, double end_time) {
  if (!enabled_ || !task_should_be_reported_)
    return;
  double layout_threshold = thresholds_[kLongLayout];
  double layout_time = per_task_style_and_layout_time_.InSecondsF();
  if (layout_threshold && layout_time > layout_threshold) {
    ClientThresholds* client_thresholds = subscriptions_.at(kLongLayout);
    DCHECK(client_thresholds);
    for (const auto& it : *client_thresholds) {
      if (it.value < layout_time)
        it.key->ReportLongLayout(layout_time);
    }
  }

  double task_time = end_time - start_time;
  if (thresholds_[kLongTask] && task_time > thresholds_[kLongTask]) {
    ClientThresholds* client_thresholds = subscriptions_.at(kLongTask);
    for (const auto& it : *client_thresholds) {
      if (it.value < task_time) {
        it.key->ReportLongTask(
            start_time, end_time,
            task_has_multiple_contexts_ ? nullptr : task_execution_context_,
            task_has_multiple_contexts_, sub_task_attributions_);
      }
    }
  }
}

void PerformanceMonitor::InnerReportGenericViolation(
    ExecutionContext* context,
    Violation violation,
    const String& text,
    double time,
    std::unique_ptr<SourceLocation> location) {
  ClientThresholds* client_thresholds = subscriptions_.at(violation);
  if (!client_thresholds)
    return;
  if (!location)
    location = SourceLocation::Capture(context);
  for (const auto& it : *client_thresholds) {
    if (it.value < time)
      it.key->ReportGenericViolation(violation, text, time, location.get());
  }
}

void PerformanceMonitor::Trace(blink::Visitor* visitor) {
  visitor->Trace(local_root_);
  visitor->Trace(task_execution_context_);
  visitor->Trace(subscriptions_);
}

}  // namespace blink
