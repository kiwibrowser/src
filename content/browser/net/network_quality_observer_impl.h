// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NET_NETWORK_QUALITY_OBSERVER_IMPL_H_
#define CONTENT_BROWSER_NET_NETWORK_QUALITY_OBSERVER_IMPL_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/public/browser/network_quality_observer_factory.h"
#include "net/nqe/effective_connection_type.h"
#include "net/nqe/effective_connection_type_observer.h"
#include "net/nqe/network_quality.h"
#include "net/nqe/rtt_throughput_estimates_observer.h"

namespace net {
class NetworkQualityEstimator;
}

namespace content {

// Listens for changes to the network quality and manages sending updates to
// each RenderProcess via mojo.
class CONTENT_EXPORT NetworkQualityObserverImpl
    : public net::EffectiveConnectionTypeObserver,
      public net::RTTAndThroughputEstimatesObserver {
 public:
  explicit NetworkQualityObserverImpl(
      net::NetworkQualityEstimator* network_quality_estimator);

  ~NetworkQualityObserverImpl() override;

 private:
  class UiThreadObserver;

  // net::EffectiveConnectionTypeObserver implementation:
  void OnEffectiveConnectionTypeChanged(
      net::EffectiveConnectionType type) override;

  // net::RTTAndThroughputEstimatesObserver implementation:
  void OnRTTOrThroughputEstimatesComputed(
      base::TimeDelta http_rtt,
      base::TimeDelta transport_rtt,
      int32_t downstream_throughput_kbps) override;

  // |ui_thread_observer_| is owned by |this|, and interacts with
  // the render processes. It is created on the IO thread but afterwards, should
  // only be accessed on the UI thread. |ui_thread_observer_| is guaranteed to
  // be non-null during the lifetime of |this|.
  std::unique_ptr<UiThreadObserver> ui_thread_observer_;

  // |network_quality_estimator_| is guaranteed to be non-null during the
  // lifetime of |this|.
  net::NetworkQualityEstimator* network_quality_estimator_;

  //  The network quality when the |ui_thread_observer_| was last notified.
  net::EffectiveConnectionType last_notified_type_;
  net::nqe::internal::NetworkQuality last_notified_network_quality_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(NetworkQualityObserverImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_NET_NETWORK_QUALITY_OBSERVER_IMPL_H_