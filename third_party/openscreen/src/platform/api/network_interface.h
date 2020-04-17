// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_NETWORK_INTERFACE_H_
#define PLATFORM_API_NETWORK_INTERFACE_H_

#include <string>
#include <vector>

#include "osp_base/ip_address.h"

namespace openscreen {
namespace platform {

// On Linux, these are a signed 32-bit integer. However, on Mac, they are an
// unsigned 32-bit integer. Thus, for cross-platform compatibility, and to
// provide a special "invalid/not set" value, define the type as a 64-bit signed
// integer.
using NetworkInterfaceIndex = int64_t;
enum : NetworkInterfaceIndex { kInvalidNetworkInterfaceIndex = -1 };

struct InterfaceInfo {
  enum class Type {
    kEthernet = 0,
    kWifi,
    kOther,
  };

  // TODO(btolsch): Only needed until c++14.
  InterfaceInfo();
  InterfaceInfo(NetworkInterfaceIndex index,
                const uint8_t hardware_address[6],
                const std::string& name,
                Type type);
  ~InterfaceInfo();

  bool operator==(const InterfaceInfo& other) const;
  bool operator!=(const InterfaceInfo& other) const;

  void CopyHardwareAddressTo(uint8_t x[6]) const;

  // Interface index, typically as specified by the operating system,
  // identifying this interface on the host machine.
  NetworkInterfaceIndex index = kInvalidNetworkInterfaceIndex;

  // MAC address of the interface.  All 0s if unavailable.
  uint8_t hardware_address[6] = {};

  // Interface name (e.g. eth0) if available.
  std::string name;

  // Hardware type of the interface.
  Type type = Type::kOther;
};

struct IPSubnet {
  // TODO(btolsch): Only needed until c++14.
  IPSubnet();
  IPSubnet(const IPAddress& address, uint8_t prefix_length);
  ~IPSubnet();

  IPAddress address;

  // Prefix length of |address|, which is another way of specifying a subnet
  // mask.  For example, 192.168.0.10/24 is a common representation of the
  // address 192.168.0.10 with a 24-bit prefix, and therefore a subnet mask of
  // 255.255.255.0.
  uint8_t prefix_length = 0;
};

struct InterfaceAddresses {
  InterfaceInfo info = {};

  // All IP addresses associated with the interface identified by |info|.
  std::vector<IPSubnet> addresses;
};

// Returns at most one InterfaceAddresses per interface, so there will be no
// duplicate InterfaceInfo entries.
std::vector<InterfaceAddresses> GetInterfaceAddresses();

// Output, for logging.
std::ostream& operator<<(std::ostream& out, const InterfaceInfo& info);
std::ostream& operator<<(std::ostream& out, const IPSubnet& subnet);
std::ostream& operator<<(std::ostream& out, const InterfaceAddresses& ifas);

}  // namespace platform
}  // namespace openscreen

#endif  // PLATFORM_API_NETWORK_INTERFACE_H_
