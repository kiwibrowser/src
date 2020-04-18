// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_GPU_GPU_METRICS_PROVIDER_H_
#define COMPONENTS_METRICS_GPU_GPU_METRICS_PROVIDER_H_

#include "base/macros.h"
#include "components/metrics/metrics_provider.h"

namespace metrics {

// GPUMetricsProvider provides GPU-related metrics.
class GPUMetricsProvider : public MetricsProvider {
 public:
  GPUMetricsProvider();
  ~GPUMetricsProvider() override;

  // MetricsProvider:
  void ProvideSystemProfileMetrics(
      SystemProfileProto* system_profile_proto) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GPUMetricsProvider);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_GPU_GPU_METRICS_PROVIDER_H_
