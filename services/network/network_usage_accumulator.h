// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_NETWORK_USAGE_ACCUMULATOR_H_
#define SERVICES_NETWORK_NETWORK_USAGE_ACCUMULATOR_H_

#include <map>

#include "base/component_export.h"
#include "base/containers/flat_map.h"
#include "base/containers/small_map.h"
#include "base/memory/weak_ptr.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace network {

// NetworkUsageAccumulator keeps track of total network usage for active
// consumers.
class COMPONENT_EXPORT(NETWORK_SERVICE) NetworkUsageAccumulator
    : public base::SupportsWeakPtr<NetworkUsageAccumulator> {
 public:
  NetworkUsageAccumulator();
  ~NetworkUsageAccumulator();

  // Reports raw network usage to this accumulator.
  void OnBytesTransferred(uint32_t process_id,
                          uint32_t routing_id,
                          int64_t bytes_received,
                          int64_t bytes_sent);

  // Marks a process as inactive and remove the entry.
  void ClearBytesTransferredForProcess(uint32_t process_id);

  // Returns the accumulated network usage for active consumers.
  std::vector<mojom::NetworkUsagePtr> GetTotalNetworkUsages() const;

 private:
  struct NetworkUsageParam;

  // Number of active processes is usually small, but could go as high as
  // |PhysicalMemoryMB / 80|.
  // See |RenderProcessHost::GetMaxRendererProcessCount()|.
  base::small_map<
      std::map<uint32_t /* process_id */,
               base::flat_map<uint32_t /* routing_id */, NetworkUsageParam>>>
      total_network_usages_;

  DISALLOW_COPY_AND_ASSIGN(NetworkUsageAccumulator);
};

}  // namespace network

#endif  // SERVICES_NETWORK_NETWORK_USAGE_ACCUMULATOR_H_
