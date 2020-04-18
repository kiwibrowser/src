// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/latency/histograms.h"

#include <cmath>
#include <limits>

#include "base/bits.h"
#include "base/time/time.h"
#include "ui/latency/fixed_point.h"

namespace {

// Calculates percentiles in a way that can be shared by different histograms.
ui::PercentileResults PercentilesHelper(
    ui::frame_metrics::BoundaryIterator* boundary_iterator,
    const uint32_t* buckets_begin,
    const uint32_t* buckets_end,
    uint64_t total_samples) {
  ui::PercentileResults result;
  uint64_t boundary_left = 0;
  uint64_t boundary_right = boundary_iterator->Next();

  double thresholds[ui::PercentileResults::kCount];
  for (size_t i = 0; i < ui::PercentileResults::kCount; i++) {
    thresholds[i] = ui::PercentileResults::kPercentiles[i] * total_samples;
  }

  uint64_t accumulator = 0;
  size_t result_index = 0;
  for (const uint32_t* bucket = buckets_begin; bucket < buckets_end; bucket++) {
    accumulator += *bucket;
    // Multiple percentiles might be calculated from the same bucket,
    // so we use a while loop here.
    while (accumulator > thresholds[result_index]) {
      const double overage = accumulator - thresholds[result_index];
      double b0_fraction = overage / (*bucket);
      double b1_fraction = 1.0 - b0_fraction;

      // Use a linear interpolation between two buckets.
      // This assumes the samples are evenly distributed within a bucket.
      // TODO(brianderson): Consider neighboring bucket sizes and fit to a
      //   curve. http://crbug.com/821879
      const double estimate =
          b0_fraction * boundary_left + b1_fraction * boundary_right;
      result.values[result_index] = estimate;
      result_index++;
      if (result_index >= ui::PercentileResults::kCount)
        return result;
    }

    boundary_left = boundary_right;
    boundary_right = boundary_iterator->Next();
  }

  return result;
}

}  // namespace

namespace ui {

constexpr double PercentileResults::kPercentiles[];
constexpr size_t PercentileResults::kCount;

namespace frame_metrics {

constexpr size_t RatioHistogram::kBucketCount;
constexpr size_t VSyncHistogram::kBucketCount;

// Ratio Histogram.
// The distribution of each category of buckets in this historam is
// exponential, however each category is divided into N linear buckets
// depending on how much precision we want for that category. How much
// precision a category gets depends on how often we expect that bucket to be
// used and how important those buckets are.
//
// Most of the precision is allocated for values just > 1 since most ratios,
// when not ~0 will be > 1. And since ratios are likely to be near whole
// numbers (some multiple of the vsync), we only give it a precision of 1/2.
// You can think about the stride of each category as the number of vsyncs of
// precision that category will have.
//
// There will be aliasing, but because of the vsync aligned linear division
// of each category, we won't get a bucket that represents fewer vsyncs than
// its fprevious bucket.
//
// This is in contrast to the default exponential distribution of UMA
// buckets, which result in a constant precision for each bucket and would
// allocate lots of very small buckets near 0 where we don't need the
// precision.

namespace {

constexpr size_t kBucketHalfStrideFirstBucketIndex = 17;

// Within the range [16, 4096), there are 9 categories of buckets that each
// start with a power of 2. Within a category, successive buckets have a fixed
// stride. Across categories, the strides increase exponentionally, encoded
// as powers of 2 in |stride_shift|, which increases linearly.
struct RatioBucketCategory {
  uint8_t first_bucket_index;
  uint8_t stride_shift;
};
using RatioCategoryHelper = std::array<RatioBucketCategory, 9>;
constexpr RatioCategoryHelper kCategories16to4096 = {
    // first_bucket_index of each row below is the previous one + number of
    // buckets. Each entry is {first_bucket_index, stride_shift}.
    {{47, 0},      // [16, 32) stride 1 => 16 buckets.
     {63, 1},      // [32, 64) stride 2 => 16 buckets.
     {79, 3},      // [64, 128) stride 8 => 8 buckets.
     {87, 4},      // [128, 256) stride 16 => 8 buckets.
     {95, 6},      // [256, 512) stride 64 => 4 buckets
     {99, 7},      // [512, 1024) stride 128 => 4 buckets.
     {103, 9},     // [1024, 2048) stride 512 => 2 buckets.
     {105, 10},    // [2048, 4096) stride 1024 => 2 buckets.
     {107, 12}}};  // [4096, 8192) stride 4096 => 1 bucket.

// The delegate RatioBoundary::Percentiles will pass to PercentilesHelper.
struct RatioBoundaryIterator : public BoundaryIterator {
  ~RatioBoundaryIterator() override = default;

  size_t bucket = 0;
  uint64_t boundary = 0;
  RatioCategoryHelper::const_iterator b16to4096 = kCategories16to4096.begin();
  uint64_t next_boundary_to_change_category =
      32 * frame_metrics::kFixedPointMultiplier;

  uint64_t Next() override {
    if (bucket == 0) {
      // The first bucket is [0, 1).
      boundary = 1;
    } else if (bucket < kBucketHalfStrideFirstBucketIndex ||
               bucket >= kCategories16to4096.back().first_bucket_index) {
      // The start and end buckets increase in size by powers of 2.
      boundary *= 2;
    } else if (bucket < kCategories16to4096.front().first_bucket_index) {
      // The 30 buckets before 47 have a stride of .5 and represent the
      // range [1, 16).
      boundary += (frame_metrics::kFixedPointMultiplier / 2);
    } else {
      // The rest of the buckets are defined by kCategories16to4096.
      DCHECK(b16to4096 < kCategories16to4096.end());
      boundary +=
          (frame_metrics::kFixedPointMultiplier << b16to4096->stride_shift);
      // The category changes for every power of 2.
      if (boundary >= next_boundary_to_change_category) {
        next_boundary_to_change_category *= 2;
        b16to4096++;
      }
    }

    bucket++;
    return boundary;
  }
};

}  // namespace

std::unique_ptr<BoundaryIterator> CreateRatioIteratorForTesting() {
  return std::make_unique<RatioBoundaryIterator>();
}

RatioHistogram::RatioHistogram() = default;
RatioHistogram::~RatioHistogram() = default;

void RatioHistogram::AddSample(uint32_t ratio, uint32_t weight) {
  size_t bucket = 0;

  // Precomputed thresholds for the log base 2 of the ratio that help
  // determine which category of buckets the sample should go in.
  constexpr int kLog2HalfStrideStart = kFixedPointShift;
  constexpr int kLog2Cats16to4096Start = kFixedPointShift + 4;  // 2^4 = 16.
  constexpr int kLog2_4096Pow2Start = kFixedPointShift + 12;    // 2^12 = 4096.

  if (ratio == 0) {
    bucket = 0;
  } else {
    int log2 = base::bits::Log2Floor(ratio);
    DCHECK_GE(log2, 0);
    if (log2 < kLog2HalfStrideStart) {
      // [2^-16, 1) pow of 2 strides => 16 buckets. (16x1)
      bucket = 1 + log2;
    } else if (log2 < kLog2Cats16to4096Start) {
      // [1, 16) stride 1/2 => 30 buckets. (2 + 4 + 8 + 16)
      const int first_bucket_index = kBucketHalfStrideFirstBucketIndex;
      const int category_start = kFixedPointMultiplier;
      const int total_shift = kFixedPointShift - 1;  // -1 multiplies by 2.
      const int category_offset = (ratio - category_start) >> total_shift;
      bucket = first_bucket_index + category_offset;
    } else if (log2 < kLog2_4096Pow2Start) {
      // [16, 32) stride 1 => 16 buckets.
      // [32, 64) stride 2 => 16 buckets.
      // [64, 128) stride 8 => 8 buckets.
      // [128, 256) stride 16 => 8 buckets.
      // [256, 512) stride 64 => 4 buckets.
      // [512, 1024) stride 128 => 4 buckets.
      // [1024, 2048) stride 512 => 2 buckets.
      // [2048, 4096) stride 1024 => 2 buckets.
      const int category = log2 - kLog2Cats16to4096Start;
      const int category_start = 1 << log2;
      const int total_shift =
          (kFixedPointShift + kCategories16to4096[category].stride_shift);
      const int category_offset = (ratio - category_start) >> total_shift;
      bucket =
          kCategories16to4096[category].first_bucket_index + category_offset;
    } else {
      // [4096, 2^16) pow of 2 strides => 4 buckets. (4x1)
      const int category_offset = log2 - kLog2_4096Pow2Start;
      bucket = kCategories16to4096.back().first_bucket_index + category_offset;
    }
  }
  DCHECK_LT(bucket, kBucketCount);

  // Verify overflow isn't an issue.
  DCHECK_LT(weight, std::numeric_limits<BucketArray::value_type>::max() -
                        buckets_[bucket]);
  DCHECK_LT(weight, std::numeric_limits<decltype(total_samples_)>::max() -
                        total_samples_);

  buckets_[bucket] += weight;
  total_samples_ += weight;
}

PercentileResults RatioHistogram::ComputePercentiles() const {
  RatioBoundaryIterator i;
  return PercentilesHelper(&i, buckets_.data(),
                           buckets_.data() + buckets_.size(), total_samples_);
}

void RatioHistogram::Reset() {
  total_samples_ = 0;
  buckets_.fill(0);
}

// VSyncHistogram.
namespace {

// The number of buckets in bucket categories 1 through 6.
constexpr std::array<uint8_t, 6> kVSyncBucketCounts = {{12, 16, 16, 16, 31, 6}};

// Some constants used to convert values to bucket categories.
constexpr size_t kVSync1stBucketC0 = 0;
constexpr size_t kVSync1stBucketC1 = kVSync1stBucketC0 + 1;
constexpr size_t kVSync1stBucketC2 = kVSync1stBucketC1 + kVSyncBucketCounts[0];
constexpr size_t kVSync1stBucketC3 = kVSync1stBucketC2 + kVSyncBucketCounts[1];
constexpr size_t kVSync1stBucketC4 = kVSync1stBucketC3 + kVSyncBucketCounts[2];
constexpr size_t kVSync1stBucketC5 = kVSync1stBucketC4 + kVSyncBucketCounts[3];
constexpr size_t kVSync1stBucketC6 = kVSync1stBucketC5 + kVSyncBucketCounts[4];
constexpr size_t kVSyncBucketCountC6 = kVSyncBucketCounts[5];

// This iterates through the microsecond VSync boundaries.
struct VSyncBoundaryIterator : public BoundaryIterator {
  ~VSyncBoundaryIterator() override = default;

  uint8_t category_ = 0;
  uint8_t sub_bucket_ = 0;

  uint64_t Next() override {
    uint32_t boundary = 0;
    switch (category_) {
      case 0:  // Powers of two from 1 to 2048 us @ 50% precision
        boundary = 1 << sub_bucket_;
        break;
      case 1:    // Every 8 Hz from 256 Hz to 128 Hz @ 3-6% precision
      case 2:    // Every 4 Hz from 128 Hz to  64 Hz @ 3-6% precision
      case 3:    // Every 2 Hz from  64 Hz to  32 Hz @ 3-6% precision
      case 4: {  // Every 1 Hz from  32 Hz to   1 Hz @ 3-33% precision
        int hz_start = 256 >> (category_ - 1);
        int hz_stride = 8 >> (category_ - 1);
        int hz = hz_start - hz_stride * sub_bucket_;
        boundary = (base::TimeTicks::kMicrosecondsPerSecond + (hz / 2)) / hz;
        break;
      }
      case 5:  // Powers of two from 1s to 32s @ 50% precision
        boundary =
            static_cast<uint32_t>(base::TimeTicks::kMicrosecondsPerSecond) *
            (1 << sub_bucket_);
        break;
      case 6:  // The last boundary of 64s.
        // Advancing would result in out-of-bounds access of
        // kVSyncBucketCounts, so just return.
        return 64 * base::TimeTicks::kMicrosecondsPerSecond;
      default:
        NOTREACHED();
    }

    if (++sub_bucket_ >= kVSyncBucketCounts[category_]) {
      category_++;
      sub_bucket_ = 0;
    }

    return boundary;
  }
};

}  // namespace

std::unique_ptr<BoundaryIterator> CreateVSyncIteratorForTesting() {
  return std::make_unique<VSyncBoundaryIterator>();
}

VSyncHistogram::VSyncHistogram() = default;
VSyncHistogram::~VSyncHistogram() = default;

// Optimized to minimize the number of memory accesses.
void VSyncHistogram::AddSample(uint32_t microseconds, uint32_t weight) {
  size_t bucket = 0;

  static constexpr int k256HzPeriodInMicroseconds =
      base::TimeTicks::kMicrosecondsPerSecond / 256;

  if (microseconds == 0) {
    // bucket = 0;
  } else if (microseconds < k256HzPeriodInMicroseconds) {
    // Powers of two from 1 to 2048 us @ 50% precision
    bucket = kVSync1stBucketC1 + base::bits::Log2Floor(microseconds);
  } else if (microseconds < base::TimeTicks::kMicrosecondsPerSecond) {
    // [256Hz, 1Hz)
    int hz = base::TimeTicks::kMicrosecondsPerSecond / (microseconds + 0.5);
    DCHECK_LT(hz, 256);
    switch (hz / 32) {
      // Every 1 Hz from 32 Hz to 1 Hz @ 3-33% precision
      case 0:
        bucket = kVSync1stBucketC6 - hz;
        break;
      // Every 2 Hz from 64 Hz to 32 Hz @ 3-6% precision
      case 1:
        bucket = kVSync1stBucketC5 - ((hz - 30) / 2);
        break;
      // Every 4 Hz from 128 Hz to 64 Hz @ 3-6% precision
      case 2:
      case 3:
        bucket = kVSync1stBucketC4 - ((hz - 60) / 4);
        break;
      // Every 8 Hz from 256 Hz to 128 Hz @ 3-6% precision
      case 4:
      case 5:
      case 6:
      case 7:
        bucket = kVSync1stBucketC3 - ((hz - 120) / 8);
        break;
      default:
        NOTREACHED();
        return;
    }
  } else {
    // Powers of two from 1s to 32s @ 50% precision
    int seconds_log2 = base::bits::Log2Floor(
        microseconds / base::TimeTicks::kMicrosecondsPerSecond);
    DCHECK_GE(seconds_log2, 0);
    size_t offset = std::min<size_t>(kVSyncBucketCountC6 - 1, seconds_log2);
    bucket = kVSync1stBucketC6 + offset;
  }

  DCHECK_GE(bucket, 0u);
  DCHECK_LT(bucket, kVSync1stBucketC6 + kVSyncBucketCountC6);
  DCHECK_LT(bucket, kBucketCount);

  // Verify overflow isn't an issue.
  DCHECK_LT(weight, std::numeric_limits<BucketArray::value_type>::max() -
                        buckets_[bucket]);
  DCHECK_LT(weight, std::numeric_limits<decltype(total_samples_)>::max() -
                        total_samples_);

  buckets_[bucket] += weight;
  total_samples_ += weight;
}

PercentileResults VSyncHistogram::ComputePercentiles() const {
  VSyncBoundaryIterator i;
  return PercentilesHelper(&i, buckets_.data(),
                           buckets_.data() + buckets_.size(), total_samples_);
}

void VSyncHistogram::Reset() {
  total_samples_ = 0;
  buckets_.fill(0);
}

}  // namespace frame_metrics
}  // namespace ui
