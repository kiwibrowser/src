// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_NQE_UI_NETWORK_QUALITY_ESTIMATOR_SERVICE_TEST_UTIL_H_
#define CHROME_BROWSER_NET_NQE_UI_NETWORK_QUALITY_ESTIMATOR_SERVICE_TEST_UTIL_H_

#include "base/time/time.h"
#include "net/nqe/effective_connection_type.h"

namespace nqe_test_util {

// Forces NetworkQualityEstimator to report |type| to all its
// EffectiveConnectionTypeObservers.
// This blocks execution on the calling thread until the IO task runs and
// replies.
void OverrideEffectiveConnectionTypeAndWait(net::EffectiveConnectionType type);

// Forces NetworkQualityEstimator to report |rtt| to all its
// RTTAndThroughputEstimatesObservers.
// This blocks execution on the calling thread until the IO task runs and
// replies.
void OverrideRTTsAndWait(base::TimeDelta rtt);

}  // namespace nqe_test_util

#endif  // CHROME_BROWSER_NET_NQE_UI_NETWORK_QUALITY_ESTIMATOR_SERVICE_TEST_UTIL_H_
