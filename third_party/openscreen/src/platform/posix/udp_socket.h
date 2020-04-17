// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PLATFORM_POSIX_UDP_SOCKET_H_
#define PLATFORM_POSIX_UDP_SOCKET_H_

#include "platform/api/udp_socket.h"

namespace openscreen {
namespace platform {

struct UdpSocketPosix : public UdpSocket {
  const int fd;
  const UdpSocket::Version version;

  UdpSocketPosix(int fd, Version version);

  static const UdpSocketPosix* From(const UdpSocket* socket) {
    return static_cast<const UdpSocketPosix*>(socket);
  }

  static UdpSocketPosix* From(UdpSocket* socket) {
    return static_cast<UdpSocketPosix*>(socket);
  }
};

}  // namespace platform
}  // namespace openscreen

#endif  // PLATFORM_POSIX_UDP_SOCKET_H_
