// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/mock_host_connection_metrics_logger.h"

#include "chromeos/components/tether/active_host.h"
#include "chromeos/components/tether/ble_connection_manager.h"

namespace chromeos {

namespace tether {

MockHostConnectionMetricsLogger::MockHostConnectionMetricsLogger(
    BleConnectionManager* connection_manager,
    ActiveHost* active_host)
    : HostConnectionMetricsLogger(connection_manager, active_host) {}

MockHostConnectionMetricsLogger::~MockHostConnectionMetricsLogger() = default;

}  // namespace tether

}  // namespace chromeos
