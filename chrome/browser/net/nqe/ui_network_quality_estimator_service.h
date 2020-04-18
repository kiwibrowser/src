// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_NQE_UI_NETWORK_QUALITY_ESTIMATOR_SERVICE_H_
#define CHROME_BROWSER_NET_NQE_UI_NETWORK_QUALITY_ESTIMATOR_SERVICE_H_

#include <stdint.h>

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/nqe/cached_network_quality.h"
#include "net/nqe/effective_connection_type.h"
#include "net/nqe/network_id.h"
#include "net/nqe/network_quality_estimator.h"

class PrefRegistrySimple;
class Profile;

namespace net {
class EffectiveConnectionTypeObserver;
class NetworkQualitiesPrefsManager;
class RTTAndThroughputEstimatesObserver;
}

// UI service to determine the current EffectiveConnectionType.
class UINetworkQualityEstimatorService
    : public KeyedService,
      public net::NetworkQualityEstimator::NetworkQualityProvider {
 public:
  explicit UINetworkQualityEstimatorService(Profile* profile);
  ~UINetworkQualityEstimatorService() override;

  // NetworkQualityProvider implementation:
  // Must be called on the UI thread.
  net::EffectiveConnectionType GetEffectiveConnectionType() const override;
  void AddEffectiveConnectionTypeObserver(
      net::EffectiveConnectionTypeObserver* observer) override;
  void RemoveEffectiveConnectionTypeObserver(
      net::EffectiveConnectionTypeObserver* observer) override;
  base::Optional<base::TimeDelta> GetHttpRTT() const override;
  base::Optional<base::TimeDelta> GetTransportRTT() const override;
  base::Optional<int32_t> GetDownstreamThroughputKbps() const override;
  void AddRTTAndThroughputEstimatesObserver(
      net::RTTAndThroughputEstimatesObserver* observer) override;
  void RemoveRTTAndThroughputEstimatesObserver(
      net::RTTAndThroughputEstimatesObserver* observer) override;

  // Registers the profile-specific network quality estimator prefs.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  // Clear the network quality estimator prefs.
  void ClearPrefs();

  // Tests can manually set EffectiveConnectionType, but browser tests should
  // expect that the EffectiveConnectionType could change.
  void SetEffectiveConnectionTypeForTesting(net::EffectiveConnectionType type);

  // Reads the prefs from the disk, parses them into a map of NetworkIDs and
  // CachedNetworkQualities, and returns the map.
  std::map<net::nqe::internal::NetworkID,
           net::nqe::internal::CachedNetworkQuality>
  ForceReadPrefsForTesting() const;

 private:
  class IONetworkQualityObserver;

  // Notifies |observer| of the current effective connection type if |observer|
  // is still registered as an observer.
  void NotifyEffectiveConnectionTypeObserverIfPresent(
      net::EffectiveConnectionTypeObserver* observer) const;

  // Notifies |observer| of the current effective connection type if |observer|
  // is still registered as an observer.
  void NotifyRTTAndThroughputObserverIfPresent(
      net::RTTAndThroughputEstimatesObserver* observer) const;

  // KeyedService implementation:
  void Shutdown() override;

  // Called when the EffectiveConnectionType has updated to |type|.
  // NetworkQualityEstimator::EffectiveConnectionType is an estimate of the
  // quality of the network that may differ from the actual network type
  // reported by NetworkchangeNotifier::GetConnectionType.
  void EffectiveConnectionTypeChanged(net::EffectiveConnectionType type);

  // Called when the estimated HTTP RTT, estimated transport RTT or estimated
  // downstream throughput is computed.
  void RTTOrThroughputComputed(base::TimeDelta http_rtt,
                               base::TimeDelta transport_rtt,
                               int32_t downstream_throughput_kbps);

  // The current EffectiveConnectionType.
  net::EffectiveConnectionType type_;

  // Current network quality metrics.
  base::TimeDelta http_rtt_;
  base::TimeDelta transport_rtt_;
  int32_t downstream_throughput_kbps_;

  // IO thread based observer that is owned by this service. Created on the UI
  // thread, but used and deleted on the IO thread.
  std::unique_ptr<IONetworkQualityObserver> io_observer_;

  // Observer list for changes in effective connection type.
  base::ObserverList<net::EffectiveConnectionTypeObserver>
      effective_connection_type_observer_list_;

  // Observer list for changes in RTT or throughput values.
  base::ObserverList<net::RTTAndThroughputEstimatesObserver>
      rtt_throughput_observer_list_;

  // Prefs manager that is owned by this service. Created on the UI thread, but
  // used and deleted on the IO thread.
  std::unique_ptr<net::NetworkQualitiesPrefsManager> prefs_manager_;

  base::WeakPtrFactory<UINetworkQualityEstimatorService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(UINetworkQualityEstimatorService);
};

#endif  // CHROME_BROWSER_NET_NQE_UI_NETWORK_QUALITY_ESTIMATOR_SERVICE_H_
