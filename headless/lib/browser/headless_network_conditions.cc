// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/lib/browser/headless_network_conditions.h"

namespace headless {

HeadlessNetworkConditions::HeadlessNetworkConditions()
    : HeadlessNetworkConditions(false) {}

HeadlessNetworkConditions::HeadlessNetworkConditions(bool offline)
    : HeadlessNetworkConditions(offline, 0, 0, 0) {}

HeadlessNetworkConditions::HeadlessNetworkConditions(bool offline,
                                                     double latency,
                                                     double download_throughput,
                                                     double upload_throughput)
    : offline(offline),
      latency(latency),
      download_throughput(download_throughput),
      upload_throughput(upload_throughput) {}

HeadlessNetworkConditions::~HeadlessNetworkConditions() = default;

}  // namespace headless
