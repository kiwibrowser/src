// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_media_route_provider.h"

#include "base/stl_util.h"
#include "chrome/common/media_router/providers/cast/cast_media_source.h"
#include "components/cast_channel/cast_message_handler.h"
#include "url/origin.h"

namespace media_router {

namespace {

static constexpr MediaRouteProviderId kProviderId = MediaRouteProviderId::CAST;

// Returns a list of origins that are valid for |source_id|. An empty list
// means all origins are valid.
std::vector<url::Origin> GetOrigins(const MediaSource::Id& source_id) {
  // Use of the mirroring app as a Cast URL is permitted for Slides as a
  // temporary workaround only. The eventual goal is to support their usecase
  // using generic Presentation API.
  // See also cast_media_source.cc.
  static const char kMirroringAppPrefix[] = "cast:0F5096E8";
  return base::StartsWith(source_id, kMirroringAppPrefix,
                          base::CompareCase::SENSITIVE)
             ? std::vector<url::Origin>(
                   {url::Origin::Create(GURL("https://docs.google.com"))})
             : std::vector<url::Origin>();
}

}  // namespace

CastMediaRouteProvider::CastMediaRouteProvider(
    mojom::MediaRouteProviderRequest request,
    mojom::MediaRouterPtrInfo media_router,
    MediaSinkServiceBase* media_sink_service,
    CastAppDiscoveryService* app_discovery_service,
    cast_channel::CastMessageHandler* message_handler,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : binding_(this),
      media_sink_service_(media_sink_service),
      app_discovery_service_(app_discovery_service),
      message_handler_(message_handler) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  DCHECK(media_sink_service_);
  DCHECK(app_discovery_service_);
  DCHECK(message_handler_);

  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&CastMediaRouteProvider::Init, base::Unretained(this),
                     std::move(request), std::move(media_router)));
}

void CastMediaRouteProvider::Init(mojom::MediaRouteProviderRequest request,
                                  mojom::MediaRouterPtrInfo media_router) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  binding_.Bind(std::move(request));
  media_router_.Bind(std::move(media_router));

  // TODO(crbug.com/816702): This needs to be set properly according to sinks
  // discovered.
  media_router_->OnSinkAvailabilityUpdated(
      kProviderId, mojom::MediaRouter::SinkAvailability::PER_SOURCE);
}

CastMediaRouteProvider::~CastMediaRouteProvider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(sink_queries_.empty());
}

void CastMediaRouteProvider::CreateRoute(const std::string& media_source,
                                         const std::string& sink_id,
                                         const std::string& presentation_id,
                                         const url::Origin& origin,
                                         int32_t tab_id,
                                         base::TimeDelta timeout,
                                         bool incognito,
                                         CreateRouteCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(
      base::nullopt, std::string("Not implemented"),
      RouteRequestResult::ResultCode::NO_SUPPORTED_PROVIDER);
}

void CastMediaRouteProvider::JoinRoute(const std::string& media_source,
                                       const std::string& presentation_id,
                                       const url::Origin& origin,
                                       int32_t tab_id,
                                       base::TimeDelta timeout,
                                       bool incognito,
                                       JoinRouteCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(
      base::nullopt, std::string("Not implemented"),
      RouteRequestResult::ResultCode::NO_SUPPORTED_PROVIDER);
}

void CastMediaRouteProvider::ConnectRouteByRouteId(
    const std::string& media_source,
    const std::string& route_id,
    const std::string& presentation_id,
    const url::Origin& origin,
    int32_t tab_id,
    base::TimeDelta timeout,
    bool incognito,
    ConnectRouteByRouteIdCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(
      base::nullopt, std::string("Not implemented"),
      RouteRequestResult::ResultCode::NO_SUPPORTED_PROVIDER);
}

void CastMediaRouteProvider::TerminateRoute(const std::string& route_id,
                                            TerminateRouteCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(
      std::string("Not implemented"),
      RouteRequestResult::ResultCode::NO_SUPPORTED_PROVIDER);
}

void CastMediaRouteProvider::SendRouteMessage(
    const std::string& media_route_id,
    const std::string& message,
    SendRouteMessageCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void CastMediaRouteProvider::SendRouteBinaryMessage(
    const std::string& media_route_id,
    const std::vector<uint8_t>& data,
    SendRouteBinaryMessageCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void CastMediaRouteProvider::StartObservingMediaSinks(
    const std::string& media_source) {
  DVLOG(1) << __func__ << ", media_source: " << media_source;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (base::ContainsKey(sink_queries_, media_source))
    return;

  std::unique_ptr<CastMediaSource> cast_source =
      CastMediaSource::From(media_source);
  if (!cast_source)
    return;

  // A broadcast request is not an actual sink query; it is used to send a
  // app precache message to receivers.
  if (cast_source->broadcast_request()) {
    // TODO(imcheng): Add metric to record broadcast usage.
    BroadcastMessageToSinks(cast_source->GetAppIds(),
                            *cast_source->broadcast_request());
    return;
  }

  sink_queries_[media_source] =
      app_discovery_service_->StartObservingMediaSinks(
          *cast_source,
          base::BindRepeating(&CastMediaRouteProvider::OnSinkQueryUpdated,
                              base::Unretained(this)));
}

void CastMediaRouteProvider::StopObservingMediaSinks(
    const std::string& media_source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  sink_queries_.erase(media_source);
}

void CastMediaRouteProvider::StartObservingMediaRoutes(
    const std::string& media_source) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::StopObservingMediaRoutes(
    const std::string& media_source) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::StartListeningForRouteMessages(
    const std::string& route_id) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::StopListeningForRouteMessages(
    const std::string& route_id) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::DetachRoute(const std::string& route_id) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::EnableMdnsDiscovery() {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::UpdateMediaSinks(const std::string& media_source) {
  app_discovery_service_->Refresh();
}

void CastMediaRouteProvider::SearchSinks(
    const std::string& sink_id,
    const std::string& media_source,
    mojom::SinkSearchCriteriaPtr search_criteria,
    SearchSinksCallback callback) {
  std::move(callback).Run(std::string());
}

void CastMediaRouteProvider::ProvideSinks(
    const std::string& provider_name,
    const std::vector<media_router::MediaSinkInternal>& sinks) {
  NOTIMPLEMENTED();
}

void CastMediaRouteProvider::CreateMediaRouteController(
    const std::string& route_id,
    mojom::MediaControllerRequest media_controller,
    mojom::MediaStatusObserverPtr observer,
    CreateMediaRouteControllerCallback callback) {
  NOTIMPLEMENTED();
  std::move(callback).Run(false);
}

void CastMediaRouteProvider::OnSinkQueryUpdated(
    const MediaSource::Id& source_id,
    const std::vector<MediaSinkInternal>& sinks) {
  DVLOG(1) << __func__ << ", source_id: " << source_id
           << ", #sinks: " << sinks.size();
  media_router_->OnSinksReceived(MediaRouteProviderId::CAST, source_id, sinks,
                                 GetOrigins(source_id));
}

void CastMediaRouteProvider::BroadcastMessageToSinks(
    const std::vector<std::string>& app_ids,
    const cast_channel::BroadcastRequest& request) {
  for (const auto& id_and_sink : media_sink_service_->GetSinks()) {
    const MediaSinkInternal& sink = id_and_sink.second;
    message_handler_->SendBroadcastMessage(sink.cast_data().cast_channel_id,
                                           app_ids, request);
  }
}

}  // namespace media_router
