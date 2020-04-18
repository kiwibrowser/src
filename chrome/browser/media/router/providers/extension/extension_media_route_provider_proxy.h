// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_EXTENSION_EXTENSION_MEDIA_ROUTE_PROVIDER_PROXY_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_EXTENSION_EXTENSION_MEDIA_ROUTE_PROVIDER_PROXY_H_

#include "base/memory/weak_ptr.h"
#include "chrome/common/media_router/mojo/media_router.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class BrowserContext;
}

namespace media_router {

class EventPageRequestManager;

// A class that forwards MediaRouteProvider calls to the implementation in the
// component extension via EventPageRequestManager.
//
// The MediaRouter implementation holds a Mojo pointer bound to this object via
// the request passed into the ctor.
//
// Calls from this object to the component extension MRP are queued with
// EventPageRequestManager. When the extension is awake, the Mojo pointer to the
// MRP is valid, so the request is executed immediately. Otherwise, when the
// extension is awakened, this object obtains a valid Mojo pointer to the MRP
// from MediaRouter, and MediaRouter makes EventPageRequestManager execute all
// the requests.
class ExtensionMediaRouteProviderProxy : public mojom::MediaRouteProvider {
 public:
  ExtensionMediaRouteProviderProxy(content::BrowserContext* context,
                                   mojom::MediaRouteProviderRequest request);
  ~ExtensionMediaRouteProviderProxy() override;

  // mojom::MediaRouteProvider implementation. Forwards the calls to
  // |media_route_provider_| through |request_manager_|.
  void CreateRoute(const std::string& media_source,
                   const std::string& sink_id,
                   const std::string& original_presentation_id,
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

  // Sets the MediaRouteProvider to forward calls to. Notifies
  // |request_manager_| that Mojo connections are ready.
  void RegisterMediaRouteProvider(
      mojom::MediaRouteProviderPtr media_route_provider);

  // Called when a Mojo connection to the component extension is invalidated.
  void OnMojoConnectionError();

  // Sets the extension ID used by |request_manager_|.
  void SetExtensionId(const std::string& extension_id);

 private:
  // These methods call the corresponding |media_route_provider_| methods.
  // Passed to |request_manager_| as requests to be run when the Mojo connection
  // to the provider is established.
  void DoCreateRoute(const std::string& media_source,
                     const std::string& sink_id,
                     const std::string& original_presentation_id,
                     const url::Origin& origin,
                     int32_t tab_id,
                     base::TimeDelta timeout,
                     bool incognito,
                     CreateRouteCallback callback);
  void DoJoinRoute(const std::string& media_source,
                   const std::string& presentation_id,
                   const url::Origin& origin,
                   int32_t tab_id,
                   base::TimeDelta timeout,
                   bool incognito,
                   JoinRouteCallback callback);
  void DoConnectRouteByRouteId(const std::string& media_source,
                               const std::string& route_id,
                               const std::string& presentation_id,
                               const url::Origin& origin,
                               int32_t tab_id,
                               base::TimeDelta timeout,
                               bool incognito,
                               ConnectRouteByRouteIdCallback callback);
  void DoTerminateRoute(const std::string& route_id,
                        TerminateRouteCallback callback);
  void DoSendRouteMessage(const std::string& media_route_id,
                          const std::string& message,
                          SendRouteMessageCallback callback);
  void DoSendRouteBinaryMessage(const std::string& media_route_id,
                                const std::vector<uint8_t>& data,
                                SendRouteBinaryMessageCallback callback);
  void DoStartObservingMediaSinks(const std::string& media_source);
  void DoStopObservingMediaSinks(const std::string& media_source);
  void DoStartObservingMediaRoutes(const std::string& media_source);
  void DoStopObservingMediaRoutes(const std::string& media_source);
  void DoStartListeningForRouteMessages(const std::string& route_id);
  void DoStopListeningForRouteMessages(const std::string& route_id);
  void DoDetachRoute(const std::string& route_id);
  void DoEnableMdnsDiscovery();
  void DoUpdateMediaSinks(const std::string& media_source);
  void DoSearchSinks(const std::string& sink_id,
                     const std::string& media_source,
                     mojom::SinkSearchCriteriaPtr search_criteria,
                     SearchSinksCallback callback);
  void DoProvideSinks(
      const std::string& provider_name,
      const std::vector<media_router::MediaSinkInternal>& sinks);
  void DoCreateMediaRouteController(
      const std::string& route_id,
      mojom::MediaControllerRequest media_controller,
      mojom::MediaStatusObserverPtr observer,
      CreateMediaRouteControllerCallback callback);

  // Mojo pointer to the MediaRouteProvider in the component extension.
  // Set to null initially, and later set to the Mojo pointer passed in via
  // RegisterMediaRouteProvider(). This is set to null again when the component
  // extension is suspended or a Mojo channel error occurs.
  mojom::MediaRouteProviderPtr media_route_provider_;

  // Binds |this| to the Mojo request passed into the ctor.
  mojo::Binding<mojom::MediaRouteProvider> binding_;

  // Request manager responsible for waking the component extension and calling
  // the requests to it.
  EventPageRequestManager* const request_manager_;

  base::WeakPtrFactory<ExtensionMediaRouteProviderProxy> weak_factory_;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_EXTENSION_EXTENSION_MEDIA_ROUTE_PROVIDER_PROXY_H_
