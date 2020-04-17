// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/testing/fake_mdns_platform_service.h"

#include <algorithm>

#include "platform/api/logging.h"

namespace openscreen {

FakeMdnsPlatformService::FakeMdnsPlatformService() = default;
FakeMdnsPlatformService::~FakeMdnsPlatformService() = default;

std::vector<MdnsPlatformService::BoundInterface>
FakeMdnsPlatformService::RegisterInterfaces(
    const std::vector<platform::NetworkInterfaceIndex>& whitelist) {
  OSP_CHECK(registered_interfaces_.empty());
  if (whitelist.empty()) {
    registered_interfaces_ = interfaces_;
  } else {
    for (const auto& interface : interfaces_) {
      if (std::find(whitelist.begin(), whitelist.end(),
                    interface.interface_info.index) != whitelist.end()) {
        registered_interfaces_.push_back(interface);
      }
    }
  }
  return registered_interfaces_;
}

void FakeMdnsPlatformService::DeregisterInterfaces(
    const std::vector<BoundInterface>& interfaces) {
  for (const auto& interface : interfaces) {
    auto index = interface.interface_info.index;
    auto it = std::find_if(registered_interfaces_.begin(),
                           registered_interfaces_.end(),
                           [index](const BoundInterface& interface) {
                             return interface.interface_info.index == index;
                           });
    OSP_CHECK(it != registered_interfaces_.end())
        << "Must deregister a previously returned interface: " << index;
    registered_interfaces_.erase(it);
  }
}

}  // namespace openscreen
