// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics_services_manager/metrics_services_manager_client.h"

namespace metrics_services_manager {

bool MetricsServicesManagerClient::IsMetricsReportingForceEnabled() {
  return false;
}

}  // namespace metrics_services_manager
