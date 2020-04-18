// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/bind.h"
#include "base/location.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/network/network_change_notifier_chromeos.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "net/dns/dns_config_service_posix.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

// DNS config services on Chrome OS are signalled by the network state handler
// rather than relying on watching files in /etc.
class NetworkChangeNotifierChromeos::DnsConfigService
    : public net::internal::DnsConfigServicePosix {
 public:
  DnsConfigService();
  ~DnsConfigService() override;

  // net::internal::DnsConfigService() overrides.
  bool StartWatching() override;

  virtual void OnNetworkChange();
};

NetworkChangeNotifierChromeos::DnsConfigService::DnsConfigService() = default;

NetworkChangeNotifierChromeos::DnsConfigService::~DnsConfigService() = default;

bool NetworkChangeNotifierChromeos::DnsConfigService::StartWatching() {
  // DNS config changes are handled and notified by the network state handlers.
  return true;
}

void NetworkChangeNotifierChromeos::DnsConfigService::OnNetworkChange() {
  InvalidateConfig();
  InvalidateHosts();
  ReadNow();
}

NetworkChangeNotifierChromeos::NetworkChangeNotifierChromeos()
    : NetworkChangeNotifier(NetworkChangeCalculatorParamsChromeos()),
      connection_type_(CONNECTION_NONE),
      max_bandwidth_mbps_(
          NetworkChangeNotifier::GetMaxBandwidthMbpsForConnectionSubtype(
              SUBTYPE_NONE)),
      task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {
  poll_callback_ = base::Bind(&NetworkChangeNotifierChromeos::PollForState,
                              weak_ptr_factory_.GetWeakPtr());
}

NetworkChangeNotifierChromeos::~NetworkChangeNotifierChromeos() = default;

void NetworkChangeNotifierChromeos::Initialize() {
  DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(this);
  NetworkHandler::Get()->network_state_handler()->AddObserver(this, FROM_HERE);

  dns_config_service_.reset(new DnsConfigService());
  dns_config_service_->WatchConfig(
      base::Bind(net::NetworkChangeNotifier::SetDnsConfig));

  PollForState();
}

void NetworkChangeNotifierChromeos::PollForState() {
  // Update initial connection state.
  DefaultNetworkChanged(
      NetworkHandler::Get()->network_state_handler()->DefaultNetwork());
}

void NetworkChangeNotifierChromeos::Shutdown() {
  dns_config_service_.reset();
  NetworkHandler::Get()->network_state_handler()->RemoveObserver(
      this, FROM_HERE);
  DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(this);
}

net::NetworkChangeNotifier::ConnectionType
NetworkChangeNotifierChromeos::GetCurrentConnectionType() const {
  // Re-evaluate connection state if we are offline since there is little
  // cost to doing so.  Since we are in the context of a const method,
  // this is done through a closure that holds a non-const reference to
  // |this|, to allow PollForState() to modify our cached state.
  // TODO(gauravsh): Figure out why we would have missed this notification.
  if (connection_type_ == CONNECTION_NONE)
    task_runner_->PostTask(FROM_HERE, poll_callback_);
  return connection_type_;
}

void NetworkChangeNotifierChromeos::GetCurrentMaxBandwidthAndConnectionType(
    double* max_bandwidth_mbps,
    ConnectionType* connection_type) const {
  *connection_type = connection_type_;
  *max_bandwidth_mbps = max_bandwidth_mbps_;
}

void NetworkChangeNotifierChromeos::SuspendDone(
    const base::TimeDelta& sleep_duration) {
  // Force invalidation of network resources on resume.
  NetworkChangeNotifier::NotifyObserversOfIPAddressChange();
}


void NetworkChangeNotifierChromeos::DefaultNetworkChanged(
    const chromeos::NetworkState* default_network) {
  bool connection_type_changed = false;
  bool ip_address_changed = false;
  bool dns_changed = false;
  bool max_bandwidth_changed = false;

  UpdateState(default_network, &connection_type_changed, &ip_address_changed,
              &dns_changed, &max_bandwidth_changed);

  if (connection_type_changed)
    NetworkChangeNotifier::NotifyObserversOfConnectionTypeChange();
  if (ip_address_changed)
    NetworkChangeNotifier::NotifyObserversOfIPAddressChange();
  if (dns_changed)
    dns_config_service_->OnNetworkChange();
  if (max_bandwidth_changed || connection_type_changed) {
    NetworkChangeNotifier::NotifyObserversOfMaxBandwidthChange(
        max_bandwidth_mbps_, connection_type_);
  }
}

void NetworkChangeNotifierChromeos::UpdateState(
    const chromeos::NetworkState* default_network,
    bool* connection_type_changed,
    bool* ip_address_changed,
    bool* dns_changed,
    bool* max_bandwidth_changed) {
  *connection_type_changed = false;
  *ip_address_changed = false;
  *dns_changed = false;
  *max_bandwidth_changed = false;

  if (!default_network || !default_network->IsConnectedState()) {
    // If we lost a default network, we must update our state and notify
    // observers, otherwise we have nothing to do.
    if (connection_type_ != CONNECTION_NONE) {
      NET_LOG_EVENT("NCNDefaultNetworkLost", service_path_);
      *ip_address_changed = true;
      *dns_changed = true;
      *connection_type_changed = true;
      *max_bandwidth_changed = true;
      connection_type_ = CONNECTION_NONE;
      max_bandwidth_mbps_ =
          GetMaxBandwidthMbpsForConnectionSubtype(SUBTYPE_NONE);
      service_path_.clear();
      ip_address_.clear();
      dns_servers_.clear();
    }
    return;
  }

  // We do have a default network and it is connected.
  net::NetworkChangeNotifier::ConnectionType new_connection_type =
      ConnectionTypeFromShill(default_network->type(),
                              default_network->network_technology());
  if (new_connection_type != connection_type_) {
    NET_LOG_EVENT(
        "NCNDefaultConnectionTypeChanged",
        base::StringPrintf("%s -> %s", ConnectionTypeToString(connection_type_),
                           ConnectionTypeToString(new_connection_type)));
    *connection_type_changed = true;
  }
  if (default_network->path() != service_path_) {
    NET_LOG_EVENT("NCNDefaultNetworkServicePathChanged",
                  base::StringPrintf("%s -> %s", service_path_.c_str(),
                                     default_network->path().c_str()));

    // If we had a default network service change, network resources
    // must always be invalidated.
    *ip_address_changed = true;
    *dns_changed = true;
  }

  std::string new_ip_address = default_network->GetIpAddress();
  if (new_ip_address != ip_address_) {
    // Is this a state update with an online->online transition?
    bool stayed_online =
        (!*connection_type_changed && connection_type_ != CONNECTION_NONE);

    bool is_suppressed = true;
    // Suppress IP address change signalling on online->online transitions
    // when getting an IP address update for the first time.
    if (!(stayed_online && ip_address_.empty())) {
      is_suppressed = false;
      *ip_address_changed = true;
    }
    NET_LOG_EVENT(base::StringPrintf("%s%s", "NCNDefaultIPAddressChanged",
                                     is_suppressed ? " (Suppressed)" : ""),
                  base::StringPrintf("%s -> %s", ip_address_.c_str(),
                                     new_ip_address.c_str()));
  }
  std::string new_dns_servers = default_network->GetDnsServersAsString();
  if (new_dns_servers != dns_servers_) {
    NET_LOG_EVENT("NCNDefaultDNSServerChanged",
                  base::StringPrintf("%s -> %s", dns_servers_.c_str(),
                                     new_dns_servers.c_str()));
    *dns_changed = true;
  }

  connection_type_ = new_connection_type;
  service_path_ = default_network->path();
  ip_address_ = new_ip_address;
  dns_servers_ = new_dns_servers;
  double old_max_bandwidth = max_bandwidth_mbps_;
  max_bandwidth_mbps_ =
      GetMaxBandwidthMbpsForConnectionSubtype(GetConnectionSubtype(
          default_network->type(), default_network->network_technology()));
  if (max_bandwidth_mbps_ != old_max_bandwidth)
    *max_bandwidth_changed = true;
}

// static
net::NetworkChangeNotifier::ConnectionType
NetworkChangeNotifierChromeos::ConnectionTypeFromShill(
    const std::string& type, const std::string& technology) {
  if (NetworkTypePattern::Ethernet().MatchesType(type))
    return CONNECTION_ETHERNET;
  else if (type == shill::kTypeWifi)
    return CONNECTION_WIFI;
  else if (type == shill::kTypeWimax)
    return CONNECTION_4G;
  else if (type == shill::kTypeBluetooth)
    return CONNECTION_BLUETOOTH;

  if (type != shill::kTypeCellular)
    return CONNECTION_UNKNOWN;

  // For cellular types, mapping depends on the technology.
  if (technology == shill::kNetworkTechnologyEvdo ||
      technology == shill::kNetworkTechnologyGsm ||
      technology == shill::kNetworkTechnologyUmts ||
      technology == shill::kNetworkTechnologyHspa) {
    return CONNECTION_3G;
  } else if (technology == shill::kNetworkTechnologyHspaPlus ||
             technology == shill::kNetworkTechnologyLte ||
             technology == shill::kNetworkTechnologyLteAdvanced) {
    return CONNECTION_4G;
  } else {
    return CONNECTION_2G;  // Default cellular type is 2G.
  }
}

// static
net::NetworkChangeNotifier::ConnectionSubtype
NetworkChangeNotifierChromeos::GetConnectionSubtype(
    const std::string& type,
    const std::string& technology) {
  if (type != shill::kTypeCellular)
    return SUBTYPE_UNKNOWN;

  if (technology == shill::kNetworkTechnology1Xrtt)
    return SUBTYPE_1XRTT;
  if (technology == shill::kNetworkTechnologyEvdo)
    return SUBTYPE_EVDO_REV_0;
  if (technology == shill::kNetworkTechnologyGsm)
    return SUBTYPE_GSM;
  if (technology == shill::kNetworkTechnologyGprs)
    return SUBTYPE_GPRS;
  if (technology == shill::kNetworkTechnologyEdge)
    return SUBTYPE_EDGE;
  if (technology == shill::kNetworkTechnologyUmts)
    return SUBTYPE_UMTS;
  if (technology == shill::kNetworkTechnologyHspa)
    return SUBTYPE_HSPA;
  if (technology == shill::kNetworkTechnologyHspaPlus)
    return SUBTYPE_HSPAP;
  if (technology == shill::kNetworkTechnologyLte)
    return SUBTYPE_LTE;
  if (technology == shill::kNetworkTechnologyLteAdvanced)
    return SUBTYPE_LTE_ADVANCED;

  return SUBTYPE_UNKNOWN;
}

// static
net::NetworkChangeNotifier::NetworkChangeCalculatorParams
NetworkChangeNotifierChromeos::NetworkChangeCalculatorParamsChromeos() {
  NetworkChangeCalculatorParams params;
  // Delay values arrived at by simple experimentation and adjusted so as to
  // produce a single signal when switching between network connections.
  params.ip_address_offline_delay_ = base::TimeDelta::FromMilliseconds(4000);
  params.ip_address_online_delay_ = base::TimeDelta::FromMilliseconds(1000);
  params.connection_type_offline_delay_ =
      base::TimeDelta::FromMilliseconds(500);
  params.connection_type_online_delay_ = base::TimeDelta::FromMilliseconds(500);
  return params;
}

}  // namespace chromeos
