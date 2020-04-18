// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/media_router/media_router_ui_base.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/media/router/media_router.h"
#include "chrome/browser/media/router/media_router_factory.h"
#include "chrome/browser/media/router/media_router_metrics.h"
#include "chrome/browser/media/router/media_routes_observer.h"
#include "chrome/browser/media/router/presentation/presentation_service_delegate_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/common/media_router/media_route.h"
#include "chrome/common/media_router/media_sink.h"
#include "chrome/common/media_router/media_source.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "chrome/common/media_router/route_request_result.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "third_party/icu/source/i18n/unicode/coll.h"
#include "url/origin.h"

#if !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)
#include "chrome/browser/media/router/providers/wired_display/wired_display_media_route_provider.h"
#include "ui/display/display.h"
#endif

namespace media_router {

namespace {

std::string TruncateHost(const std::string& host) {
  const std::string truncated =
      net::registry_controlled_domains::GetDomainAndRegistry(
          host, net::registry_controlled_domains::EXCLUDE_PRIVATE_REGISTRIES);
  // The truncation will be empty in some scenarios (e.g. host is
  // simply an IP address). Fail gracefully.
  return truncated.empty() ? host : truncated;
}

// Returns the first source in |sources| that can be connected to, or an empty
// source if there is none.  This is used by the Media Router to find such a
// matching route if it exists.
MediaSource GetSourceForRouteObserver(const std::vector<MediaSource>& sources) {
  auto source_it =
      std::find_if(sources.begin(), sources.end(), IsCastPresentationUrl);
  return source_it != sources.end() ? *source_it : MediaSource("");
}

}  // namespace

MediaRouterUIBase::MediaRouterUIBase()
    : current_route_request_id_(-1),
      route_request_counter_(0),
      initiator_(nullptr),
      weak_factory_(this) {}

MediaRouterUIBase::~MediaRouterUIBase() {
  if (query_result_manager_.get())
    query_result_manager_->RemoveObserver(this);
  if (presentation_service_delegate_.get())
    presentation_service_delegate_->RemoveDefaultPresentationRequestObserver(
        this);
  // If |start_presentation_context_| still exists, then it means presentation
  // route request was never attempted.
  if (start_presentation_context_) {
    bool presentation_sinks_available = std::any_of(
        sinks_.begin(), sinks_.end(), [](const MediaSinkWithCastModes& sink) {
          return base::ContainsKey(sink.cast_modes,
                                   MediaCastMode::PRESENTATION);
        });
    if (presentation_sinks_available) {
      start_presentation_context_->InvokeErrorCallback(
          blink::mojom::PresentationError(blink::mojom::PresentationErrorType::
                                              PRESENTATION_REQUEST_CANCELLED,
                                          "Dialog closed."));
    } else {
      start_presentation_context_->InvokeErrorCallback(
          blink::mojom::PresentationError(
              blink::mojom::PresentationErrorType::NO_AVAILABLE_SCREENS,
              "No screens found."));
    }
  }
}

void MediaRouterUIBase::InitWithDefaultMediaSource(
    content::WebContents* initiator,
    PresentationServiceDelegateImpl* delegate) {
  DCHECK(initiator);
  DCHECK(!presentation_service_delegate_);
  DCHECK(!query_result_manager_);

  InitCommon(initiator);
  if (delegate) {
    presentation_service_delegate_ = delegate->GetWeakPtr();
    presentation_service_delegate_->AddDefaultPresentationRequestObserver(this);
  }

  if (delegate && delegate->HasDefaultPresentationRequest()) {
    OnDefaultPresentationChanged(delegate->GetDefaultPresentationRequest());
  } else {
    // Register for MediaRoute updates without a media source.
    routes_observer_ = std::make_unique<UIMediaRoutesObserver>(
        GetMediaRouter(), MediaSource::Id(),
        base::BindRepeating(&MediaRouterUIBase::OnRoutesUpdated,
                            base::Unretained(this)));
  }
}

void MediaRouterUIBase::InitWithStartPresentationContext(
    content::WebContents* initiator,
    PresentationServiceDelegateImpl* delegate,
    std::unique_ptr<StartPresentationContext> context) {
  DCHECK(initiator);
  DCHECK(delegate);
  DCHECK(context);
  DCHECK(!start_presentation_context_);
  DCHECK(!query_result_manager_);

  start_presentation_context_ = std::move(context);
  presentation_service_delegate_ = delegate->GetWeakPtr();

  InitCommon(initiator);
  OnDefaultPresentationChanged(
      start_presentation_context_->presentation_request());
}

bool MediaRouterUIBase::CreateRoute(const MediaSink::Id& sink_id,
                                    MediaCastMode cast_mode) {
  base::Optional<RouteParameters> params =
      GetRouteParameters(sink_id, cast_mode);
  if (!params)
    return false;

  GetMediaRouter()->CreateRoute(params->source_id, sink_id, params->origin,
                                initiator_,
                                std::move(params->route_response_callbacks),
                                params->timeout, params->incognito);
  return true;
}

void MediaRouterUIBase::TerminateRoute(const MediaRoute::Id& route_id) {
  GetMediaRouter()->TerminateRoute(route_id);
}

void MediaRouterUIBase::MaybeReportCastingSource(
    MediaCastMode cast_mode,
    const RouteRequestResult& result) {
  if (result.result_code() == RouteRequestResult::OK)
    MediaRouterMetrics::RecordMediaRouterCastingSource(cast_mode);
}

std::vector<MediaSinkWithCastModes> MediaRouterUIBase::GetEnabledSinks() const {
#if !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)
  if (!display_observer_)
    return sinks_;

  // Filter out the wired display sink for the display that the dialog is on.
  // This is not the best place to do this because MRUI should not perform a
  // provider-specific behavior, but we currently do not have a way to
  // communicate dialog-specific information to/from the
  // WiredDisplayMediaRouteProvider.
  std::vector<MediaSinkWithCastModes> enabled_sinks;
  const std::string display_sink_id =
      WiredDisplayMediaRouteProvider::GetSinkIdForDisplay(
          display_observer_->GetCurrentDisplay());
  for (const MediaSinkWithCastModes& sink : sinks_) {
    if (sink.sink.id() != display_sink_id)
      enabled_sinks.push_back(sink);
  }
  return enabled_sinks;
#else
  return sinks_;
#endif
}

std::string MediaRouterUIBase::GetTruncatedPresentationRequestSourceName()
    const {
  GURL gurl = GetFrameURL();
  CHECK(initiator());
  return gurl.SchemeIs(extensions::kExtensionScheme)
             ? GetExtensionName(gurl, extensions::ExtensionRegistry::Get(
                                          initiator()->GetBrowserContext()))
             : TruncateHost(GetHostFromURL(gurl));
}

std::vector<MediaSource> MediaRouterUIBase::GetSourcesForCastMode(
    MediaCastMode cast_mode) const {
  return query_result_manager_->GetSourcesForCastMode(cast_mode);
}

void MediaRouterUIBase::OnResultsUpdated(
    const std::vector<MediaSinkWithCastModes>& sinks) {
  sinks_ = sinks;

  const icu::Collator* collator_ptr = collator_.get();
  std::sort(sinks_.begin(), sinks_.end(),
            [collator_ptr](const MediaSinkWithCastModes& sink1,
                           const MediaSinkWithCastModes& sink2) {
              return sink1.sink.CompareUsingCollator(sink2.sink, collator_ptr);
            });
  UpdateSinks();
}

void MediaRouterUIBase::OnRoutesUpdated(
    const std::vector<MediaRoute>& routes,
    const std::vector<MediaRoute::Id>& joinable_route_ids) {
  routes_.clear();

  for (const MediaRoute& route : routes) {
    if (route.for_display()) {
#ifndef NDEBUG
      for (const MediaRoute& existing_route : routes_) {
        if (existing_route.media_sink_id() == route.media_sink_id()) {
          DVLOG(2) << "Received another route for display with the same sink"
                   << " id as an existing route. " << route.media_route_id()
                   << " has the same sink id as "
                   << existing_route.media_sink_id() << ".";
        }
      }
#endif
      routes_.push_back(route);
    }
  }
}

void MediaRouterUIBase::OnRouteResponseReceived(
    int route_request_id,
    const MediaSink::Id& sink_id,
    MediaCastMode cast_mode,
    const base::string16& presentation_request_source_name,
    const RouteRequestResult& result) {
  DVLOG(1) << "OnRouteResponseReceived";
  // If we receive a new route that we aren't expecting, do nothing.
  if (route_request_id != current_route_request_id_)
    return;

  const MediaRoute* route = result.route();
  if (!route) {
    // The provider will handle sending an issue for a failed route request.
    DVLOG(1) << "MediaRouteResponse returned error: " << result.error();
  }

  current_route_request_id_ = -1;
}

void MediaRouterUIBase::HandleCreateSessionRequestRouteResponse(
    const RouteRequestResult&) {}

void MediaRouterUIBase::InitCommon(content::WebContents* initiator) {
  DCHECK(initiator);
  initiator_ = initiator;

  GetMediaRouter()->OnUserGesture();

  // Create |collator_| before |query_result_manager_| so that |collator_| is
  // already set up when we get a callback from |query_result_manager_|.
  UErrorCode error = U_ZERO_ERROR;
  const std::string& locale = g_browser_process->GetApplicationLocale();
  collator_.reset(
      icu::Collator::createInstance(icu::Locale(locale.c_str()), error));
  if (U_FAILURE(error)) {
    DLOG(ERROR) << "Failed to create collator for locale " << locale;
    collator_.reset();
  }

  query_result_manager_ =
      std::make_unique<QueryResultManager>(GetMediaRouter());
  query_result_manager_->AddObserver(this);

  // Use a placeholder URL as origin for mirroring.
  url::Origin origin = url::Origin::Create(GURL());

  // Desktop mirror mode is always available.
  query_result_manager_->SetSourcesForCastMode(
      MediaCastMode::DESKTOP_MIRROR, {MediaSourceForDesktop()}, origin);

  // File mirroring is always availible.
  query_result_manager_->SetSourcesForCastMode(MediaCastMode::LOCAL_FILE,
                                               {MediaSourceForTab(0)}, origin);

  SessionID::id_type tab_id = SessionTabHelper::IdForTab(initiator).id();
  if (tab_id != -1) {
    MediaSource mirroring_source(MediaSourceForTab(tab_id));
    query_result_manager_->SetSourcesForCastMode(MediaCastMode::TAB_MIRROR,
                                                 {mirroring_source}, origin);
  }

  // Get the current list of media routes, so that the WebUI will have routes
  // information at initialization.
  OnRoutesUpdated(GetMediaRouter()->GetCurrentRoutes(),
                  std::vector<MediaRoute::Id>());
#if !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)
  display_observer_ = WebContentsDisplayObserver::Create(
      initiator_, base::BindRepeating(&MediaRouterUIBase::UpdateSinks,
                                      base::Unretained(this)));
#endif
}

void MediaRouterUIBase::OnDefaultPresentationChanged(
    const content::PresentationRequest& presentation_request) {
  std::vector<MediaSource> sources =
      MediaSourcesForPresentationUrls(presentation_request.presentation_urls);
  presentation_request_ = presentation_request;
  query_result_manager_->SetSourcesForCastMode(
      MediaCastMode::PRESENTATION, sources,
      presentation_request_->frame_origin);
  // Register for MediaRoute updates.  NOTE(mfoltz): If there are multiple
  // sources that can be connected to via the dialog, this will break.  We will
  // need to observe multiple sources (keyed by sinks) in that case.  As this is
  // Cast-specific for the forseeable future, it may be simpler to plumb a new
  // observer API for this case.
  const MediaSource source_for_route_observer =
      GetSourceForRouteObserver(sources);
  routes_observer_ = std::make_unique<UIMediaRoutesObserver>(
      GetMediaRouter(), source_for_route_observer.id(),
      base::BindRepeating(&MediaRouterUIBase::OnRoutesUpdated,
                          base::Unretained(this)));
}

void MediaRouterUIBase::OnDefaultPresentationRemoved() {
  presentation_request_.reset();
  query_result_manager_->RemoveSourcesForCastMode(MediaCastMode::PRESENTATION);

  // Register for MediaRoute updates without a media source.
  routes_observer_ = std::make_unique<UIMediaRoutesObserver>(
      GetMediaRouter(), MediaSource::Id(),
      base::BindRepeating(&MediaRouterUIBase::OnRoutesUpdated,
                          base::Unretained(this)));
}

base::Optional<RouteParameters> MediaRouterUIBase::GetRouteParameters(
    const MediaSink::Id& sink_id,
    MediaCastMode cast_mode) {
  DCHECK(query_result_manager_);
  DCHECK(initiator_);

  RouteParameters params;

  // Note that there is a rarely-encountered bug, where the MediaCastMode to
  // MediaSource mapping could have been updated, between when the user clicked
  // on the UI to start a create route request, and when this function is
  // called. However, since the user does not have visibility into the
  // MediaSource, and that it occurs very rarely in practice, we leave it as-is
  // for now.
  std::unique_ptr<MediaSource> source =
      query_result_manager_->GetSourceForCastModeAndSink(cast_mode, sink_id);

  if (!source) {
    LOG(ERROR) << "No corresponding MediaSource for cast mode "
               << static_cast<int>(cast_mode) << " and sink " << sink_id;
    return base::nullopt;
  }
  params.source_id = source->id();

  bool for_presentation_source = cast_mode == MediaCastMode::PRESENTATION;
  if (for_presentation_source && !presentation_request_) {
    DLOG(ERROR) << "Requested to create a route for presentation, but "
                << "presentation request is missing.";
    return base::nullopt;
  }

  current_route_request_id_ = ++route_request_counter_;
  params.origin = for_presentation_source ? presentation_request_->frame_origin
                                          : url::Origin::Create(GURL());
  DVLOG(1) << "DoCreateRoute: origin: " << params.origin;

  // There are 3 cases. In cases (1) and (3) the MediaRouterUIBase will need to
  // be notified. In case (2) the dialog will be closed.
  // (1) Non-presentation route request (e.g., mirroring). No additional
  //     notification necessary.
  // (2) Presentation route request for a PresentationRequest.start() call.
  //     The StartPresentationContext will need to be answered with the route
  //     response.
  // (3) Browser-initiated presentation route request. If successful,
  //     PresentationServiceDelegateImpl will have to be notified. Note that we
  //     treat subsequent route requests from a Presentation API-initiated
  //     dialogs as browser-initiated.
  if (!for_presentation_source || !start_presentation_context_) {
    params.route_response_callbacks.push_back(base::BindOnce(
        &MediaRouterUIBase::OnRouteResponseReceived, weak_factory_.GetWeakPtr(),
        current_route_request_id_, sink_id, cast_mode,
        base::UTF8ToUTF16(GetTruncatedPresentationRequestSourceName())));
  }
  if (for_presentation_source) {
    if (start_presentation_context_) {
      // |start_presentation_context_| will be nullptr after this call, as the
      // object will be transferred to the callback.
      params.route_response_callbacks.push_back(
          base::BindOnce(&StartPresentationContext::HandleRouteResponse,
                         std::move(start_presentation_context_)));
      params.route_response_callbacks.push_back(base::BindOnce(
          &MediaRouterUIBase::HandleCreateSessionRequestRouteResponse,
          weak_factory_.GetWeakPtr()));
    } else if (presentation_service_delegate_) {
      params.route_response_callbacks.push_back(base::BindOnce(
          &PresentationServiceDelegateImpl::OnRouteResponse,
          presentation_service_delegate_, *presentation_request_));
    }
  }

  params.route_response_callbacks.push_back(
      base::BindOnce(&MediaRouterUIBase::MaybeReportCastingSource,
                     weak_factory_.GetWeakPtr(), cast_mode));

  params.timeout = GetRouteRequestTimeout(cast_mode);
  CHECK(initiator());
  params.incognito = initiator()->GetBrowserContext()->IsOffTheRecord();

  return base::make_optional(std::move(params));
}

GURL MediaRouterUIBase::GetFrameURL() const {
  return presentation_request_ ? presentation_request_->frame_origin.GetURL()
                               : GURL();
}

MediaRouterUIBase::UIMediaRoutesObserver::UIMediaRoutesObserver(
    MediaRouter* router,
    const MediaSource::Id& source_id,
    const RoutesUpdatedCallback& callback)
    : MediaRoutesObserver(router, source_id), callback_(callback) {
  DCHECK(!callback_.is_null());
}

MediaRouterUIBase::UIMediaRoutesObserver::~UIMediaRoutesObserver() {}

void MediaRouterUIBase::UIMediaRoutesObserver::OnRoutesUpdated(
    const std::vector<MediaRoute>& routes,
    const std::vector<MediaRoute::Id>& joinable_route_ids) {
  callback_.Run(routes, joinable_route_ids);
}

MediaRouter* MediaRouterUIBase::GetMediaRouter() const {
  CHECK(initiator());
  return MediaRouterFactory::GetApiForBrowserContext(
      initiator()->GetBrowserContext());
}

}  // namespace media_router
