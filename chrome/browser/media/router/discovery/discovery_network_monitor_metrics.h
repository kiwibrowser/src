// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DISCOVERY_NETWORK_MONITOR_METRICS_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DISCOVERY_NETWORK_MONITOR_METRICS_H_

#include "base/time/time.h"

namespace media_router {

// This enum indicates the state of the host's network connectivity according to
// both net::NetworkChangeNotifier and DiscoveryNetworkMonitor.  If
// DiscoveryNetworkMonitor reports the network identifier is unknown, we would
// also like to record what net::NetworkChangeNotifier says about the connection
// type.
//
// It is tied to the UMA histogram MediaRouter.NetworkMonitor.ConnectionType so
// new values should only be added at the end, but before kTotalCount.
enum class DiscoveryNetworkMonitorConnectionType {
  // net::NetworkChangeNotifier reports the connection type is Wifi and we have
  // a valid network identifier from DiscoveryNetworkMonitor.
  kWifi = 0,
  // net::NetworkChangeNotifier reports the connection type is ethernet and we
  // have a valid network identifier from DiscoveryNetworkMonitor.
  kEthernet = 1,
  // net::NetworkChangeNotifier reports the connection type is Wifi but we don't
  // have a valid network identifier from DiscoveryNetworkMonitor.
  kUnknownReportedAsWifi = 2,
  // net::NetworkChangeNotifier reports the connection type is ethernet but we
  // don't have a valid network identifier from DiscoveryNetworkMonitor.
  kUnknownReportedAsEthernet = 3,
  // net::NetworkChangeNotifier reports the connection type is something other
  // than Wifi and ethernet and we don't have a valid network identifier from
  // DiscoveryNetworkMonitor.
  kUnknownReportedAsOther = 4,
  // net::NetworkChangeNotifier reports the connection type is unknown and we
  // don't have a valid network identifier from DiscoveryNetworkMonitor.
  kUnknown = 5,
  // DiscoveryNetworkMonitor reports that the host is disconnected from all
  // networks.  This comes directly from net::NetworkChangeNotifier so there is
  // no potential inconsistency to record.
  kDisconnected = 6,

  // NOTE: Add entries only immediately above this line.
  kTotalCount = 7,
};

class DiscoveryNetworkMonitorMetrics {
 public:
  virtual ~DiscoveryNetworkMonitorMetrics();

  // This records the time since the last network change event when
  // DiscoveryNetworkMonitor emits a new network change event to its observers.
  virtual void RecordTimeBetweenNetworkChangeEvents(base::TimeDelta delta);

  // This records the connection type as reported by both
  // DiscoveryNetworkMonitor and net::NetworkChangeNotifier.
  virtual void RecordConnectionType(
      DiscoveryNetworkMonitorConnectionType connection_type);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DISCOVERY_NETWORK_MONITOR_METRICS_H_
