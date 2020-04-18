// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/presentation/browser_presentation_connection_proxy.h"

#include <memory>
#include <vector>

#include "chrome/browser/media/router/media_router.h"

namespace media_router {

namespace {

void OnMessageReceivedByRenderer(bool success) {
  DLOG_IF(ERROR, !success)
      << "Renderer PresentationConnection failed to process message!";
}

}  // namespace

BrowserPresentationConnectionProxy::BrowserPresentationConnectionProxy(
    MediaRouter* router,
    const MediaRoute::Id& route_id,
    blink::mojom::PresentationConnectionRequest receiver_connection_request,
    blink::mojom::PresentationConnectionPtr controller_connection_ptr)
    : RouteMessageObserver(router, route_id),
      router_(router),
      route_id_(route_id),
      binding_(this),
      target_connection_ptr_(std::move(controller_connection_ptr)) {
  DCHECK(router);
  DCHECK(target_connection_ptr_);

  binding_.Bind(std::move(receiver_connection_request));
  target_connection_ptr_->DidChangeState(
      blink::mojom::PresentationConnectionState::CONNECTED);
}

BrowserPresentationConnectionProxy::~BrowserPresentationConnectionProxy() {}

void BrowserPresentationConnectionProxy::OnMessage(
    content::PresentationConnectionMessage message,
    OnMessageCallback on_message_callback) {
  DVLOG(2) << "BrowserPresentationConnectionProxy::OnMessage";
  if (message.is_binary()) {
    router_->SendRouteBinaryMessage(
        route_id_,
        std::make_unique<std::vector<uint8_t>>(std::move(message.data.value())),
        std::move(on_message_callback));
  } else {
    router_->SendRouteMessage(route_id_, message.message.value(),
                              std::move(on_message_callback));
  }
}

void BrowserPresentationConnectionProxy::OnMessagesReceived(
    const std::vector<content::PresentationConnectionMessage>& messages) {
  DVLOG(2) << __func__ << ", number of messages : " << messages.size();
  // TODO(imcheng): It would be slightly more efficient to send messages in
  // a single batch.
  for (const auto& message : messages) {
    target_connection_ptr_->OnMessage(
        message, base::BindOnce(&OnMessageReceivedByRenderer));
  }
}
}  // namespace media_router
