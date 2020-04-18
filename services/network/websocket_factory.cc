// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/websocket_factory.h"

#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/network/network_context.h"
#include "services/network/network_service.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/websocket.h"
#include "url/origin.h"

namespace network {

class WebSocketFactory::Delegate final : public WebSocket::Delegate {
 public:
  Delegate(WebSocketFactory* factory, int32_t process_id)
      : factory_(factory), process_id_(process_id), weak_factory_(this) {}
  ~Delegate() override {}

  net::URLRequestContext* GetURLRequestContext() override {
    return factory_->context_->url_request_context();
  }

  void OnLostConnectionToClient(WebSocket* impl) override {
    factory_->OnLostConnectionToClient(impl);
  }

  void OnSSLCertificateError(
      std::unique_ptr<net::WebSocketEventInterface::SSLErrorCallbacks>
          callbacks,
      const GURL& url,
      int process_id,
      int render_frame_id,
      const net::SSLInfo& ssl_info,
      bool fatal) override {
    DCHECK(!callbacks_);
    callbacks_ = std::move(callbacks);

    NetworkService* network_service = factory_->context_->network_service();
    // See content::ResourceType defined in
    // content/public/common/resource_type.h. This is
    // RESOURCE_TYPE_SUB_RESOURCE.
    constexpr int resource_type = 6;
    // We need to provide a request ID which we don't have. Provide an
    // invalid ID.
    constexpr uint32_t request_id = static_cast<uint32_t>(-1);

    network_service->client()->OnSSLCertificateError(
        process_id, render_frame_id, request_id, resource_type, url, ssl_info,
        fatal,
        base::BindRepeating(&Delegate::OnSSLCertificateErrorResponse,
                            weak_factory_.GetWeakPtr(), ssl_info));
  }

  void ReportBadMessage(BadMessageReason reason, WebSocket* impl) override {
    OnLostConnectionToClient(impl);
  }

  bool CanReadRawCookies() override {
    return factory_->context_->network_service()->HasRawHeadersAccess(
        process_id_);
  }

  void OnCreateURLRequest(int child_id,
                          int frame_id,
                          net::URLRequest* request) override {}

 private:
  void OnSSLCertificateErrorResponse(const net::SSLInfo& ssl_info,
                                     int net_error) {
    if (net_error == net::OK) {
      callbacks_->ContinueSSLRequest();
      return;
    }

    callbacks_->CancelSSLRequest(net_error, &ssl_info);
  }

  // |factory_| outlives this object.
  WebSocketFactory* const factory_;
  const int process_id_;
  std::unique_ptr<net::WebSocketEventInterface::SSLErrorCallbacks> callbacks_;

  base::WeakPtrFactory<Delegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Delegate);
};

WebSocketFactory::WebSocketFactory(NetworkContext* context)
    : context_(context) {}

WebSocketFactory::~WebSocketFactory() {}

void WebSocketFactory::CreateWebSocket(mojom::WebSocketRequest request,
                                       int32_t process_id,
                                       int32_t render_frame_id,
                                       const url::Origin& origin) {
  if (throttler_.HasTooManyPendingConnections(process_id)) {
    // Too many websockets!
    request.ResetWithReason(
        mojom::WebSocket::kInsufficientResources,
        "Error in connection establishment: net::ERR_INSUFFICIENT_RESOURCES");
    return;
  }
  connections_.insert(std::make_unique<WebSocket>(
      std::make_unique<Delegate>(this, process_id), std::move(request),
      throttler_.IssuePendingConnectionTracker(process_id), process_id,
      render_frame_id, origin, throttler_.CalculateDelay(process_id)));
}

void WebSocketFactory::OnLostConnectionToClient(WebSocket* impl) {
  auto it = connections_.find(impl);
  DCHECK(it != connections_.end());
  impl->GoAway();
  connections_.erase(it);
}

}  // namespace network
