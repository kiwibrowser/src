// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_HOST_UDP_H_
#define CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_HOST_UDP_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <set>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/browser/renderer_host/p2p/socket_host.h"
#include "content/common/content_export.h"
#include "content/common/p2p_socket_type.h"
#include "net/base/ip_endpoint.h"
#include "net/socket/diff_serv_code_point.h"
#include "net/socket/udp_server_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "third_party/webrtc/rtc_base/asyncpacketsocket.h"

namespace net {
class NetLog;
}  // namespace net

namespace content {

class P2PMessageThrottler;

class CONTENT_EXPORT P2PSocketHostUdp : public P2PSocketHost {
 public:
  typedef base::Callback<std::unique_ptr<net::DatagramServerSocket>(
      net::NetLog* net_log)>
      DatagramServerSocketFactory;
  P2PSocketHostUdp(IPC::Sender* message_sender,
                   int socket_id,
                   P2PMessageThrottler* throttler,
                   net::NetLog* net_log,
                   const DatagramServerSocketFactory& socket_factory);
  P2PSocketHostUdp(IPC::Sender* message_sender,
                   int socket_id,
                   P2PMessageThrottler* throttler,
                   net::NetLog* net_log);
  ~P2PSocketHostUdp() override;

  // P2PSocketHost overrides.
  bool Init(const net::IPEndPoint& local_address,
            uint16_t min_port,
            uint16_t max_port,
            const P2PHostAndIPEndPoint& remote_address) override;
  void Send(const net::IPEndPoint& to,
            const std::vector<char>& data,
            const rtc::PacketOptions& options,
            uint64_t packet_id,
            const net::NetworkTrafficAnnotationTag traffic_annotation) override;
  std::unique_ptr<P2PSocketHost> AcceptIncomingTcpConnection(
      const net::IPEndPoint& remote_address,
      int id) override;
  bool SetOption(P2PSocketOption option, int value) override;

 private:
  friend class P2PSocketHostUdpTest;

  typedef std::set<net::IPEndPoint> ConnectedPeerSet;

  struct PendingPacket {
    PendingPacket(const net::IPEndPoint& to,
                  const std::vector<char>& content,
                  const rtc::PacketOptions& options,
                  uint64_t id,
                  const net::NetworkTrafficAnnotationTag traffic_annotation);
    PendingPacket(const PendingPacket& other);
    ~PendingPacket();
    net::IPEndPoint to;
    scoped_refptr<net::IOBuffer> data;
    int size;
    rtc::PacketOptions packet_options;
    uint64_t id;
    const net::NetworkTrafficAnnotationTag traffic_annotation;
  };

  void OnError();

  void DoRead();
  void OnRecv(int result);
  void HandleReadResult(int result);

  void DoSend(const PendingPacket& packet);
  void OnSend(uint64_t packet_id,
              int32_t transport_sequence_number,
              base::TimeTicks send_time,
              int result);
  void HandleSendResult(uint64_t packet_id,
                        int32_t transport_sequence_number,
                        base::TimeTicks send_time,
                        int result);
  int SetSocketDiffServCodePointInternal(net::DiffServCodePoint dscp);
  static std::unique_ptr<net::DatagramServerSocket> DefaultSocketFactory(
      net::NetLog* net_log);

  std::unique_ptr<net::DatagramServerSocket> socket_;
  scoped_refptr<net::IOBuffer> recv_buffer_;
  net::IPEndPoint recv_address_;

  base::circular_deque<PendingPacket> send_queue_;
  bool send_pending_;
  net::DiffServCodePoint last_dscp_;

  // Set of peer for which we have received STUN binding request or
  // response or relay allocation request or response.
  ConnectedPeerSet connected_peers_;
  P2PMessageThrottler* throttler_;

  net::NetLog* net_log_;

  // Callback object that returns a new socket when invoked.
  DatagramServerSocketFactory socket_factory_;

  DISALLOW_COPY_AND_ASSIGN(P2PSocketHostUdp);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_HOST_UDP_H_
