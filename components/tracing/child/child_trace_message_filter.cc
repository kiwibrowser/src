// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/tracing/child/child_trace_message_filter.h"

#include <memory>

#include "base/metrics/statistics_recorder.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/trace_event.h"
#include "components/tracing/common/tracing_messages.h"
#include "ipc/ipc_channel.h"

using base::trace_event::MemoryDumpManager;
using base::trace_event::TraceLog;

namespace tracing {

namespace {

const int kMinTimeBetweenHistogramChangesInSeconds = 10;

}  // namespace

ChildTraceMessageFilter::ChildTraceMessageFilter(
    base::SingleThreadTaskRunner* ipc_task_runner)
    : enabled_tracing_modes_(0),
      sender_(nullptr),
      ipc_task_runner_(ipc_task_runner) {}

void ChildTraceMessageFilter::OnFilterAdded(IPC::Channel* channel) {
  sender_ = channel;
  sender_->Send(new TracingHostMsg_ChildSupportsTracing());
}

void ChildTraceMessageFilter::SetSenderForTesting(IPC::Sender* sender) {
  sender_ = sender;
}

void ChildTraceMessageFilter::OnFilterRemoved() {
  sender_ = nullptr;
}

bool ChildTraceMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ChildTraceMessageFilter, message)
    IPC_MESSAGE_HANDLER(TracingMsg_SetTracingProcessId, OnSetTracingProcessId)
    IPC_MESSAGE_HANDLER(TracingMsg_SetUMACallback, OnSetUMACallback)
    IPC_MESSAGE_HANDLER(TracingMsg_ClearUMACallback, OnClearUMACallback)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

ChildTraceMessageFilter::~ChildTraceMessageFilter() {}

void ChildTraceMessageFilter::OnSetTracingProcessId(
    uint64_t tracing_process_id) {
  MemoryDumpManager::GetInstance()->set_tracing_process_id(tracing_process_id);
}

void ChildTraceMessageFilter::OnHistogramChanged(
    const std::string& histogram_name,
    base::Histogram::Sample reference_lower_value,
    base::Histogram::Sample reference_upper_value,
    bool repeat,
    base::Histogram::Sample actual_value) {
  if (actual_value < reference_lower_value ||
      actual_value > reference_upper_value) {
    if (!repeat) {
      ipc_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(
              &ChildTraceMessageFilter::SendAbortBackgroundTracingMessage,
              this));
    }
  }

  ipc_task_runner_->PostTask(
      FROM_HERE, base::Bind(&ChildTraceMessageFilter::SendTriggerMessage, this,
                            histogram_name));
}

void ChildTraceMessageFilter::SendTriggerMessage(
    const std::string& histogram_name) {
  if (!histogram_last_changed_.is_null()) {
    base::Time computed_next_allowed_time =
        histogram_last_changed_ +
        base::TimeDelta::FromSeconds(kMinTimeBetweenHistogramChangesInSeconds);
    if (computed_next_allowed_time > TRACE_TIME_NOW())
      return;
  }
  histogram_last_changed_ = TRACE_TIME_NOW();

  if (sender_)
    sender_->Send(new TracingHostMsg_TriggerBackgroundTrace(histogram_name));
}

void ChildTraceMessageFilter::SendAbortBackgroundTracingMessage() {
  if (sender_)
    sender_->Send(new TracingHostMsg_AbortBackgroundTrace());
}

void ChildTraceMessageFilter::OnSetUMACallback(
    const std::string& histogram_name,
    int histogram_lower_value,
    int histogram_upper_value,
    bool repeat) {
  histogram_last_changed_ = base::Time();
  base::StatisticsRecorder::SetCallback(
      histogram_name, base::Bind(&ChildTraceMessageFilter::OnHistogramChanged,
                                 this, histogram_name, histogram_lower_value,
                                 histogram_upper_value, repeat));

  base::HistogramBase* existing_histogram =
      base::StatisticsRecorder::FindHistogram(histogram_name);
  if (!existing_histogram)
    return;

  std::unique_ptr<base::HistogramSamples> samples =
      existing_histogram->SnapshotSamples();
  if (!samples)
    return;

  std::unique_ptr<base::SampleCountIterator> sample_iterator =
      samples->Iterator();
  if (!sample_iterator)
    return;

  while (!sample_iterator->Done()) {
    base::HistogramBase::Sample min;
    int64_t max;
    base::HistogramBase::Count count;
    sample_iterator->Get(&min, &max, &count);

    if (min >= histogram_lower_value && max <= histogram_upper_value) {
      ipc_task_runner_->PostTask(
          FROM_HERE, base::Bind(&ChildTraceMessageFilter::SendTriggerMessage,
                                this, histogram_name));
      break;
    } else if (!repeat) {
      ipc_task_runner_->PostTask(
          FROM_HERE,
          base::Bind(
              &ChildTraceMessageFilter::SendAbortBackgroundTracingMessage,
              this));
      break;
    }

    sample_iterator->Next();
  }
}

void ChildTraceMessageFilter::OnClearUMACallback(
    const std::string& histogram_name) {
  histogram_last_changed_ = base::Time();
  base::StatisticsRecorder::ClearCallback(histogram_name);
}

}  // namespace tracing
