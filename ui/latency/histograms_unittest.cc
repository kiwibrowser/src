// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/latency/histograms.h"

#include <algorithm>

#include "base/metrics/bucket_ranges.h"
#include "base/metrics/sample_vector.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/latency/fixed_point.h"
#include "ui/latency/frame_metrics_test_common.h"

namespace ui {
namespace frame_metrics {

// Verifies the ratio boundaries generated internally match the reference
// boundaries.
TEST(FrameMetricsHistogramsTest, RatioBoundariesDirect) {
  const TestRatioBoundaries kTestRatioBoundaries;
  std::unique_ptr<BoundaryIterator> ratio_impl =
      CreateRatioIteratorForTesting();
  for (uint32_t boundary : kTestRatioBoundaries.boundaries) {
    if (boundary == 0)
      continue;
    EXPECT_EQ(boundary, ratio_impl->Next());
  }
}

// Verifies the VSync boundaries generated internally match the reference
// boundaries.
TEST(FrameMetricsHistogramsTest, VSyncBoundariesDirect) {
  std::unique_ptr<BoundaryIterator> vsync_impl =
      CreateVSyncIteratorForTesting();
  for (uint32_t boundary : kTestVSyncBoundries) {
    if (boundary == 0)
      continue;
    EXPECT_EQ(boundary, vsync_impl->Next());
  }
}

// Results should be 0 if no samples have been added yet.
TEST(FrameMetricsHistogramsTest, ResultsAreZeroWithoutSamples) {
  RatioHistogram ratio_histogram;
  EXPECT_EQ(0, ratio_histogram.ComputePercentiles().values[0]);
  EXPECT_EQ(0, ratio_histogram.ComputePercentiles().values[1]);

  VSyncHistogram vsync_histogram;
  EXPECT_EQ(0, vsync_histogram.ComputePercentiles().values[0]);
  EXPECT_EQ(0, vsync_histogram.ComputePercentiles().values[1]);
}

// A non-zero value implies samples were added since, even if those samples
// were zero, they would go into the [0,N) bucket and result in a non-zero
// estimate.
TEST(FrameMetricsHistogramsTest, ResultsAreNonZeroWithSamplesOfZero) {
  RatioHistogram ratio_histogram;
  ratio_histogram.AddSample(0, 1);
  EXPECT_LT(0, ratio_histogram.ComputePercentiles().values[0]);
  EXPECT_LT(0, ratio_histogram.ComputePercentiles().values[1]);

  VSyncHistogram vsync_histogram;
  vsync_histogram.AddSample(0, 1);
  EXPECT_LT(0, vsync_histogram.ComputePercentiles().values[0]);
  EXPECT_LT(0, vsync_histogram.ComputePercentiles().values[1]);
}

template <typename ReferenceBoundaryT>
void BoundaryTestCommon(const ReferenceBoundaryT& reference_boundaries,
                        std::unique_ptr<Histogram> histogram) {
  PercentileResults percentiles;

  for (size_t i = 0; i < reference_boundaries.size() - 1; i++) {
    uint64_t bucket_start = reference_boundaries[i];
    uint64_t bucket_end = reference_boundaries[i + 1];

    // Verify values within the current bucket don't affect percentile.
    // This also checks the first value in the bucket.
    uint32_t stride = std::max<uint32_t>(1u, (bucket_end - bucket_start) / 8);
    for (uint64_t value = bucket_start; value < bucket_end; value += stride) {
      histogram->AddSample(value, 1);
      percentiles = histogram->ComputePercentiles();
      histogram->Reset();
      EXPECT_LE(bucket_start, percentiles.values[0]);
      EXPECT_GT(bucket_end, percentiles.values[0]);
    }

    // Verify the value just before the next bucket doesn't affect percentile.
    histogram->AddSample(bucket_end - 1, 1);
    percentiles = histogram->ComputePercentiles();
    histogram->Reset();
    EXPECT_LE(bucket_start, percentiles.values[0]);
    EXPECT_GT(bucket_end, percentiles.values[0]);
  }
}

TEST(FrameMetricsHistogramsTest, RatioBoundaries) {
  const TestRatioBoundaries kTestRatioBoundaries;
  BoundaryTestCommon(kTestRatioBoundaries, std::make_unique<RatioHistogram>());
}

TEST(FrameMetricsHistogramsTest, VSyncBoundaries) {
  const TestRatioBoundaries kTestRatioBoundaries;
  BoundaryTestCommon(kTestVSyncBoundries, std::make_unique<VSyncHistogram>());
}

template <typename ReferenceBoundaryT>
void PercentilesTestCommon(const ReferenceBoundaryT& reference_boundaries,
                           std::unique_ptr<Histogram> histogram,
                           int percentile_index) {
  double percentile = PercentileResults::kPercentiles[percentile_index];
  PercentileResults percentiles;
  for (size_t i = 0; i < reference_boundaries.size() - 1; i++) {
    uint64_t bucket_start = reference_boundaries[i];
    uint64_t bucket_end = reference_boundaries[i + 1];

    // Add samples to current bucket.
    // Where the samples are added in the current bucket should not affect the
    // result.
    uint32_t stride = std::max<uint32_t>(1u, (bucket_end - bucket_start) / 100);
    int samples_added_inside = 0;
    for (uint64_t value = bucket_start; value < bucket_end; value += stride) {
      histogram->AddSample(value, 10);
      samples_added_inside += 10;
    }

    // Add samples to left and right of current bucket.
    // Don't worry about doing this for the left most and right most buckets.
    int samples_added_left = 0;
    int samples_added_outside = 0;
    if (i != 0 && i < reference_boundaries.size() - 2) {
      samples_added_outside = 10000;
      samples_added_left = samples_added_outside * percentile;
      histogram->AddSample(bucket_start / 3, samples_added_left);
      histogram->AddSample(bucket_start * 3,
                           samples_added_outside - samples_added_left);
    }

    percentiles = histogram->ComputePercentiles();
    histogram->Reset();

    double index = (samples_added_inside + samples_added_outside) * percentile -
                   samples_added_left;
    double w = index / samples_added_inside;
    double expected_value = bucket_end * w + bucket_start * (1.0 - w);
    EXPECT_DOUBLE_EQ(expected_value, percentiles.values[percentile_index]);
  }
}

TEST(FrameMetricsHistogramsTest, RatioPercentiles50th) {
  const TestRatioBoundaries kTestRatioBoundaries;
  PercentilesTestCommon(kTestRatioBoundaries,
                        std::make_unique<RatioHistogram>(), 0);
}

TEST(FrameMetricsHistogramsTest, RatioPercentiles99th) {
  const TestRatioBoundaries kTestRatioBoundaries;
  PercentilesTestCommon(kTestRatioBoundaries,
                        std::make_unique<RatioHistogram>(), 1);
}

TEST(FrameMetricsHistogramsTest, VSyncPercentiles50th) {
  const TestRatioBoundaries kTestRatioBoundaries;
  PercentilesTestCommon(kTestVSyncBoundries, std::make_unique<VSyncHistogram>(),
                        0);
}

TEST(FrameMetricsHistogramsTest, VSyncPercentiles99th) {
  const TestRatioBoundaries kTestRatioBoundaries;
  PercentilesTestCommon(kTestVSyncBoundries, std::make_unique<VSyncHistogram>(),
                        1);
}

}  // namespace frame_metrics
}  // namespace ui
