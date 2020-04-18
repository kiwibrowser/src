// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/features.h"

namespace network {
namespace features {

const base::Feature kNetworkErrorLogging{"NetworkErrorLogging",
                                         base::FEATURE_DISABLED_BY_DEFAULT};
// Enables the network service.
const base::Feature kNetworkService{"NetworkService",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

// Out of Blink CORS
const base::Feature kOutOfBlinkCORS{"OutOfBlinkCORS",
                                    base::FEATURE_DISABLED_BY_DEFAULT};

// Port some content::ResourceScheduler functionalities to renderer.
const base::Feature kRendererSideResourceScheduler{
    "RendererSideResourceScheduler", base::FEATURE_DISABLED_BY_DEFAULT};

const base::Feature kReporting{"Reporting", base::FEATURE_DISABLED_BY_DEFAULT};

// Based on the field trial parameters, this feature will override the value of
// the maximum number of delayable requests allowed in flight. The number of
// delayable requests allowed in flight will be based on the network's
// effective connection type ranges and the
// corresponding number of delayable requests in flight specified in the
// experiment configuration. Based on field trial parameters, this experiment
// may also throttle delayable requests based on the number of non-delayable
// requests in-flight times a weighting factor.
const base::Feature kThrottleDelayable{"ThrottleDelayable",
                                       base::FEATURE_ENABLED_BY_DEFAULT};

// When kPriorityRequestsDelayableOnSlowConnections is enabled, HTTP
// requests fetched from a SPDY/QUIC/H2 proxies can be delayed by the
// ResourceScheduler just as HTTP/1.1 resources are. However, requests from such
// servers are not subject to kMaxNumDelayableRequestsPerHostPerClient limit.
const base::Feature kDelayRequestsOnMultiplexedConnections{
    "DelayRequestsOnMultiplexedConnections", base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
}  // namespace network
