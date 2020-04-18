// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_BROWSER_PRESENTATION_CONNECTION_PROXY_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_BROWSER_PRESENTATION_CONNECTION_PROXY_H_

#include <vector>

#include "chrome/browser/media/router/route_message_observer.h"
#include "chrome/common/media_router/media_route.h"
#include "content/public/browser/presentation_service_delegate.h"
#include "content/public/common/presentation_connection_message.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media_router {

class MediaRouter;

// This class represents a browser side PresentationConnection. It connects with
// PresentationConnection owned by a render frame to enable message exchange.
// Message received on this class is further routed to Media Router. State of
// browser side PresentationConnection is always 'connected'.
//
// |SetTargetConnection| sets |target_connection_| to mojo handle of
// PresentationConnection object owned a render frame, and transits state of
// |target_connection_| to 'connected'.
//
// Send message from render frame to media router:
// blink::PresentationConnection::send();
//     -> (mojo call to browser side PresentationConnection)
//         -> BrowserPresentationConnectionProxy::OnMessage();
//             -> MediaRouter::SendRouteMessage();
//
// Instance of this class is only created for remotely rendered presentations.
// It is owned by PresentationFrame. When PresentationFrame gets destroyed or
// |route_| is closed or terminated, instance of this class will be destroyed.

class BrowserPresentationConnectionProxy
    : public blink::mojom::PresentationConnection,
      public RouteMessageObserver {
 public:
  using OnMessageCallback = base::OnceCallback<void(bool)>;

  // |router|: media router instance not owned by this class;
  // |route_id|: underlying media route. |target_connection_ptr_| sends message
  // to media route with |route_id|;
  // |receiver_connection_request|: mojo interface request to be bind with this
  // object;
  // |controller_connection_ptr|: mojo interface ptr of controlling frame's
  // connection proxy object.
  BrowserPresentationConnectionProxy(
      MediaRouter* router,
      const MediaRoute::Id& route_id,
      blink::mojom::PresentationConnectionRequest receiver_connection_request,
      blink::mojom::PresentationConnectionPtr controller_connection_ptr);
  ~BrowserPresentationConnectionProxy() override;

  // blink::mojom::PresentationConnection implementation
  void OnMessage(content::PresentationConnectionMessage message,
                 OnMessageCallback on_message_callback) override;

  // Underlying media route is always connected. Media route class does not
  // support state change.
  void DidChangeState(
      blink::mojom::PresentationConnectionState state) override {}

  // Underlying media route is always connected. Media route class does not
  // support state change.
  void RequestClose() override {}

  // RouteMessageObserver implementation.
  void OnMessagesReceived(
      const std::vector<content::PresentationConnectionMessage>& messages)
      override;

 private:
  // |router_| not owned by this class.
  MediaRouter* const router_;
  const MediaRoute::Id route_id_;

  mojo::Binding<blink::mojom::PresentationConnection> binding_;
  blink::mojom::PresentationConnectionPtr target_connection_ptr_;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PRESENTATION_BROWSER_PRESENTATION_CONNECTION_PROXY_H_
