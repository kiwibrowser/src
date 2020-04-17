// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/network_interface.h"

// clang-format: off
#include <sys/socket.h>
// clang-format: on

#include <linux/ethtool.h>
#include <linux/if_arp.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/sockios.h>
#include <linux/wireless.h>
#include <netinet/ip.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>

#include "absl/strings/string_view.h"
#include "osp_base/ip_address.h"
#include "osp_base/scoped_pipe.h"
#include "platform/api/logging.h"

namespace openscreen {
namespace platform {
namespace {

constexpr int kNetlinkRecvmsgBufSize = 8192;

// Safely reads the system name for the interface from the (probably)
// null-terminated string |kernel_name| and returns a std::string.
std::string GetInterfaceName(absl::string_view kernel_name) {
  OSP_CHECK_LT(kernel_name.length(), IFNAMSIZ);
  return std::string(kernel_name);
}

// Returns the type of the interface identified by the name |ifname|, if it can
// be determined, otherwise returns InterfaceInfo::Type::kOther.
InterfaceInfo::Type GetInterfaceType(const std::string& ifname) {
  // Determine type after name has been set.
  ScopedFd s(socket(AF_INET6, SOCK_DGRAM, 0));
  if (!s) {
    s = ScopedFd(socket(AF_INET, SOCK_DGRAM, 0));
    if (!s)
      return InterfaceInfo::Type::kOther;
  }

  // Note: This uses Wireless Extensions to test the interface, which is
  // deprecated.  However, it's much easier than using the new nl80211
  // interface for this purpose.  If Wireless Extensions are ever actually
  // removed though, this will need to use nl80211.
  struct iwreq wr;
  static_assert(sizeof(wr.ifr_name) == IFNAMSIZ,
                "expected size of interface name fields");
  OSP_CHECK_LT(ifname.size(), IFNAMSIZ);
  strncpy(wr.ifr_name, ifname.c_str(), IFNAMSIZ);
  if (ioctl(s.get(), SIOCGIWNAME, &wr) != -1)
    return InterfaceInfo::Type::kWifi;

  struct ethtool_cmd ecmd;
  ecmd.cmd = ETHTOOL_GSET;
  struct ifreq ifr;
  static_assert(sizeof(ifr.ifr_name) == IFNAMSIZ,
                "expected size of interface name fields");
  OSP_CHECK_LT(ifname.size(), IFNAMSIZ);
  strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ);
  ifr.ifr_data = &ecmd;
  if (ioctl(s.get(), SIOCETHTOOL, &ifr) != -1)
    return InterfaceInfo::Type::kEthernet;

  return InterfaceInfo::Type::kOther;
}

// Reads an interface's name, hardware address, and type from |rta| and places
// the results in |info|.  |rta| is the first attribute structure returned as
// part of an RTM_NEWLINK message.  |attrlen| is the total length of the buffer
// pointed to by |rta|.
void GetInterfaceAttributes(struct rtattr* rta,
                            unsigned int attrlen,
                            InterfaceInfo* info) {
  for (; RTA_OK(rta, attrlen); rta = RTA_NEXT(rta, attrlen)) {
    if (rta->rta_type == IFLA_IFNAME) {
      info->name =
          GetInterfaceName(reinterpret_cast<const char*>(RTA_DATA(rta)));
    } else if (rta->rta_type == IFLA_ADDRESS) {
      OSP_CHECK_EQ(sizeof(info->hardware_address), RTA_PAYLOAD(rta));
      std::memcpy(info->hardware_address, RTA_DATA(rta),
                  sizeof(info->hardware_address));
    }
  }

  info->type = GetInterfaceType(info->name);
}

InterfaceAddresses* FindOrAddAddressesForIndex(
    std::vector<InterfaceAddresses>* address_list,
    const std::vector<InterfaceInfo>& info_list,
    NetworkInterfaceIndex index) {
  const auto info_it = std::find_if(
      info_list.begin(), info_list.end(),
      [index](const InterfaceInfo& info) { return info.index == index; });
  if (info_it == info_list.end())
    return nullptr;

  auto addr_it = std::find_if(address_list->begin(), address_list->end(),
                              [index](const InterfaceAddresses& addresses) {
                                return addresses.info.index == index;
                              });
  if (addr_it != address_list->end())
    return &(*addr_it);

  address_list->emplace_back();
  InterfaceAddresses& addr = address_list->back();
  addr.info = *info_it;
  return &addr;
}

// Reads the IPv4 address that comes from an RTM_NEWADDR message and places the
// result in |address|.  |rta| is the first attribute structure returned by the
// message and |attrlen| is the total length of the buffer pointed to by |rta|.
// |ifname| is the name of the interface to which we believe the address belongs
// based on interface index matching.  It is only used for sanity checking in
// debug builds.
void GetIPv4Address(struct rtattr* rta,
                    unsigned int attrlen,
                    const std::string& ifname,
                    IPAddress* address) {
  bool have_local = false;
  IPAddress local;
  for (; RTA_OK(rta, attrlen); rta = RTA_NEXT(rta, attrlen)) {
    if (rta->rta_type == IFA_LABEL) {
      OSP_DCHECK_EQ(ifname, reinterpret_cast<const char*>(RTA_DATA(rta)));
    } else if (rta->rta_type == IFA_ADDRESS) {
      OSP_DCHECK_EQ(IPAddress::kV4Size, RTA_PAYLOAD(rta));
      *address = IPAddress(IPAddress::Version::kV4,
                           static_cast<uint8_t*>(RTA_DATA(rta)));
    } else if (rta->rta_type == IFA_LOCAL) {
      OSP_DCHECK_EQ(IPAddress::kV4Size, RTA_PAYLOAD(rta));
      have_local = true;
      local = IPAddress(IPAddress::Version::kV4,
                        static_cast<uint8_t*>(RTA_DATA(rta)));
    }
  }
  if (have_local)
    *address = local;
}

// Reads the IPv6 address that comes from an RTM_NEWADDR message and places the
// result in |address|.  |rta| is the first attribute structure returned by the
// message and |attrlen| is the total length of the buffer pointed to by |rta|.
// |ifname| is the name of the interface to which we believe the address belongs
// based on interface index matching.  It is only used for sanity checking in
// debug builds.
void GetIPv6Address(struct rtattr* rta,
                    unsigned int attrlen,
                    const std::string& ifname,
                    IPAddress* address) {
  bool have_local = false;
  IPAddress local;
  for (; RTA_OK(rta, attrlen); rta = RTA_NEXT(rta, attrlen)) {
    if (rta->rta_type == IFA_LABEL) {
      OSP_DCHECK_EQ(ifname, reinterpret_cast<const char*>(RTA_DATA(rta)));
    } else if (rta->rta_type == IFA_ADDRESS) {
      OSP_DCHECK_EQ(IPAddress::kV6Size, RTA_PAYLOAD(rta));
      *address = IPAddress(IPAddress::Version::kV6,
                           static_cast<uint8_t*>(RTA_DATA(rta)));
    } else if (rta->rta_type == IFA_LOCAL) {
      OSP_DCHECK_EQ(IPAddress::kV6Size, RTA_PAYLOAD(rta));
      have_local = true;
      local = IPAddress(IPAddress::Version::kV6,
                        static_cast<uint8_t*>(RTA_DATA(rta)));
    }
  }
  if (have_local)
    *address = local;
}

std::vector<InterfaceInfo> GetLinkInfo() {
  ScopedFd fd(socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE));
  if (!fd) {
    OSP_LOG_WARN << "netlink socket() failed: " << errno << " - "
                 << strerror(errno);
    return {};
  }

  {
    // nl_pid = 0 for the kernel.
    struct sockaddr_nl peer = {};
    peer.nl_family = AF_NETLINK;
    struct {
      struct nlmsghdr header;
      struct ifinfomsg msg;
    } request;

    request.header.nlmsg_len = sizeof(request);
    request.header.nlmsg_type = RTM_GETLINK;
    request.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
    request.header.nlmsg_seq = 0;
    request.header.nlmsg_pid = 0;
    request.msg.ifi_family = AF_UNSPEC;
    struct iovec iov = {&request, request.header.nlmsg_len};
    struct msghdr msg = {&peer,
                         sizeof(peer),
                         &iov,
                         /* msg_iovlen */ 1,
                         /* msg_control */ nullptr,
                         /* msg_controllen */ 0,
                         /* msg_flags */ 0};
    if (sendmsg(fd.get(), &msg, 0) < 0) {
      OSP_LOG_ERROR << "netlink sendmsg() failed: " << errno << " - "
                    << strerror(errno);
      return {};
    }
  }

  std::vector<InterfaceInfo> info_list;
  {
    char buf[kNetlinkRecvmsgBufSize];
    struct iovec iov = {buf, sizeof(buf)};
    struct sockaddr_nl source_address;
    struct msghdr msg;
    struct nlmsghdr* netlink_header;

    msg = {&source_address,           sizeof(source_address), &iov,
           /* msg_iovlen */ 1,
           /* msg_control */ nullptr,
           /* msg_controllen */ 0,
           /* msg_flags */ 0};

    bool done = false;
    while (!done) {
      size_t len = recvmsg(fd.get(), &msg, 0);

      for (netlink_header = reinterpret_cast<struct nlmsghdr*>(buf);
           NLMSG_OK(netlink_header, len);
           netlink_header = NLMSG_NEXT(netlink_header, len)) {
        // The end of multipart message.
        if (netlink_header->nlmsg_type == NLMSG_DONE) {
          done = true;
          break;
        } else if (netlink_header->nlmsg_type == NLMSG_ERROR) {
          done = true;
          OSP_LOG_ERROR << "netlink error msg: "
                        << reinterpret_cast<struct nlmsgerr*>(
                               NLMSG_DATA(netlink_header))
                               ->error;
          continue;
        } else if ((netlink_header->nlmsg_flags & NLM_F_MULTI) == 0) {
          // If this is not a multi-part message, we don't need to wait for an
          // NLMSG_DONE message; this is the only message.
          done = true;
        }

        // RTM_NEWLINK messages describe existing network links on the host.
        if (netlink_header->nlmsg_type != RTM_NEWLINK)
          continue;

        struct ifinfomsg* interface_info =
            static_cast<struct ifinfomsg*>(NLMSG_DATA(netlink_header));
        // Only process non-loopback interfaces which are active (up).
        if ((interface_info->ifi_flags & IFF_LOOPBACK) ||
            ((interface_info->ifi_flags & IFF_UP) == 0)) {
          continue;
        }
        info_list.emplace_back();
        InterfaceInfo& info = info_list.back();
        info.index = interface_info->ifi_index;
        GetInterfaceAttributes(IFLA_RTA(interface_info),
                               IFLA_PAYLOAD(netlink_header), &info);
      }
    }
  }

  return info_list;
}

std::vector<InterfaceAddresses> GetAddressInfo(
    const std::vector<InterfaceInfo>& info_list) {
  ScopedFd fd(socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE));
  if (!fd) {
    OSP_LOG_ERROR << "netlink socket() failed: " << errno << " - "
                  << strerror(errno);
    return {};
  }

  {
    // nl_pid = 0 for the kernel.
    struct sockaddr_nl peer = {};
    peer.nl_family = AF_NETLINK;
    struct {
      struct nlmsghdr header;
      struct ifaddrmsg msg;
    } request;

    request.header.nlmsg_len = sizeof(request);
    request.header.nlmsg_type = RTM_GETADDR;
    request.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
    request.header.nlmsg_seq = 1;
    request.header.nlmsg_pid = 0;
    request.msg.ifa_family = AF_UNSPEC;
    struct iovec iov = {&request, request.header.nlmsg_len};
    struct msghdr msg = {&peer,
                         sizeof(peer),
                         &iov,
                         /* msg_iovlen */ 1,
                         /* msg_control */ nullptr,
                         /* msg_controllen */ 0,
                         /* msg_flags */ 0};
    if (sendmsg(fd.get(), &msg, 0) < 0) {
      OSP_LOG_ERROR << "sendmsg failed: " << errno << " - " << strerror(errno);
      return {};
    }
  }

  std::vector<InterfaceAddresses> address_list;
  {
    char buf[kNetlinkRecvmsgBufSize];
    struct iovec iov = {buf, sizeof(buf)};
    struct sockaddr_nl source_address;
    struct msghdr msg;
    struct nlmsghdr* netlink_header;

    msg = {&source_address,           sizeof(source_address), &iov,
           /* msg_iovlen */ 1,
           /* msg_control */ nullptr,
           /* msg_controllen */ 0,
           /* msg_flags */ 0};
    bool done = false;
    while (!done) {
      size_t len = recvmsg(fd.get(), &msg, 0);

      for (netlink_header = reinterpret_cast<struct nlmsghdr*>(buf);
           NLMSG_OK(netlink_header, len);
           netlink_header = NLMSG_NEXT(netlink_header, len)) {
        if (netlink_header->nlmsg_type == NLMSG_DONE) {
          done = true;
          break;
        } else if (netlink_header->nlmsg_type == NLMSG_ERROR) {
          done = true;
          OSP_LOG_ERROR << "netlink error msg: "
                        << reinterpret_cast<struct nlmsgerr*>(
                               NLMSG_DATA(netlink_header))
                               ->error;
          continue;
        } else if ((netlink_header->nlmsg_flags & NLM_F_MULTI) == 0) {
          // If this is not a multi-part message, we don't need to wait for an
          // NLMSG_DONE message; this is the only message.
          done = true;
        }

        if (netlink_header->nlmsg_type != RTM_NEWADDR)
          continue;

        struct ifaddrmsg* interface_address =
            static_cast<struct ifaddrmsg*>(NLMSG_DATA(netlink_header));
        InterfaceAddresses* addresses = FindOrAddAddressesForIndex(
            &address_list, info_list, interface_address->ifa_index);
        if (!addresses) {
          OSP_DVLOG << "skipping address for interface "
                    << interface_address->ifa_index;
          continue;
        }

        if (interface_address->ifa_family == AF_INET) {
          addresses->addresses.emplace_back();
          IPSubnet& address = addresses->addresses.back();
          address.prefix_length = interface_address->ifa_prefixlen;

          GetIPv4Address(IFA_RTA(interface_address),
                         IFA_PAYLOAD(netlink_header), addresses->info.name,
                         &address.address);
        } else if (interface_address->ifa_family == AF_INET6) {
          addresses->addresses.emplace_back();
          IPSubnet& address = addresses->addresses.back();
          address.prefix_length = interface_address->ifa_prefixlen;

          GetIPv6Address(IFA_RTA(interface_address),
                         IFA_PAYLOAD(netlink_header), addresses->info.name,
                         &address.address);
        } else {
          OSP_LOG_ERROR << "Unknown address family: "
                        << interface_address->ifa_family;
        }
      }
    }
  }

  return address_list;
}

}  // namespace

std::vector<InterfaceAddresses> GetInterfaceAddresses() {
  return GetAddressInfo(GetLinkInfo());
}

}  // namespace platform
}  // namespace openscreen
