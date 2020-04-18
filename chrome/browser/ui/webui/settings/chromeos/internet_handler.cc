// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/chromeos/internet_handler.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/values.h"
#include "chrome/browser/chromeos/options/network_config_view.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/tether/tether_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/common/net.mojom.h"
#include "components/arc/connection_holder.h"
#include "components/onc/onc_constants.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "extensions/browser/api/vpn_provider/vpn_service.h"
#include "extensions/browser/api/vpn_provider/vpn_service_factory.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/events/event_constants.h"

namespace chromeos {

namespace {

const char kAddNetworkMessage[] = "addNetwork";
const char kConfigureNetworkMessage[] = "configureNetwork";
const char kRequestArcVpnProviders[] = "requestArcVpnProviders";
const char kSendArcVpnProviders[] = "sendArcVpnProviders";
const char kRequestGmsCoreNotificationsDisabledDeviceNames[] =
    "requestGmsCoreNotificationsDisabledDeviceNames";
const char kSendGmsCoreNotificationsDisabledDeviceNames[] =
    "sendGmsCoreNotificationsDisabledDeviceNames";

std::string ServicePathFromGuid(const std::string& guid) {
  const NetworkState* network =
      NetworkHandler::Get()->network_state_handler()->GetNetworkStateFromGuid(
          guid);
  return network ? network->path() : "";
}

Profile* GetProfileForPrimaryUser() {
  return ProfileHelper::Get()->GetProfileByUser(
      user_manager::UserManager::Get()->GetPrimaryUser());
}

std::unique_ptr<base::DictionaryValue> ArcVpnProviderToValue(
    const app_list::ArcVpnProviderManager::ArcVpnProvider* arc_vpn_provider) {
  std::unique_ptr<base::DictionaryValue> serialized_entry =
      std::make_unique<base::DictionaryValue>();
  serialized_entry->SetString("PackageName", arc_vpn_provider->package_name);
  serialized_entry->SetString("ProviderName", arc_vpn_provider->app_name);
  serialized_entry->SetString("AppID", arc_vpn_provider->app_id);
  serialized_entry->SetDouble("LastLaunchTime",
                              arc_vpn_provider->last_launch_time.ToDoubleT());
  return serialized_entry;
}

}  // namespace

namespace settings {

InternetHandler::InternetHandler(Profile* profile) : profile_(profile) {
  DCHECK(profile_);

  arc_vpn_provider_manager_ = app_list::ArcVpnProviderManager::Get(profile_);
  if (arc_vpn_provider_manager_)
    arc_vpn_provider_manager_->AddObserver(this);

  TetherService* tether_service = TetherService::Get(profile);
  gms_core_notifications_state_tracker_ =
      tether_service ? tether_service->GetGmsCoreNotificationsStateTracker()
                     : nullptr;
  if (gms_core_notifications_state_tracker_)
    gms_core_notifications_state_tracker_->AddObserver(this);
}

InternetHandler::~InternetHandler() {
  if (arc_vpn_provider_manager_)
    arc_vpn_provider_manager_->RemoveObserver(this);
  if (gms_core_notifications_state_tracker_)
    gms_core_notifications_state_tracker_->RemoveObserver(this);
}

void InternetHandler::RegisterMessages() {
  // TODO(stevenjb): Eliminate once network configuration UI is integrated
  // into settings.
  web_ui()->RegisterMessageCallback(
      kAddNetworkMessage, base::BindRepeating(&InternetHandler::AddNetwork,
                                              base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kConfigureNetworkMessage,
      base::BindRepeating(&InternetHandler::ConfigureNetwork,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kRequestArcVpnProviders,
      base::BindRepeating(&InternetHandler::RequestArcVpnProviders,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      kRequestGmsCoreNotificationsDisabledDeviceNames,
      base::BindRepeating(
          &InternetHandler::RequestGmsCoreNotificationsDisabledDeviceNames,
          base::Unretained(this)));
}

void InternetHandler::OnJavascriptAllowed() {}

void InternetHandler::OnJavascriptDisallowed() {}

void InternetHandler::OnArcVpnProviderRemoved(const std::string& package_name) {
  if (arc_vpn_providers_.find(package_name) == arc_vpn_providers_.end())
    return;
  arc_vpn_providers_.erase(package_name);
  SendArcVpnProviders();
}

void InternetHandler::OnArcVpnProvidersRefreshed(
    const std::vector<
        std::unique_ptr<app_list::ArcVpnProviderManager::ArcVpnProvider>>&
        arc_vpn_providers) {
  SetArcVpnProviders(arc_vpn_providers);
}

void InternetHandler::OnArcVpnProviderUpdated(
    app_list::ArcVpnProviderManager::ArcVpnProvider* arc_vpn_provider) {
  arc_vpn_providers_[arc_vpn_provider->package_name] =
      ArcVpnProviderToValue(arc_vpn_provider);
  SendArcVpnProviders();
}

void InternetHandler::OnGmsCoreNotificationStateChanged() {
  SetGmsCoreNotificationsDisabledDeviceNames();
}

void InternetHandler::AddNetwork(const base::ListValue* args) {
  std::string onc_type;
  if (args->GetSize() < 1 || !args->GetString(0, &onc_type)) {
    NOTREACHED() << "Invalid args for: " << kAddNetworkMessage;
    return;
  }

  if (onc_type == ::onc::network_type::kVPN) {
    std::string app_id;
    if (args->GetSize() >= 2)
      args->GetString(1, &app_id);
    if (app_id.empty()) {
      // Show the "add network" dialog for the built-in OpenVPN/L2TP provider.
      NetworkConfigView::ShowForType(shill::kTypeVPN);
      return;
    }
    // Request to launch Arc VPN provider.
    const auto* arc_app_list_prefs = ArcAppListPrefs::Get(profile_);
    if (arc_app_list_prefs && arc_app_list_prefs->GetApp(app_id)) {
      arc::LaunchApp(profile_, app_id, ui::EF_NONE);
      return;
    }
    // Request that the third-party VPN provider identified by |provider_id|
    // show its "add network" dialog.
    VpnServiceFactory::GetForBrowserContext(GetProfileForPrimaryUser())
        ->SendShowAddDialogToExtension(app_id);
  } else if (onc_type == ::onc::network_type::kWiFi) {
    NetworkConfigView::ShowForType(shill::kTypeWifi);
  } else {
    LOG(ERROR) << "Unsupported type for: " << kAddNetworkMessage;
  }
}

void InternetHandler::ConfigureNetwork(const base::ListValue* args) {
  std::string guid;
  if (args->GetSize() < 1 || !args->GetString(0, &guid)) {
    NOTREACHED() << "Invalid args for: " << kConfigureNetworkMessage;
    return;
  }

  const std::string service_path = ServicePathFromGuid(guid);
  if (service_path.empty()) {
    LOG(ERROR) << "Network not found: " << guid;
    return;
  }

  const NetworkState* network =
      NetworkHandler::Get()->network_state_handler()->GetNetworkState(
          service_path);
  if (!network) {
    LOG(ERROR) << "Network not found with service_path: " << service_path;
    return;
  }

  if (network->type() == shill::kTypeVPN) {
    if (profile_ != GetProfileForPrimaryUser())
      return;

    if (network->vpn_provider_type() == shill::kProviderThirdPartyVpn) {
      // Request that the third-party VPN provider used by the |network| show a
      // configuration dialog for it.
      VpnServiceFactory::GetForBrowserContext(profile_)
          ->SendShowConfigureDialogToExtension(network->vpn_provider_id(),
                                               network->name());
      return;
    } else if (network->vpn_provider_type() == shill::kProviderArcVpn) {
      auto* net_instance = ARC_GET_INSTANCE_FOR_METHOD(
          arc::ArcServiceManager::Get()->arc_bridge_service()->net(),
          ConfigureAndroidVpn);
      if (!net_instance) {
        LOG(ERROR) << "User requested VPN configuration but API is unavailable";
        return;
      }
      net_instance->ConfigureAndroidVpn();
      return;
    }
  }

  NetworkConfigView::ShowForNetworkId(network->guid());
}

void InternetHandler::RequestArcVpnProviders(const base::ListValue* args) {
  if (!arc_vpn_provider_manager_)
    return;

  AllowJavascript();
  SetArcVpnProviders(arc_vpn_provider_manager_->GetArcVpnProviders());
}

void InternetHandler::RequestGmsCoreNotificationsDisabledDeviceNames(
    const base::ListValue* args) {
  AllowJavascript();
  SetGmsCoreNotificationsDisabledDeviceNames();
}

void InternetHandler::SetArcVpnProviders(
    const std::vector<
        std::unique_ptr<app_list::ArcVpnProviderManager::ArcVpnProvider>>&
        arc_vpn_providers) {
  arc_vpn_providers_.clear();
  for (const auto& arc_vpn_provider : arc_vpn_providers) {
    arc_vpn_providers_[arc_vpn_provider->package_name] =
        ArcVpnProviderToValue(arc_vpn_provider.get());
  }
  SendArcVpnProviders();
}

void InternetHandler::SendArcVpnProviders() {
  if (!IsJavascriptAllowed())
    return;

  base::ListValue arc_vpn_providers_value;
  for (const auto& iter : arc_vpn_providers_) {
    arc_vpn_providers_value.GetList().push_back(iter.second->Clone());
  }
  FireWebUIListener(kSendArcVpnProviders, arc_vpn_providers_value);
}

void InternetHandler::SetGmsCoreNotificationsDisabledDeviceNames() {
  if (!gms_core_notifications_state_tracker_) {
    // No device names should be present in the list if
    // |gms_core_notifications_state_tracker_| is null.
    DCHECK(device_names_without_notifications_.empty());
    return;
  }

  device_names_without_notifications_.clear();

  const std::vector<std::string> device_names =
      gms_core_notifications_state_tracker_
          ->GetGmsCoreNotificationsDisabledDeviceNames();
  for (const auto& device_name : device_names) {
    device_names_without_notifications_.emplace_back(
        std::make_unique<base::Value>(device_name));
  }
  SendGmsCoreNotificationsDisabledDeviceNames();
}

void InternetHandler::SendGmsCoreNotificationsDisabledDeviceNames() {
  if (!IsJavascriptAllowed())
    return;

  base::ListValue device_names_value;
  for (const auto& device_name : device_names_without_notifications_)
    device_names_value.GetList().push_back(device_name->Clone());

  FireWebUIListener(kSendGmsCoreNotificationsDisabledDeviceNames,
                    device_names_value);
}

gfx::NativeWindow InternetHandler::GetNativeWindow() const {
  return web_ui()->GetWebContents()->GetTopLevelNativeWindow();
}

void InternetHandler::SetGmsCoreNotificationsStateTrackerForTesting(
    chromeos::tether::GmsCoreNotificationsStateTracker*
        gms_core_notifications_state_tracker) {
  if (gms_core_notifications_state_tracker_)
    gms_core_notifications_state_tracker_->RemoveObserver(this);

  gms_core_notifications_state_tracker_ = gms_core_notifications_state_tracker;
  gms_core_notifications_state_tracker_->AddObserver(this);
}

}  // namespace settings

}  // namespace chromeos
