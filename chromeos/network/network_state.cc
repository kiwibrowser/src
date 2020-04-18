// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/network_state.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chromeos/network/network_profile_handler.h"
#include "chromeos/network/network_type_pattern.h"
#include "chromeos/network/network_util.h"
#include "chromeos/network/onc/onc_utils.h"
#include "chromeos/network/shill_property_util.h"
#include "chromeos/network/tether_constants.h"
#include "components/device_event_log/device_event_log.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace {

const char kErrorUnknown[] = "Unknown";

const char kDefaultCellularNetworkPath[] = "/cellular";

bool IsCaptivePortalState(const base::DictionaryValue& properties, bool log) {
  std::string state;
  properties.GetStringWithoutPathExpansion(shill::kStateProperty, &state);
  if (state != shill::kStatePortal)
    return false;
  std::string portal_detection_phase, portal_detection_status;
  if (!properties.GetStringWithoutPathExpansion(
          shill::kPortalDetectionFailedPhaseProperty,
          &portal_detection_phase) ||
      !properties.GetStringWithoutPathExpansion(
          shill::kPortalDetectionFailedStatusProperty,
          &portal_detection_status)) {
    // If Shill (or a stub) has not set PortalDetectionFailedStatus
    // or PortalDetectionFailedPhase, assume we are in captive portal state.
    return true;
  }

  // Shill reports the phase in which it determined that the device is behind a
  // captive portal. We only want to rely only on incorrect content being
  // returned and ignore other reasons.
  bool is_captive_portal =
      portal_detection_phase == shill::kPortalDetectionPhaseContent &&
      portal_detection_status == shill::kPortalDetectionStatusFailure;

  if (log) {
    std::string name;
    properties.GetStringWithoutPathExpansion(shill::kNameProperty, &name);
    if (name.empty())
      properties.GetStringWithoutPathExpansion(shill::kSSIDProperty, &name);
    if (!is_captive_portal) {
      NET_LOG(EVENT) << "State is 'portal' but not in captive portal state:"
                     << " name=" << name << " phase=" << portal_detection_phase
                     << " status=" << portal_detection_status;
    } else {
      NET_LOG(EVENT) << "Network is in captive portal state: " << name;
    }
  }

  return is_captive_portal;
}

std::string GetStringFromDictionary(const base::DictionaryValue& dict,
                                    const char* key) {
  const base::Value* v = dict.FindKey(key);
  return v ? v->GetString() : std::string();
}

}  // namespace

namespace chromeos {

NetworkState::NetworkState(const std::string& path)
    : ManagedState(MANAGED_TYPE_NETWORK, path) {}

NetworkState::~NetworkState() = default;

bool NetworkState::PropertyChanged(const std::string& key,
                                   const base::Value& value) {
  // Keep care that these properties are the same as in |GetProperties|.
  if (ManagedStatePropertyChanged(key, value))
    return true;
  if (key == shill::kSignalStrengthProperty) {
    return GetIntegerValue(key, value, &signal_strength_);
  } else if (key == shill::kStateProperty) {
    std::string saved_state = connection_state_;
    if (GetStringValue(key, value, &connection_state_)) {
      if (connection_state_ != saved_state)
        last_connection_state_ = saved_state;
      return true;
    } else {
      return false;
    }
  } else if (key == shill::kVisibleProperty) {
    return GetBooleanValue(key, value, &visible_);
  } else if (key == shill::kConnectableProperty) {
    return GetBooleanValue(key, value, &connectable_);
  } else if (key == shill::kErrorProperty) {
    if (!GetStringValue(key, value, &error_))
      return false;
    if (ErrorIsValid(error_))
      last_error_ = error_;
    else
      error_.clear();
    return true;
  } else if (key == shill::kWifiFrequency) {
    return GetIntegerValue(key, value, &frequency_);
  } else if (key == shill::kActivationTypeProperty) {
    return GetStringValue(key, value, &activation_type_);
  } else if (key == shill::kActivationStateProperty) {
    return GetStringValue(key, value, &activation_state_);
  } else if (key == shill::kRoamingStateProperty) {
    return GetStringValue(key, value, &roaming_);
  } else if (key == shill::kPaymentPortalProperty) {
    const base::DictionaryValue* olp;
    if (!value.GetAsDictionary(&olp))
      return false;
    return olp->GetStringWithoutPathExpansion(shill::kPaymentPortalURL,
                                              &payment_url_);
  } else if (key == shill::kSecurityClassProperty) {
    return GetStringValue(key, value, &security_class_);
  } else if (key == shill::kEapMethodProperty) {
    return GetStringValue(key, value, &eap_method_);
  } else if (key == shill::kEapKeyMgmtProperty) {
    return GetStringValue(key, value, &eap_key_mgmt_);
  } else if (key == shill::kNetworkTechnologyProperty) {
    return GetStringValue(key, value, &network_technology_);
  } else if (key == shill::kDeviceProperty) {
    return GetStringValue(key, value, &device_path_);
  } else if (key == shill::kGuidProperty) {
    return GetStringValue(key, value, &guid_);
  } else if (key == shill::kProfileProperty) {
    return GetStringValue(key, value, &profile_path_);
  } else if (key == shill::kWifiHexSsid) {
    std::string ssid_hex;
    if (!GetStringValue(key, value, &ssid_hex))
      return false;
    raw_ssid_.clear();
    return base::HexStringToBytes(ssid_hex, &raw_ssid_);
  } else if (key == shill::kWifiBSsid) {
    return GetStringValue(key, value, &bssid_);
  } else if (key == shill::kPriorityProperty) {
    return GetIntegerValue(key, value, &priority_);
  } else if (key == shill::kOutOfCreditsProperty) {
    return GetBooleanValue(key, value, &cellular_out_of_credits_);
  } else if (key == shill::kProxyConfigProperty) {
    std::string proxy_config_str;
    if (!value.GetAsString(&proxy_config_str)) {
      NET_LOG(ERROR) << "Failed to parse " << path() << "." << key;
      return false;
    }

    proxy_config_.Clear();
    if (proxy_config_str.empty())
      return true;

    std::unique_ptr<base::DictionaryValue> proxy_config_dict(
        onc::ReadDictionaryFromJson(proxy_config_str));
    if (proxy_config_dict) {
      // Warning: The DictionaryValue returned from
      // ReadDictionaryFromJson/JSONParser is an optimized derived class that
      // doesn't allow releasing ownership of nested values. A Swap in the wrong
      // order leads to memory access errors.
      proxy_config_.MergeDictionary(proxy_config_dict.get());
    } else {
      NET_LOG(ERROR) << "Failed to parse " << path() << "." << key;
    }
    return true;
  } else if (key == shill::kProviderProperty) {
    std::string vpn_provider_type;
    const base::DictionaryValue* dict;
    if (!value.GetAsDictionary(&dict) ||
        !dict->GetStringWithoutPathExpansion(shill::kTypeProperty,
                                             &vpn_provider_type)) {
      NET_LOG(ERROR) << "Failed to parse " << path() << "." << key;
      return false;
    }

    if (vpn_provider_type == shill::kProviderThirdPartyVpn ||
        vpn_provider_type == shill::kProviderArcVpn) {
      // If the network uses a third-party or Arc VPN provider,
      // |shill::kHostProperty| contains the extension ID or Arc package name.
      if (!dict->GetStringWithoutPathExpansion(shill::kHostProperty,
                                               &vpn_provider_id_)) {
        NET_LOG(ERROR) << "Failed to parse " << path() << "." << key;
        return false;
      }
    } else {
      vpn_provider_id_.clear();
    }

    vpn_provider_type_ = vpn_provider_type;
    return true;
  } else if (key == shill::kTetheringProperty) {
    return GetStringValue(key, value, &tethering_state_);
  }
  return false;
}

bool NetworkState::InitialPropertiesReceived(
    const base::DictionaryValue& properties) {
  NET_LOG(EVENT) << "InitialPropertiesReceived: " << name() << " (" << path()
                 << ") State: " << connection_state_
                 << " Visible: " << visible_;
  if (!properties.HasKey(shill::kTypeProperty)) {
    NET_LOG(ERROR) << "NetworkState has no type: "
                   << shill_property_util::GetNetworkIdFromProperties(
                          properties);
    return false;
  }

  // By convention, all visible WiFi and WiMAX networks have a
  // SignalStrength > 0.
  if ((type() == shill::kTypeWifi || type() == shill::kTypeWimax) &&
      visible() && signal_strength_ <= 0) {
    signal_strength_ = 1;
  }

  // Any change to connection state will trigger a complete property update,
  // so we update is_captive_portal_ here.
  is_captive_portal_ = IsCaptivePortalState(properties, true /* log */);

  // Ensure that the network has a valid name.
  return UpdateName(properties);
}

void NetworkState::GetStateProperties(base::DictionaryValue* dictionary) const {
  ManagedState::GetStateProperties(dictionary);

  // Properties shared by all types.
  dictionary->SetKey(shill::kGuidProperty, base::Value(guid()));
  dictionary->SetKey(shill::kSecurityClassProperty,
                     base::Value(security_class()));
  dictionary->SetKey(shill::kProfileProperty, base::Value(profile_path()));
  dictionary->SetKey(shill::kPriorityProperty, base::Value(priority_));

  if (visible())
    dictionary->SetKey(shill::kStateProperty, base::Value(connection_state()));
  if (!device_path().empty())
    dictionary->SetKey(shill::kDeviceProperty, base::Value(device_path()));

  // VPN properties.
  if (NetworkTypePattern::VPN().MatchesType(type())) {
    // Shill sends VPN provider properties in a nested dictionary. |dictionary|
    // must replicate that nested structure.
    std::unique_ptr<base::DictionaryValue> provider_property(
        new base::DictionaryValue);
    provider_property->SetKey(shill::kTypeProperty,
                              base::Value(vpn_provider_type_));
    if (vpn_provider_type_ == shill::kProviderThirdPartyVpn ||
        vpn_provider_type_ == shill::kProviderArcVpn) {
      provider_property->SetKey(shill::kHostProperty,
                                base::Value(vpn_provider_id_));
    }
    dictionary->SetWithoutPathExpansion(shill::kProviderProperty,
                                        std::move(provider_property));
  }

  // Tether properties
  if (NetworkTypePattern::Tether().MatchesType(type())) {
    dictionary->SetKey(kTetherBatteryPercentage,
                       base::Value(battery_percentage()));
    dictionary->SetKey(kTetherCarrier, base::Value(carrier()));
    dictionary->SetKey(kTetherHasConnectedToHost,
                       base::Value(tether_has_connected_to_host()));
    dictionary->SetKey(kTetherSignalStrength, base::Value(signal_strength()));

    // Tether networks do not share some of the wireless/mobile properties added
    // below; exit early to avoid having these properties applied.
    return;
  }

  // Wireless properties
  if (!NetworkTypePattern::Wireless().MatchesType(type()))
    return;

  if (visible()) {
    dictionary->SetKey(shill::kConnectableProperty, base::Value(connectable()));
    dictionary->SetKey(shill::kSignalStrengthProperty,
                       base::Value(signal_strength()));
  }

  // Wifi properties
  if (NetworkTypePattern::WiFi().MatchesType(type())) {
    dictionary->SetKey(shill::kWifiBSsid, base::Value(bssid_));
    dictionary->SetKey(shill::kEapMethodProperty, base::Value(eap_method()));
    dictionary->SetKey(shill::kWifiFrequency, base::Value(frequency_));
    dictionary->SetKey(shill::kWifiHexSsid, base::Value(GetHexSsid()));
  }

  // Mobile properties
  if (NetworkTypePattern::Mobile().MatchesType(type())) {
    dictionary->SetKey(shill::kNetworkTechnologyProperty,
                       base::Value(network_technology()));
    dictionary->SetKey(shill::kActivationStateProperty,
                       base::Value(activation_state()));
    dictionary->SetKey(shill::kRoamingStateProperty, base::Value(roaming()));
    dictionary->SetKey(shill::kOutOfCreditsProperty,
                       base::Value(cellular_out_of_credits()));
  }
}

void NetworkState::IPConfigPropertiesChanged(
    const base::DictionaryValue& properties) {
  ipv4_config_.Clear();
  ipv4_config_.MergeDictionary(&properties);
}

std::string NetworkState::GetIpAddress() const {
  return GetStringFromDictionary(ipv4_config_, shill::kAddressProperty);
}

std::string NetworkState::GetGateway() const {
  return GetStringFromDictionary(ipv4_config_, shill::kGatewayProperty);
}

GURL NetworkState::GetWebProxyAutoDiscoveryUrl() const {
  std::string url = GetStringFromDictionary(
      ipv4_config_, shill::kWebProxyAutoDiscoveryUrlProperty);
  if (url.empty())
    return GURL();
  GURL gurl(url);
  if (!gurl.is_valid()) {
    NET_LOG(ERROR) << "Invalid WebProxyAutoDiscoveryUrl: " << path() << ": "
                   << url;
    return GURL();
  }
  return gurl;
}

bool NetworkState::RequiresActivation() const {
  return type() == shill::kTypeCellular &&
         activation_state() != shill::kActivationStateActivated &&
         activation_state() != shill::kActivationStateUnknown;
}

bool NetworkState::SecurityRequiresPassphraseOnly() const {
  return type() == shill::kTypeWifi &&
         (security_class() == shill::kSecurityPsk ||
          security_class() == shill::kSecurityWep);
}

std::string NetworkState::connection_state() const {
  if (!visible())
    return shill::kStateDisconnect;
  return connection_state_;
}

void NetworkState::set_connection_state(const std::string connection_state) {
  last_connection_state_ = connection_state_;
  connection_state_ = connection_state;
}

bool NetworkState::IsUsingMobileData() const {
  return type() == shill::kTypeCellular || type() == chromeos::kTypeTether ||
         tethering_state() == shill::kTetheringConfirmedState;
}

bool NetworkState::IsDynamicWep() const {
  return security_class_ == shill::kSecurityWep &&
         eap_key_mgmt_ == shill::kKeyManagementIEEE8021X;
}

bool NetworkState::IsConnectedState() const {
  return visible() && StateIsConnected(connection_state_);
}

bool NetworkState::IsConnectingState() const {
  return visible() && StateIsConnecting(connection_state_);
}

bool NetworkState::IsConnectingOrConnected() const {
  return visible() && (StateIsConnecting(connection_state_) ||
                       StateIsConnected(connection_state_));
}

bool NetworkState::IsReconnecting() const {
  return visible() && StateIsConnecting(connection_state_) &&
         StateIsConnected(last_connection_state_);
}

bool NetworkState::IsInProfile() const {
  // kTypeEthernetEap is always saved. We need this check because it does
  // not show up in the visible list, but its properties may not be available
  // when it first shows up in ServiceCompleteList. See crbug.com/355117.
  return !profile_path_.empty() || type() == shill::kTypeEthernetEap;
}

bool NetworkState::IsNonProfileType() const {
  return type() == kTypeTether || IsDefaultCellular();
}

bool NetworkState::IsPrivate() const {
  return !profile_path_.empty() &&
         profile_path_ != NetworkProfileHandler::GetSharedProfilePath();
}

bool NetworkState::IsDefaultCellular() const {
  return type() == shill::kTypeCellular &&
         path() == kDefaultCellularNetworkPath;
}

std::string NetworkState::GetHexSsid() const {
  return base::HexEncode(raw_ssid().data(), raw_ssid().size());
}

std::string NetworkState::GetDnsServersAsString() const {
  const base::Value* listv = ipv4_config_.FindKey(shill::kNameServersProperty);
  if (!listv)
    return std::string();
  std::string result;
  for (const auto& v : listv->GetList()) {
    if (!result.empty())
      result += ",";
    result += v.GetString();
  }
  return result;
}

std::string NetworkState::GetNetmask() const {
  const base::Value* v = ipv4_config_.FindKey(shill::kPrefixlenProperty);
  int prefixlen = v ? v->GetInt() : -1;
  return network_util::PrefixLengthToNetmask(prefixlen);
}

std::string NetworkState::GetSpecifier() const {
  if (!update_received()) {
    NET_LOG(ERROR) << "GetSpecifier called before update: " << path();
    return std::string();
  }
  if (type() == shill::kTypeWifi)
    return name() + "_" + security_class_;
  if (type() != shill::kTypeCellular && !name().empty())
    return name();
  return type();  // For unnamed networks, i.e. Ethernet and Cellular.
}

void NetworkState::SetGuid(const std::string& guid) {
  guid_ = guid;
}

bool NetworkState::UpdateName(const base::DictionaryValue& properties) {
  std::string updated_name =
      shill_property_util::GetNameFromProperties(path(), properties);
  if (updated_name != name()) {
    set_name(updated_name);
    return true;
  }
  return false;
}

std::string NetworkState::GetErrorState() const {
  if (ErrorIsValid(error()))
    return error();
  return last_error();
}

// static
bool NetworkState::StateIsConnected(const std::string& connection_state) {
  return (connection_state == shill::kStateReady ||
          connection_state == shill::kStateOnline ||
          connection_state == shill::kStatePortal);
}

// static
bool NetworkState::StateIsConnecting(const std::string& connection_state) {
  return (connection_state == shill::kStateAssociation ||
          connection_state == shill::kStateConfiguration ||
          connection_state == shill::kStateCarrier);
}

// static
bool NetworkState::NetworkStateIsCaptivePortal(
    const base::DictionaryValue& shill_properties) {
  return IsCaptivePortalState(shill_properties, false /* log */);
}

// static
bool NetworkState::ErrorIsValid(const std::string& error) {
  // Shill uses "Unknown" to indicate an unset or cleared error state.
  return !error.empty() && error != kErrorUnknown;
}

// static
std::unique_ptr<NetworkState> NetworkState::CreateDefaultCellular(
    const std::string& device_path) {
  auto new_state = std::make_unique<NetworkState>(kDefaultCellularNetworkPath);
  new_state->set_type(shill::kTypeCellular);
  new_state->set_update_received();
  new_state->set_visible(true);
  new_state->device_path_ = device_path;
  return new_state;
}

}  // namespace chromeos
