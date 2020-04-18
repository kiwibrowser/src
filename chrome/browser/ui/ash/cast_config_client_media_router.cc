// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/cast_config_client_media_router.h"

#include <string>
#include <utility>
#include <vector>

#include "ash/public/interfaces/constants.mojom.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/media/router/media_router.h"
#include "chrome/browser/media/router/media_router_factory.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/media/router/media_routes_observer.h"
#include "chrome/browser/media/router/media_sinks_observer.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "chrome/common/url_constants.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

namespace {

media_router::MediaRouter* media_router_for_test_ = nullptr;

// Returns the MediaRouter instance for the current primary profile.
media_router::MediaRouter* GetMediaRouter() {
  if (media_router_for_test_)
    return media_router_for_test_;

  auto* router = media_router::MediaRouterFactory::GetApiForBrowserContext(
      ProfileManager::GetPrimaryUserProfile());
  DCHECK(router);
  return router;
}

// The media router will sometimes append " (Tab)" to the tab title. This
// function will remove that data from the inout param |string|.
std::string StripEndingTab(const std::string& str) {
  static const char ending[] = " (Tab)";
  if (base::EndsWith(str, ending, base::CompareCase::SENSITIVE))
    return str.substr(0, str.size() - strlen(ending));
  return str;
}

}  // namespace

// This class caches the values that the observers give us so we can query them
// at any point in time. It also emits a device refresh event when new data is
// available.
class CastDeviceCache : public media_router::MediaRoutesObserver,
                        public media_router::MediaSinksObserver {
 public:
  using MediaSinks = std::vector<media_router::MediaSink>;
  using MediaRoutes = std::vector<media_router::MediaRoute>;
  using MediaRouteIds = std::vector<media_router::MediaRoute::Id>;

  explicit CastDeviceCache(ash::mojom::CastConfigClient* cast_config_client);
  ~CastDeviceCache() override;

  // This may call cast_config_client->RequestDeviceRefresh() before
  // returning.
  void Init();

  const MediaSinks& sinks() const { return sinks_; }
  const MediaRoutes& routes() const { return routes_; }

 private:
  // media_router::MediaSinksObserver:
  void OnSinksReceived(const MediaSinks& sinks) override;

  // media_router::MediaRoutesObserver:
  void OnRoutesUpdated(const MediaRoutes& routes,
                       const MediaRouteIds& unused_joinable_route_ids) override;

  MediaSinks sinks_;
  MediaRoutes routes_;

  // Not owned.
  ash::mojom::CastConfigClient* cast_config_client_;

  DISALLOW_COPY_AND_ASSIGN(CastDeviceCache);
};

CastDeviceCache::CastDeviceCache(
    ash::mojom::CastConfigClient* cast_config_client)
    : MediaRoutesObserver(GetMediaRouter()),
      MediaSinksObserver(
          GetMediaRouter(),
          media_router::MediaSourceForDesktop(),
          url::Origin::Create(GURL(chrome::kChromeUIMediaRouterURL))),
      cast_config_client_(cast_config_client) {}

CastDeviceCache::~CastDeviceCache() {}

void CastDeviceCache::Init() {
  CHECK(MediaSinksObserver::Init());
}

void CastDeviceCache::OnSinksReceived(const MediaSinks& sinks) {
  sinks_.clear();
  for (const media_router::MediaSink& sink : sinks) {
    // The media router adds a MediaSink instance that doesn't have a name. Make
    // sure to filter that sink out from the UI so it is not rendered, as it
    // will be a line that only has a icon with no apparent meaning.
    if (sink.name().empty())
      continue;

    // Hide all sinks which have a domain (ie, castouts) to meet privacy
    // requirements. This will be enabled once UI can display the domain. See
    // crbug.com/624016.
    if (sink.domain() && !sink.domain()->empty())
      continue;

    sinks_.push_back(sink);
  }

  cast_config_client_->RequestDeviceRefresh();
}

void CastDeviceCache::OnRoutesUpdated(
    const MediaRoutes& routes,
    const MediaRouteIds& unused_joinable_route_ids) {
  routes_ = routes;
  cast_config_client_->RequestDeviceRefresh();
}

////////////////////////////////////////////////////////////////////////////////
// CastConfigClientMediaRouter:

void CastConfigClientMediaRouter::SetMediaRouterForTest(
    media_router::MediaRouter* media_router) {
  media_router_for_test_ = media_router;
}

CastConfigClientMediaRouter::CastConfigClientMediaRouter() : binding_(this) {
  // TODO(jdufault): This should use a callback interface once there is an
  // equivalent. See crbug.com/666005.
  registrar_.Add(this, chrome::NOTIFICATION_LOGIN_USER_PROFILE_PREPARED,
                 content::NotificationService::AllSources());

  // When starting up, we need to connect to ash and set ourselves as the
  // client.
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(ash::mojom::kServiceName, &cast_config_);

  // Register this object as the client interface implementation.
  ash::mojom::CastConfigClientAssociatedPtrInfo ptr_info;
  binding_.Bind(mojo::MakeRequest(&ptr_info));
  cast_config_->SetClient(std::move(ptr_info));
}

CastConfigClientMediaRouter::~CastConfigClientMediaRouter() {}

CastDeviceCache* CastConfigClientMediaRouter::devices() {
  // The CastDeviceCache instance is lazily allocated because the MediaRouter
  // component is not ready when the constructor is invoked.
  if (!devices_ && GetMediaRouter()) {
    devices_ = std::make_unique<CastDeviceCache>(this);
    devices_->Init();
  }

  return devices_.get();
}

void CastConfigClientMediaRouter::RequestDeviceRefresh() {
  // The media router component isn't ready yet.
  if (!devices())
    return;

  // We failed to connect to ash; don't crash in release.
  if (!cast_config_) {
    NOTREACHED();
    return;
  }

  // Build the old-style SinkAndRoute set out of the MediaRouter
  // source/sink/route setup. We first map the existing sinks, and then we
  // update those sinks with activity information.

  std::vector<ash::mojom::SinkAndRoutePtr> items;

  for (const media_router::MediaSink& sink : devices()->sinks()) {
    ash::mojom::SinkAndRoutePtr sr = ash::mojom::SinkAndRoute::New();
    sr->route = ash::mojom::CastRoute::New();
    sr->sink = ash::mojom::CastSink::New();
    sr->sink->id = sink.id();
    sr->sink->name = sink.name();
    sr->sink->domain = sink.domain().value_or(std::string());
    // TODO(crbug.com/788854): Replace SinkIconType with
    // ash::mojom::SinkIconType.
    sr->sink->sink_icon_type =
        static_cast<ash::mojom::SinkIconType>(sink.icon_type());
    items.push_back(std::move(sr));
  }

  for (const media_router::MediaRoute& route : devices()->routes()) {
    if (!route.for_display())
      continue;

    for (ash::mojom::SinkAndRoutePtr& item : items) {
      if (item->sink->id == route.media_sink_id()) {
        item->route->id = route.media_route_id();
        item->route->title = StripEndingTab(route.description());
        item->route->is_local_source = route.is_local();

        // Default to a tab/app capture. This will display the media router
        // description. This means we will properly support DIAL casts.
        item->route->content_source = ash::mojom::ContentSource::TAB;
        if (media_router::IsDesktopMirroringMediaSource(route.media_source()))
          item->route->content_source = ash::mojom::ContentSource::DESKTOP;

        break;
      }
    }
  }

  cast_config_->OnDevicesUpdated(std::move(items));
}

void CastConfigClientMediaRouter::CastToSink(ash::mojom::CastSinkPtr sink) {
  // TODO(imcheng): Pass in tab casting timeout.
  GetMediaRouter()->CreateRoute(
      media_router::MediaSourceForDesktop().id(), sink->id,
      url::Origin::Create(GURL("http://cros-cast-origin/")), nullptr,
      std::vector<media_router::MediaRouteResponseCallback>(),
      base::TimeDelta(), false);
}

void CastConfigClientMediaRouter::StopCasting(ash::mojom::CastRoutePtr route) {
  GetMediaRouter()->TerminateRoute(route->id);
}

void CastConfigClientMediaRouter::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_LOGIN_USER_PROFILE_PREPARED:
      // The active profile has changed, which means that the media router has
      // as well. Reset the device cache to ensure we are using up-to-date
      // object instances.
      devices_.reset();
      RequestDeviceRefresh();
      break;
  }
}
