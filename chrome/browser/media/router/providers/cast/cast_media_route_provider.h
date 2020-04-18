// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MEDIA_ROUTE_PROVIDER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MEDIA_ROUTE_PROVIDER_H_

#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "chrome/browser/media/router/providers/cast/cast_app_discovery_service.h"
#include "chrome/browser/media/router/providers/cast/dual_media_sink_service.h"
#include "chrome/common/media_router/mojo/media_router.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace cast_channel {
class CastMessageHandler;
}

namespace url {
class Origin;
}

namespace media_router {

// MediaRouteProvider for Cast sinks. This class may be created on any sequence.
// All other methods, however, must be called on the task runner provided
// during construction.
class CastMediaRouteProvider : public mojom::MediaRouteProvider {
 public:
  CastMediaRouteProvider(
      mojom::MediaRouteProviderRequest request,
      mojom::MediaRouterPtrInfo media_router,
      MediaSinkServiceBase* media_sink_service,
      CastAppDiscoveryService* app_discovery_service,
      cast_channel::CastMessageHandler* message_handler,
      const scoped_refptr<base::SequencedTaskRunner>& task_runner);
  ~CastMediaRouteProvider() override;

  // mojom::MediaRouteProvider:
  void CreateRoute(const std::string& media_source,
                   const std::string& sink_id,
                   const std::string& presentation_id,
                   const url::Origin& origin,
                   int32_t tab_id,
                   base::TimeDelta timeout,
                   bool incognito,
                   CreateRouteCallback callback) override;
  void JoinRoute(const std::string& media_source,
                 const std::string& presentation_id,
                 const url::Origin& origin,
                 int32_t tab_id,
                 base::TimeDelta timeout,
                 bool incognito,
                 JoinRouteCallback callback) override;
  void ConnectRouteByRouteId(const std::string& media_source,
                             const std::string& route_id,
                             const std::string& presentation_id,
                             const url::Origin& origin,
                             int32_t tab_id,
                             base::TimeDelta timeout,
                             bool incognito,
                             ConnectRouteByRouteIdCallback callback) override;
  void TerminateRoute(const std::string& route_id,
                      TerminateRouteCallback callback) override;
  void SendRouteMessage(const std::string& media_route_id,
                        const std::string& message,
                        SendRouteMessageCallback callback) override;
  void SendRouteBinaryMessage(const std::string& media_route_id,
                              const std::vector<uint8_t>& data,
                              SendRouteBinaryMessageCallback callback) override;
  void StartObservingMediaSinks(const std::string& media_source) override;
  void StopObservingMediaSinks(const std::string& media_source) override;
  void StartObservingMediaRoutes(const std::string& media_source) override;
  void StopObservingMediaRoutes(const std::string& media_source) override;
  void StartListeningForRouteMessages(const std::string& route_id) override;
  void StopListeningForRouteMessages(const std::string& route_id) override;
  void DetachRoute(const std::string& route_id) override;
  void EnableMdnsDiscovery() override;
  void UpdateMediaSinks(const std::string& media_source) override;
  void SearchSinks(const std::string& sink_id,
                   const std::string& media_source,
                   mojom::SinkSearchCriteriaPtr search_criteria,
                   SearchSinksCallback callback) override;
  void ProvideSinks(
      const std::string& provider_name,
      const std::vector<media_router::MediaSinkInternal>& sinks) override;
  void CreateMediaRouteController(
      const std::string& route_id,
      mojom::MediaControllerRequest media_controller,
      mojom::MediaStatusObserverPtr observer,
      CreateMediaRouteControllerCallback callback) override;

 private:
  void Init(mojom::MediaRouteProviderRequest request,
            mojom::MediaRouterPtrInfo media_router);

  // Notifies |media_router_| that results for a sink query has been updated.
  void OnSinkQueryUpdated(const MediaSource::Id& source_id,
                          const std::vector<MediaSinkInternal>& sinks);

  // Broadcasts a message with |app_ids| and |requests| to all sinks.
  void BroadcastMessageToSinks(const std::vector<std::string>& app_ids,
                               const cast_channel::BroadcastRequest& request);

  // Binds |this| to the Mojo request passed into the ctor.
  mojo::Binding<mojom::MediaRouteProvider> binding_;

  // Mojo pointer to the Media Router.
  mojom::MediaRouterPtr media_router_;

  // Non-owned pointer to the Cast MediaSinkServiceBase.
  MediaSinkServiceBase* const media_sink_service_;

  // Non-owned pointer to the CastAppDiscoveryService instance.
  CastAppDiscoveryService* const app_discovery_service_;

  // Non-owned pointer to the CastMessageHandler instance.
  cast_channel::CastMessageHandler* const message_handler_;

  // Registered sink queries.
  base::flat_map<MediaSource::Id, CastAppDiscoveryService::Subscription>
      sink_queries_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(CastMediaRouteProvider);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_MEDIA_ROUTE_PROVIDER_H_
