// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/sampling_metrics_provider.h"

#include "base/metrics/sparse_histogram.h"
#include "chrome/browser/metrics/chrome_metrics_services_manager_client.h"
#include "components/metrics/metrics_provider.h"

namespace metrics {

SamplingMetricsProvider::SamplingMetricsProvider() {}
SamplingMetricsProvider::~SamplingMetricsProvider() {}

void SamplingMetricsProvider::ProvideStabilityMetrics(
    SystemProfileProto* system_profile_proto) {
  int sample_rate;
  // Only log the sample rate if it's defined.
  if (ChromeMetricsServicesManagerClient::GetSamplingRatePerMille(
          &sample_rate)) {
    base::HistogramBase* histogram = base::SparseHistogram::FactoryGet(
        "UMA.SamplingRatePerMille",
        base::HistogramBase::kUmaStabilityHistogramFlag);
    histogram->Add(sample_rate);
  }
}

}  // namespace metrics
