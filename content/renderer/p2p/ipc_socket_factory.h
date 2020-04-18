// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_P2P_IPC_SOCKET_FACTORY_H_
#define CONTENT_RENDERER_P2P_IPC_SOCKET_FACTORY_H_

#include <stdint.h>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/common/content_export.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "third_party/webrtc/p2p/base/packetsocketfactory.h"

namespace content {

class P2PSocketDispatcher;

// IpcPacketSocketFactory implements rtc::PacketSocketFactory
// interface for libjingle using IPC-based P2P sockets. The class must
// be used on a thread that is a libjingle thread (implements
// rtc::Thread) and also has associated base::MessageLoop. Each
// socket created by the factory must be used on the thread it was
// created on.
class IpcPacketSocketFactory : public rtc::PacketSocketFactory {
 public:
  CONTENT_EXPORT explicit IpcPacketSocketFactory(
      P2PSocketDispatcher* socket_dispatcher,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);
  ~IpcPacketSocketFactory() override;

  rtc::AsyncPacketSocket* CreateUdpSocket(
      const rtc::SocketAddress& local_address,
      uint16_t min_port,
      uint16_t max_port) override;
  rtc::AsyncPacketSocket* CreateServerTcpSocket(
      const rtc::SocketAddress& local_address,
      uint16_t min_port,
      uint16_t max_port,
      int opts) override;
  rtc::AsyncPacketSocket* CreateClientTcpSocket(
      const rtc::SocketAddress& local_address,
      const rtc::SocketAddress& remote_address,
      const rtc::ProxyInfo& proxy_info,
      const std::string& user_agent,
      int opts) override;
  rtc::AsyncResolverInterface* CreateAsyncResolver() override;

 private:
  P2PSocketDispatcher* socket_dispatcher_;
  const net::NetworkTrafficAnnotationTag traffic_annotation_;

  DISALLOW_COPY_AND_ASSIGN(IpcPacketSocketFactory);
};

}  // namespace content

#endif  // CONTENT_RENDERER_P2P_IPC_SOCKET_FACTORY_H_
