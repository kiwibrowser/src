// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_WEBSOCKET_H_
#define SERVICES_NETWORK_WEBSOCKET_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/websockets/websocket_event_interface.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "services/network/websocket_throttler.h"
#include "url/origin.h"

class GURL;

namespace net {
class HttpRequestHeaders;
class URLRequestContext;
class WebSocketChannel;
class SSLInfo;
}  // namespace net

namespace network {

// Host of net::WebSocketChannel.
class COMPONENT_EXPORT(NETWORK_SERVICE) WebSocket
    : public network::mojom::WebSocket {
 public:
  class Delegate {
   public:
    enum class BadMessageReason {
      kUnexpectedAddChannelRequest,
      kUnexpectedSendFrame,
    };
    virtual ~Delegate() {}

    virtual net::URLRequestContext* GetURLRequestContext() = 0;
    // This function may delete |impl|.
    virtual void OnLostConnectionToClient(WebSocket* impl) = 0;
    virtual void OnSSLCertificateError(
        std::unique_ptr<net::WebSocketEventInterface::SSLErrorCallbacks>
            callbacks,
        const GURL& url,
        int child_id,
        int frame_id,
        const net::SSLInfo& ssl_info,
        bool fatal) = 0;
    // This function may delete |impl|.
    virtual void ReportBadMessage(BadMessageReason reason, WebSocket* impl) = 0;
    virtual bool CanReadRawCookies() = 0;
    virtual void OnCreateURLRequest(int child_id,
                                    int frame_id,
                                    net::URLRequest* request) = 0;
  };

  WebSocket(std::unique_ptr<Delegate> delegate,
            mojom::WebSocketRequest request,
            WebSocketThrottler::PendingConnection pending_connection_tracker,
            int child_id,
            int frame_id,
            url::Origin origin,
            base::TimeDelta delay);
  ~WebSocket() override;

  // The renderer process is going away.
  // This function is virtual for testing.
  virtual void GoAway();

  // mojom::WebSocket methods:
  void AddChannelRequest(const GURL& url,
                         const std::vector<std::string>& requested_protocols,
                         const GURL& site_for_cookies,
                         const std::string& user_agent_override,
                         mojom::WebSocketClientPtr client) override;
  void SendFrame(bool fin,
                 mojom::WebSocketMessageType type,
                 const std::vector<uint8_t>& data) override;
  void SendFlowControl(int64_t quota) override;
  void StartClosingHandshake(uint16_t code, const std::string& reason) override;

  bool handshake_succeeded() const { return handshake_succeeded_; }

 protected:
  class WebSocketEventHandler;

  void OnConnectionError();
  void AddChannel(const GURL& socket_url,
                  const std::vector<std::string>& requested_protocols,
                  const GURL& site_for_cookies,
                  const net::HttpRequestHeaders& additional_headers);

  std::unique_ptr<Delegate> delegate_;
  mojo::Binding<mojom::WebSocket> binding_;

  mojom::WebSocketClientPtr client_;

  WebSocketThrottler::PendingConnection pending_connection_tracker_;

  // The channel we use to send events to the network.
  std::unique_ptr<net::WebSocketChannel> channel_;

  // Delay used for per-renderer WebSocket throttling.
  base::TimeDelta delay_;

  // SendFlowControl() is delayed when OnFlowControl() is called before
  // AddChannel() is called.
  // Zero indicates there is no pending SendFlowControl().
  int64_t pending_flow_control_quota_;

  int child_id_;
  int frame_id_;

  // The web origin to use for the WebSocket.
  const url::Origin origin_;

  // handshake_succeeded_ is set and used by WebSocketManager to manage
  // counters for per-renderer WebSocket throttling.
  bool handshake_succeeded_;

  base::WeakPtrFactory<WebSocket> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebSocket);
};

}  // namespace network

#endif  // SERVICES_NETWORK_WEBSOCKET_H_
