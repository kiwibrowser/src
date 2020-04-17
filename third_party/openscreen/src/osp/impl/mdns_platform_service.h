// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_MDNS_PLATFORM_SERVICE_H_
#define OSP_IMPL_MDNS_PLATFORM_SERVICE_H_

#include <vector>

#include "platform/api/event_waiter.h"
#include "platform/api/network_interface.h"
#include "platform/api/udp_socket.h"
#include "platform/base/event_loop.h"

namespace openscreen {

class MdnsPlatformService {
 public:
  struct BoundInterface {
    BoundInterface(const platform::InterfaceInfo& interface_info,
                   const platform::IPSubnet& subnet,
                   platform::UdpSocket* socket);
    ~BoundInterface();

    bool operator==(const BoundInterface& other) const;
    bool operator!=(const BoundInterface& other) const;

    platform::InterfaceInfo interface_info;
    platform::IPSubnet subnet;
    platform::UdpSocket* socket;
  };

  virtual ~MdnsPlatformService() = default;

  virtual std::vector<BoundInterface> RegisterInterfaces(
      const std::vector<platform::NetworkInterfaceIndex>& whitelist) = 0;
  virtual void DeregisterInterfaces(
      const std::vector<BoundInterface>& registered_interfaces) = 0;
};

}  // namespace openscreen

#endif  // OSP_IMPL_MDNS_PLATFORM_SERVICE_H_
