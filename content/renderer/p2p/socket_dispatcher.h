// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// P2PSocketDispatcher is a per-renderer object that dispatchers all
// P2P messages received from the browser and relays all P2P messages
// sent to the browser. P2PSocketClient instances register themselves
// with the dispatcher using RegisterClient() and UnregisterClient().
//
// Relationship of classes.
//
//       P2PSocketHost                     P2PSocketClient
//            ^                                   ^
//            |                                   |
//            v                  IPC              v
//  P2PSocketDispatcherHost  <--------->  P2PSocketDispatcher
//
// P2PSocketDispatcher receives and dispatches messages on the
// IO thread.

#ifndef CONTENT_RENDERER_P2P_SOCKET_DISPATCHER_H_
#define CONTENT_RENDERER_P2P_SOCKET_DISPATCHER_H_

#include <stdint.h>

#include <vector>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/observer_list_threadsafe.h"
#include "base/synchronization/lock.h"
#include "content/common/content_export.h"
#include "content/common/p2p_socket_type.h"
#include "content/renderer/p2p/network_list_manager.h"
#include "ipc/message_filter.h"
#include "net/base/ip_address.h"
#include "net/base/network_interfaces.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace net {
class IPEndPoint;
}  // namespace net

namespace content {

class NetworkListObserver;
class P2PAsyncAddressResolver;
class P2PSocketClientImpl;

class CONTENT_EXPORT P2PSocketDispatcher : public IPC::MessageFilter,
                                           public NetworkListManager {
 public:
  explicit P2PSocketDispatcher(base::SingleThreadTaskRunner* ipc_task_runner);

  // NetworkListManager interface:
  void AddNetworkListObserver(
      NetworkListObserver* network_list_observer) override;
  void RemoveNetworkListObserver(
      NetworkListObserver* network_list_observer) override;

  bool connected() { return connected_; }

 protected:
  ~P2PSocketDispatcher() override;

 private:
  friend class P2PAsyncAddressResolver;
  friend class P2PSocketClientImpl;

  // Send a message asynchronously.
  virtual void Send(IPC::Message* message);

  // IPC::MessageFilter override. Called on IO thread.
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnFilterAdded(IPC::Channel* channel) override;
  void OnFilterRemoved() override;
  void OnChannelClosing() override;
  void OnChannelConnected(int32_t peer_pid) override;

  base::SingleThreadTaskRunner* task_runner();

  // Called by P2PSocketClient.
  int RegisterClient(P2PSocketClientImpl* client);
  void UnregisterClient(int id);
  void SendP2PMessage(IPC::Message* msg);

  // Called by DnsRequest.
  int RegisterHostAddressRequest(P2PAsyncAddressResolver* request);
  void UnregisterHostAddressRequest(int id);

  // Incoming message handlers.
  void OnNetworkListChanged(const net::NetworkInterfaceList& networks,
                            const net::IPAddress& default_ipv4_local_address,
                            const net::IPAddress& default_ipv6_local_address);
  void OnGetHostAddressResult(int32_t request_id,
                              const net::IPAddressList& addresses);
  void OnSocketCreated(int socket_id,
                       const net::IPEndPoint& local_address,
                       const net::IPEndPoint& remote_address);
  void OnIncomingTcpConnection(int socket_id, const net::IPEndPoint& address);
  void OnSendComplete(int socket_id, const P2PSendPacketMetrics& send_metrics);
  void OnError(int socket_id);
  void OnDataReceived(int socket_id, const net::IPEndPoint& address,
                      const std::vector<char>& data,
                      const base::TimeTicks& timestamp);

  P2PSocketClientImpl* GetClient(int socket_id);

  scoped_refptr<base::SingleThreadTaskRunner> ipc_task_runner_;
  base::IDMap<P2PSocketClientImpl*> clients_;

  base::IDMap<P2PAsyncAddressResolver*> host_address_requests_;

  bool network_notifications_started_;
  scoped_refptr<base::ObserverListThreadSafe<NetworkListObserver>>
      network_list_observers_;

  IPC::Sender* sender_;

  // To indicate whether IPC could be invoked on this dispatcher.
  bool connected_ = false;

  DISALLOW_COPY_AND_ASSIGN(P2PSocketDispatcher);
};

}  // namespace content

#endif  // CONTENT_RENDERER_P2P_SOCKET_DISPATCHER_H_
