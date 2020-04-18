// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/network/vpn_list.h"

#include <utility>

#include "base/logging.h"

namespace ash {

VPNProvider::VPNProvider() = default;

VPNProvider VPNProvider::CreateBuiltInVPNProvider() {
  VPNProvider vpn_provider;
  vpn_provider.provider_type = BUILT_IN_VPN;
  return vpn_provider;
}

VPNProvider VPNProvider::CreateThirdPartyVPNProvider(
    const std::string& extension_id,
    const std::string& third_party_provider_name) {
  DCHECK(!extension_id.empty());
  DCHECK(!third_party_provider_name.empty());

  VPNProvider vpn_provider;
  vpn_provider.provider_type = THIRD_PARTY_VPN;
  vpn_provider.app_id = extension_id;
  vpn_provider.provider_name = third_party_provider_name;
  return vpn_provider;
}

VPNProvider VPNProvider::CreateArcVPNProvider(
    const std::string& package_name,
    const std::string& app_name,
    const std::string& app_id,
    const base::Time last_launch_time) {
  DCHECK(!app_id.empty());
  DCHECK(!app_name.empty());
  DCHECK(!package_name.empty());
  DCHECK(!last_launch_time.is_null());

  VPNProvider vpn_provider;
  vpn_provider.provider_type = ARC_VPN;
  vpn_provider.app_id = app_id;
  vpn_provider.provider_name = app_name;
  vpn_provider.package_name = package_name;
  vpn_provider.last_launch_time = last_launch_time;
  return vpn_provider;
}

VPNProvider::VPNProvider(const VPNProvider& other) {
  provider_type = other.provider_type;
  app_id = other.app_id;
  provider_name = other.provider_name;
  package_name = other.package_name;
  last_launch_time = other.last_launch_time;
}

bool VPNProvider::operator==(const VPNProvider& other) const {
  return provider_type == other.provider_type && app_id == other.app_id &&
         provider_name == other.provider_name &&
         package_name == other.package_name;
}

VpnList::Observer::~Observer() = default;

VpnList::VpnList() {
  AddBuiltInProvider();
}

VpnList::~VpnList() = default;

bool VpnList::HaveThirdPartyOrArcVPNProviders() const {
  for (const VPNProvider& extension_provider : extension_vpn_providers_) {
    if (extension_provider.provider_type == VPNProvider::THIRD_PARTY_VPN)
      return true;
  }
  return arc_vpn_providers_.size() > 0;
}

void VpnList::AddObserver(Observer* observer) {
  observer_list_.AddObserver(observer);
}

void VpnList::RemoveObserver(Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void VpnList::BindRequest(mojom::VpnListRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void VpnList::SetThirdPartyVpnProviders(
    std::vector<mojom::ThirdPartyVpnProviderPtr> providers) {
  extension_vpn_providers_.clear();
  extension_vpn_providers_.reserve(providers.size() + 1);
  // Add the OpenVPN provider.
  AddBuiltInProvider();
  // Append the extension-backed providers.
  for (const auto& provider : providers) {
    extension_vpn_providers_.push_back(VPNProvider::CreateThirdPartyVPNProvider(
        provider->extension_id, provider->name));
  }
  NotifyObservers();
}

void VpnList::SetArcVpnProviders(
    std::vector<mojom::ArcVpnProviderPtr> arc_providers) {
  arc_vpn_providers_.clear();
  arc_vpn_providers_.reserve(arc_providers.size());

  for (const auto& arc_provider : arc_providers) {
    arc_vpn_providers_.push_back(VPNProvider::CreateArcVPNProvider(
        arc_provider->package_name, arc_provider->app_name,
        arc_provider->app_id, arc_provider->last_launch_time));
  }
  NotifyObservers();
}

void VpnList::AddOrUpdateArcVPNProvider(mojom::ArcVpnProviderPtr arc_provider) {
  bool provider_found = false;
  for (auto& arc_vpn_provider : arc_vpn_providers_) {
    if (arc_vpn_provider.package_name == arc_provider->package_name) {
      arc_vpn_provider.provider_name = arc_provider->app_name;
      arc_vpn_provider.app_id = arc_provider->app_id;
      arc_vpn_provider.last_launch_time = arc_provider->last_launch_time;
      provider_found = true;
      break;
    }
  }
  // This is a newly install ArcVPNProvider.
  if (!provider_found) {
    arc_vpn_providers_.push_back(VPNProvider::CreateArcVPNProvider(
        arc_provider->package_name, arc_provider->app_name,
        arc_provider->app_id, arc_provider->last_launch_time));
  }
  NotifyObservers();
}

void VpnList::RemoveArcVPNProvider(const std::string& package_name) {
  for (auto iter = arc_vpn_providers_.begin(); iter != arc_vpn_providers_.end();
       iter++) {
    if (iter->package_name == package_name) {
      arc_vpn_providers_.erase(iter);
      NotifyObservers();
      break;
    }
  }
}

void VpnList::NotifyObservers() {
  for (auto& observer : observer_list_)
    observer.OnVPNProvidersChanged();
}

void VpnList::AddBuiltInProvider() {
  // The VPNProvider() constructor generates the built-in provider and has no
  // extension ID.
  extension_vpn_providers_.push_back(VPNProvider::CreateBuiltInVPNProvider());
}

}  // namespace ash
