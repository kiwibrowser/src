// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/testing/fake_mdns_platform_service.h"

#include <cstdint>

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace {

platform::UdpSocket* const kDefaultSocket =
    reinterpret_cast<platform::UdpSocket*>(static_cast<uintptr_t>(16));
platform::UdpSocket* const kSecondSocket =
    reinterpret_cast<platform::UdpSocket*>(static_cast<uintptr_t>(24));

class FakeMdnsPlatformServiceTest : public ::testing::Test {
 protected:
  const uint8_t mac1_[6] = {11, 22, 33, 44, 55, 66};
  const uint8_t mac2_[6] = {12, 23, 34, 45, 56, 67};
  std::vector<MdnsPlatformService::BoundInterface> bound_interfaces_{
      MdnsPlatformService::BoundInterface{
          platform::InterfaceInfo{1, mac1_, "eth0",
                                  platform::InterfaceInfo::Type::kEthernet},
          platform::IPSubnet{IPAddress{192, 168, 3, 2}, 24}, kDefaultSocket},
      MdnsPlatformService::BoundInterface{
          platform::InterfaceInfo{2, mac2_, "eth1",
                                  platform::InterfaceInfo::Type::kEthernet},
          platform::IPSubnet{
              IPAddress{1, 2, 3, 4, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 8}, 24},
          kSecondSocket}};
};

}  // namespace

TEST_F(FakeMdnsPlatformServiceTest, SimpleRegistration) {
  FakeMdnsPlatformService platform_service;
  std::vector<MdnsPlatformService::BoundInterface> bound_interfaces{
      bound_interfaces_[0]};

  platform_service.set_interfaces(bound_interfaces);

  auto registered_interfaces = platform_service.RegisterInterfaces({});
  EXPECT_EQ(bound_interfaces, registered_interfaces);
  platform_service.DeregisterInterfaces(registered_interfaces);

  registered_interfaces = platform_service.RegisterInterfaces({});
  EXPECT_EQ(bound_interfaces, registered_interfaces);
  platform_service.DeregisterInterfaces(registered_interfaces);
  platform_service.set_interfaces({});

  registered_interfaces = platform_service.RegisterInterfaces({});
  EXPECT_TRUE(registered_interfaces.empty());
  platform_service.DeregisterInterfaces(registered_interfaces);

  std::vector<MdnsPlatformService::BoundInterface> new_interfaces{
      bound_interfaces_[1]};

  platform_service.set_interfaces(new_interfaces);

  registered_interfaces = platform_service.RegisterInterfaces({});
  EXPECT_EQ(new_interfaces, registered_interfaces);
  platform_service.DeregisterInterfaces(registered_interfaces);
}

TEST_F(FakeMdnsPlatformServiceTest, ObeyIndexWhitelist) {
  FakeMdnsPlatformService platform_service;
  platform_service.set_interfaces(bound_interfaces_);

  auto eth0_only = platform_service.RegisterInterfaces({1});
  EXPECT_EQ(
      (std::vector<MdnsPlatformService::BoundInterface>{bound_interfaces_[0]}),
      eth0_only);
  platform_service.DeregisterInterfaces(eth0_only);

  auto eth1_only = platform_service.RegisterInterfaces({2});
  EXPECT_EQ(
      (std::vector<MdnsPlatformService::BoundInterface>{bound_interfaces_[1]}),
      eth1_only);
  platform_service.DeregisterInterfaces(eth1_only);

  auto both = platform_service.RegisterInterfaces({1, 2});
  EXPECT_EQ(bound_interfaces_, both);
  platform_service.DeregisterInterfaces(both);
}

TEST_F(FakeMdnsPlatformServiceTest, PartialDeregister) {
  FakeMdnsPlatformService platform_service;
  platform_service.set_interfaces(bound_interfaces_);

  auto both = platform_service.RegisterInterfaces({});
  std::vector<MdnsPlatformService::BoundInterface> eth0_only{
      bound_interfaces_[0]};
  std::vector<MdnsPlatformService::BoundInterface> eth1_only{
      bound_interfaces_[1]};
  platform_service.DeregisterInterfaces(eth0_only);
  platform_service.DeregisterInterfaces(eth1_only);
}

}  // namespace openscreen
