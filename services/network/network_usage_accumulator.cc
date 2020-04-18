// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/network_usage_accumulator.h"

namespace network {

struct NetworkUsageAccumulator::NetworkUsageParam {
  int64_t total_bytes_received = 0;
  int64_t total_bytes_sent = 0;
};

NetworkUsageAccumulator::NetworkUsageAccumulator() = default;

NetworkUsageAccumulator::~NetworkUsageAccumulator() = default;

void NetworkUsageAccumulator::OnBytesTransferred(uint32_t process_id,
                                                 uint32_t routing_id,
                                                 int64_t bytes_received,
                                                 int64_t bytes_sent) {
  auto& entry = total_network_usages_[process_id][routing_id];
  entry.total_bytes_received += bytes_received;
  entry.total_bytes_sent += bytes_sent;
}

std::vector<mojom::NetworkUsagePtr>
NetworkUsageAccumulator::GetTotalNetworkUsages() const {
  std::vector<mojom::NetworkUsagePtr> total_network_usages;
  for (const auto& process_iter : total_network_usages_) {
    for (const auto& routing_iter : process_iter.second) {
      auto usage = mojom::NetworkUsage::New();
      usage->process_id = process_iter.first;
      usage->routing_id = routing_iter.first;
      usage->total_bytes_received = routing_iter.second.total_bytes_received;
      usage->total_bytes_sent = routing_iter.second.total_bytes_sent;
      total_network_usages.push_back(std::move(usage));
    }
  }
  return total_network_usages;
}

void NetworkUsageAccumulator::ClearBytesTransferredForProcess(
    uint32_t process_id) {
  total_network_usages_.erase(process_id);
}

}  // namespace network
