// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/discovery_network_monitor_metrics.h"

#include "base/metrics/histogram_macros.h"

namespace media_router {

DiscoveryNetworkMonitorMetrics::~DiscoveryNetworkMonitorMetrics() {}

void DiscoveryNetworkMonitorMetrics::RecordTimeBetweenNetworkChangeEvents(
    base::TimeDelta delta) {
  UMA_HISTOGRAM_MEDIUM_TIMES(
      "MediaRouter.NetworkMonitor.NetworkChangeEventDelta", delta);
}

void DiscoveryNetworkMonitorMetrics::RecordConnectionType(
    DiscoveryNetworkMonitorConnectionType connection_type) {
  DCHECK_LT(connection_type,
            DiscoveryNetworkMonitorConnectionType::kTotalCount);
  UMA_HISTOGRAM_ENUMERATION("MediaRouter.NetworkMonitor.ConnectionType",
                            connection_type,
                            DiscoveryNetworkMonitorConnectionType::kTotalCount);
}

}  // namespace media_router
