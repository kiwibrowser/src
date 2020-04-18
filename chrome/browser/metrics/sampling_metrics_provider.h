// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_SAMPLING_METRICS_PROVIDER_H_
#define CHROME_BROWSER_METRICS_SAMPLING_METRICS_PROVIDER_H_

#include "components/metrics/metrics_provider.h"

namespace metrics {

// Provides metrics related to sampling of metrics reporting clients. In
// particular, the rate at which clients are sampled.
class SamplingMetricsProvider : public MetricsProvider {
 public:
  SamplingMetricsProvider();
  ~SamplingMetricsProvider() override;

 private:
  // MetricsProvider:
  void ProvideStabilityMetrics(
      SystemProfileProto* system_profile_proto) override;

  DISALLOW_COPY_AND_ASSIGN(SamplingMetricsProvider);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_SAMPLING_METRICS_PROVIDER_H_
