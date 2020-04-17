// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_TESTING_FAKE_MDNS_PLATFORM_SERVICE_H_
#define OSP_IMPL_TESTING_FAKE_MDNS_PLATFORM_SERVICE_H_

#include <vector>

#include "osp/impl/mdns_platform_service.h"

namespace openscreen {

class FakeMdnsPlatformService final : public MdnsPlatformService {
 public:
  FakeMdnsPlatformService();
  ~FakeMdnsPlatformService() override;

  void set_interfaces(const std::vector<BoundInterface>& interfaces) {
    interfaces_ = interfaces;
  }

  // PlatformService overrides.
  std::vector<BoundInterface> RegisterInterfaces(
      const std::vector<platform::NetworkInterfaceIndex>&
          interface_index_whitelist) override;
  void DeregisterInterfaces(
      const std::vector<BoundInterface>& registered_interfaces) override;

 private:
  std::vector<BoundInterface> registered_interfaces_;
  std::vector<BoundInterface> interfaces_;
};

}  // namespace openscreen

#endif  // OSP_IMPL_TESTING_FAKE_MDNS_PLATFORM_SERVICE_H_
