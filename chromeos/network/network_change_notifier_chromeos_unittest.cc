// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/network_change_notifier_chromeos.h"

#include <stddef.h>

#include <string>

#include "base/message_loop/message_loop.h"
#include "base/strings/string_split.h"
#include "chromeos/network/network_change_notifier_factory_chromeos.h"
#include "chromeos/network/network_state.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

const char kDnsServers1[] = "192.168.0.1,192.168.0.2";
const char kDnsServers2[] = "192.168.3.1,192.168.3.2";
const char kIpAddress1[] = "192.168.1.1";
const char kIpAddress2[] = "192.168.1.2";
const char kService1[] = "/service/1";
const char kService2[] = "/service/2";
const char kService3[] = "/service/3";

// These values come from
// http://w3c.github.io/netinfo/#underlying-connection-technology. For types
// that have unknown subtypes (wifi and ethernet) positive infinity is used as
// per the spec.
const double kExpectedNoneMaxBandwidth = 0;
const double kExpectedWifiMaxBandwidth =
    std::numeric_limits<double>::infinity();
const double kExpectedEthernetMaxBandwidth =
    std::numeric_limits<double>::infinity();
const double kExpectedLteMaxBandwidth = 100;
const double kExpectedEvdoMaxBandwidth = 2.46;
const double kExpectedHspaMaxBandwidth = 3.6;

struct NotifierState {
  net::NetworkChangeNotifier::ConnectionType type;
  const char* service_path;
  const char* ip_address;
  const char* dns_servers;
  double max_bandwidth;
};

struct DefaultNetworkState {
  bool is_connected;
  const char* type;
  const char* network_technology;
  const char* service_path;
  const char* ip_address;
  const char* dns_servers;
};

struct NotifierUpdateTestCase {
  const char* test_description;
  NotifierState initial_state;
  DefaultNetworkState default_network_state;
  NotifierState expected_state;
  bool expected_type_changed;
  bool expected_ip_changed;
  bool expected_dns_changed;
  bool expected_max_bandwidth_changed;
};

} // namespace

using net::NetworkChangeNotifier;

TEST(NetworkChangeNotifierChromeosTest, ConnectionTypeFromShill) {
  struct TypeMapping {
    const char* shill_type;
    const char* technology;
    NetworkChangeNotifier::ConnectionType connection_type;
  };
  TypeMapping type_mappings[] = {
    { shill::kTypeEthernet, "", NetworkChangeNotifier::CONNECTION_ETHERNET },
    { shill::kTypeWifi, "", NetworkChangeNotifier::CONNECTION_WIFI },
    { shill::kTypeWimax, "", NetworkChangeNotifier::CONNECTION_4G },
    { "unknown type", "unknown technology",
      NetworkChangeNotifier::CONNECTION_UNKNOWN },
    { shill::kTypeCellular, shill::kNetworkTechnology1Xrtt,
      NetworkChangeNotifier::CONNECTION_2G },
    { shill::kTypeCellular, shill::kNetworkTechnologyGprs,
      NetworkChangeNotifier::CONNECTION_2G },
    { shill::kTypeCellular, shill::kNetworkTechnologyEdge,
      NetworkChangeNotifier::CONNECTION_2G },
    { shill::kTypeCellular, shill::kNetworkTechnologyEvdo,
      NetworkChangeNotifier::CONNECTION_3G },
    { shill::kTypeCellular, shill::kNetworkTechnologyGsm,
      NetworkChangeNotifier::CONNECTION_3G },
    { shill::kTypeCellular, shill::kNetworkTechnologyUmts,
      NetworkChangeNotifier::CONNECTION_3G },
    { shill::kTypeCellular, shill::kNetworkTechnologyHspa,
      NetworkChangeNotifier::CONNECTION_3G },
    { shill::kTypeCellular, shill::kNetworkTechnologyHspaPlus,
      NetworkChangeNotifier::CONNECTION_4G },
    { shill::kTypeCellular, shill::kNetworkTechnologyLte,
      NetworkChangeNotifier::CONNECTION_4G },
    { shill::kTypeCellular, shill::kNetworkTechnologyLteAdvanced,
      NetworkChangeNotifier::CONNECTION_4G },
    { shill::kTypeCellular, "unknown technology",
      NetworkChangeNotifier::CONNECTION_2G }
  };

  for (size_t i = 0; i < arraysize(type_mappings); ++i) {
    NetworkChangeNotifier::ConnectionType type =
        NetworkChangeNotifierChromeos::ConnectionTypeFromShill(
            type_mappings[i].shill_type, type_mappings[i].technology);
    EXPECT_EQ(type_mappings[i].connection_type, type);
  }
}

class NetworkChangeNotifierChromeosUpdateTest : public testing::Test {
 protected:
  NetworkChangeNotifierChromeosUpdateTest() : default_network_("") {
  }
  ~NetworkChangeNotifierChromeosUpdateTest() override = default;

  void SetNotifierState(const NotifierState& notifier_state) {
    notifier_.connection_type_ = notifier_state.type;
    notifier_.service_path_ = notifier_state.service_path;
    notifier_.ip_address_ = notifier_state.ip_address;
    notifier_.max_bandwidth_mbps_ = notifier_state.max_bandwidth;
    notifier_.dns_servers_ = notifier_state.dns_servers;
  }

  void VerifyNotifierState(const NotifierState& notifier_state) {
    EXPECT_EQ(notifier_state.type, notifier_.connection_type_);
    EXPECT_EQ(notifier_state.service_path, notifier_.service_path_);
    EXPECT_EQ(notifier_state.ip_address, notifier_.ip_address_);
    EXPECT_EQ(notifier_state.max_bandwidth, notifier_.max_bandwidth_mbps_);
    EXPECT_EQ(notifier_state.dns_servers, notifier_.dns_servers_);
  }

  // Sets the default network state used for notifier updates.
  void SetDefaultNetworkState(
      const DefaultNetworkState& default_network_state) {
    default_network_.visible_ = true;
    if (default_network_state.is_connected)
      default_network_.connection_state_ = shill::kStateOnline;
    else
      default_network_.connection_state_ = shill::kStateConfiguration;
    default_network_.type_ = default_network_state.type;
    default_network_.network_technology_ =
        default_network_state.network_technology;
    default_network_.path_ = default_network_state.service_path;
    default_network_.ipv4_config_.SetKey(
        shill::kAddressProperty, base::Value(default_network_state.ip_address));
    std::vector<std::string> dns_servers =
        base::SplitString(default_network_state.dns_servers, ",",
                          base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    base::ListValue dns_servers_value;
    dns_servers_value.AppendStrings(dns_servers);
    default_network_.ipv4_config_.SetKey(shill::kNameServersProperty,
                                         std::move(dns_servers_value));
  }

  // Process an default network update based on the state of |default_network_|.
  void ProcessDefaultNetworkUpdate(bool* type_changed,
                                   bool* ip_changed,
                                   bool* dns_changed,
                                   bool* max_bandwidth_changed) {
    notifier_.UpdateState(&default_network_, type_changed, ip_changed,
                          dns_changed, max_bandwidth_changed);
  }

 private:
  base::MessageLoop message_loop_;
  NetworkState default_network_;
  NetworkChangeNotifierChromeos notifier_;
};

NotifierUpdateTestCase test_cases[] = {
    {"Online -> Offline",
     {NetworkChangeNotifier::CONNECTION_ETHERNET,
      kService1,
      kIpAddress1,
      kDnsServers1,
      kExpectedEthernetMaxBandwidth},
     {false, shill::kTypeEthernet, "", kService1, "", ""},
     {NetworkChangeNotifier::CONNECTION_NONE,
      "",
      "",
      "",
      kExpectedNoneMaxBandwidth},
     true,
     true,
     true,
     true},
    {"Offline -> Offline",
     {NetworkChangeNotifier::CONNECTION_NONE,
      "",
      "",
      "",
      kExpectedNoneMaxBandwidth},
     {false, shill::kTypeEthernet, "", kService1, kIpAddress1, kDnsServers1},
     {NetworkChangeNotifier::CONNECTION_NONE,
      "",
      "",
      "",
      kExpectedNoneMaxBandwidth},
     false,
     false,
     false,
     false},
    {"Offline -> Online",
     {NetworkChangeNotifier::CONNECTION_NONE,
      "",
      "",
      "",
      kExpectedNoneMaxBandwidth},
     {true, shill::kTypeEthernet, "", kService1, kIpAddress1, kDnsServers1},
     {NetworkChangeNotifier::CONNECTION_ETHERNET,
      kService1,
      kIpAddress1,
      kDnsServers1,
      kExpectedEthernetMaxBandwidth},
     true,
     true,
     true,
     true},
    {"Online -> Online (new default service, different connection type)",
     {NetworkChangeNotifier::CONNECTION_ETHERNET,
      kService1,
      kIpAddress1,
      kDnsServers1,
      kExpectedEthernetMaxBandwidth},
     {true, shill::kTypeWifi, "", kService2, kIpAddress1, kDnsServers1},
     {NetworkChangeNotifier::CONNECTION_WIFI,
      kService2,
      kIpAddress1,
      kDnsServers1,
      kExpectedWifiMaxBandwidth},
     true,
     true,
     true,
     false},
    {"Online -> Online (new default service, same connection type)",
     {NetworkChangeNotifier::CONNECTION_WIFI,
      kService2,
      kIpAddress1,
      kDnsServers1,
      kExpectedWifiMaxBandwidth},
     {true, shill::kTypeWifi, "", kService3, kIpAddress1, kDnsServers1},
     {NetworkChangeNotifier::CONNECTION_WIFI,
      kService3,
      kIpAddress1,
      kDnsServers1,
      kExpectedWifiMaxBandwidth},
     false,
     true,
     true,
     false},
    {"Online -> Online (same default service, first IP address update)",
     {NetworkChangeNotifier::CONNECTION_WIFI,
      kService3,
      "",
      kDnsServers1,
      kExpectedWifiMaxBandwidth},
     {true, shill::kTypeWifi, "", kService3, kIpAddress2, kDnsServers1},
     {NetworkChangeNotifier::CONNECTION_WIFI,
      kService3,
      kIpAddress2,
      kDnsServers1,
      kExpectedWifiMaxBandwidth},
     false,
     false,
     false,
     false},
    {"Online -> Online (same default service, new IP address, same DNS)",
     {NetworkChangeNotifier::CONNECTION_WIFI,
      kService3,
      kIpAddress1,
      kDnsServers1,
      kExpectedWifiMaxBandwidth},
     {true, shill::kTypeWifi, "", kService3, kIpAddress2, kDnsServers1},
     {NetworkChangeNotifier::CONNECTION_WIFI,
      kService3,
      kIpAddress2,
      kDnsServers1,
      kExpectedWifiMaxBandwidth},
     false,
     true,
     false,
     false},
    {"Online -> Online (same default service, same IP address, new DNS)",
     {NetworkChangeNotifier::CONNECTION_WIFI,
      kService3,
      kIpAddress2,
      kDnsServers1,
      kExpectedWifiMaxBandwidth},
     {true, shill::kTypeWifi, "", kService3, kIpAddress2, kDnsServers2},
     {NetworkChangeNotifier::CONNECTION_WIFI,
      kService3,
      kIpAddress2,
      kDnsServers2,
      kExpectedWifiMaxBandwidth},
     false,
     false,
     true,
     false},
    {"Online -> Online (change of technology but not connection type)",
     {NetworkChangeNotifier::CONNECTION_3G,
      kService3,
      kIpAddress2,
      kDnsServers1,
      kExpectedEvdoMaxBandwidth},
     {true,
      shill::kTypeCellular,
      shill::kNetworkTechnologyHspa,
      kService3,
      kIpAddress2,
      kDnsServers1},
     {NetworkChangeNotifier::CONNECTION_3G,
      kService3,
      kIpAddress2,
      kDnsServers1,
      kExpectedHspaMaxBandwidth},
     false,
     false,
     false,
     true},
    {"Online -> Online (change of technology and connection type)",
     {NetworkChangeNotifier::CONNECTION_3G,
      kService3,
      kIpAddress2,
      kDnsServers1,
      kExpectedEvdoMaxBandwidth},
     {true,
      shill::kTypeCellular,
      shill::kNetworkTechnologyLte,
      kService3,
      kIpAddress2,
      kDnsServers1},
     {NetworkChangeNotifier::CONNECTION_4G,
      kService3,
      kIpAddress2,
      kDnsServers1,
      kExpectedLteMaxBandwidth},
     true,
     false,
     false,
     true}};

TEST_F(NetworkChangeNotifierChromeosUpdateTest, UpdateDefaultNetwork) {
  for (size_t i = 0; i < arraysize(test_cases); ++i) {
    SCOPED_TRACE(test_cases[i].test_description);
    SetNotifierState(test_cases[i].initial_state);
    SetDefaultNetworkState(test_cases[i].default_network_state);
    bool type_changed = false, ip_changed = false, dns_changed = false,
         max_bandwidth_changed = false;
    ProcessDefaultNetworkUpdate(&type_changed, &ip_changed, &dns_changed,
                                &max_bandwidth_changed);
    VerifyNotifierState(test_cases[i].expected_state);
    EXPECT_EQ(test_cases[i].expected_type_changed, type_changed);
    EXPECT_EQ(test_cases[i].expected_ip_changed, ip_changed);
    EXPECT_EQ(test_cases[i].expected_dns_changed, dns_changed);
    EXPECT_EQ(test_cases[i].expected_max_bandwidth_changed,
              max_bandwidth_changed);
  }
}

}  // namespace chromeos
