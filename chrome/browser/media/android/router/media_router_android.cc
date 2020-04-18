// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/android/router/media_router_android.h"

#include <string>
#include <utility>

#include "base/guid.h"
#include "base/logging.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/media/router/media_routes_observer.h"
#include "chrome/browser/media/router/media_sinks_observer.h"
#include "chrome/browser/media/router/route_message_observer.h"
#include "chrome/common/media_router/route_request_result.h"
#include "content/public/browser/browser_context.h"
#include "content/public/common/presentation_connection_message.h"
#include "url/gurl.h"

namespace media_router {

MediaRouterAndroid::MediaRouteRequest::MediaRouteRequest(
    const MediaSource& source,
    const std::string& presentation_id,
    std::vector<MediaRouteResponseCallback> callbacks)
    : media_source(source),
      presentation_id(presentation_id),
      callbacks(std::move(callbacks)) {}

MediaRouterAndroid::MediaRouteRequest::~MediaRouteRequest() {}

MediaRouterAndroid::MediaRouterAndroid(content::BrowserContext*)
    : bridge_(new MediaRouterAndroidBridge(this)) {}

MediaRouterAndroid::~MediaRouterAndroid() {
}

const MediaRoute* MediaRouterAndroid::FindRouteBySource(
    const MediaSource::Id& source_id) const {
  for (const auto& route : active_routes_) {
    if (route.media_source().id() == source_id)
      return &route;
  }
  return nullptr;
}

void MediaRouterAndroid::CreateRoute(
    const MediaSource::Id& source_id,
    const MediaSink::Id& sink_id,
    const url::Origin& origin,
    content::WebContents* web_contents,
    std::vector<MediaRouteResponseCallback> callbacks,
    base::TimeDelta timeout,
    bool incognito) {
  // TODO(avayvod): Implement timeouts (crbug.com/583036).
  std::string presentation_id = MediaRouterBase::CreatePresentationId();

  int tab_id = -1;
  TabAndroid* tab = web_contents
      ? TabAndroid::FromWebContents(web_contents) : nullptr;
  if (tab)
    tab_id = tab->GetAndroidId();

  bool is_incognito = web_contents
      && web_contents->GetBrowserContext()->IsOffTheRecord();

  int route_request_id =
      route_requests_.Add(std::make_unique<MediaRouteRequest>(
          MediaSource(source_id), presentation_id, std::move(callbacks)));
  bridge_->CreateRoute(source_id, sink_id, presentation_id, origin, tab_id,
                       is_incognito, route_request_id);
}

void MediaRouterAndroid::ConnectRouteByRouteId(
    const MediaSource::Id& source,
    const MediaRoute::Id& route_id,
    const url::Origin& origin,
    content::WebContents* web_contents,
    std::vector<MediaRouteResponseCallback> callbacks,
    base::TimeDelta timeout,
    bool incognito) {
  NOTIMPLEMENTED();
}

void MediaRouterAndroid::JoinRoute(
    const MediaSource::Id& source_id,
    const std::string& presentation_id,
    const url::Origin& origin,
    content::WebContents* web_contents,
    std::vector<MediaRouteResponseCallback> callbacks,
    base::TimeDelta timeout,
    bool incognito) {
  // TODO(avayvod): Implement timeouts (crbug.com/583036).
  int tab_id = -1;
  TabAndroid* tab = web_contents
      ? TabAndroid::FromWebContents(web_contents) : nullptr;
  if (tab)
    tab_id = tab->GetAndroidId();

  DVLOG(2) << "JoinRoute: " << source_id << ", " << presentation_id << ", "
           << origin.GetURL().spec() << ", " << tab_id;

  int request_id = route_requests_.Add(std::make_unique<MediaRouteRequest>(
      MediaSource(source_id), presentation_id, std::move(callbacks)));
  bridge_->JoinRoute(source_id, presentation_id, origin, tab_id, request_id);
}

void MediaRouterAndroid::TerminateRoute(const MediaRoute::Id& route_id) {
  bridge_->TerminateRoute(route_id);
}

void MediaRouterAndroid::SendRouteMessage(const MediaRoute::Id& route_id,
                                          const std::string& message,
                                          SendRouteMessageCallback callback) {
  int callback_id = message_callbacks_.Add(
      std::make_unique<SendRouteMessageCallback>(std::move(callback)));
  bridge_->SendRouteMessage(route_id, message, callback_id);
}

void MediaRouterAndroid::SendRouteBinaryMessage(
    const MediaRoute::Id& route_id,
    std::unique_ptr<std::vector<uint8_t>> data,
    SendRouteMessageCallback callback) {
  // Binary messaging is not supported on Android.
  std::move(callback).Run(false);
}

void MediaRouterAndroid::OnUserGesture() {
}

void MediaRouterAndroid::SearchSinks(
    const MediaSink::Id& sink_id,
    const MediaSource::Id& source_id,
    const std::string& search_input,
    const std::string& domain,
    MediaSinkSearchResponseCallback sink_callback) {
  NOTIMPLEMENTED();
}

void MediaRouterAndroid::DetachRoute(const MediaRoute::Id& route_id) {
  bridge_->DetachRoute(route_id);
  RemoveRoute(route_id);
  NotifyPresentationConnectionClose(
      route_id, blink::mojom::PresentationConnectionCloseReason::CLOSED,
      "Remove route");
}

bool MediaRouterAndroid::RegisterMediaSinksObserver(
    MediaSinksObserver* observer) {
  const std::string& source_id = observer->source().id();
  auto& observer_list = sinks_observers_[source_id];
  if (!observer_list) {
    observer_list = std::make_unique<base::ObserverList<MediaSinksObserver>>();
  } else {
    DCHECK(!observer_list->HasObserver(observer));
  }

  observer_list->AddObserver(observer);
  return bridge_->StartObservingMediaSinks(source_id);
}

void MediaRouterAndroid::UnregisterMediaSinksObserver(
    MediaSinksObserver* observer) {
  const std::string& source_id = observer->source().id();
  auto it = sinks_observers_.find(source_id);
  if (it == sinks_observers_.end() || !it->second->HasObserver(observer))
    return;

  // If we are removing the final observer for the source, then stop
  // observing sinks for it.
  // might_have_observers() is reliable here on the assumption that this call
  // is not inside the ObserverList iteration.
  it->second->RemoveObserver(observer);
  if (!it->second->might_have_observers()) {
    sinks_observers_.erase(source_id);
    bridge_->StopObservingMediaSinks(source_id);
  }
}

void MediaRouterAndroid::RegisterMediaRoutesObserver(
    MediaRoutesObserver* observer) {
  DVLOG(2) << "Added MediaRoutesObserver: " << observer;
  if (!observer->source_id().empty())
    NOTIMPLEMENTED() << "Joinable routes query not implemented.";

  routes_observers_.AddObserver(observer);
}

void MediaRouterAndroid::UnregisterMediaRoutesObserver(
    MediaRoutesObserver* observer) {
  if (!routes_observers_.HasObserver(observer))
    return;
  routes_observers_.RemoveObserver(observer);
}

void MediaRouterAndroid::RegisterRouteMessageObserver(
    RouteMessageObserver* observer) {
  const MediaRoute::Id& route_id = observer->route_id();
  auto& observer_list = message_observers_[route_id];
  if (!observer_list) {
    observer_list =
        std::make_unique<base::ObserverList<RouteMessageObserver>>();
  } else {
    DCHECK(!observer_list->HasObserver(observer));
  }

  observer_list->AddObserver(observer);
}

void MediaRouterAndroid::UnregisterRouteMessageObserver(
    RouteMessageObserver* observer) {
  const MediaRoute::Id& route_id = observer->route_id();
  auto* observer_list = message_observers_[route_id].get();
  DCHECK(observer_list->HasObserver(observer));

  observer_list->RemoveObserver(observer);
  if (!observer_list->might_have_observers())
    message_observers_.erase(route_id);
}

void MediaRouterAndroid::OnSinksReceived(const std::string& source_urn,
                                         const std::vector<MediaSink>& sinks) {
  auto it = sinks_observers_.find(source_urn);
  if (it != sinks_observers_.end()) {
    // TODO(imcheng): Pass origins to OnSinksUpdated (crbug.com/594858).
    for (auto& observer : *it->second)
      observer.OnSinksUpdated(sinks, std::vector<url::Origin>());
  }
}

void MediaRouterAndroid::OnRouteCreated(const MediaRoute::Id& route_id,
                                        const MediaSink::Id& sink_id,
                                        int route_request_id,
                                        bool is_local) {
  MediaRouteRequest* request = route_requests_.Lookup(route_request_id);
  if (!request)
    return;

  MediaRoute route(route_id, request->media_source, sink_id, std::string(),
                   is_local, true);  // TODO(avayvod): Populate for_display.

  std::unique_ptr<RouteRequestResult> result =
      RouteRequestResult::FromSuccess(route, request->presentation_id);
  for (MediaRouteResponseCallback& callback : request->callbacks)
    std::move(callback).Run(*result);

  route_requests_.Remove(route_request_id);

  active_routes_.push_back(route);
  for (auto& observer : routes_observers_)
    observer.OnRoutesUpdated(active_routes_, std::vector<MediaRoute::Id>());
}

void MediaRouterAndroid::OnRouteRequestError(const std::string& error_text,
                                             int route_request_id) {
  MediaRouteRequest* request = route_requests_.Lookup(route_request_id);
  if (!request)
    return;

  // TODO(imcheng): Provide a more specific result code.
  std::unique_ptr<RouteRequestResult> result = RouteRequestResult::FromError(
      error_text, RouteRequestResult::UNKNOWN_ERROR);
  for (MediaRouteResponseCallback& callback : request->callbacks)
    std::move(callback).Run(*result);

  route_requests_.Remove(route_request_id);
}

void MediaRouterAndroid::OnRouteClosed(const MediaRoute::Id& route_id) {
  RemoveRoute(route_id);
  NotifyPresentationConnectionStateChange(
      route_id, blink::mojom::PresentationConnectionState::TERMINATED);
}

void MediaRouterAndroid::OnRouteClosedWithError(const MediaRoute::Id& route_id,
                                                const std::string& message) {
  RemoveRoute(route_id);
  NotifyPresentationConnectionClose(
      route_id,
      blink::mojom::PresentationConnectionCloseReason::CONNECTION_ERROR,
      message);
}

void MediaRouterAndroid::OnMessageSentResult(bool success, int callback_id) {
  SendRouteMessageCallback* callback = message_callbacks_.Lookup(callback_id);
  std::move(*callback).Run(success);
  message_callbacks_.Remove(callback_id);
}

void MediaRouterAndroid::OnMessage(const MediaRoute::Id& route_id,
                                   const std::string& message) {
  auto it = message_observers_.find(route_id);
  if (it == message_observers_.end())
    return;

  std::vector<content::PresentationConnectionMessage> messages;
  messages.emplace_back(message);
  for (auto& observer : *it->second.get())
    observer.OnMessagesReceived(messages);
}

void MediaRouterAndroid::RemoveRoute(const MediaRoute::Id& route_id) {
  for (auto it = active_routes_.begin(); it != active_routes_.end(); ++it)
    if (it->media_route_id() == route_id) {
      active_routes_.erase(it);
      break;
    }

  for (auto& observer : routes_observers_)
    observer.OnRoutesUpdated(active_routes_, std::vector<MediaRoute::Id>());
}

std::unique_ptr<content::MediaController>
MediaRouterAndroid::GetMediaController(const MediaRoute::Id& route_id) {
  return bridge_->GetMediaController(route_id);
}

}  // namespace media_router
