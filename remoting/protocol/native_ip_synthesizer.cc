// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/native_ip_synthesizer.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "net/base/sys_addrinfo.h"
#include "third_party/webrtc/rtc_base/ipaddress.h"
#include "third_party/webrtc/rtc_base/socketaddress.h"

namespace remoting {
namespace protocol {

// static
rtc::SocketAddress ToNativeSocket(const rtc::SocketAddress& original_socket) {
#if defined(OS_IOS)
  // Currently only iOS needs the extra translation step. Android emulates an
  // IPv4 network stack.

  addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_DEFAULT;

  addrinfo* result = nullptr;
  // getaddrinfo() will resolve an IPv4 address into its curresponding IPv4/IPv6
  // address connectable on current network environment. Note that this doesn't
  // really send out a DNS request on iOS.
  int error = getaddrinfo(original_socket.ipaddr().ToString().c_str(), nullptr,
                          &hints, &result);
  if (error) {
    LOG(ERROR) << "getaddrinfo() failed for " << gai_strerror(error);
    return original_socket;
  }

  if (!result) {
    return original_socket;
  }

  rtc::SocketAddress new_socket;
  bool success = rtc::SocketAddressFromSockAddrStorage(
      *reinterpret_cast<sockaddr_storage*>(result->ai_addr), &new_socket);
  DCHECK(success);
  freeaddrinfo(result);
  new_socket.SetPort(original_socket.port());
  return new_socket;
#else
  return original_socket;
#endif  // defined(OS_IOS)
}

}  // namespace protocol
}  // namespace remoting
