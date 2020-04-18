// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/serialization/metric_sample.h"

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"

namespace metrics {

MetricSample::MetricSample(MetricSample::SampleType sample_type,
                           const std::string& metric_name,
                           int sample,
                           int min,
                           int max,
                           int bucket_count)
    : type_(sample_type),
      name_(metric_name),
      sample_(sample),
      min_(min),
      max_(max),
      bucket_count_(bucket_count) {
}

MetricSample::~MetricSample() {
}

bool MetricSample::IsValid() const {
  return name().find(' ') == std::string::npos &&
         name().find('\0') == std::string::npos && !name().empty();
}

std::string MetricSample::ToString() const {
  if (type_ == CRASH) {
    return base::StringPrintf("crash%c%s%c",
                              '\0',
                              name().c_str(),
                              '\0');
  }
  if (type_ == SPARSE_HISTOGRAM) {
    return base::StringPrintf("sparsehistogram%c%s %d%c",
                              '\0',
                              name().c_str(),
                              sample_,
                              '\0');
  }
  if (type_ == LINEAR_HISTOGRAM) {
    return base::StringPrintf("linearhistogram%c%s %d %d%c",
                              '\0',
                              name().c_str(),
                              sample_,
                              max_,
                              '\0');
  }
  if (type_ == HISTOGRAM) {
    return base::StringPrintf("histogram%c%s %d %d %d %d%c",
                              '\0',
                              name().c_str(),
                              sample_,
                              min_,
                              max_,
                              bucket_count_,
                              '\0');
  }
  // The type can only be USER_ACTION.
  CHECK_EQ(type_, USER_ACTION);
  return base::StringPrintf("useraction%c%s%c", '\0', name().c_str(), '\0');
}

int MetricSample::sample() const {
  CHECK_NE(type_, USER_ACTION);
  CHECK_NE(type_, CRASH);
  return sample_;
}

int MetricSample::min() const {
  CHECK_EQ(type_, HISTOGRAM);
  return min_;
}

int MetricSample::max() const {
  CHECK_NE(type_, CRASH);
  CHECK_NE(type_, USER_ACTION);
  CHECK_NE(type_, SPARSE_HISTOGRAM);
  return max_;
}

int MetricSample::bucket_count() const {
  CHECK_EQ(type_, HISTOGRAM);
  return bucket_count_;
}

// static
std::unique_ptr<MetricSample> MetricSample::CrashSample(
    const std::string& crash_name) {
  return std::unique_ptr<MetricSample>(
      new MetricSample(CRASH, crash_name, 0, 0, 0, 0));
}

// static
std::unique_ptr<MetricSample> MetricSample::HistogramSample(
    const std::string& histogram_name,
    int sample,
    int min,
    int max,
    int bucket_count) {
  return std::unique_ptr<MetricSample>(new MetricSample(
      HISTOGRAM, histogram_name, sample, min, max, bucket_count));
}

// static
std::unique_ptr<MetricSample> MetricSample::ParseHistogram(
    const std::string& serialized_histogram) {
  std::vector<base::StringPiece> parts = base::SplitStringPiece(
      serialized_histogram, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  if (parts.size() != 5)
    return std::unique_ptr<MetricSample>();
  int sample, min, max, bucket_count;
  if (parts[0].empty() || !base::StringToInt(parts[1], &sample) ||
      !base::StringToInt(parts[2], &min) ||
      !base::StringToInt(parts[3], &max) ||
      !base::StringToInt(parts[4], &bucket_count)) {
    return std::unique_ptr<MetricSample>();
  }

  return HistogramSample(parts[0].as_string(), sample, min, max, bucket_count);
}

// static
std::unique_ptr<MetricSample> MetricSample::SparseHistogramSample(
    const std::string& histogram_name,
    int sample) {
  return std::unique_ptr<MetricSample>(
      new MetricSample(SPARSE_HISTOGRAM, histogram_name, sample, 0, 0, 0));
}

// static
std::unique_ptr<MetricSample> MetricSample::ParseSparseHistogram(
    const std::string& serialized_histogram) {
  std::vector<base::StringPiece> parts = base::SplitStringPiece(
      serialized_histogram, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (parts.size() != 2)
    return std::unique_ptr<MetricSample>();
  int sample;
  if (parts[0].empty() || !base::StringToInt(parts[1], &sample))
    return std::unique_ptr<MetricSample>();

  return SparseHistogramSample(parts[0].as_string(), sample);
}

// static
std::unique_ptr<MetricSample> MetricSample::LinearHistogramSample(
    const std::string& histogram_name,
    int sample,
    int max) {
  return std::unique_ptr<MetricSample>(
      new MetricSample(LINEAR_HISTOGRAM, histogram_name, sample, 0, max, 0));
}

// static
std::unique_ptr<MetricSample> MetricSample::ParseLinearHistogram(
    const std::string& serialized_histogram) {
  std::vector<base::StringPiece> parts = base::SplitStringPiece(
      serialized_histogram, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  int sample, max;
  if (parts.size() != 3)
    return std::unique_ptr<MetricSample>();
  if (parts[0].empty() || !base::StringToInt(parts[1], &sample) ||
      !base::StringToInt(parts[2], &max)) {
    return std::unique_ptr<MetricSample>();
  }

  return LinearHistogramSample(parts[0].as_string(), sample, max);
}

// static
std::unique_ptr<MetricSample> MetricSample::UserActionSample(
    const std::string& action_name) {
  return std::unique_ptr<MetricSample>(
      new MetricSample(USER_ACTION, action_name, 0, 0, 0, 0));
}

bool MetricSample::IsEqual(const MetricSample& metric) {
  return type_ == metric.type_ && name_ == metric.name_ &&
         sample_ == metric.sample_ && min_ == metric.min_ &&
         max_ == metric.max_ && bucket_count_ == metric.bucket_count_;
}

}  // namespace metrics
