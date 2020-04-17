// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_PUBLIC_NETWORK_METRICS_H_
#define OSP_PUBLIC_NETWORK_METRICS_H_

#include <cstdint>

#include "osp_base/time.h"

namespace openscreen {

// Holds a set of metrics, captured over a specific range of time, about the
// behavior of a network service running in the library.
struct NetworkMetrics {
  NetworkMetrics() = default;
  ~NetworkMetrics() = default;

  // The range of time over which the metrics were collected; end_timestamp >
  // start_timestamp
  timestamp_t start_timestamp = 0;
  timestamp_t end_timestamp = 0;

  // The number of packets and bytes sent over the timestamp range.
  uint64_t packets_sent = 0;
  uint64_t bytes_sent = 0;

  // The number of packets and bytes received over the timestamp range.
  uint64_t packets_received = 0;
  uint64_t bytes_received = 0;

  // The maximum number of connections over the timestamp range.  The
  // latter two fields break this down by connections to ipv4 and ipv6
  // endpoints.
  size_t max_connections = 0;
  size_t max_ipv4_connections = 0;
  size_t max_ipv6_connections = 0;
};

}  // namespace openscreen

#endif  // OSP_PUBLIC_NETWORK_METRICS_H_
