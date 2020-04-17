// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_PUBLIC_SERVICE_INFO_H_
#define OSP_PUBLIC_SERVICE_INFO_H_

#include <cstdint>
#include <string>

#include "osp_base/ip_address.h"
#include "osp_base/macros.h"
#include "platform/api/network_interface.h"

namespace openscreen {

// This contains canonical information about a specific Open Screen service
// found on the network via our discovery mechanism (mDNS).
struct ServiceInfo {
  ServiceInfo() = default;
  ServiceInfo(ServiceInfo&&) MAYBE_NOEXCEPT = default;
  ServiceInfo(const ServiceInfo&) MAYBE_NOEXCEPT = default;

  ServiceInfo& operator=(ServiceInfo&&) MAYBE_NOEXCEPT = default;
  ServiceInfo& operator=(const ServiceInfo&) MAYBE_NOEXCEPT = default;

  bool operator==(const ServiceInfo& other) const;
  bool operator!=(const ServiceInfo& other) const;

  bool Update(std::string friendly_name,
              platform::NetworkInterfaceIndex network_interface_index,
              const IPEndpoint& v4_endpoint,
              const IPEndpoint& v6_endpoint);

  // Identifier uniquely identifying the Open Screen service.
  std::string service_id;

  // User visible name of the Open Screen service in UTF-8.
  std::string friendly_name;

  // The index of the network interface that the screen was discovered on.
  platform::NetworkInterfaceIndex network_interface_index =
      platform::kInvalidNetworkInterfaceIndex;

  // The network endpoints to create a new connection to the Open Screen
  // service.
  IPEndpoint v4_endpoint;
  IPEndpoint v6_endpoint;
};

}  // namespace openscreen

#endif  // OSP_PUBLIC_SERVICE_INFO_H_
