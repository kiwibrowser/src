// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/media_router_desktop.h"

#include "base/bind_helpers.h"
#include "base/strings/string_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/media/router/media_router_factory.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/browser/media/router/mojo/media_route_controller.h"
#include "chrome/browser/media/router/mojo/media_router_mojo_metrics.h"
#include "chrome/browser/media/router/providers/cast/cast_media_route_provider.h"
#include "chrome/browser/media/router/providers/cast/chrome_cast_message_handler.h"
#include "chrome/browser/media/router/providers/wired_display/wired_display_media_route_provider.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "components/cast_channel/cast_socket_service.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/common/extension.h"
#if defined(OS_WIN)
#include "chrome/browser/media/router/mojo/media_route_provider_util_win.h"
#endif

namespace media_router {

MediaRouterDesktop::~MediaRouterDesktop() = default;

// static
void MediaRouterDesktop::BindToRequest(const extensions::Extension* extension,
                                       content::BrowserContext* context,
                                       mojom::MediaRouterRequest request,
                                       content::RenderFrameHost* source) {
  MediaRouterDesktop* impl = static_cast<MediaRouterDesktop*>(
      MediaRouterFactory::GetApiForBrowserContext(context));
  DCHECK(impl);

  impl->BindToMojoRequest(std::move(request), *extension);
}

void MediaRouterDesktop::OnUserGesture() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  MediaRouterMojoImpl::OnUserGesture();
  // Allow MRPM to intelligently update sinks and observers by passing in a
  // media source.
  UpdateMediaSinks(MediaSourceForDesktop().id());

  media_sink_service_->OnUserGesture();

#if defined(OS_WIN)
  EnsureMdnsDiscoveryEnabled();
#endif
}

base::Value MediaRouterDesktop::GetState() const {
  return media_sink_service_status_.GetStatusAsValue();
}

base::Optional<MediaRouteProviderId>
MediaRouterDesktop::GetProviderIdForPresentation(
    const std::string& presentation_id) {
  // TODO(takumif): Once the Android Media Router also uses MediaRouterMojoImpl,
  // we must support these presentation IDs in Android as well.
  if (presentation_id == kAutoJoinPresentationId ||
      base::StartsWith(presentation_id, kCastPresentationIdPrefix,
                       base::CompareCase::SENSITIVE)) {
    return MediaRouteProviderId::EXTENSION;
  }
  return MediaRouterMojoImpl::GetProviderIdForPresentation(presentation_id);
}

MediaRouterDesktop::MediaRouterDesktop(content::BrowserContext* context)
    : MediaRouterDesktop(context, DualMediaSinkService::GetInstance()) {
  InitializeMediaRouteProviders();
#if defined(OS_WIN)
  CanFirewallUseLocalPorts(
      base::BindOnce(&MediaRouterDesktop::OnFirewallCheckComplete,
                     weak_factory_.GetWeakPtr()));
#endif
}

MediaRouterDesktop::MediaRouterDesktop(content::BrowserContext* context,
                                       DualMediaSinkService* media_sink_service)
    : MediaRouterMojoImpl(context),
      cast_provider_(nullptr, base::OnTaskRunnerDeleter(nullptr)),
      dial_provider_(nullptr, base::OnTaskRunnerDeleter(nullptr)),
      media_sink_service_(media_sink_service),
      weak_factory_(this) {
  InitializeMediaRouteProviders();
}

void MediaRouterDesktop::RegisterMediaRouteProvider(
    MediaRouteProviderId provider_id,
    mojom::MediaRouteProviderPtr media_route_provider_ptr,
    mojom::MediaRouter::RegisterMediaRouteProviderCallback callback) {
  auto config = mojom::MediaRouteProviderConfig::New();
  // Enabling browser side discovery / sink query means disabling extension side
  // discovery / sink query. We are migrating discovery from the external Media
  // Route Provider to the Media Router (https://crbug.com/687383), so we need
  // to disable it in the provider.
  config->enable_cast_discovery = !media_router::CastDiscoveryEnabled();
  config->enable_dial_sink_query =
      !media_router::DialMediaRouteProviderEnabled();
  config->enable_cast_sink_query =
      !media_router::CastMediaRouteProviderEnabled();
  std::move(callback).Run(instance_id(), std::move(config));

  SyncStateToMediaRouteProvider(provider_id);

  if (provider_id == MediaRouteProviderId::EXTENSION) {
    RegisterExtensionMediaRouteProvider(std::move(media_route_provider_ptr));
  } else {
    media_route_provider_ptr.set_connection_error_handler(
        base::BindOnce(&MediaRouterDesktop::OnProviderConnectionError,
                       weak_factory_.GetWeakPtr(), provider_id));
    media_route_providers_[provider_id] = std::move(media_route_provider_ptr);
  }
}

void MediaRouterDesktop::OnSinksReceived(
    MediaRouteProviderId provider_id,
    const std::string& media_source,
    const std::vector<MediaSinkInternal>& internal_sinks,
    const std::vector<url::Origin>& origins) {
  media_sink_service_status_.UpdateAvailableSinks(provider_id, media_source,
                                                  internal_sinks);
  MediaRouterMojoImpl::OnSinksReceived(provider_id, media_source,
                                       internal_sinks, origins);
}

void MediaRouterDesktop::GetMediaSinkServiceStatus(
    mojom::MediaRouter::GetMediaSinkServiceStatusCallback callback) {
  std::move(callback).Run(media_sink_service_status_.GetStatusAsJSONString());
}

void MediaRouterDesktop::RegisterExtensionMediaRouteProvider(
    mojom::MediaRouteProviderPtr extension_provider_ptr) {
  ProvideSinksToExtension();
#if defined(OS_WIN)
  // The extension MRP already turns on mDNS discovery for platforms other than
  // Windows. It only relies on this signalling from MR on Windows to avoid
  // triggering a firewall prompt out of the context of MR from the user's
  // perspective. This particular call reminds the extension to enable mDNS
  // discovery when it wakes up, has been upgraded, etc.
  if (should_enable_mdns_discovery_)
    EnsureMdnsDiscoveryEnabled();
#endif
  // Now that we have a Mojo pointer to the extension MRP, we reset the Mojo
  // pointers to extension-side route controllers and request them to be bound
  // to new implementations. This must happen before EventPageRequestManager
  // executes commands to the MRP and its route controllers. Commands to the
  // route controllers, once executed, will be queued in Mojo pipes until the
  // Mojo requests are bound to implementations.
  // TODO(takumif): Once we have route controllers for MRPs other than the
  // extension MRP, we'll need to group them by MRP so that below is performed
  // only for extension route controllers.
  for (const auto& pair : route_controllers_)
    InitMediaRouteController(pair.second);
  extension_provider_proxy_->RegisterMediaRouteProvider(
      std::move(extension_provider_ptr));
}

void MediaRouterDesktop::BindToMojoRequest(
    mojo::InterfaceRequest<mojom::MediaRouter> request,
    const extensions::Extension& extension) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  MediaRouterMojoImpl::BindToMojoRequest(std::move(request));
  extension_provider_proxy_->SetExtensionId(extension.id());
  if (!provider_version_was_recorded_) {
    MediaRouterMojoMetrics::RecordMediaRouteProviderVersion(extension);
    provider_version_was_recorded_ = true;
  }
}

void MediaRouterDesktop::ProvideSinksToExtension() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DVLOG(1) << "ProvideSinksToExtension";
  // If calling |ProvideSinksToExtension| for the first time, add a callback to
  // be notified of sink updates.
  if (!media_sink_service_subscription_) {
    media_sink_service_subscription_ =
        media_sink_service_->AddSinksDiscoveredCallback(base::BindRepeating(
            &MediaRouterDesktop::ProvideSinks, base::Unretained(this)));
  }

  // Sync the current list of sinks to the extension.
  for (const auto& provider_and_sinks : media_sink_service_->current_sinks())
    ProvideSinks(provider_and_sinks.first, provider_and_sinks.second);
}

void MediaRouterDesktop::ProvideSinks(
    const std::string& provider_name,
    const std::vector<MediaSinkInternal>& sinks) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DVLOG(1) << "Provider [" << provider_name << "] found " << sinks.size()
           << " devices...";
  media_route_providers_[MediaRouteProviderId::EXTENSION]->ProvideSinks(
      provider_name, sinks);

  media_sink_service_status_.UpdateDiscoveredSinks(provider_name, sinks);
}

void MediaRouterDesktop::InitializeMediaRouteProviders() {
  InitializeExtensionMediaRouteProviderProxy();
  if (base::FeatureList::IsEnabled(features::kLocalScreenCasting))
    InitializeWiredDisplayMediaRouteProvider();
  if (CastMediaRouteProviderEnabled())
    InitializeCastMediaRouteProvider();
  if (DialMediaRouteProviderEnabled())
    InitializeDialMediaRouteProvider();
}

void MediaRouterDesktop::InitializeExtensionMediaRouteProviderProxy() {
  mojom::MediaRouteProviderPtr extension_provider_proxy_ptr;
  extension_provider_proxy_ =
      std::make_unique<ExtensionMediaRouteProviderProxy>(
          context(), mojo::MakeRequest(&extension_provider_proxy_ptr));
  media_route_providers_[MediaRouteProviderId::EXTENSION] =
      std::move(extension_provider_proxy_ptr);
}

void MediaRouterDesktop::InitializeWiredDisplayMediaRouteProvider() {
  mojom::MediaRouterPtr media_router_ptr;
  MediaRouterMojoImpl::BindToMojoRequest(mojo::MakeRequest(&media_router_ptr));
  mojom::MediaRouteProviderPtr wired_display_provider_ptr;
  wired_display_provider_ = std::make_unique<WiredDisplayMediaRouteProvider>(
      mojo::MakeRequest(&wired_display_provider_ptr),
      std::move(media_router_ptr), Profile::FromBrowserContext(context()));
  RegisterMediaRouteProvider(MediaRouteProviderId::WIRED_DISPLAY,
                             std::move(wired_display_provider_ptr),
                             base::DoNothing());
}

void MediaRouterDesktop::InitializeCastMediaRouteProvider() {
  auto task_runner =
      cast_channel::CastSocketService::GetInstance()->task_runner();
  mojom::MediaRouterPtr media_router_ptr;
  MediaRouterMojoImpl::BindToMojoRequest(mojo::MakeRequest(&media_router_ptr));
  mojom::MediaRouteProviderPtr cast_provider_ptr;
  cast_provider_ =
      std::unique_ptr<CastMediaRouteProvider, base::OnTaskRunnerDeleter>(
          new CastMediaRouteProvider(
              mojo::MakeRequest(&cast_provider_ptr),
              media_router_ptr.PassInterface(),
              media_sink_service_->GetCastMediaSinkServiceImpl(),
              media_sink_service_->cast_app_discovery_service(),
              GetCastMessageHandler(), task_runner),
          base::OnTaskRunnerDeleter(task_runner));
  RegisterMediaRouteProvider(MediaRouteProviderId::CAST,
                             std::move(cast_provider_ptr), base::DoNothing());
}

void MediaRouterDesktop::InitializeDialMediaRouteProvider() {
  mojom::MediaRouterPtr media_router_ptr;
  MediaRouterMojoImpl::BindToMojoRequest(mojo::MakeRequest(&media_router_ptr));
  mojom::MediaRouteProviderPtr dial_provider_ptr;

  auto* dial_media_sink_service =
      media_sink_service_->GetDialMediaSinkServiceImpl();
  auto task_runner = dial_media_sink_service->task_runner();
  dial_provider_ =
      std::unique_ptr<DialMediaRouteProvider, base::OnTaskRunnerDeleter>(
          new DialMediaRouteProvider(mojo::MakeRequest(&dial_provider_ptr),
                                     media_router_ptr.PassInterface(),
                                     dial_media_sink_service, task_runner),
          base::OnTaskRunnerDeleter(task_runner));
  RegisterMediaRouteProvider(MediaRouteProviderId::DIAL,
                             std::move(dial_provider_ptr), base::DoNothing());
}

#if defined(OS_WIN)
void MediaRouterDesktop::EnsureMdnsDiscoveryEnabled() {
  if (media_router::CastDiscoveryEnabled()) {
    media_sink_service_->StartMdnsDiscovery();
  } else {
    media_route_providers_[MediaRouteProviderId::EXTENSION]
        ->EnableMdnsDiscovery();
  }

  // Record that we enabled mDNS discovery, so that we will know to enable again
  // when we reconnect to the component extension.
  should_enable_mdns_discovery_ = true;
}

void MediaRouterDesktop::OnFirewallCheckComplete(
    bool firewall_can_use_local_ports) {
  if (firewall_can_use_local_ports)
    EnsureMdnsDiscoveryEnabled();
}
#endif

}  // namespace media_router
