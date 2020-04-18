// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/external_metrics.h"

#include <stddef.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/statistics_recorder.h"
#include "base/metrics/user_metrics.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/metrics/chromeos_metrics_provider.h"
#include "components/metrics/serialization/metric_sample.h"
#include "components/metrics/serialization/serialization_utils.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace chromeos {

namespace {

bool CheckValues(const std::string& name,
                 int minimum,
                 int maximum,
                 uint32_t bucket_count) {
  if (!base::Histogram::InspectConstructionArguments(
      name, &minimum, &maximum, &bucket_count))
    return false;
  base::HistogramBase* histogram =
      base::StatisticsRecorder::FindHistogram(name);
  if (!histogram)
    return true;
  return histogram->HasConstructionArguments(minimum, maximum, bucket_count);
}

bool CheckLinearValues(const std::string& name, int maximum) {
  return CheckValues(name, 1, maximum, maximum + 1);
}

// The interval between external metrics collections.
constexpr base::TimeDelta kExternalMetricsCollectionInterval =
    base::TimeDelta::FromSeconds(30);
constexpr char kEventsFilePath[] = "/var/lib/metrics/uma-events";

}  // namespace

ExternalMetrics::ExternalMetrics() : uma_events_file_(kEventsFilePath) {
}

ExternalMetrics::~ExternalMetrics() {}

void ExternalMetrics::Start() {
  ScheduleCollector();
}

// static
scoped_refptr<ExternalMetrics> ExternalMetrics::CreateForTesting(
    const std::string& filename) {
  scoped_refptr<ExternalMetrics> external_metrics(new ExternalMetrics());
  external_metrics->uma_events_file_ = filename;
  return external_metrics;
}

void ExternalMetrics::RecordActionUI(const std::string& action_string) {
  base::RecordComputedAction(action_string);
}

void ExternalMetrics::RecordAction(const std::string& action) {
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(&ExternalMetrics::RecordActionUI, this, action));
}

void ExternalMetrics::RecordCrashUI(const std::string& crash_kind) {
  ChromeOSMetricsProvider::LogCrash(crash_kind);
}

void ExternalMetrics::RecordCrash(const std::string& crash_kind) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&ExternalMetrics::RecordCrashUI, this, crash_kind));
}

void ExternalMetrics::RecordHistogram(const metrics::MetricSample& sample) {
  CHECK_EQ(metrics::MetricSample::HISTOGRAM, sample.type());
  if (!CheckValues(
          sample.name(), sample.min(), sample.max(), sample.bucket_count())) {
    DLOG(ERROR) << "Invalid histogram: " << sample.name();
    return;
  }

  base::UmaHistogramCustomCounts(sample.name(), sample.sample(), sample.min(),
                                 sample.max(), sample.bucket_count());
}

void ExternalMetrics::RecordLinearHistogram(
    const metrics::MetricSample& sample) {
  CHECK_EQ(metrics::MetricSample::LINEAR_HISTOGRAM, sample.type());
  if (!CheckLinearValues(sample.name(), sample.max())) {
    DLOG(ERROR) << "Invalid linear histogram: " << sample.name();
    return;
  }
  base::UmaHistogramExactLinear(sample.name(), sample.sample(), sample.max());
}

void ExternalMetrics::RecordSparseHistogram(
    const metrics::MetricSample& sample) {
  CHECK_EQ(metrics::MetricSample::SPARSE_HISTOGRAM, sample.type());
  base::UmaHistogramSparse(sample.name(), sample.sample());
}

int ExternalMetrics::CollectEvents() {
  std::vector<std::unique_ptr<metrics::MetricSample>> samples;
  metrics::SerializationUtils::ReadAndTruncateMetricsFromFile(uma_events_file_,
                                                              &samples);

  for (auto it = samples.begin(); it != samples.end(); ++it) {
    const metrics::MetricSample& sample = **it;

    // Do not use the UMA_HISTOGRAM_... macros here.  They cache the Histogram
    // instance and thus only work if |sample.name()| is constant.
    switch (sample.type()) {
      case metrics::MetricSample::CRASH:
        RecordCrash(sample.name());
        break;
      case metrics::MetricSample::USER_ACTION:
        RecordAction(sample.name());
        break;
      case metrics::MetricSample::HISTOGRAM:
        RecordHistogram(sample);
        break;
      case metrics::MetricSample::LINEAR_HISTOGRAM:
        RecordLinearHistogram(sample);
        break;
      case metrics::MetricSample::SPARSE_HISTOGRAM:
        RecordSparseHistogram(sample);
        break;
    }
  }

  return samples.size();
}

void ExternalMetrics::CollectEventsAndReschedule() {
  CollectEvents();
  ScheduleCollector();
}

void ExternalMetrics::ScheduleCollector() {
  base::PostDelayedTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&chromeos::ExternalMetrics::CollectEventsAndReschedule,
                     this),
      kExternalMetricsCollectionInterval);
}

}  // namespace chromeos
