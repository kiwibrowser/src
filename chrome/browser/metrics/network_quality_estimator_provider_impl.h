// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_NETWORK_QUALITY_ESTIMATOR_PROVIDER_IMPL_H_
#define CHROME_BROWSER_METRICS_NETWORK_QUALITY_ESTIMATOR_PROVIDER_IMPL_H_

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/metrics/net/network_metrics_provider.h"

class IOThread;

namespace metrics {

// Implements NetworkMetricsProvider::NetworkQualityEstimatorProvider. Provides
// NetworkQualityEstimator by querying the IOThread. Lives on UI thread.
class NetworkQualityEstimatorProviderImpl
    : public NetworkMetricsProvider::NetworkQualityEstimatorProvider {
 public:
  explicit NetworkQualityEstimatorProviderImpl(IOThread* io_thread);
  ~NetworkQualityEstimatorProviderImpl() override;

 private:
  // NetworkMetricsProvider::NetworkQualityEstimatorProvider:
  scoped_refptr<base::SequencedTaskRunner> GetTaskRunner() override;
  void PostReplyNetworkQualityEstimator(
      base::Callback<void(net::NetworkQualityEstimator*)> io_callback) override;

  IOThread* io_thread_;

  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<NetworkQualityEstimatorProviderImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkQualityEstimatorProviderImpl);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_NETWORK_QUALITY_ESTIMATOR_PROVIDER_IMPL_H_
