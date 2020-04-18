// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_DISPATCHER_HOST_H_
#define CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_DISPATCHER_HOST_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "content/browser/renderer_host/p2p/socket_host_throttler.h"
#include "content/common/p2p_socket_type.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/base/network_change_notifier.h"

namespace net {
struct MutableNetworkTrafficAnnotationTag;
class URLRequestContextGetter;
}

namespace network {
class ProxyResolvingClientSocketFactory;
}

namespace content {

class P2PSocketHost;
class ResourceContext;

class P2PSocketDispatcherHost
    : public content::BrowserMessageFilter,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  P2PSocketDispatcherHost(content::ResourceContext* resource_context,
                          net::URLRequestContextGetter* url_context);

  // content::BrowserMessageFilter overrides.
  void OnChannelClosing() override;
  void OnDestruct() const override;
  bool OnMessageReceived(const IPC::Message& message) override;

  // net::NetworkChangeNotifier::NetworkChangeObserver interface.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;
  // Starts the RTP packet header dumping. Must be called on the IO thread.
  void StartRtpDump(
      bool incoming,
      bool outgoing,
      const RenderProcessHost::WebRtcRtpPacketCallback& packet_callback);

  // Stops the RTP packet header dumping. Must be Called on the UI thread.
  void StopRtpDumpOnUIThread(bool incoming, bool outgoing);

 protected:
  ~P2PSocketDispatcherHost() override;

 private:
  friend struct BrowserThread::DeleteOnThread<BrowserThread::IO>;
  friend class base::DeleteHelper<P2PSocketDispatcherHost>;

  typedef std::map<int, std::unique_ptr<P2PSocketHost>> SocketsMap;

  class DnsRequest;

  P2PSocketHost* LookupSocket(int socket_id);

  // Handlers for the messages coming from the renderer.
  void OnStartNetworkNotifications();
  void OnStopNetworkNotifications();
  void OnGetHostAddress(const std::string& host_name, int32_t request_id);

  void OnCreateSocket(P2PSocketType type,
                      int socket_id,
                      const net::IPEndPoint& local_address,
                      const P2PPortRange& port_range,
                      const P2PHostAndIPEndPoint& remote_address);
  void OnAcceptIncomingTcpConnection(int listen_socket_id,
                                     const net::IPEndPoint& remote_address,
                                     int connected_socket_id);
  void OnSend(
      int socket_id,
      const std::vector<char>& data,
      const P2PPacketInfo& packet_info,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation);
  void OnSetOption(int socket_id, P2PSocketOption option, int value);
  void OnDestroySocket(int socket_id);

  void DoGetNetworkList();
  void SendNetworkList(const net::NetworkInterfaceList& list,
                       const net::IPAddress& default_ipv4_local_address,
                       const net::IPAddress& default_ipv6_local_address);

  // This connects a UDP socket to a public IP address and gets local
  // address. Since it binds to the "any" address (0.0.0.0 or ::) internally, it
  // retrieves the default local address.
  net::IPAddress GetDefaultLocalAddress(int family);

  void OnAddressResolved(DnsRequest* request,
                         const net::IPAddressList& addresses);

  void StopRtpDumpOnIOThread(bool incoming, bool outgoing);

  content::ResourceContext* resource_context_;
  scoped_refptr<net::URLRequestContextGetter> url_context_;
  // Initialized on browser IO thread.
  std::unique_ptr<network::ProxyResolvingClientSocketFactory>
      proxy_resolving_socket_factory_;

  SocketsMap sockets_;

  bool monitoring_networks_;

  std::set<std::unique_ptr<DnsRequest>> dns_requests_;
  P2PMessageThrottler throttler_;

  net::IPAddress default_ipv4_local_address_;
  net::IPAddress default_ipv6_local_address_;

  bool dump_incoming_rtp_packet_;
  bool dump_outgoing_rtp_packet_;
  RenderProcessHost::WebRtcRtpPacketCallback packet_callback_;

  // Used to call DoGetNetworkList, which may briefly block since getting the
  // default local address involves creating a dummy socket.
  const scoped_refptr<base::SequencedTaskRunner> network_list_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(P2PSocketDispatcherHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_P2P_SOCKET_DISPATCHER_HOST_H_
