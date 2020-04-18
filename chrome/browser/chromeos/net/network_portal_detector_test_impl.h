// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_NET_NETWORK_PORTAL_DETECTOR_TEST_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_NET_NETWORK_PORTAL_DETECTOR_TEST_IMPL_H_

#include <map>
#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "chromeos/network/portal_detector/network_portal_detector.h"

namespace chromeos {

class NetworkPortalDetectorTestImpl : public NetworkPortalDetector {
 public:
  NetworkPortalDetectorTestImpl();
  ~NetworkPortalDetectorTestImpl() override;

  void SetDefaultNetworkForTesting(const std::string& guid);
  void SetDetectionResultsForTesting(const std::string& guid,
                                     const CaptivePortalState& state);
  void NotifyObserversForTesting();

  // NetworkPortalDetector implementation:
  void AddObserver(Observer* observer) override;
  void AddAndFireObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  CaptivePortalState GetCaptivePortalState(
      const std::string& service_path) override;
  bool IsEnabled() override;
  void Enable(bool start_detection) override;
  bool StartPortalDetection(bool force) override;
  void SetStrategy(PortalDetectorStrategy::StrategyId id) override;

  PortalDetectorStrategy::StrategyId strategy_id() const {
    return strategy_id_;
  }

 private:
  using NetworkId = std::string;
  using CaptivePortalStateMap = std::map<NetworkId, CaptivePortalState>;

  base::ObserverList<Observer> observers_;
  std::unique_ptr<NetworkState> default_network_;
  CaptivePortalStateMap portal_state_map_;
  PortalDetectorStrategy::StrategyId strategy_id_;

  DISALLOW_COPY_AND_ASSIGN(NetworkPortalDetectorTestImpl);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_NET_NETWORK_PORTAL_DETECTOR_TEST_IMPL_H_
