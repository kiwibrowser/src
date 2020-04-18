// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/ukm_time_aggregator.h"

#include "services/metrics/public/cpp/ukm_entry_builder.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/time.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

UkmTimeAggregator::ScopedUkmTimer::ScopedUkmTimer(
    UkmTimeAggregator* aggregator,
    size_t metric_index,
    CustomCountHistogram* histogram_counter)
    : aggregator_(aggregator),
      metric_index_(metric_index),
      histogram_counter_(histogram_counter),
      start_time_(CurrentTimeTicks()) {}

UkmTimeAggregator::ScopedUkmTimer::ScopedUkmTimer(ScopedUkmTimer&& other)
    : aggregator_(other.aggregator_),
      metric_index_(other.metric_index_),
      histogram_counter_(other.histogram_counter_),
      start_time_(other.start_time_) {
  other.aggregator_ = nullptr;
}

UkmTimeAggregator::ScopedUkmTimer::~ScopedUkmTimer() {
  if (aggregator_) {
    aggregator_->RecordSample(metric_index_, start_time_, CurrentTimeTicks(),
                              histogram_counter_);
  }
}

UkmTimeAggregator::UkmTimeAggregator(String event_name,
                                     int64_t source_id,
                                     ukm::UkmRecorder* recorder,
                                     const Vector<String>& metric_names,
                                     TimeDelta event_frequency)
    : event_name_(std::move(event_name)),
      source_id_(source_id),
      recorder_(recorder),
      event_frequency_(event_frequency),
      last_flushed_time_(CurrentTimeTicks()) {
  metric_records_.ReserveCapacity(metric_names.size());
  for (const auto& metric_name : metric_names) {
    auto& record = metric_records_.emplace_back();
    record.worst_case_metric_name = metric_name;
    record.worst_case_metric_name.append(".WorstCase");
    record.average_metric_name = metric_name;
    record.average_metric_name.append(".Average");
  }
}

UkmTimeAggregator::~UkmTimeAggregator() {
  Flush(TimeTicks());
}

UkmTimeAggregator::ScopedUkmTimer UkmTimeAggregator::GetScopedTimer(
    size_t metric_index,
    CustomCountHistogram* histogram_counter) {
  return ScopedUkmTimer(this, metric_index, histogram_counter);
}

void UkmTimeAggregator::RecordSample(size_t metric_index,
                                     TimeTicks start,
                                     TimeTicks end,
                                     CustomCountHistogram* histogram_counter) {
  FlushIfNeeded(end);

  // Record the UMA if we have a counter.
  TimeDelta duration = end - start;
  if (histogram_counter)
    histogram_counter->Count(duration.InMicroseconds());

  // Append the duration to the appropriate metrics record.
  DCHECK_LT(metric_index, metric_records_.size());
  auto& record = metric_records_[metric_index];
  if (duration > record.worst_case_duration)
    record.worst_case_duration = duration;
  record.total_duration += duration;
  ++record.sample_count;
  has_data_ = true;
}

void UkmTimeAggregator::FlushIfNeeded(TimeTicks current_time) {
  if (current_time >= last_flushed_time_ + event_frequency_)
    Flush(current_time);
}

void UkmTimeAggregator::Flush(TimeTicks current_time) {
  last_flushed_time_ = current_time;
  if (!has_data_)
    return;

  ukm::UkmEntryBuilder builder(source_id_, event_name_.Utf8().data());
  for (auto& record : metric_records_) {
    if (record.sample_count == 0)
      continue;
    builder.SetMetric(record.worst_case_metric_name.Utf8().data(),
                      record.worst_case_duration.InMicroseconds());
    builder.SetMetric(record.average_metric_name.Utf8().data(),
                      record.total_duration.InMicroseconds() /
                          static_cast<int64_t>(record.sample_count));
    record.reset();
  }
  builder.Record(recorder_);
  has_data_ = false;
}

}  // namespace blink
