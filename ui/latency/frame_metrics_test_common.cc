// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/latency/frame_metrics_test_common.h"

#include "base/logging.h"

namespace ui {
namespace frame_metrics {

double TestStreamAnalyzerClient::TransformResult(double result) const {
  return result * result_scale;
}

template <>
void AddSamplesHelper(StreamAnalyzer* analyzer,
                      uint64_t value,
                      uint64_t weight,
                      size_t iterations) {
  DCHECK_LE(value, std::numeric_limits<uint32_t>::max());
  DCHECK_LE(weight, std::numeric_limits<uint32_t>::max());
  for (size_t i = 0; i < iterations; i++) {
    analyzer->AddSample(value, weight);
  }
}

TestRatioBoundaries::TestRatioBoundaries() {
  const uint32_t one = kFixedPointMultiplier;
  const uint32_t half = one / 2;
  // [0, 2^-16) => 1 bucket.
  int i = 0;
  boundaries[i++] = 0;
  // [2^-16,1) pow of 2 strides => 16 buckets. (16x1)
  for (int j = 0; j < 16; j++)
    boundaries[i++] = 1ULL << j;
  // [1,16) stride 1/2 => 30 buckets. (2 + 4 + 8 + 16)
  for (int j = 0; j < 30; j++)
    boundaries[i++] = one + (j * half);
  // [16,32) stride 1 => 16 buckets.
  for (int j = 0; j < 16; j++)
    boundaries[i++] = (16 + j) * one;
  // [32,64) stride 2 => 16 buckets.
  for (int j = 0; j < 16; j++)
    boundaries[i++] = (32 + 2 * j) * one;
  // [64,128) stride 8 => 8 buckets.
  for (int j = 0; j < 8; j++)
    boundaries[i++] = (64 + 8 * j) * one;
  // [128, 256) stride 16 => 8 buckets.
  for (int j = 0; j < 8; j++)
    boundaries[i++] = (128 + 16 * j) * one;
  // [256, 512) stride 64 => 4 buckets.
  for (int j = 0; j < 4; j++)
    boundaries[i++] = (256 + 64 * j) * one;
  // [512, 1024) stride 128 => 4 buckets.
  for (int j = 0; j < 4; j++)
    boundaries[i++] = (512 + 128 * j) * one;
  // [1024, 2048) stride 512 => 2 buckets.
  for (int j = 0; j < 2; j++)
    boundaries[i++] = (1024 + 512 * j) * one;
  // [2048, 4096) stride 1024 => 2 buckets.
  for (int j = 0; j < 2; j++)
    boundaries[i++] = (2048 + 1024 * j) * one;
  // [4096, 2^16) pow of 2 strides => 4 buckets. (4x1)
  for (int j = 0; j < 4; j++)
    boundaries[i++] = (4096ULL << j) * one;
  boundaries[i++] = 1ULL << 32;
  DCHECK_EQ(112, i);
}

TestHistogram::TestHistogram() = default;
TestHistogram::~TestHistogram() = default;

void TestHistogram::AddSample(uint32_t value, uint32_t weight) {
  added_samples_.push_back({value, weight});
}

PercentileResults TestHistogram::ComputePercentiles() const {
  return results_;
}

std::vector<TestHistogram::ValueWeightPair>
TestHistogram::GetAndResetAllAddedSamples() {
  return std::move(added_samples_);
}

void TestHistogram::SetResults(PercentileResults results) {
  results_ = results;
}

}  // namespace frame_metrics
}  // namespace ui
