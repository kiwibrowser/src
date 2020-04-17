// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_QUIC_QUIC_CONNECTION_FACTORY_H_
#define OSP_IMPL_QUIC_QUIC_CONNECTION_FACTORY_H_

#include <memory>
#include <vector>

#include "osp/impl/quic/quic_connection.h"
#include "osp_base/ip_address.h"

namespace openscreen {

// This interface provides a way to make new QUIC connections to endpoints.  It
// also provides a way to receive incoming QUIC connections (as a server).
class QuicConnectionFactory {
 public:
  class ServerDelegate {
   public:
    virtual ~ServerDelegate() = default;

    virtual QuicConnection::Delegate* NextConnectionDelegate(
        const IPEndpoint& source) = 0;
    virtual void OnIncomingConnection(
        std::unique_ptr<QuicConnection> connection) = 0;
  };

  virtual ~QuicConnectionFactory() = default;

  // Initializes a server socket listening on |port| where new connection
  // callbacks are sent to |delegate|.
  virtual void SetServerDelegate(ServerDelegate* delegate,
                                 const std::vector<IPEndpoint>& endpoints) = 0;

  // Listen for incoming network packets on both client and server sockets and
  // dispatch any results.
  virtual void RunTasks() = 0;

  virtual std::unique_ptr<QuicConnection> Connect(
      const IPEndpoint& endpoint,
      QuicConnection::Delegate* connection_delegate) = 0;
};

}  // namespace openscreen

#endif  // OSP_IMPL_QUIC_QUIC_CONNECTION_FACTORY_H_
