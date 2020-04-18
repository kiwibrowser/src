// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/vpn_list.h"

#include <algorithm>
#include <vector>

#include "ash/public/interfaces/vpn_list.mojom.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"

using ash::mojom::ThirdPartyVpnProvider;
using ash::mojom::ThirdPartyVpnProviderPtr;
using ash::mojom::ArcVpnProvider;
using ash::mojom::ArcVpnProviderPtr;

namespace ash {

namespace {

class TestVpnListObserver : public VpnList::Observer {
 public:
  TestVpnListObserver() = default;
  ~TestVpnListObserver() override = default;

  // VpnList::Observer:
  void OnVPNProvidersChanged() override { change_count_++; }

  int change_count_ = 0;
};

}  // namespace

using VpnListTest = testing::Test;

TEST_F(VpnListTest, BuiltInProvider) {
  VpnList vpn_list;

  // The VPN list should only contain the built-in provider.
  ASSERT_EQ(1u, vpn_list.extension_vpn_providers().size());
  VPNProvider provider = vpn_list.extension_vpn_providers()[0];
  EXPECT_EQ(provider.provider_type, VPNProvider::BUILT_IN_VPN);
  EXPECT_TRUE(provider.app_id.empty());
}

TEST_F(VpnListTest, ThirdPartyProviders) {
  VpnList vpn_list;

  // The VPN list should only contain the built-in provider.
  EXPECT_EQ(1u, vpn_list.extension_vpn_providers().size());

  // Add some third party (extension-backed) providers.
  std::vector<ThirdPartyVpnProviderPtr> third_party_providers;
  ThirdPartyVpnProviderPtr third_party1 = ThirdPartyVpnProvider::New();
  third_party1->name = "name1";
  third_party1->extension_id = "extension_id1";
  third_party_providers.push_back(std::move(third_party1));

  ThirdPartyVpnProviderPtr third_party2 = ThirdPartyVpnProvider::New();
  third_party2->name = "name2";
  third_party2->extension_id = "extension_id2";
  third_party_providers.push_back(std::move(third_party2));

  vpn_list.SetThirdPartyVpnProviders(std::move(third_party_providers));

  // Mojo types will be converted to internal ash types.
  VPNProvider extension_provider1 =
      VPNProvider::CreateThirdPartyVPNProvider("extension_id1", "name1");
  VPNProvider extension_provider2 =
      VPNProvider::CreateThirdPartyVPNProvider("extension_id2", "name2");

  // List contains the extension-backed providers. Order doesn't matter.
  std::vector<VPNProvider> extension_providers =
      vpn_list.extension_vpn_providers();
  EXPECT_EQ(3u, extension_providers.size());
  EXPECT_EQ(1u, std::count(extension_providers.begin(),
                           extension_providers.end(), extension_provider1));
  EXPECT_EQ(1u, std::count(extension_providers.begin(),
                           extension_providers.end(), extension_provider2));
}

TEST_F(VpnListTest, ArcProviders) {
  VpnList vpn_list;

  // Initial refresh.
  base::Time launchTime1 = base::Time::Now();
  std::vector<ArcVpnProviderPtr> arc_vpn_providers;
  ArcVpnProviderPtr arc_vpn_provider1 = ArcVpnProvider::New();
  arc_vpn_provider1->package_name = "package.name.foo1";
  arc_vpn_provider1->app_name = "ArcVPNMonster1";
  arc_vpn_provider1->app_id = "arc_app_id1";
  arc_vpn_provider1->last_launch_time = launchTime1;
  arc_vpn_providers.push_back(std::move(arc_vpn_provider1));

  vpn_list.SetArcVpnProviders(std::move(arc_vpn_providers));

  VPNProvider arc_provider1 = VPNProvider::CreateArcVPNProvider(
      "package.name.foo1", "ArcVPNMonster1", "arc_app_id1", launchTime1);

  std::vector<VPNProvider> arc_providers = vpn_list.arc_vpn_providers();
  EXPECT_EQ(1u, arc_providers.size());
  EXPECT_EQ(1u, std::count(arc_providers.begin(), arc_providers.end(),
                           arc_provider1));
  EXPECT_EQ(launchTime1, arc_providers[0].last_launch_time);

  // package.name.foo2 gets installed.
  ArcVpnProviderPtr arc_vpn_provider2 = ArcVpnProvider::New();
  arc_vpn_provider2->package_name = "package.name.foo2";
  arc_vpn_provider2->app_name = "ArcVPNMonster2";
  arc_vpn_provider2->app_id = "arc_app_id2";
  arc_vpn_provider2->last_launch_time = base::Time::Now();

  vpn_list.AddOrUpdateArcVPNProvider(std::move(arc_vpn_provider2));
  VPNProvider arc_provider2 = VPNProvider::CreateArcVPNProvider(
      "package.name.foo2", "ArcVPNMonster2", "arc_app_id2", base::Time::Now());

  arc_providers = vpn_list.arc_vpn_providers();
  EXPECT_EQ(2u, arc_providers.size());
  EXPECT_EQ(1u, std::count(arc_providers.begin(), arc_providers.end(),
                           arc_provider1));
  EXPECT_EQ(1u, std::count(arc_providers.begin(), arc_providers.end(),
                           arc_provider2));

  // package.name.foo1 gets uninstalled.
  vpn_list.RemoveArcVPNProvider("package.name.foo1");

  arc_providers = vpn_list.arc_vpn_providers();
  EXPECT_EQ(1u, arc_providers.size());
  EXPECT_EQ(1u, std::count(arc_providers.begin(), arc_providers.end(),
                           arc_provider2));

  // package.name.foo2 changes due to update or system language change.
  base::Time launchTime2 = base::Time::Now();
  ArcVpnProviderPtr arc_vpn_provider2_rename = ArcVpnProvider::New();
  arc_vpn_provider2_rename->package_name = "package.name.foo2";
  arc_vpn_provider2_rename->app_name = "ArcVPNMonster2Rename";
  arc_vpn_provider2_rename->app_id = "arc_app_id2_rename";
  arc_vpn_provider2_rename->last_launch_time = launchTime2;

  vpn_list.AddOrUpdateArcVPNProvider(std::move(arc_vpn_provider2_rename));
  arc_provider2.provider_name = "ArcVPNMonster2Rename";
  arc_provider2.app_id = "arc_app_id2_rename";

  arc_providers = vpn_list.arc_vpn_providers();
  EXPECT_EQ(1u, arc_providers.size());
  EXPECT_EQ(1u, std::count(arc_providers.begin(), arc_providers.end(),
                           arc_provider2));
  EXPECT_EQ(launchTime2, arc_providers[0].last_launch_time);
}

TEST_F(VpnListTest, Observers) {
  VpnList vpn_list;

  // Observers are not notified when they are added.
  TestVpnListObserver observer;
  vpn_list.AddObserver(&observer);
  EXPECT_EQ(0, observer.change_count_);

  // Add a third party (extension-backed) provider.
  std::vector<ThirdPartyVpnProviderPtr> third_party_providers;
  ThirdPartyVpnProviderPtr third_party1 = ThirdPartyVpnProvider::New();
  third_party1->name = "name1";
  third_party1->extension_id = "extension_id1";
  third_party_providers.push_back(std::move(third_party1));
  vpn_list.SetThirdPartyVpnProviders(std::move(third_party_providers));

  // Observer was notified.
  EXPECT_EQ(1, observer.change_count_);

  vpn_list.RemoveObserver(&observer);
}

}  // namespace ash
