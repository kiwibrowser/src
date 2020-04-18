// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/latency/stream_analyzer.h"

namespace ui {

StreamAnalysis::StreamAnalysis() = default;
StreamAnalysis::~StreamAnalysis() = default;

namespace frame_metrics {

StreamAnalyzer::StreamAnalyzer(
    const StreamAnalyzerClient* client,
    const SharedWindowedAnalyzerClient* shared_client,
    std::vector<uint32_t> thresholds,
    std::unique_ptr<Histogram> histogram)
    : client_(client),
      histogram_(std::move(histogram)),
      windowed_analyzer_(client, shared_client) {
  thresholds_.reserve(thresholds.size());
  for (const uint32_t& t : thresholds)
    thresholds_.emplace_back(t);
}

StreamAnalyzer::~StreamAnalyzer() = default;

void StreamAnalyzer::Reset() {
  StartNewReportPeriod();
  windowed_analyzer_.ResetHistory();
}

void StreamAnalyzer::StartNewReportPeriod() {
  histogram_->Reset();
  windowed_analyzer_.ResetWorstValues();
  for (auto& t : thresholds_)
    t.ResetAccumulators();

  total_weight_ = 0;
  accumulator_ = 0;
  root_accumulator_ = 0;
  square_accumulator_ = Accumulator96b();
}

void StreamAnalyzer::AddSample(const uint32_t value, const uint32_t weight) {
  DCHECK_GT(weight, 0u);

  const uint64_t weighted_value = static_cast<uint64_t>(weight) * value;
  const uint64_t weighted_root = weight * std::sqrt(static_cast<double>(value) *
                                                    kFixedPointRootMultiplier);
  const Accumulator96b weighted_square(value, weight);

  // Verify overflow isn't an issue.
  // square_accumulator_ has DCHECKs internally, so we don't worry about
  // checking that here.
  DCHECK_LT(weighted_value,
            std::numeric_limits<decltype(accumulator_)>::max() - accumulator_);
  DCHECK_LT(weighted_root,
            std::numeric_limits<decltype(root_accumulator_)>::max() -
                root_accumulator_);
  DCHECK_LT(weight, std::numeric_limits<decltype(total_weight_)>::max() -
                        total_weight_);

  histogram_->AddSample(value, weight);
  windowed_analyzer_.AddSample(value, weight, weighted_value, weighted_root,
                               weighted_square);

  for (auto& t : thresholds_) {
    if (value >= t.threshold)
      t.ge_weight += weight;
    else
      t.lt_weight += weight;
  }

  total_weight_ += weight;
  accumulator_ += weighted_value;
  root_accumulator_ += weighted_root;
  square_accumulator_.Add(weighted_square);
}

double StreamAnalyzer::ComputeMean() const {
  double result = static_cast<double>(accumulator_) / total_weight_;
  return client_->TransformResult(result);
}

double StreamAnalyzer::ComputeRMS() const {
  double mean_square = square_accumulator_.ToDouble() / total_weight_;
  double result = std::sqrt(mean_square);
  return client_->TransformResult(result);
}

double StreamAnalyzer::ComputeSMR() const {
  double mean_root = static_cast<double>(root_accumulator_) / total_weight_;
  double result = (mean_root * mean_root) / kFixedPointRootMultiplier;
  return client_->TransformResult(result);
}

double StreamAnalyzer::VarianceHelper(double accum, double square_accum) const {
  double mean = accum / total_weight_;
  double mean_squared = mean * mean;
  double mean_square = square_accum / total_weight_;
  double variance = mean_square - mean_squared;
  // This approach to calculating the standard deviation isn't numerically
  // stable if the variance is very small relative to the mean, which might
  // result in a negative variance. Clamp it to 0.
  return std::max(0.0, variance);
}

double StreamAnalyzer::ComputeStdDev() const {
  double variance =
      VarianceHelper(accumulator_, square_accumulator_.ToDouble());
  double std_dev = std::sqrt(variance);
  return client_->TransformResult(std_dev);
}

double StreamAnalyzer::ComputeVarianceOfRoots() const {
  double normalized_root =
      static_cast<double>(root_accumulator_) / kFixedPointRootMultiplierSqrt;
  double variance = VarianceHelper(normalized_root, accumulator_);
  return client_->TransformResult(variance);
}

void StreamAnalyzer::ThresholdState::ResetAccumulators() {
  ge_weight = 0;
  lt_weight = 0;
}

std::vector<ThresholdResult> StreamAnalyzer::ComputeThresholds() const {
  std::vector<ThresholdResult> results;
  results.reserve(thresholds_.size());
  for (const auto& t : thresholds_) {
    double threshold = client_->TransformResult(t.threshold);
    double ge_fraction =
        static_cast<double>(t.ge_weight) / (t.ge_weight + t.lt_weight);
    results.push_back({threshold, ge_fraction});
  }
  return results;
}

PercentileResults StreamAnalyzer::ComputePercentiles() const {
  PercentileResults result;
  result = histogram_->ComputePercentiles();
  for (size_t i = 0; i < PercentileResults::kCount; i++) {
    result.values[i] = client_->TransformResult(result.values[i]);
  }
  return result;
}

void StreamAnalyzer::ComputeSummary(StreamAnalysis* results) const {
  results->mean = ComputeMean();
  results->rms = ComputeRMS();
  results->smr = ComputeSMR();
  results->std_dev = ComputeStdDev();
  results->variance_of_roots = ComputeVarianceOfRoots();
  results->thresholds = ComputeThresholds();
  results->percentiles = ComputePercentiles();
  results->worst_mean = windowed_analyzer_.ComputeWorstMean();
  results->worst_rms = windowed_analyzer_.ComputeWorstRMS();
  results->worst_smr = windowed_analyzer_.ComputeWorstSMR();
}

std::unique_ptr<base::trace_event::ConvertableToTraceFormat>
StreamAnalyzer::AsValue() const {
  auto state = std::make_unique<base::trace_event::TracedValue>();
  AsValueInto(state.get());
  return std::move(state);
}

void StreamAnalyzer::AsValueInto(base::trace_event::TracedValue* state) const {
  state->SetDouble("mean", ComputeMean());

  state->SetDouble("rms", ComputeRMS());
  state->SetDouble("smr", ComputeSMR());

  state->SetDouble("std_dev", ComputeStdDev());
  state->SetDouble("variance_of_roots", ComputeVarianceOfRoots());

  state->BeginArray("percentiles");
  PercentileResults result = ComputePercentiles();
  for (size_t i = 0; i < PercentileResults::kCount; i++) {
    state->BeginArray();
    state->AppendDouble(PercentileResults::kPercentiles[i]);
    state->AppendDouble(result.values[i]);
    state->EndArray();
  }
  state->EndArray();

  state->BeginArray("thresholds");
  std::vector<ThresholdResult> thresholds(ComputeThresholds());
  for (const auto& t : thresholds) {
    state->BeginArray();
    state->AppendDouble(t.threshold);
    state->AppendDouble(t.ge_fraction);
    state->EndArray();
  }
  state->EndArray();

  windowed_analyzer_.AsValueInto(state);
}

}  // namespace frame_metrics
}  // namespace ui
