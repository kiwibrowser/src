// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_API_UDP_SOCKET_H_
#define PLATFORM_API_UDP_SOCKET_H_

#include <cstdint>
#include <memory>

#include "osp_base/error.h"
#include "osp_base/ip_address.h"
#include "osp_base/macros.h"
#include "platform/api/network_interface.h"

namespace openscreen {
namespace platform {

class UdpSocket;

// Platform-specific deleter of a UdpSocket instance returned by
// UdpSocket::Create().
struct UdpSocketDeleter {
  void operator()(UdpSocket* socket) const;
};

using UdpSocketUniquePtr = std::unique_ptr<UdpSocket, UdpSocketDeleter>;

// An open UDP socket for sending/receiving datagrams to/from either specific
// endpoints or over IP multicast.
//
// Usage: The socket is created and opened by calling the Create() method. This
// returns a unique pointer that auto-closes/destroys the socket when it goes
// out-of-scope.
//
// Platform implementation note: There must only be one platform-specific
// implementation of UdpSocket linked into the library/application. For that
// reason, none of the methods here are declared virtual (i.e., the overhead is
// pure waste). However, UdpSocket can be subclassed to include all extra
// private state, such as OS-specific handles. See UdpSocketPosix for a
// reference implementation.
class UdpSocket {
 public:
  // Constants used to specify how we want packets sent from this socket.
  enum class DscpMode : uint8_t {
    // Default value set by the system on creation of a new socket.
    kUnspecified = 0x0,

    // Mode for Audio only.
    kAudioOnly = 0xb8,

    // Mode for Audio + Video.
    kAudioVideo = 0x88,

    // Mode for low priority operations such as trace log data.
    kLowPriority = 0x20
  };

  using Version = IPAddress::Version;

  // Creates a new, scoped UdpSocket within the IPv4 or IPv6 family.
  static ErrorOr<UdpSocketUniquePtr> Create(Version version);

  // Returns true if |socket| belongs to the IPv4/IPv6 address family.
  bool IsIPv4() const;
  bool IsIPv6() const;

  // Sets the socket for address reuse, binds to the address/port.
  Error Bind(const IPEndpoint& local_endpoint);

  // Sets the device to use for outgoing multicast packets on the socket.
  Error SetMulticastOutboundInterface(NetworkInterfaceIndex ifindex);

  // Joins to the multicast group at the given address, using the specified
  // interface.
  Error JoinMulticastGroup(const IPAddress& address,
                           NetworkInterfaceIndex ifindex);

  // Performs a non-blocking read on the socket, returning the number of bytes
  // received. Note that a non-Error return value of 0 is a valid result,
  // indicating an empty message has been received. Also note that
  // Error::Code::kAgain might be returned if there is no message currently
  // ready for receive, which can be expected during normal operation. |src| and
  // |original_destination| are optional output arguments that provide the
  // source of the message and its intended destination, respectively.
  ErrorOr<size_t> ReceiveMessage(void* data,
                                 size_t length,
                                 IPEndpoint* src,
                                 IPEndpoint* original_destination);

  // Sends a message and returns the number of bytes sent, on success.
  // Error::Code::kAgain might be returned to indicate the operation would
  // block, which can be expected during normal operation.
  Error SendMessage(const void* data, size_t length, const IPEndpoint& dest);

  // Sets the DSCP value to use for all messages sent from this socket.
  Error SetDscp(DscpMode state);

 protected:
  UdpSocket();
  ~UdpSocket();

 private:
  OSP_DISALLOW_COPY_AND_ASSIGN(UdpSocket);
};

}  // namespace platform
}  // namespace openscreen

#endif  // PLATFORM_API_UDP_SOCKET_H_
