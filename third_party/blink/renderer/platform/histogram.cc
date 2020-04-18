// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/histogram.h"

#include "base/metrics/histogram.h"
#include "base/metrics/sparse_histogram.h"

namespace blink {

CustomCountHistogram::CustomCountHistogram(const char* name,
                                           base::HistogramBase::Sample min,
                                           base::HistogramBase::Sample max,
                                           int32_t bucket_count) {
  histogram_ = base::Histogram::FactoryGet(
      name, min, max, bucket_count,
      base::HistogramBase::kUmaTargetedHistogramFlag);
}

CustomCountHistogram::CustomCountHistogram(base::HistogramBase* histogram)
    : histogram_(histogram) {}

void CustomCountHistogram::Count(base::HistogramBase::Sample sample) {
  histogram_->Add(sample);
}

BooleanHistogram::BooleanHistogram(const char* name)
    : CustomCountHistogram(base::BooleanHistogram::FactoryGet(
          name,
          base::HistogramBase::kUmaTargetedHistogramFlag)) {}

EnumerationHistogram::EnumerationHistogram(
    const char* name,
    base::HistogramBase::Sample boundary_value)
    : CustomCountHistogram(base::LinearHistogram::FactoryGet(
          name,
          1,
          boundary_value,
          boundary_value + 1,
          base::HistogramBase::kUmaTargetedHistogramFlag)) {}

SparseHistogram::SparseHistogram(const char* name) {
  histogram_ = base::SparseHistogram::FactoryGet(
      name, base::HistogramBase::kUmaTargetedHistogramFlag);
}

void SparseHistogram::Sample(base::HistogramBase::Sample sample) {
  histogram_->Add(sample);
}

LinearHistogram::LinearHistogram(const char* name,
                                 base::HistogramBase::Sample min,
                                 base::HistogramBase::Sample max,
                                 int32_t bucket_count)
    : CustomCountHistogram(base::LinearHistogram::FactoryGet(
          name,
          min,
          max,
          bucket_count,
          base::HistogramBase::kUmaTargetedHistogramFlag)) {}

}  // namespace blink
