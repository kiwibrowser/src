// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_METRICS_METRICS_MOJO_SERVICE_H_
#define SERVICES_METRICS_METRICS_MOJO_SERVICE_H_

#include <memory>

#include "services/service_manager/public/cpp/service.h"

namespace metrics {

// Creates a metrics service implementation which records data in the current
// process. In order to capture any UKM data, the current process should have a
// UkmService object created and configured, so this should currently only be
// called in the browser process.
std::unique_ptr<service_manager::Service> CreateMetricsService();

}  // namespace metrics

#endif  // SERVICES_METRICS_METRICS_MOJO_SERVICE_H_
