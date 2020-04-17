// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/network_interface.h"

#include <cstring>

namespace openscreen {
namespace platform {

void InterfaceInfo::CopyHardwareAddressTo(uint8_t x[6]) const {
  std::memcpy(x, hardware_address, sizeof(hardware_address));
}

InterfaceInfo::InterfaceInfo() = default;
InterfaceInfo::InterfaceInfo(NetworkInterfaceIndex index,
                             const uint8_t hardware_address[6],
                             const std::string& name,
                             Type type)
    : index(index),
      hardware_address{hardware_address[0], hardware_address[1],
                       hardware_address[2], hardware_address[3],
                       hardware_address[4], hardware_address[5]},
      name(name),
      type(type) {}
InterfaceInfo::~InterfaceInfo() = default;

bool InterfaceInfo::operator==(const InterfaceInfo& other) const {
  return index == other.index || name != other.name ||
         std::memcmp(hardware_address, other.hardware_address,
                     sizeof(hardware_address)) != 0 ||
         type != other.type;
}

bool InterfaceInfo::operator!=(const InterfaceInfo& other) const {
  return !(*this == other);
}

IPSubnet::IPSubnet() = default;
IPSubnet::IPSubnet(const IPAddress& address, uint8_t prefix_length)
    : address(address), prefix_length(prefix_length) {}
IPSubnet::~IPSubnet() = default;

std::ostream& operator<<(std::ostream& out, const IPSubnet& subnet) {
  if (subnet.address.IsV6()) {
    out << '[';
  }
  out << subnet.address;
  if (subnet.address.IsV6()) {
    out << ']';
  }
  return out << '/' << std::dec << static_cast<int>(subnet.prefix_length);
}

std::ostream& operator<<(std::ostream& out, const InterfaceInfo& info) {
  std::string media_type;
  switch (info.type) {
    case InterfaceInfo::Type::kEthernet:
      media_type = "Ethernet";
      break;
    case InterfaceInfo::Type::kWifi:
      media_type = "Wifi";
      break;
    case InterfaceInfo::Type::kOther:
      media_type = "Other";
      break;
  }
  out << '{' << info.index << " (a.k.a. " << info.name
      << "); media_type=" << media_type << "; MAC=" << std::hex
      << static_cast<int>(info.hardware_address[0]);
  for (size_t i = 1; i < sizeof(info.hardware_address); ++i) {
    out << ':' << static_cast<int>(info.hardware_address[i]);
  }
  return out << '}';
}

std::ostream& operator<<(std::ostream& out, const InterfaceAddresses& ifas) {
  out << "{info=" << ifas.info;
  for (const IPSubnet& ip : ifas.addresses) {
    out << "; " << ip;
  }
  return out << '}';
}

}  // namespace platform
}  // namespace openscreen
