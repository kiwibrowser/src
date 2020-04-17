// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/discovery_network_list.h"

#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/media/router/discovery/discovery_network_list_wifi.h"
#include "net/base/net_errors.h"

#if !defined(OS_MACOSX)
#include <netpacket/packet.h>
#else
#include <net/if_dl.h>
#endif

namespace media_router {
namespace {

#if !defined(OS_MACOSX)
using sll = struct sockaddr_ll;
#define SOCKET_ARP_TYPE(s) ((s)->sll_hatype)
#define SOCKET_ADDRESS_LEN(s) ((s)->sll_halen)
#define SOCKET_ADDRESS(s) ((s)->sll_addr)
#else  // defined(OS_MACOSX)
#define AF_PACKET AF_LINK
using sll = struct sockaddr_dl;
#define SOCKET_ARP_TYPE(s) ((s)->sdl_type)
#define SOCKET_ADDRESS_LEN(s) ((s)->sdl_alen)
#define SOCKET_ADDRESS(s) (LLADDR(s))
#endif

}  // namespace

std::vector<DiscoveryNetworkInfo> GetDiscoveryNetworkInfoList() {
  std::vector<DiscoveryNetworkInfo> network_ids;

  return network_ids;
}

}  // namespace media_router
