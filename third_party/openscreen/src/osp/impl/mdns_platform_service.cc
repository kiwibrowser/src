// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/mdns_platform_service.h"

#include <cstring>

#include "platform/api/logging.h"

namespace openscreen {

MdnsPlatformService::BoundInterface::BoundInterface(
    const platform::InterfaceInfo& interface_info,
    const platform::IPSubnet& subnet,
    platform::UdpSocket* socket)
    : interface_info(interface_info), subnet(subnet), socket(socket) {
  OSP_DCHECK(socket);
}

MdnsPlatformService::BoundInterface::~BoundInterface() = default;

bool MdnsPlatformService::BoundInterface::operator==(
    const MdnsPlatformService::BoundInterface& other) const {
  if (interface_info != other.interface_info)
    return false;

  if (subnet.address != other.subnet.address ||
      subnet.prefix_length != other.subnet.prefix_length) {
    return false;
  }

  if (socket != other.socket)
    return false;

  return true;
}

bool MdnsPlatformService::BoundInterface::operator!=(
    const MdnsPlatformService::BoundInterface& other) const {
  return !(*this == other);
}

}  // namespace openscreen
