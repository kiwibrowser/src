// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_P2P_SOCKET_CLIENT_DELEGATE_H_
#define CONTENT_RENDERER_P2P_SOCKET_CLIENT_DELEGATE_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "content/common/p2p_socket_type.h"
#include "net/base/ip_endpoint.h"

namespace content {

class P2PSocketClient;

class P2PSocketClientDelegate {
 public:
  virtual ~P2PSocketClientDelegate() { }

  // Called after the socket has been opened with the local endpoint address
  // as argument. Please note that in the precence of multiple interfaces,
  // you should not rely on the local endpoint address if possible.
  virtual void OnOpen(const net::IPEndPoint& local_address,
                      const net::IPEndPoint& remote_address) = 0;

  // For a socket that is listening on incoming TCP connectsion, this
  // function is called when a new client connects.
  virtual void OnIncomingTcpConnection(const net::IPEndPoint& address,
                                       P2PSocketClient* client) = 0;

  // Called once for each Send() call after the send is complete.
  virtual void OnSendComplete(const P2PSendPacketMetrics& send_metrics) = 0;

  // Called if an non-retryable error occurs.
  virtual void OnError() = 0;

  // Called when data is received on the socket.
  virtual void OnDataReceived(const net::IPEndPoint& address,
                              const std::vector<char>& data,
                              const base::TimeTicks& timestamp) = 0;
};

}  // namespace content

#endif  // CONTENT_RENDERER_P2P_SOCKET_CLIENT_DELEGATE_H_
