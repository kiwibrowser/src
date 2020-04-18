// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_GLUE_RESOLVING_CLIENT_SOCKET_FACTORY_H_
#define JINGLE_GLUE_RESOLVING_CLIENT_SOCKET_FACTORY_H_

#include <memory>

namespace net {
class ClientSocketHandle;
class HostPortPair;
class SSLClientSocket;
class StreamSocket;
}  // namespace net

// TODO(sanjeevr): Move this to net/

namespace jingle_glue {

// Interface for a ClientSocketFactory that creates ClientSockets that can
// resolve host names and tunnel through proxies.
class ResolvingClientSocketFactory {
 public:
  virtual ~ResolvingClientSocketFactory() { }
  // Method to create a transport socket using a HostPortPair.
  virtual std::unique_ptr<net::StreamSocket> CreateTransportClientSocket(
      const net::HostPortPair& host_and_port) = 0;

  virtual std::unique_ptr<net::SSLClientSocket> CreateSSLClientSocket(
      std::unique_ptr<net::ClientSocketHandle> transport_socket,
      const net::HostPortPair& host_and_port) = 0;
};

}  // namespace jingle_glue

#endif  // JINGLE_GLUE_RESOLVING_CLIENT_SOCKET_FACTORY_H_
