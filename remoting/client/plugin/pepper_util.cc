// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/pepper_util.h"

#include "base/bit_cast.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/sys_byteorder.h"
#include "ppapi/cpp/module.h"
#include "ppapi/cpp/net_address.h"
#include "third_party/webrtc/rtc_base/socketaddress.h"

namespace remoting {

bool SocketAddressToPpNetAddressWithPort(
    const pp::InstanceHandle& instance,
    const rtc::SocketAddress& address,
    pp::NetAddress* pp_address,
    uint16_t port) {
  switch (address.ipaddr().family()) {
    case AF_INET: {
      in_addr ipv4_addr = address.ipaddr().ipv4_address();
      PP_NetAddress_IPv4 ip_addr;
      ip_addr.port = base::HostToNet16(port);
      memcpy(&ip_addr.addr, &ipv4_addr, sizeof(ip_addr.addr));
      *pp_address = pp::NetAddress(instance, ip_addr);
      return true;
    }
    case AF_INET6: {
      in6_addr ipv6_addr = address.ipaddr().ipv6_address();
      PP_NetAddress_IPv6 ip_addr;
      ip_addr.port = base::HostToNet16(port);
      memcpy(&ip_addr.addr, &ipv6_addr, sizeof(ip_addr.addr));
      *pp_address = pp::NetAddress(instance, ip_addr);
      return true;
    }
    default: {
      LOG(WARNING) << "Unknown address family: " << address.ipaddr().family();
      return false;
    }
  }
}

bool SocketAddressToPpNetAddress(const pp::InstanceHandle& instance,
                                 const rtc::SocketAddress& address,
                                 pp::NetAddress* pp_net_address) {
  return SocketAddressToPpNetAddressWithPort(instance,
                                             address,
                                             pp_net_address,
                                             address.port());
}

void PpNetAddressToSocketAddress(const pp::NetAddress& pp_net_address,
                                 rtc::SocketAddress* address) {
  switch (pp_net_address.GetFamily()) {
    case PP_NETADDRESS_FAMILY_IPV4: {
      PP_NetAddress_IPv4 ipv4_addr;
      CHECK(pp_net_address.DescribeAsIPv4Address(&ipv4_addr));
      address->SetIP(rtc::IPAddress(
                         bit_cast<in_addr>(ipv4_addr.addr)));
      address->SetPort(base::NetToHost16(ipv4_addr.port));
      return;
    }
    case PP_NETADDRESS_FAMILY_IPV6: {
      PP_NetAddress_IPv6 ipv6_addr;
      CHECK(pp_net_address.DescribeAsIPv6Address(&ipv6_addr));
      address->SetIP(rtc::IPAddress(
                         bit_cast<in6_addr>(ipv6_addr.addr)));
      address->SetPort(base::NetToHost16(ipv6_addr.port));
      return;
    }
    case PP_NETADDRESS_FAMILY_UNSPECIFIED: {
      break;
    }
  };

  NOTREACHED();
  address->Clear();
}

}  // namespace remoting
