// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/net/arc_net_host_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/posix/eintr_wrapper.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chromeos/login/login_state.h"
#include "chromeos/network/managed_network_configuration_handler.h"
#include "chromeos/network/network_connection_handler.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_type_pattern.h"
#include "chromeos/network/network_util.h"
#include "chromeos/network/onc/onc_utils.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_browser_context_keyed_service_factory_base.h"
#include "components/arc/arc_features.h"
#include "components/user_manager/user_manager.h"
#include "third_party/cros_system_api/dbus/shill/dbus-constants.h"

namespace {

constexpr int kGetNetworksListLimit = 100;

chromeos::NetworkStateHandler* GetStateHandler() {
  return chromeos::NetworkHandler::Get()->network_state_handler();
}

chromeos::ManagedNetworkConfigurationHandler* GetManagedConfigurationHandler() {
  return chromeos::NetworkHandler::Get()
      ->managed_network_configuration_handler();
}

chromeos::NetworkConnectionHandler* GetNetworkConnectionHandler() {
  return chromeos::NetworkHandler::Get()->network_connection_handler();
}

bool IsDeviceOwner() {
  // Check whether the logged-in Chrome OS user is allowed to add or remove WiFi
  // networks. The user account state changes immediately after boot. There is a
  // small window when this may return an incorrect state. However, after things
  // settle down this is guranteed to reflect the correct user account state.
  return user_manager::UserManager::Get()->GetActiveUser()->GetAccountId() ==
         user_manager::UserManager::Get()->GetOwnerAccountId();
}

std::string GetStringFromOncDictionary(const base::DictionaryValue* dict,
                                       const char* key,
                                       bool required) {
  std::string value;
  dict->GetString(key, &value);
  if (required && value.empty())
    NOTREACHED() << "Required parameter " << key << " was not found.";
  return value;
}

arc::mojom::SecurityType TranslateONCWifiSecurityType(
    const base::DictionaryValue* dict) {
  std::string type = GetStringFromOncDictionary(dict, onc::wifi::kSecurity,
                                                true /* required */);
  if (type == onc::wifi::kWEP_PSK)
    return arc::mojom::SecurityType::WEP_PSK;
  if (type == onc::wifi::kWEP_8021X)
    return arc::mojom::SecurityType::WEP_8021X;
  if (type == onc::wifi::kWPA_PSK)
    return arc::mojom::SecurityType::WPA_PSK;
  if (type == onc::wifi::kWPA_EAP)
    return arc::mojom::SecurityType::WPA_EAP;
  return arc::mojom::SecurityType::NONE;
}

arc::mojom::WiFiPtr TranslateONCWifi(const base::DictionaryValue* dict) {
  arc::mojom::WiFiPtr wifi = arc::mojom::WiFi::New();

  // Optional; defaults to 0.
  dict->GetInteger(onc::wifi::kFrequency, &wifi->frequency);

  wifi->bssid =
      GetStringFromOncDictionary(dict, onc::wifi::kBSSID, false /* required */);
  wifi->hex_ssid = GetStringFromOncDictionary(dict, onc::wifi::kHexSSID,
                                              true /* required */);

  // Optional; defaults to false.
  dict->GetBoolean(onc::wifi::kHiddenSSID, &wifi->hidden_ssid);

  wifi->security = TranslateONCWifiSecurityType(dict);

  // Optional; defaults to 0.
  dict->GetInteger(onc::wifi::kSignalStrength, &wifi->signal_strength);

  return wifi;
}

std::vector<std::string> TranslateStringArray(const base::ListValue* list) {
  std::vector<std::string> strings;

  for (size_t i = 0; i < list->GetSize(); i++) {
    std::string value;
    list->GetString(i, &value);
    DCHECK(!value.empty());
    strings.push_back(value);
  }

  return strings;
}

arc::mojom::IPConfigurationPtr TranslateONCIPConfig(
    const base::DictionaryValue* ip_dict) {
  arc::mojom::IPConfigurationPtr configuration =
      arc::mojom::IPConfiguration::New();

  if (ip_dict->FindKey(onc::ipconfig::kIPAddress)) {
    configuration->ip_address = GetStringFromOncDictionary(
        ip_dict, onc::ipconfig::kIPAddress, true /* required */);
    if (!ip_dict->GetInteger(onc::ipconfig::kRoutingPrefix,
                             &configuration->routing_prefix)) {
      NOTREACHED();
    }
    configuration->gateway = GetStringFromOncDictionary(
        ip_dict, onc::ipconfig::kGateway, true /* required */);
  }

  const base::ListValue* dns_list;
  if (ip_dict->GetList(onc::ipconfig::kNameServers, &dns_list))
    configuration->name_servers = TranslateStringArray(dns_list);

  std::string type = GetStringFromOncDictionary(ip_dict, onc::ipconfig::kType,
                                                true /* required */);
  configuration->type = type == onc::ipconfig::kIPv6
                            ? arc::mojom::IPAddressType::IPV6
                            : arc::mojom::IPAddressType::IPV4;

  configuration->web_proxy_auto_discovery_url = GetStringFromOncDictionary(
      ip_dict, onc::ipconfig::kWebProxyAutoDiscoveryUrl, false /* required */);

  return configuration;
}

std::vector<arc::mojom::IPConfigurationPtr> TranslateONCIPConfigs(
    const base::ListValue* list) {
  std::vector<arc::mojom::IPConfigurationPtr> configs;

  for (size_t i = 0; i < list->GetSize(); i++) {
    const base::DictionaryValue* ip_dict = nullptr;

    list->GetDictionary(i, &ip_dict);
    DCHECK(ip_dict);
    configs.push_back(TranslateONCIPConfig(ip_dict));
  }
  return configs;
}

arc::mojom::ConnectionStateType TranslateONCConnectionState(
    const base::DictionaryValue* dict) {
  std::string connection_state = GetStringFromOncDictionary(
      dict, onc::network_config::kConnectionState, false /* required */);

  if (connection_state == onc::connection_state::kConnected)
    return arc::mojom::ConnectionStateType::CONNECTED;
  if (connection_state == onc::connection_state::kConnecting)
    return arc::mojom::ConnectionStateType::CONNECTING;
  return arc::mojom::ConnectionStateType::NOT_CONNECTED;
}

void TranslateONCNetworkTypeDetails(const base::DictionaryValue* dict,
                                    arc::mojom::NetworkConfiguration* mojo) {
  std::string type = GetStringFromOncDictionary(
      dict, onc::network_config::kType, true /* required */);
  if (type == onc::network_type::kCellular) {
    mojo->type = arc::mojom::NetworkType::CELLULAR;
  } else if (type == onc::network_type::kEthernet) {
    mojo->type = arc::mojom::NetworkType::ETHERNET;
  } else if (type == onc::network_type::kVPN) {
    mojo->type = arc::mojom::NetworkType::VPN;
  } else if (type == onc::network_type::kWiFi) {
    mojo->type = arc::mojom::NetworkType::WIFI;

    const base::DictionaryValue* wifi_dict = nullptr;
    dict->GetDictionary(onc::network_config::kWiFi, &wifi_dict);
    DCHECK(wifi_dict);
    mojo->wifi = TranslateONCWifi(wifi_dict);
  } else if (type == onc::network_type::kWimax) {
    mojo->type = arc::mojom::NetworkType::WIMAX;
  } else {
    NOTREACHED();
  }
}

arc::mojom::NetworkConfigurationPtr TranslateONCConfiguration(
    const base::DictionaryValue* dict) {
  arc::mojom::NetworkConfigurationPtr mojo =
      arc::mojom::NetworkConfiguration::New();

  mojo->connection_state = TranslateONCConnectionState(dict);

  mojo->guid = GetStringFromOncDictionary(dict, onc::network_config::kGUID,
                                          true /* required */);

  // crbug.com/761708 - VPNs do not currently have an IPConfigs array,
  // so in order to fetch the parameters (particularly the DNS server list),
  // fall back to StaticIPConfig or SavedIPConfig.
  const base::ListValue* ip_config_list = nullptr;
  const base::DictionaryValue* ip_dict = nullptr;
  if (dict->GetList(onc::network_config::kIPConfigs, &ip_config_list)) {
    mojo->ip_configs = TranslateONCIPConfigs(ip_config_list);
  } else if (dict->GetDictionary(onc::network_config::kStaticIPConfig,
                                 &ip_dict) ||
             dict->GetDictionary(onc::network_config::kSavedIPConfig,
                                 &ip_dict)) {
    std::vector<arc::mojom::IPConfigurationPtr> configs;
    configs.push_back(TranslateONCIPConfig(ip_dict));
    mojo->ip_configs = std::move(configs);
  }

  mojo->guid = GetStringFromOncDictionary(dict, onc::network_config::kGUID,
                                          true /* required */);
  mojo->mac_address = GetStringFromOncDictionary(
      dict, onc::network_config::kMacAddress, false /* required */);
  TranslateONCNetworkTypeDetails(dict, mojo.get());

  return mojo;
}

const chromeos::NetworkState* GetShillBackedNetwork(
    const chromeos::NetworkState* network) {
  if (!network)
    return nullptr;

  // Non-Tether networks are already backed by Shill.
  if (!chromeos::NetworkTypePattern::Tether().MatchesType(network->type()))
    return network;

  // Tether networks which are not connected are also not backed by Shill.
  if (!network->IsConnectedState())
    return nullptr;

  // Connected Tether networks delegate to an underlying Wi-Fi network.
  DCHECK(!network->tether_guid().empty());
  return GetStateHandler()->GetNetworkStateFromGuid(network->tether_guid());
}

void ForgetNetworkSuccessCallback(
    base::OnceCallback<void(arc::mojom::NetworkResult)> callback) {
  std::move(callback).Run(arc::mojom::NetworkResult::SUCCESS);
}

void ForgetNetworkFailureCallback(
    base::OnceCallback<void(arc::mojom::NetworkResult)> callback,
    const std::string& error_name,
    std::unique_ptr<base::DictionaryValue> error_data) {
  VLOG(1) << "ForgetNetworkFailureCallback: " << error_name;
  std::move(callback).Run(arc::mojom::NetworkResult::FAILURE);
}

void StartConnectSuccessCallback(
    base::OnceCallback<void(arc::mojom::NetworkResult)> callback) {
  std::move(callback).Run(arc::mojom::NetworkResult::SUCCESS);
}

void StartConnectFailureCallback(
    base::OnceCallback<void(arc::mojom::NetworkResult)> callback,
    const std::string& error_name,
    std::unique_ptr<base::DictionaryValue> error_data) {
  VLOG(1) << "StartConnectFailureCallback: " << error_name;
  std::move(callback).Run(arc::mojom::NetworkResult::FAILURE);
}

void StartDisconnectSuccessCallback(
    base::OnceCallback<void(arc::mojom::NetworkResult)> callback) {
  std::move(callback).Run(arc::mojom::NetworkResult::SUCCESS);
}

void StartDisconnectFailureCallback(
    base::OnceCallback<void(arc::mojom::NetworkResult)> callback,
    const std::string& error_name,
    std::unique_ptr<base::DictionaryValue> error_data) {
  VLOG(1) << "StartDisconnectFailureCallback: " << error_name;
  std::move(callback).Run(arc::mojom::NetworkResult::FAILURE);
}

void GetDefaultNetworkSuccessCallback(
    base::OnceCallback<void(arc::mojom::NetworkConfigurationPtr,
                            arc::mojom::NetworkConfigurationPtr)> callback,
    const std::string& service_path,
    const base::DictionaryValue& dictionary) {
  // TODO(cernekee): Figure out how to query Chrome for the default physical
  // service if a VPN is connected, rather than just reporting the
  // default logical service in both fields.
  std::move(callback).Run(TranslateONCConfiguration(&dictionary),
                          TranslateONCConfiguration(&dictionary));
}

void GetDefaultNetworkFailureCallback(
    base::OnceCallback<void(arc::mojom::NetworkConfigurationPtr,
                            arc::mojom::NetworkConfigurationPtr)> callback,
    const std::string& error_name,
    std::unique_ptr<base::DictionaryValue> error_data) {
  LOG(ERROR) << "Failed to query default logical network";
  std::move(callback).Run(nullptr, nullptr);
}

void DefaultNetworkFailureCallback(
    const std::string& error_name,
    std::unique_ptr<base::DictionaryValue> error_data) {
  LOG(ERROR) << "Failed to query default logical network";
}

void ArcVpnSuccessCallback() {
  DVLOG(1) << "ArcVpnSuccessCallback";
}

void ArcVpnErrorCallback(const std::string& error_name,
                         std::unique_ptr<base::DictionaryValue> error_data) {
  LOG(ERROR) << "ArcVpnErrorCallback: " << error_name;
}

}  // namespace

namespace arc {
namespace {

// Singleton factory for ArcNetHostImpl.
class ArcNetHostImplFactory
    : public internal::ArcBrowserContextKeyedServiceFactoryBase<
          ArcNetHostImpl,
          ArcNetHostImplFactory> {
 public:
  // Factory name used by ArcBrowserContextKeyedServiceFactoryBase.
  static constexpr const char* kName = "ArcNetHostImplFactory";

  static ArcNetHostImplFactory* GetInstance() {
    return base::Singleton<ArcNetHostImplFactory>::get();
  }

 private:
  friend base::DefaultSingletonTraits<ArcNetHostImplFactory>;
  ArcNetHostImplFactory() = default;
  ~ArcNetHostImplFactory() override = default;
};

}  // namespace

// static
ArcNetHostImpl* ArcNetHostImpl::GetForBrowserContext(
    content::BrowserContext* context) {
  return ArcNetHostImplFactory::GetForBrowserContext(context);
}

ArcNetHostImpl::ArcNetHostImpl(content::BrowserContext* context,
                               ArcBridgeService* bridge_service)
    : arc_bridge_service_(bridge_service), weak_factory_(this) {
  arc_bridge_service_->net()->SetHost(this);
  arc_bridge_service_->net()->AddObserver(this);
}

ArcNetHostImpl::~ArcNetHostImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (observing_network_state_) {
    GetStateHandler()->RemoveObserver(this, FROM_HERE);
    GetNetworkConnectionHandler()->RemoveObserver(this);
  }
  arc_bridge_service_->net()->RemoveObserver(this);
  arc_bridge_service_->net()->SetHost(nullptr);
}

void ArcNetHostImpl::OnConnectionReady() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (chromeos::NetworkHandler::IsInitialized()) {
    GetStateHandler()->AddObserver(this, FROM_HERE);
    GetNetworkConnectionHandler()->AddObserver(this);
    observing_network_state_ = true;
  }

  // If the default network is an ARC VPN, that means Chrome is restarting
  // after a crash but shill still thinks a VPN is connected. Nuke it.
  const chromeos::NetworkState* default_network =
      GetShillBackedNetwork(GetStateHandler()->DefaultNetwork());
  if (default_network && default_network->type() == shill::kTypeVPN &&
      default_network->vpn_provider_type() == shill::kProviderArcVpn) {
    VLOG(0) << "Disconnecting stale ARC VPN " << default_network->path();
    GetNetworkConnectionHandler()->DisconnectNetwork(
        default_network->path(), base::Bind(&ArcVpnSuccessCallback),
        base::Bind(&ArcVpnErrorCallback));
  }
}

void ArcNetHostImpl::OnConnectionClosed() {
  // Make sure shill doesn't leave an ARC VPN connected after Android
  // goes down.
  AndroidVpnStateChanged(arc::mojom::ConnectionStateType::NOT_CONNECTED);

  if (!observing_network_state_)
    return;

  GetStateHandler()->RemoveObserver(this, FROM_HERE);
  GetNetworkConnectionHandler()->RemoveObserver(this);
  observing_network_state_ = false;
}

void ArcNetHostImpl::GetNetworksDeprecated(
    mojom::GetNetworksRequestType type,
    GetNetworksDeprecatedCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  mojom::NetworkDataPtr data = mojom::NetworkData::New();
  bool configured_only = true;
  bool visible_only = false;
  if (type == mojom::GetNetworksRequestType::VISIBLE_ONLY) {
    configured_only = false;
    visible_only = true;
  }

  // Retrieve list of nearby WiFi networks.
  chromeos::NetworkTypePattern network_pattern =
      chromeos::onc::NetworkTypePatternFromOncType(onc::network_type::kWiFi);
  std::unique_ptr<base::ListValue> network_properties_list =
      chromeos::network_util::TranslateNetworkListToONC(
          network_pattern, configured_only, visible_only,
          kGetNetworksListLimit);

  // Extract info for each network and add it to the list.
  // Even if there's no WiFi, an empty (size=0) list must be returned and not a
  // null one. The explicitly sized New() constructor ensures the non-null
  // property.
  std::vector<mojom::WifiConfigurationPtr> networks;
  for (const auto& value : *network_properties_list) {
    mojom::WifiConfigurationPtr wc = mojom::WifiConfiguration::New();

    const base::DictionaryValue* network_dict = nullptr;
    value.GetAsDictionary(&network_dict);
    DCHECK(network_dict);

    // kName is a post-processed version of kHexSSID.
    std::string tmp;
    network_dict->GetString(onc::network_config::kName, &tmp);
    DCHECK(!tmp.empty());
    wc->ssid = tmp;

    tmp.clear();
    network_dict->GetString(onc::network_config::kGUID, &tmp);
    DCHECK(!tmp.empty());
    wc->guid = tmp;

    const base::DictionaryValue* wifi_dict = nullptr;
    network_dict->GetDictionary(onc::network_config::kWiFi, &wifi_dict);
    DCHECK(wifi_dict);

    if (!wifi_dict->GetInteger(onc::wifi::kFrequency, &wc->frequency))
      wc->frequency = 0;
    if (!wifi_dict->GetInteger(onc::wifi::kSignalStrength,
                               &wc->signal_strength))
      wc->signal_strength = 0;

    if (!wifi_dict->GetString(onc::wifi::kSecurity, &tmp))
      NOTREACHED();
    DCHECK(!tmp.empty());
    wc->security = tmp;

    if (!wifi_dict->GetString(onc::wifi::kBSSID, &tmp))
      NOTREACHED();
    DCHECK(!tmp.empty());
    wc->bssid = tmp;

    mojom::VisibleNetworkDetailsPtr details =
        mojom::VisibleNetworkDetails::New();
    details->frequency = wc->frequency;
    details->signal_strength = wc->signal_strength;
    details->bssid = wc->bssid;
    wc->details = mojom::NetworkDetails::New();
    wc->details->set_visible(std::move(details));

    networks.push_back(std::move(wc));
  }
  data->networks = std::move(networks);
  std::move(callback).Run(std::move(data));
}

void ArcNetHostImpl::GetNetworks(mojom::GetNetworksRequestType type,
                                 GetNetworksCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Retrieve list of configured or visible WiFi networks.
  bool configured_only = type == mojom::GetNetworksRequestType::CONFIGURED_ONLY;
  chromeos::NetworkTypePattern network_pattern =
      chromeos::onc::NetworkTypePatternFromOncType(onc::network_type::kWiFi);
  std::unique_ptr<base::ListValue> network_properties_list =
      chromeos::network_util::TranslateNetworkListToONC(
          network_pattern, configured_only, !configured_only /* visible_only */,
          kGetNetworksListLimit);

  std::vector<mojom::NetworkConfigurationPtr> networks;
  for (const auto& value : *network_properties_list) {
    const base::DictionaryValue* network_dict = nullptr;
    value.GetAsDictionary(&network_dict);
    DCHECK(network_dict);
    networks.push_back(TranslateONCConfiguration(network_dict));
  }

  std::move(callback).Run(mojom::GetNetworksResponseType::New(
      arc::mojom::NetworkResult::SUCCESS, std::move(networks)));
}

void ArcNetHostImpl::CreateNetworkSuccessCallback(
    base::OnceCallback<void(const std::string&)> callback,
    const std::string& service_path,
    const std::string& guid) {
  VLOG(1) << "CreateNetworkSuccessCallback";

  cached_guid_ = guid;
  cached_service_path_ = service_path;

  std::move(callback).Run(guid);
}

void ArcNetHostImpl::CreateNetworkFailureCallback(
    base::OnceCallback<void(const std::string&)> callback,
    const std::string& error_name,
    std::unique_ptr<base::DictionaryValue> error_data) {
  VLOG(1) << "CreateNetworkFailureCallback: " << error_name;
  std::move(callback).Run(std::string());
}

void ArcNetHostImpl::CreateNetwork(mojom::WifiConfigurationPtr cfg,
                                   CreateNetworkCallback callback) {
  if (!IsDeviceOwner()) {
    std::move(callback).Run(std::string());
    return;
  }

  std::unique_ptr<base::DictionaryValue> properties(new base::DictionaryValue);
  std::unique_ptr<base::DictionaryValue> wifi_dict(new base::DictionaryValue);

  if (!cfg->hexssid.has_value() || !cfg->details) {
    std::move(callback).Run(std::string());
    return;
  }
  mojom::ConfiguredNetworkDetailsPtr details =
      std::move(cfg->details->get_configured());
  if (!details) {
    std::move(callback).Run(std::string());
    return;
  }

  properties->SetKey(onc::network_config::kType,
                     base::Value(onc::network_config::kWiFi));
  wifi_dict->SetKey(onc::wifi::kHexSSID, base::Value(cfg->hexssid.value()));
  wifi_dict->SetKey(onc::wifi::kAutoConnect, base::Value(details->autoconnect));
  if (cfg->security.empty()) {
    wifi_dict->SetKey(onc::wifi::kSecurity,
                      base::Value(onc::wifi::kSecurityNone));
  } else {
    wifi_dict->SetKey(onc::wifi::kSecurity, base::Value(cfg->security));
    if (details->passphrase.has_value()) {
      wifi_dict->SetKey(onc::wifi::kPassphrase,
                        base::Value(details->passphrase.value()));
    }
  }
  properties->SetWithoutPathExpansion(onc::network_config::kWiFi,
                                      std::move(wifi_dict));

  std::string user_id_hash = chromeos::LoginState::Get()->primary_user_hash();
  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto repeating_callback =
      base::AdaptCallbackForRepeating(std::move(callback));
  GetManagedConfigurationHandler()->CreateConfiguration(
      user_id_hash, *properties,
      base::Bind(&ArcNetHostImpl::CreateNetworkSuccessCallback,
                 weak_factory_.GetWeakPtr(), repeating_callback),
      base::Bind(&ArcNetHostImpl::CreateNetworkFailureCallback,
                 weak_factory_.GetWeakPtr(), repeating_callback));
}

bool ArcNetHostImpl::GetNetworkPathFromGuid(const std::string& guid,
                                            std::string* path) {
  const chromeos::NetworkState* network =
      GetShillBackedNetwork(GetStateHandler()->GetNetworkStateFromGuid(guid));
  if (network) {
    *path = network->path();
    return true;
  }

  if (cached_guid_ == guid) {
    *path = cached_service_path_;
    return true;
  }
  return false;
}

void ArcNetHostImpl::ForgetNetwork(const std::string& guid,
                                   ForgetNetworkCallback callback) {
  if (!IsDeviceOwner()) {
    std::move(callback).Run(mojom::NetworkResult::FAILURE);
    return;
  }

  std::string path;
  if (!GetNetworkPathFromGuid(guid, &path)) {
    std::move(callback).Run(mojom::NetworkResult::FAILURE);
    return;
  }

  cached_guid_.clear();
  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto repeating_callback =
      base::AdaptCallbackForRepeating(std::move(callback));
  GetManagedConfigurationHandler()->RemoveConfiguration(
      path, base::Bind(&ForgetNetworkSuccessCallback, repeating_callback),
      base::Bind(&ForgetNetworkFailureCallback, repeating_callback));
}

void ArcNetHostImpl::StartConnect(const std::string& guid,
                                  StartConnectCallback callback) {
  std::string path;
  if (!GetNetworkPathFromGuid(guid, &path)) {
    std::move(callback).Run(mojom::NetworkResult::FAILURE);
    return;
  }

  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto repeating_callback =
      base::AdaptCallbackForRepeating(std::move(callback));
  GetNetworkConnectionHandler()->ConnectToNetwork(
      path, base::Bind(&StartConnectSuccessCallback, repeating_callback),
      base::Bind(&StartConnectFailureCallback, repeating_callback),
      false /* check_error_state */, chromeos::ConnectCallbackMode::ON_STARTED);
}

void ArcNetHostImpl::StartDisconnect(const std::string& guid,
                                     StartDisconnectCallback callback) {
  std::string path;
  if (!GetNetworkPathFromGuid(guid, &path)) {
    std::move(callback).Run(mojom::NetworkResult::FAILURE);
    return;
  }

  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto repeating_callback =
      base::AdaptCallbackForRepeating(std::move(callback));
  GetNetworkConnectionHandler()->DisconnectNetwork(
      path, base::Bind(&StartDisconnectSuccessCallback, repeating_callback),
      base::Bind(&StartDisconnectFailureCallback, repeating_callback));
}

void ArcNetHostImpl::GetWifiEnabledState(GetWifiEnabledStateCallback callback) {
  bool is_enabled = GetStateHandler()->IsTechnologyEnabled(
      chromeos::NetworkTypePattern::WiFi());
  std::move(callback).Run(is_enabled);
}

void ArcNetHostImpl::SetWifiEnabledState(bool is_enabled,
                                         SetWifiEnabledStateCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  chromeos::NetworkStateHandler::TechnologyState state =
      GetStateHandler()->GetTechnologyState(
          chromeos::NetworkTypePattern::WiFi());
  // WiFi can't be enabled or disabled in these states.
  if ((state == chromeos::NetworkStateHandler::TECHNOLOGY_PROHIBITED) ||
      (state == chromeos::NetworkStateHandler::TECHNOLOGY_UNINITIALIZED) ||
      (state == chromeos::NetworkStateHandler::TECHNOLOGY_UNAVAILABLE)) {
    VLOG(1) << "SetWifiEnabledState failed due to WiFi state: " << state;
    std::move(callback).Run(false);
    return;
  }
  GetStateHandler()->SetTechnologyEnabled(
      chromeos::NetworkTypePattern::WiFi(), is_enabled,
      chromeos::network_handler::ErrorCallback());
  std::move(callback).Run(true);
}

void ArcNetHostImpl::StartScan() {
  GetStateHandler()->RequestScan(chromeos::NetworkTypePattern::WiFi());
}

void ArcNetHostImpl::ScanCompleted(const chromeos::DeviceState* /*unused*/) {
  auto* net_instance =
      ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->net(), ScanCompleted);
  if (!net_instance)
    return;

  net_instance->ScanCompleted();
}

const chromeos::NetworkState* ArcNetHostImpl::GetDefaultNetworkFromChrome() {
  // If an Android VPN is connected, report the underlying physical
  // connection only.  Never tell Android about its own VPN.
  // If a Chrome OS VPN is connected, report the Chrome OS VPN as the
  // default connection.
  if (arc_vpn_service_path_.empty()) {
    return GetShillBackedNetwork(GetStateHandler()->DefaultNetwork());
  }

  return GetShillBackedNetwork(GetStateHandler()->ConnectedNetworkByType(
      chromeos::NetworkTypePattern::NonVirtual()));
}

void ArcNetHostImpl::GetDefaultNetwork(GetDefaultNetworkCallback callback) {
  const chromeos::NetworkState* default_network = GetDefaultNetworkFromChrome();

  if (!default_network) {
    VLOG(1) << "GetDefaultNetwork: no default network";
    std::move(callback).Run(nullptr, nullptr);
    return;
  }
  VLOG(1) << "GetDefaultNetwork: default network is "
          << default_network->path();
  std::string user_id_hash = chromeos::LoginState::Get()->primary_user_hash();
  // TODO(crbug.com/730593): Remove AdaptCallbackForRepeating() by updating
  // the callee interface.
  auto repeating_callback =
      base::AdaptCallbackForRepeating(std::move(callback));
  GetManagedConfigurationHandler()->GetProperties(
      user_id_hash, default_network->path(),
      base::Bind(&GetDefaultNetworkSuccessCallback, repeating_callback),
      base::Bind(&GetDefaultNetworkFailureCallback, repeating_callback));
}

void ArcNetHostImpl::DefaultNetworkSuccessCallback(
    const std::string& service_path,
    const base::DictionaryValue& dictionary) {
  auto* net_instance = ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->net(),
                                                   DefaultNetworkChanged);
  if (!net_instance)
    return;

  net_instance->DefaultNetworkChanged(TranslateONCConfiguration(&dictionary),
                                      TranslateONCConfiguration(&dictionary));
}

void ArcNetHostImpl::UpdateDefaultNetwork() {
  const chromeos::NetworkState* default_network = GetDefaultNetworkFromChrome();

  if (!default_network) {
    VLOG(1) << "No default network";
    auto* net_instance = ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->net(),
                                                     DefaultNetworkChanged);
    if (net_instance)
      net_instance->DefaultNetworkChanged(nullptr, nullptr);
    return;
  }

  VLOG(1) << "New default network: " << default_network->path() << " ("
          << default_network->type() << ")";
  std::string user_id_hash = chromeos::LoginState::Get()->primary_user_hash();
  GetManagedConfigurationHandler()->GetProperties(
      user_id_hash, default_network->path(),
      base::Bind(&ArcNetHostImpl::DefaultNetworkSuccessCallback,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&DefaultNetworkFailureCallback));
}

void ArcNetHostImpl::DefaultNetworkChanged(
    const chromeos::NetworkState* network) {
  UpdateDefaultNetwork();
}

void ArcNetHostImpl::DeviceListChanged() {
  auto* net_instance = ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->net(),
                                                   WifiEnabledStateChanged);
  if (!net_instance)
    return;

  bool is_enabled = GetStateHandler()->IsTechnologyEnabled(
      chromeos::NetworkTypePattern::WiFi());
  net_instance->WifiEnabledStateChanged(is_enabled);
}

std::string ArcNetHostImpl::LookupArcVpnServicePath() {
  chromeos::NetworkStateHandler::NetworkStateList state_list;
  GetStateHandler()->GetNetworkListByType(
      chromeos::NetworkTypePattern::VPN(), true /* configured_only */,
      false /* visible_only */, kGetNetworksListLimit, &state_list);

  for (const chromeos::NetworkState* state : state_list) {
    const chromeos::NetworkState* shill_backed_network =
        GetShillBackedNetwork(state);
    if (!shill_backed_network)
      continue;
    if (shill_backed_network->vpn_provider_type() == shill::kProviderArcVpn) {
      return shill_backed_network->path();
    }
  }
  return std::string();
}

void ArcNetHostImpl::ConnectArcVpn(const std::string& service_path,
                                   const std::string& /* guid */) {
  DVLOG(1) << "ConnectArcVpn " << service_path;
  arc_vpn_service_path_ = service_path;

  GetNetworkConnectionHandler()->ConnectToNetwork(
      service_path, base::Bind(&ArcVpnSuccessCallback),
      base::Bind(&ArcVpnErrorCallback), false /* check_error_state */,
      chromeos::ConnectCallbackMode::ON_COMPLETED);
}

std::unique_ptr<base::Value> ArcNetHostImpl::TranslateStringListToValue(
    const std::vector<std::string>& string_list) {
  std::unique_ptr<base::Value> result =
      std::make_unique<base::Value>(base::Value::Type::LIST);
  for (const auto& item : string_list) {
    result->GetList().emplace_back(item);
  }
  return result;
}

std::unique_ptr<base::DictionaryValue>
ArcNetHostImpl::TranslateVpnConfigurationToOnc(
    const mojom::AndroidVpnConfiguration& cfg) {
  std::unique_ptr<base::DictionaryValue> top_dict =
      std::make_unique<base::DictionaryValue>();

  // Name, Type
  top_dict->SetKey(
      onc::network_config::kName,
      base::Value(cfg.session_name.empty() ? cfg.app_label : cfg.session_name));
  top_dict->SetKey(onc::network_config::kType,
                   base::Value(onc::network_config::kVPN));

  // StaticIPConfig dictionary
  top_dict->SetKey(onc::network_config::kIPAddressConfigType,
                   base::Value(onc::network_config::kIPConfigTypeStatic));
  top_dict->SetKey(onc::network_config::kNameServersConfigType,
                   base::Value(onc::network_config::kIPConfigTypeStatic));

  std::unique_ptr<base::DictionaryValue> ip_dict =
      std::make_unique<base::DictionaryValue>();
  ip_dict->SetKey(onc::ipconfig::kType, base::Value(onc::ipconfig::kIPv4));
  ip_dict->SetKey(onc::ipconfig::kIPAddress, base::Value(cfg.ipv4_gateway));
  ip_dict->SetKey(onc::ipconfig::kRoutingPrefix, base::Value(32));
  ip_dict->SetKey(onc::ipconfig::kGateway, base::Value(cfg.ipv4_gateway));

  ip_dict->SetWithoutPathExpansion(onc::ipconfig::kNameServers,
                                   TranslateStringListToValue(cfg.nameservers));
  ip_dict->SetWithoutPathExpansion(onc::ipconfig::kSearchDomains,
                                   TranslateStringListToValue(cfg.domains));
  ip_dict->SetWithoutPathExpansion(
      onc::ipconfig::kIncludedRoutes,
      TranslateStringListToValue(cfg.split_include));
  ip_dict->SetWithoutPathExpansion(
      onc::ipconfig::kExcludedRoutes,
      TranslateStringListToValue(cfg.split_exclude));

  top_dict->SetWithoutPathExpansion(onc::network_config::kStaticIPConfig,
                                    std::move(ip_dict));

  // VPN dictionary
  std::unique_ptr<base::DictionaryValue> vpn_dict =
      std::make_unique<base::DictionaryValue>();
  vpn_dict->SetKey(onc::vpn::kHost, base::Value(cfg.app_name));
  vpn_dict->SetKey(onc::vpn::kType, base::Value(onc::vpn::kArcVpn));

  // ARCVPN dictionary
  std::unique_ptr<base::DictionaryValue> arcvpn_dict =
      std::make_unique<base::DictionaryValue>();
  arcvpn_dict->SetKey(
      onc::arc_vpn::kTunnelChrome,
      base::Value(cfg.tunnel_chrome_traffic ? "true" : "false"));
  vpn_dict->SetWithoutPathExpansion(onc::vpn::kArcVpn, std::move(arcvpn_dict));

  top_dict->SetWithoutPathExpansion(onc::network_config::kVPN,
                                    std::move(vpn_dict));

  return top_dict;
}

void ArcNetHostImpl::AndroidVpnConnected(
    mojom::AndroidVpnConfigurationPtr cfg) {
  std::unique_ptr<base::DictionaryValue> properties =
      TranslateVpnConfigurationToOnc(*cfg);

  if (!base::FeatureList::IsEnabled(arc::kVpnFeature)) {
    VLOG(1) << "AndroidVpnConnected: feature is disabled; ignoring";
    return;
  }

  std::string service_path = LookupArcVpnServicePath();
  if (!service_path.empty()) {
    VLOG(1) << "AndroidVpnConnected: reusing " << service_path;
    GetManagedConfigurationHandler()->SetProperties(
        service_path, *properties,
        base::Bind(&ArcNetHostImpl::ConnectArcVpn, weak_factory_.GetWeakPtr(),
                   service_path, std::string()),
        base::Bind(&ArcVpnErrorCallback));
    return;
  }

  VLOG(1) << "AndroidVpnConnected: creating new ARC VPN";
  std::string user_id_hash = chromeos::LoginState::Get()->primary_user_hash();
  GetManagedConfigurationHandler()->CreateConfiguration(
      user_id_hash, *properties,
      base::Bind(&ArcNetHostImpl::ConnectArcVpn, weak_factory_.GetWeakPtr()),
      base::Bind(&ArcVpnErrorCallback));
}

void ArcNetHostImpl::AndroidVpnStateChanged(mojom::ConnectionStateType state) {
  VLOG(1) << "AndroidVpnStateChanged: state=" << state
          << " service=" << arc_vpn_service_path_;

  if (state != arc::mojom::ConnectionStateType::NOT_CONNECTED ||
      arc_vpn_service_path_.empty()) {
    return;
  }

  // DisconnectNetwork() invokes DisconnectRequested() through the
  // observer interface, so make sure it doesn't generate an unwanted
  // mojo call to Android.
  std::string service_path(arc_vpn_service_path_);
  arc_vpn_service_path_.clear();

  GetNetworkConnectionHandler()->DisconnectNetwork(
      service_path, base::Bind(&ArcVpnSuccessCallback),
      base::Bind(&ArcVpnErrorCallback));
}

void ArcNetHostImpl::DisconnectArcVpn() {
  arc_vpn_service_path_.clear();

  auto* net_instance = ARC_GET_INSTANCE_FOR_METHOD(arc_bridge_service_->net(),
                                                   DisconnectAndroidVpn);
  if (!net_instance) {
    LOG(ERROR) << "User requested VPN disconnection but API is unavailable";
    return;
  }
  net_instance->DisconnectAndroidVpn();
}

void ArcNetHostImpl::DisconnectRequested(const std::string& service_path) {
  if (arc_vpn_service_path_ != service_path) {
    return;
  }

  // This code path is taken when a user clicks the blue Disconnect button
  // in Chrome OS.  Chrome is about to send the Disconnect call to shill,
  // so update our local state and tell Android to disconnect the VPN.
  VLOG(1) << "DisconnectRequested " << service_path;
  DisconnectArcVpn();
}

void ArcNetHostImpl::NetworkConnectionStateChanged(
    const chromeos::NetworkState* network) {
  // DefaultNetworkChanged() won't be invoked if an ARC VPN is the default
  // network and the underlying physical connection changed, so check for
  // that condition here.  This is invoked any time any service state
  // changes.
  UpdateDefaultNetwork();

  const chromeos::NetworkState* shill_backed_network =
      GetShillBackedNetwork(network);
  if (!shill_backed_network)
    return;

  if (arc_vpn_service_path_ != shill_backed_network->path() ||
      shill_backed_network->IsConnectingOrConnected()) {
    return;
  }

  // This code path is taken when shill disconnects the Android VPN
  // service.  This can happen if a user tries to connect to a Chrome OS
  // VPN, and shill's VPNProvider::DisconnectAll() forcibly disconnects
  // all other VPN services to avoid a conflict.
  VLOG(1) << "NetworkConnectionStateChanged " << shill_backed_network->path();
  DisconnectArcVpn();
}

void ArcNetHostImpl::NetworkListChanged() {
  // This is invoked any time the list of services is reordered or changed.
  // During the transition when a new service comes online, it will
  // temporarily be ranked below "inferior" services.  This callback
  // informs us that shill's ordering has been updated.
  UpdateDefaultNetwork();
}

void ArcNetHostImpl::OnShuttingDown() {
  DCHECK(observing_network_state_);
  GetStateHandler()->RemoveObserver(this, FROM_HERE);
  GetNetworkConnectionHandler()->RemoveObserver(this);
  observing_network_state_ = false;
}

}  // namespace arc
