// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/posix/udp_socket.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <memory>

#include "absl/types/optional.h"
#include "osp_base/error.h"
#include "platform/api/logging.h"
#include "platform/posix/udp_socket.h"

namespace openscreen {
namespace platform {
namespace {

constexpr bool IsPowerOf2(uint32_t x) {
  return (x > 0) && ((x & (x - 1)) == 0);
}

static_assert(IsPowerOf2(alignof(struct cmsghdr)),
              "std::align requires power-of-2 alignment");

using IPv4NetworkInterfaceIndex = decltype(ip_mreqn().imr_ifindex);
using IPv6NetworkInterfaceIndex = decltype(ipv6_mreq().ipv6mr_interface);

ErrorOr<int> CreateNonBlockingUdpSocket(int domain) {
  int fd = socket(domain, SOCK_DGRAM, 0);
  if (fd == -1) {
    return Error(Error::Code::kInitializationFailure, strerror(errno));
  }
  // On non-Linux, the SOCK_NONBLOCK option is not available, so use the
  // more-portable method of calling fcntl() to set this behavior.
  if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
    close(fd);
    return Error(Error::Code::kInitializationFailure, strerror(errno));
  }
  return fd;
}

}  // namespace

UdpSocket::UdpSocket() = default;
UdpSocket::~UdpSocket() = default;

UdpSocketPosix::UdpSocketPosix(int fd, UdpSocket::Version version)
    : fd(fd), version(version) {}

// static
ErrorOr<UdpSocketUniquePtr> UdpSocket::Create(UdpSocket::Version version) {
  int domain;
  switch (version) {
    case Version::kV4:
      domain = AF_INET;
      break;
    case Version::kV6:
      domain = AF_INET6;
      break;
  }
  const ErrorOr<int> fd = CreateNonBlockingUdpSocket(domain);
  if (!fd) {
    return fd.error();
  }
  return UdpSocketUniquePtr(
      static_cast<UdpSocket*>(new UdpSocketPosix(fd.value(), version)));
}

bool UdpSocket::IsIPv4() const {
  return UdpSocketPosix::From(this)->version == UdpSocket::Version::kV4;
}

bool UdpSocket::IsIPv6() const {
  return UdpSocketPosix::From(this)->version == UdpSocket::Version::kV6;
}

void UdpSocketDeleter::operator()(UdpSocket* socket_api) const {
  auto* const socket = UdpSocketPosix::From(socket_api);
  close(socket->fd);
  delete socket;
}

Error UdpSocket::Bind(const IPEndpoint& endpoint) {
  auto* const socket = UdpSocketPosix::From(this);

  // This is effectively a boolean passed to setsockopt() to allow a future
  // bind() on the same socket to succeed, even if the address is already in
  // use. This is pretty much universally the desired behavior.
  const int reuse_addr = 1;
  if (setsockopt(socket->fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
                 sizeof(reuse_addr)) == -1) {
    return Error(Error::Code::kSocketOptionSettingFailure, strerror(errno));
  }

  switch (socket->version) {
    case UdpSocket::Version::kV4: {
      struct sockaddr_in address;
      address.sin_family = AF_INET;
      address.sin_port = htons(endpoint.port);
      endpoint.address.CopyToV4(
          reinterpret_cast<uint8_t*>(&address.sin_addr.s_addr));
      if (bind(socket->fd, reinterpret_cast<struct sockaddr*>(&address),
               sizeof(address)) == -1) {
        return Error(Error::Code::kSocketBindFailure, strerror(errno));
      }
      return Error::Code::kNone;
    }

    case UdpSocket::Version::kV6: {
      struct sockaddr_in6 address;
      address.sin6_family = AF_INET6;
      address.sin6_flowinfo = 0;
      address.sin6_port = htons(endpoint.port);
      endpoint.address.CopyToV6(reinterpret_cast<uint8_t*>(&address.sin6_addr));
      address.sin6_scope_id = 0;
      if (bind(socket->fd, reinterpret_cast<struct sockaddr*>(&address),
               sizeof(address)) == -1) {
        return Error(Error::Code::kSocketBindFailure, strerror(errno));
      }
      return Error::Code::kNone;
    }
  }

  OSP_NOTREACHED();
  return Error::Code::kGenericPlatformError;
}

Error UdpSocket::SetMulticastOutboundInterface(NetworkInterfaceIndex ifindex) {
  auto* const socket = UdpSocketPosix::From(this);

  switch (socket->version) {
    case UdpSocket::Version::kV4: {
      struct ip_mreqn multicast_properties;
      // Appropriate address is set based on |imr_ifindex| when set.
      multicast_properties.imr_address.s_addr = INADDR_ANY;
      multicast_properties.imr_multiaddr.s_addr = INADDR_ANY;
      multicast_properties.imr_ifindex =
          static_cast<IPv4NetworkInterfaceIndex>(ifindex);
      if (setsockopt(socket->fd, IPPROTO_IP, IP_MULTICAST_IF,
                     &multicast_properties,
                     sizeof(multicast_properties)) == -1) {
        return Error(Error::Code::kSocketOptionSettingFailure, strerror(errno));
      }
      return Error::Code::kNone;
    }

    case UdpSocket::Version::kV6: {
      const auto index = static_cast<IPv6NetworkInterfaceIndex>(ifindex);
      if (setsockopt(socket->fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &index,
                     sizeof(index)) == -1) {
        return Error(Error::Code::kSocketOptionSettingFailure, strerror(errno));
      }
      return Error::Code::kNone;
    }
  }

  OSP_NOTREACHED();
  return Error::Code::kGenericPlatformError;
}

Error UdpSocket::JoinMulticastGroup(const IPAddress& address,
                                    NetworkInterfaceIndex ifindex) {
  auto* const socket = UdpSocketPosix::From(this);

  switch (socket->version) {
    case UdpSocket::Version::kV4: {
      // Passed as data to setsockopt().  1 means return IP_PKTINFO control data
      // in recvmsg() calls.
      const int enable_pktinfo = 1;
      if (setsockopt(socket->fd, IPPROTO_IP, IP_PKTINFO, &enable_pktinfo,
                     sizeof(enable_pktinfo)) == -1) {
        return Error(Error::Code::kSocketOptionSettingFailure, strerror(errno));
      }
      struct ip_mreqn multicast_properties;
      // Appropriate address is set based on |imr_ifindex| when set.
      multicast_properties.imr_address.s_addr = INADDR_ANY;
      multicast_properties.imr_ifindex =
          static_cast<IPv4NetworkInterfaceIndex>(ifindex);
      static_assert(sizeof(multicast_properties.imr_multiaddr) == 4u,
                    "IPv4 address requires exactly 4 bytes");
      address.CopyToV4(
          reinterpret_cast<uint8_t*>(&multicast_properties.imr_multiaddr));
      if (setsockopt(socket->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                     &multicast_properties,
                     sizeof(multicast_properties)) == -1) {
        return Error(Error::Code::kSocketOptionSettingFailure, strerror(errno));
      }
      return Error::Code::kNone;
    }

    case UdpSocket::Version::kV6: {
      // Passed as data to setsockopt().  1 means return IPV6_PKTINFO control
      // data in recvmsg() calls.
      const int enable_pktinfo = 1;
      if (setsockopt(socket->fd, IPPROTO_IPV6, IPV6_RECVPKTINFO,
                     &enable_pktinfo, sizeof(enable_pktinfo)) == -1) {
        return Error(Error::Code::kSocketOptionSettingFailure, strerror(errno));
      }
      struct ipv6_mreq multicast_properties = {
          {/* filled-in below */},
          static_cast<IPv6NetworkInterfaceIndex>(ifindex),
      };
      static_assert(sizeof(multicast_properties.ipv6mr_multiaddr) == 16u,
                    "IPv6 address requires exactly 16 bytes");
      address.CopyToV6(
          reinterpret_cast<uint8_t*>(&multicast_properties.ipv6mr_multiaddr));
      // Portability note: All platforms support IPV6_JOIN_GROUP, which is
      // synonymous with IPV6_ADD_MEMBERSHIP.
      if (setsockopt(socket->fd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                     &multicast_properties,
                     sizeof(multicast_properties)) == -1) {
        return Error(Error::Code::kSocketOptionSettingFailure, strerror(errno));
      }
      return Error::Code::kNone;
    }
  }

  OSP_NOTREACHED();
  return Error::Code::kGenericPlatformError;
}

namespace {

// Examine |posix_errno| to determine whether the specific cause of a failure
// was transient or hard, and return the appropriate error response.
Error ChooseError(decltype(errno) posix_errno, Error::Code hard_error_code) {
  if (posix_errno == EAGAIN || posix_errno == EWOULDBLOCK ||
      posix_errno == ENOBUFS) {
    return Error(Error::Code::kAgain, strerror(errno));
  }
  return Error(hard_error_code, strerror(errno));
}

}  // namespace

ErrorOr<size_t> UdpSocket::ReceiveMessage(void* data,
                                          size_t length,
                                          IPEndpoint* src,
                                          IPEndpoint* original_destination) {
  auto* const socket = UdpSocketPosix::From(this);

  struct iovec iov = {data, length};
  char control_buf[1024];
  size_t cmsg_size = sizeof(control_buf) - sizeof(struct cmsghdr) + 1;
  void* cmsg_buf = control_buf;
  std::align(alignof(struct cmsghdr), sizeof(cmsg_buf), cmsg_buf, cmsg_size);
  switch (socket->version) {
    case UdpSocket::Version::kV4: {
      struct sockaddr_in sa;
      struct msghdr msg;
      msg.msg_name = &sa;
      msg.msg_namelen = sizeof(sa);
      msg.msg_iov = &iov;
      msg.msg_iovlen = 1;
      msg.msg_control = cmsg_buf;
      msg.msg_controllen = cmsg_size;
      msg.msg_flags = 0;

      ssize_t num_bytes_received = recvmsg(socket->fd, &msg, 0);
      if (num_bytes_received == -1) {
        return ChooseError(errno, Error::Code::kSocketReadFailure);
      }
      OSP_DCHECK_GE(num_bytes_received, 0);

      if (src) {
        src->address =
            IPAddress(IPAddress::Version::kV4,
                      reinterpret_cast<const uint8_t*>(&sa.sin_addr.s_addr));
        src->port = ntohs(sa.sin_port);
      }

      // For multicast sockets, the packet's original destination address may be
      // the host address (since we called bind()) but it may also be a
      // multicast address.  This may be relevant for handling multicast data;
      // specifically, mDNSResponder requires this information to work properly.
      if (original_destination) {
        *original_destination = IPEndpoint{{}, 0};
        if ((msg.msg_flags & MSG_CTRUNC) == 0) {
          for (struct cmsghdr* cmh = CMSG_FIRSTHDR(&msg); cmh;
               cmh = CMSG_NXTHDR(&msg, cmh)) {
            if (cmh->cmsg_level != IPPROTO_IP || cmh->cmsg_type != IP_PKTINFO)
              continue;

            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            if (getsockname(socket->fd,
                            reinterpret_cast<struct sockaddr*>(&addr),
                            &addr_len) == -1) {
              break;
            }
            // |original_destination->port| will be 0 if this line isn't
            // reached.
            original_destination->port = ntohs(addr.sin_port);

            struct in_pktinfo* pktinfo =
                reinterpret_cast<struct in_pktinfo*>(CMSG_DATA(cmh));
            original_destination->address =
                IPAddress(IPAddress::Version::kV4,
                          reinterpret_cast<const uint8_t*>(&pktinfo->ipi_addr));
            break;
          }
        }
      }

      return num_bytes_received;
    }

    case UdpSocket::Version::kV6: {
      struct sockaddr_in6 sa;
      struct msghdr msg;
      msg.msg_name = &sa;
      msg.msg_namelen = sizeof(sa);
      msg.msg_iov = &iov;
      msg.msg_iovlen = 1;
      msg.msg_control = cmsg_buf;
      msg.msg_controllen = cmsg_size;
      msg.msg_flags = 0;

      ssize_t num_bytes_received = recvmsg(socket->fd, &msg, 0);
      if (num_bytes_received == -1) {
        return ChooseError(errno, Error::Code::kSocketReadFailure);
      }
      OSP_DCHECK_GE(num_bytes_received, 0);

      if (src) {
        src->address =
            IPAddress(IPAddress::Version::kV6,
                      reinterpret_cast<const uint8_t*>(&sa.sin6_addr.s6_addr));
        src->port = ntohs(sa.sin6_port);
      }

      // For multicast sockets, the packet's original destination address may be
      // the host address (since we called bind()) but it may also be a
      // multicast address.  This may be relevant for handling multicast data;
      // specifically, mDNSResponder requires this information to work properly.
      if (original_destination) {
        *original_destination = IPEndpoint{{}, 0};
        if ((msg.msg_flags & MSG_CTRUNC) == 0) {
          for (struct cmsghdr* cmh = CMSG_FIRSTHDR(&msg); cmh;
               cmh = CMSG_NXTHDR(&msg, cmh)) {
            if (cmh->cmsg_level != IPPROTO_IPV6 ||
                cmh->cmsg_type != IPV6_PKTINFO) {
              continue;
            }
            struct sockaddr_in6 addr;
            socklen_t addr_len = sizeof(addr);
            if (getsockname(socket->fd,
                            reinterpret_cast<struct sockaddr*>(&addr),
                            &addr_len) == -1) {
              break;
            }
            // |original_destination->port| will be 0 if this line isn't
            // reached.
            original_destination->port = ntohs(addr.sin6_port);

            struct in6_pktinfo* pktinfo =
                reinterpret_cast<struct in6_pktinfo*>(CMSG_DATA(cmh));
            original_destination->address = IPAddress(
                IPAddress::Version::kV6,
                reinterpret_cast<const uint8_t*>(&pktinfo->ipi6_addr));
            break;
          }
        }
      }

      return num_bytes_received;
    }
  }

  OSP_NOTREACHED();
  return Error::Code::kGenericPlatformError;
}

Error UdpSocket::SendMessage(const void* data,
                             size_t length,
                             const IPEndpoint& dest) {
  auto* const socket = UdpSocketPosix::From(this);

  struct iovec iov = {const_cast<void*>(data), length};
  struct msghdr msg;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = nullptr;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;

  ssize_t num_bytes_sent = -2;
  switch (socket->version) {
    case UdpSocket::Version::kV4: {
      struct sockaddr_in sa = {
          .sin_family = AF_INET,
          .sin_port = htons(dest.port),
      };
      dest.address.CopyToV4(reinterpret_cast<uint8_t*>(&sa.sin_addr.s_addr));
      msg.msg_name = &sa;
      msg.msg_namelen = sizeof(sa);
      num_bytes_sent = sendmsg(socket->fd, &msg, 0);
      break;
    }

    case UdpSocket::Version::kV6: {
      struct sockaddr_in6 sa = {};
      sa.sin6_family = AF_INET6;
      sa.sin6_flowinfo = 0;
      sa.sin6_scope_id = 0;
      sa.sin6_port = htons(dest.port);
      dest.address.CopyToV6(reinterpret_cast<uint8_t*>(&sa.sin6_addr.s6_addr));
      msg.msg_name = &sa;
      msg.msg_namelen = sizeof(sa);
      num_bytes_sent = sendmsg(socket->fd, &msg, 0);
      break;
    }
  }

  if (num_bytes_sent == -1) {
    return ChooseError(errno, Error::Code::kSocketSendFailure);
  }
  // Sanity-check: UDP datagram sendmsg() is all or nothing.
  OSP_DCHECK_EQ(static_cast<size_t>(num_bytes_sent), length);
  return Error::Code::kNone;
}

Error UdpSocket::SetDscp(UdpSocket::DscpMode state) {
  auto* const socket = UdpSocketPosix::From(this);

  constexpr auto kSettingLevel = IPPROTO_IP;
  uint8_t code_array[1] = {static_cast<uint8_t>(state)};
  auto code = setsockopt(socket->fd, kSettingLevel, IP_TOS, code_array,
                         sizeof(uint8_t));

  if (code == EBADF || code == ENOTSOCK || code == EFAULT) {
    OSP_VLOG << "BAD SOCKET PROVIDED. CODE: " << code;
    return Error::Code::kSocketOptionSettingFailure;
  } else if (code == EINVAL) {
    OSP_VLOG << "INVALID DSCP INFO PROVIDED";
    return Error::Code::kSocketOptionSettingFailure;
  } else if (code == ENOPROTOOPT) {
    OSP_VLOG << "INVALID DSCP SETTING LEVEL PROVIDED: " << kSettingLevel;
    return Error::Code::kSocketOptionSettingFailure;
  }

  return Error::Code::kNone;
}

}  // namespace platform
}  // namespace openscreen
