// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_STATE_H_
#define CHROMEOS_NETWORK_NETWORK_STATE_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/values.h"
#include "chromeos/network/managed_state.h"
#include "components/onc/onc_constants.h"
#include "url/gurl.h"

namespace base {
class DictionaryValue;
class Value;
}

namespace chromeos {

// Simple class to provide network state information about a network service.
// This class should always be passed as a const* and should never be held
// on to. Store network_state->path() (defined in ManagedState) instead and
// call NetworkStateHandler::GetNetworkState(path) to retrieve the state for
// the network.
//
// Note: NetworkStateHandler will store an entry for each member of
// Manager.ServiceCompleteList. The visible() method indicates whether the
// network is visible, and the IsInProfile() method indicates whether the
// network is saved in a profile.
class CHROMEOS_EXPORT NetworkState : public ManagedState {
 public:
  explicit NetworkState(const std::string& path);
  ~NetworkState() override;

  // ManagedState overrides
  // If you change this method, update GetProperties too.
  bool PropertyChanged(const std::string& key,
                       const base::Value& value) override;
  bool InitialPropertiesReceived(
      const base::DictionaryValue& properties) override;
  void GetStateProperties(base::DictionaryValue* dictionary) const override;

  void IPConfigPropertiesChanged(const base::DictionaryValue& properties);

  // Returns true if the network requires a service activation.
  bool RequiresActivation() const;

  // Returns true if the network security type requires a passphrase only.
  bool SecurityRequiresPassphraseOnly() const;

  // Accessors
  bool visible() const { return visible_; }
  void set_visible(bool visible) { visible_ = visible; }
  const std::string& security_class() const { return security_class_; }
  const std::string& device_path() const { return device_path_; }
  const std::string& guid() const { return guid_; }
  const std::string& profile_path() const { return profile_path_; }
  const std::string& error() const { return error_; }
  const std::string& last_error() const { return last_error_; }
  void clear_last_error() { last_error_.clear(); }

  // Returns |connection_state_| if visible, kStateDisconnect otherwise.
  std::string connection_state() const;
  void set_connection_state(const std::string connection_state);

  const base::DictionaryValue& proxy_config() const { return proxy_config_; }

  const base::DictionaryValue& ipv4_config() const { return ipv4_config_; }
  std::string GetIpAddress() const;
  std::string GetGateway() const;
  GURL GetWebProxyAutoDiscoveryUrl() const;

  // Wireless property accessors
  bool connectable() const { return connectable_; }
  void set_connectable(bool connectable) { connectable_ = connectable; }
  bool is_captive_portal() const { return is_captive_portal_; }
  int signal_strength() const { return signal_strength_; }
  void set_signal_strength(int signal_strength) {
    signal_strength_ = signal_strength;
  }

  // Wifi property accessors
  const std::string& eap_method() const { return eap_method_; }
  const std::vector<uint8_t>& raw_ssid() const { return raw_ssid_; }

  // Cellular property accessors
  const std::string& network_technology() const {
    return network_technology_;
  }
  const std::string& activation_type() const { return activation_type_; }
  const std::string& activation_state() const { return activation_state_; }
  const std::string& roaming() const { return roaming_; }
  const std::string& payment_url() const { return payment_url_; }
  bool cellular_out_of_credits() const { return cellular_out_of_credits_; }
  const std::string& tethering_state() const { return tethering_state_; }

  // VPN property accessors
  const std::string& vpn_provider_type() const { return vpn_provider_type_; }
  const std::string& vpn_provider_id() const { return vpn_provider_id_; }

  // Tether accessors and setters.
  int battery_percentage() const { return battery_percentage_; }
  void set_battery_percentage(int battery_percentage) {
    battery_percentage_ = battery_percentage;
  }
  const std::string& carrier() const { return carrier_; }
  void set_carrier(const std::string& carrier) { carrier_ = carrier; }
  bool tether_has_connected_to_host() const {
    return tether_has_connected_to_host_;
  }
  void set_tether_has_connected_to_host(bool tether_has_connected_to_host) {
    tether_has_connected_to_host_ = tether_has_connected_to_host;
  }
  const std::string& tether_guid() const { return tether_guid_; }
  void set_tether_guid(const std::string& guid) { tether_guid_ = guid; }

  // Returns true if current connection is using mobile data.
  bool IsUsingMobileData() const;

  // Returns true if the network securty is WEP_8021x (Dynamic WEP)
  bool IsDynamicWep() const;

  // Returns true if |connection_state_| is a connected/connecting state.
  bool IsConnectedState() const;
  bool IsConnectingState() const;
  bool IsConnectingOrConnected() const;

  // Returns true if |last_connection_state_| is connected, and
  // |connection_state_| is connecting.
  bool IsReconnecting() const;

  // Returns true if this is a network stored in a profile.
  bool IsInProfile() const;

  // Returns true if the network is never stored in a profile (e.g. Tether and
  // default Cellular).
  bool IsNonProfileType() const;

  // Returns true if the network properties are stored in a user profile.
  bool IsPrivate() const;

  // Returns true if the network is a default Cellular network (see
  // NetworkStateHandler::EnsureCellularNetwork()).
  bool IsDefaultCellular() const;

  // Returns the |raw_ssid| as a hex-encoded string
  std::string GetHexSsid() const;

  // Returns a comma separated string of name servers.
  std::string GetDnsServersAsString() const;

  // Converts the prefix length to a netmask string.
  std::string GetNetmask() const;

  // Returns a specifier for identifying this network in the absence of a GUID.
  // This should only be used by NetworkStateHandler for keeping track of
  // GUIDs assigned to unsaved networks.
  std::string GetSpecifier() const;

  // Set the GUID. Called exclusively by NetworkStateHandler.
  void SetGuid(const std::string& guid);

  // Returns |error_| if valid, otherwise returns |last_error_|.
  std::string GetErrorState() const;

  // Helpers (used e.g. when a state, error, or shill dictionary is cached)
  static bool StateIsConnected(const std::string& connection_state);
  static bool StateIsConnecting(const std::string& connection_state);
  static bool NetworkStateIsCaptivePortal(
      const base::DictionaryValue& shill_properties);
  static bool ErrorIsValid(const std::string& error);
  static std::unique_ptr<NetworkState> CreateDefaultCellular(
      const std::string& device_path);

 private:
  friend class MobileActivatorTest;
  friend class NetworkStateHandler;
  friend class NetworkChangeNotifierChromeosUpdateTest;
  FRIEND_TEST_ALL_PREFIXES(NetworkStateTest, TetherProperties);

  // Updates |name_| from WiFi.HexSSID if provided, and validates |name_|.
  // Returns true if |name_| changes.
  bool UpdateName(const base::DictionaryValue& properties);

  // Set to true if the network is a member of Manager.Services.
  bool visible_ = false;

  // Network Service properties. Avoid adding any additional properties here.
  // Instead use NetworkConfigurationHandler::GetProperties() to asynchronously
  // request properties from Shill.
  std::string security_class_;
  std::string eap_method_;  // Needed for WiFi EAP networks
  std::string eap_key_mgmt_;  // Needed for identifying Dynamic WEP networks
  std::string device_path_;
  std::string guid_;
  std::string tether_guid_;  // Used to double link a Tether and Wi-Fi network.
  std::string connection_state_;
  std::string last_connection_state_;
  std::string profile_path_;
  std::vector<uint8_t> raw_ssid_;  // Unknown encoding. Not necessarily UTF-8.
  int priority_ = 0;

  // Reflects the current Shill Service.Error property. This might get cleared
  // by Shill shortly after a failure.
  std::string error_;

  // Last non empty Service.Error property. Cleared by NetworkConnectionHandler
  // when a connection attempt is initiated.
  std::string last_error_;

  // Cached copy of the Shill Service IPConfig object. For ipv6 properties use
  // the ip_configs_ property in the corresponding DeviceState.
  base::DictionaryValue ipv4_config_;

  // Wireless properties, used for icons and Connect logic.
  bool connectable_ = false;
  bool is_captive_portal_ = false;
  int signal_strength_ = 0;
  std::string bssid_;  // For ARC
  int frequency_ = 0;  // For ARC

  // Cellular properties, used for icons, Connect, and Activation.
  std::string network_technology_;
  std::string activation_type_;
  std::string activation_state_;
  std::string roaming_;
  std::string payment_url_;
  bool cellular_out_of_credits_ = false;
  std::string tethering_state_;

  // VPN properties, used to construct the display name and to show the correct
  // configuration dialog.
  std::string vpn_provider_type_;
  // Extension ID or Arc package name for extension or Arc provider VPNs.
  std::string vpn_provider_id_;

  // Tether properties.
  std::string carrier_;
  int battery_percentage_ = 0;

  // Whether the current device has already connected to the tether host device
  // providing the hotspot corresponding to this NetworkState.
  // Note: this means that the current device has already connected to the
  // tether host, but it does not necessarily mean that the current device has
  // connected to the Tether network corresponding to this NetworkState.
  bool tether_has_connected_to_host_ = false;

  // TODO(pneubeck): Remove this once (Managed)NetworkConfigurationHandler
  // provides proxy configuration. crbug.com/241775
  base::DictionaryValue proxy_config_;

  DISALLOW_COPY_AND_ASSIGN(NetworkState);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_STATE_H_
