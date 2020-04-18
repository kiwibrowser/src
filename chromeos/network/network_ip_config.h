// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_IP_CONFIG_H_
#define CHROMEOS_NETWORK_NETWORK_IP_CONFIG_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "chromeos/chromeos_export.h"

namespace chromeos {

// ipconfig types (see flimflam/files/doc/ipconfig-api.txt)
enum IPConfigType {
  IPCONFIG_TYPE_UNKNOWN,
  IPCONFIG_TYPE_IPV4,
  IPCONFIG_TYPE_IPV6,
  IPCONFIG_TYPE_DHCP,
  IPCONFIG_TYPE_BOOTP,  // Not Used.
  IPCONFIG_TYPE_ZEROCONF,
  IPCONFIG_TYPE_DHCP6,
  IPCONFIG_TYPE_PPP,
};

// IP Configuration.
struct CHROMEOS_EXPORT NetworkIPConfig {
  NetworkIPConfig(const std::string& device_path, IPConfigType type,
                  const std::string& address, const std::string& netmask,
                  const std::string& gateway, const std::string& name_servers);
  ~NetworkIPConfig();

  std::string ToString() const;

  std::string device_path;  // This looks like "/device/0011aa22bb33"
  IPConfigType type;
  std::string address;
  std::string netmask;
  std::string gateway;
  std::string name_servers;
};

typedef std::vector<NetworkIPConfig> NetworkIPConfigVector;

// Used to return the list of IP configs and hardware address from an
// asynchronous call to Shill. The hardware address is usually a MAC address
// like "0011AA22BB33". |hardware_address| will be an empty string, if no
// hardware address is found.
typedef base::Callback<void(const NetworkIPConfigVector& ip_configs,
                            const std::string& hardware_address)>
    NetworkGetIPConfigsCallback;

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_IP_CONFIG_H_
