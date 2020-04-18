// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/loader/chrome_resource_dispatcher_host_delegate.h"

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/client_hints/client_hints.h"
#include "chrome/browser/component_updater/component_updater_resource_throttle.h"
#include "chrome/browser/download/download_request_limiter.h"
#include "chrome/browser/download/download_resource_throttle.h"
#include "chrome/browser/loader/chrome_navigation_data.h"
#include "chrome/browser/loader/predictor_resource_throttle.h"
#include "chrome/browser/loader/safe_browsing_resource_throttle.h"
#include "chrome/browser/net/loading_predictor_observer.h"
#include "chrome/browser/page_load_metrics/metrics_web_contents_observer.h"
#include "chrome/browser/plugins/plugin_prefs.h"
#include "chrome/browser/plugins/plugin_utils.h"
#include "chrome/browser/prerender/prerender_manager.h"
#include "chrome/browser/prerender/prerender_manager_factory.h"
#include "chrome/browser/prerender/prerender_resource_throttle.h"
#include "chrome/browser/prerender/prerender_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/signin/chrome_signin_helper.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/data_reduction_proxy/content/browser/content_lofi_decider.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_config.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_io_data.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_util.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "components/google/core/browser/google_util.h"
#include "components/nacl/common/buildflags.h"
#include "components/offline_pages/buildflags/buildflags.h"
#include "components/offline_pages/core/request_header/offline_page_navigation_ui_data.h"
#include "components/policy/content/policy_blacklist_navigation_throttle.h"
#include "components/policy/core/common/cloud/policy_header_io_helper.h"
#include "components/previews/content/previews_content_util.h"
#include "components/previews/content/previews_io_data.h"
#include "components/previews/core/previews_decider.h"
#include "components/previews/core/previews_experiments.h"
#include "components/previews/core/previews_user_data.h"
#include "components/safe_browsing/features.h"
#include "components/variations/net/variations_http_headers.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_data.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/plugin_service_filter.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/stream_info.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/content_features.h"
#include "content/public/common/previews_state.h"
#include "extensions/buildflags/buildflags.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_endpoint.h"
#include "net/base/load_flags.h"
#include "net/base/load_timing_info.h"
#include "net/base/request_priority.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/resource_response.h"
#include "third_party/protobuf/src/google/protobuf/repeated_field.h"

#if BUILDFLAG(ENABLE_NACL)
#include "chrome/browser/component_updater/pnacl_component_installer.h"
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/api/streams_private/streams_private_api.h"
#include "chrome/browser/extensions/user_script_listener.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "extensions/browser/extension_throttle_manager.h"
#include "extensions/browser/guest_view/web_view/web_view_renderer_state.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/user_script.h"
#endif

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
#include "chrome/browser/offline_pages/downloads/resource_throttle.h"
#include "chrome/browser/offline_pages/offliner_user_data.h"
#include "chrome/browser/offline_pages/resource_loading_observer.h"
#endif

#if defined(OS_ANDROID)
#include "chrome/browser/android/download/intercept_download_resource_throttle.h"
#include "chrome/browser/loader/data_reduction_proxy_resource_throttle_android.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#endif

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/login/signin/merge_session_resource_throttle.h"
#include "chrome/browser/chromeos/login/signin/merge_session_throttling_utils.h"
#endif

using content::BrowserThread;
using content::LoginDelegate;
using content::RenderViewHost;
using content::ResourceRequestInfo;
using content::ResourceType;

#if BUILDFLAG(ENABLE_EXTENSIONS)
using extensions::Extension;
using extensions::StreamsPrivateAPI;
#endif

#if defined(OS_ANDROID)
using navigation_interception::InterceptNavigationDelegate;
#endif

namespace {

void NotifyDownloadInitiatedOnUI(
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter) {
  content::WebContents* web_contents = wc_getter.Run();
  if (!web_contents)
    return;

  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_DOWNLOAD_INITIATED,
      content::Source<content::WebContents>(web_contents),
      content::NotificationService::NoDetails());
}

prerender::PrerenderManager* GetPrerenderManager(
    content::WebContents* web_contents) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!web_contents)
    return NULL;

  content::BrowserContext* browser_context = web_contents->GetBrowserContext();
  if (!browser_context)
    return NULL;

  return prerender::PrerenderManagerFactory::GetForBrowserContext(
      browser_context);
}

void UpdatePrerenderNetworkBytesCallback(content::WebContents* web_contents,
                                         int64_t bytes) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // PrerenderContents::FromWebContents handles the NULL case.
  prerender::PrerenderContents* prerender_contents =
      prerender::PrerenderContents::FromWebContents(web_contents);

  if (prerender_contents)
    prerender_contents->AddNetworkBytes(bytes);

  prerender::PrerenderManager* prerender_manager =
      GetPrerenderManager(web_contents);
  if (prerender_manager)
    prerender_manager->AddProfileNetworkBytesIfEnabled(bytes);
}

#if BUILDFLAG(ENABLE_NACL)
void AppendComponentUpdaterThrottles(
    net::URLRequest* request,
    const ResourceRequestInfo& info,
    content::ResourceContext* resource_context,
    ResourceType resource_type,
    std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles) {
  if (info.IsPrerendering())
    return;

  const char* crx_id = NULL;
  component_updater::ComponentUpdateService* cus =
      g_browser_process->component_updater();
  if (!cus)
    return;
  // Check for PNaCl pexe request.
  if (resource_type == content::RESOURCE_TYPE_OBJECT) {
    const net::HttpRequestHeaders& headers = request->extra_request_headers();
    std::string accept_headers;
    if (headers.GetHeader("Accept", &accept_headers)) {
      if (accept_headers.find("application/x-pnacl") != std::string::npos &&
          pnacl::NeedsOnDemandUpdate())
        crx_id = "hnimpnehoodheedghdeeijklkeaacbdc";
    }
  }

  if (crx_id) {
    // We got a component we need to install, so throttle the resource
    // until the component is installed.
    throttles->push_back(base::WrapUnique(
        component_updater::GetOnDemandResourceThrottle(cus, crx_id)));
  }
}
#endif  // BUILDFLAG(ENABLE_NACL)

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
// Translate content::ResourceType to a type to use for Offliners.
offline_pages::ResourceLoadingObserver::ResourceDataType
ConvertResourceTypeToResourceDataType(content::ResourceType type) {
  switch (type) {
    case content::RESOURCE_TYPE_STYLESHEET:
      return offline_pages::ResourceLoadingObserver::ResourceDataType::TEXT_CSS;
    case content::RESOURCE_TYPE_IMAGE:
      return offline_pages::ResourceLoadingObserver::ResourceDataType::IMAGE;
    case content::RESOURCE_TYPE_XHR:
      return offline_pages::ResourceLoadingObserver::ResourceDataType::XHR;
    default:
      return offline_pages::ResourceLoadingObserver::ResourceDataType::OTHER;
  }
}

void NotifyUIThreadOfRequestStarted(
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    ResourceType resource_type) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // TODO(petewil) We're not sure why yet, but we do sometimes see that
  // web_contents_getter returning null.  Until we find out why, avoid crashing.
  // crbug.com/742370
  if (web_contents_getter.is_null())
    return;

  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return;

  // If we are producing an offline version of the page, track resource loading.
  offline_pages::ResourceLoadingObserver* resource_tracker =
      offline_pages::OfflinerUserData::ResourceLoadingObserverFromWebContents(
          web_contents);
  if (resource_tracker) {
    offline_pages::ResourceLoadingObserver::ResourceDataType data_type =
        ConvertResourceTypeToResourceDataType(resource_type);
    resource_tracker->ObserveResourceLoading(data_type, true /* STARTED */);
  }
}
#endif

void NotifyUIThreadOfRequestComplete(
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    const content::ResourceRequestInfo::FrameTreeNodeIdGetter&
        frame_tree_node_id_getter,
    const GURL& url,
    const net::HostPortPair& host_port_pair,
    const content::GlobalRequestID& request_id,
    int render_process_id,
    int render_frame_id,
    ResourceType resource_type,
    bool is_download,
    bool was_cached,
    std::unique_ptr<data_reduction_proxy::DataReductionProxyData>
        data_reduction_proxy_data,
    int net_error,
    int64_t total_received_bytes,
    int64_t raw_body_bytes,
    int64_t original_content_length,
    base::TimeTicks request_creation_time,
    std::unique_ptr<net::LoadTimingInfo> load_timing_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return;

  if (!was_cached) {
    UpdatePrerenderNetworkBytesCallback(web_contents, total_received_bytes);
  }

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  // If we are producing an offline version of the page, track resource loading.
  offline_pages::ResourceLoadingObserver* resource_tracker =
      offline_pages::OfflinerUserData::ResourceLoadingObserverFromWebContents(
          web_contents);
  if (resource_tracker) {
    offline_pages::ResourceLoadingObserver::ResourceDataType data_type =
        ConvertResourceTypeToResourceDataType(resource_type);
    resource_tracker->ObserveResourceLoading(data_type, false /* COMPLETED */);
    if (!was_cached)
      resource_tracker->OnNetworkBytesChanged(total_received_bytes);
  }
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)

  if (!is_download) {
    page_load_metrics::MetricsWebContentsObserver* metrics_observer =
        page_load_metrics::MetricsWebContentsObserver::FromWebContents(
            web_contents);
    if (metrics_observer) {
      // Will be null for main or sub frame resources, when browser-side
      // navigation is enabled.
      content::RenderFrameHost* render_frame_host_or_null =
          content::RenderFrameHost::FromID(render_process_id, render_frame_id);
      metrics_observer->OnRequestComplete(
          url, host_port_pair, frame_tree_node_id_getter.Run(), request_id,
          render_frame_host_or_null, resource_type, was_cached,
          std::move(data_reduction_proxy_data), raw_body_bytes,
          original_content_length, request_creation_time, net_error,
          std::move(load_timing_info));
    }
  }
}

void LogCommittedPreviewsDecision(
    ProfileIOData* io_data,
    const GURL& url,
    previews::PreviewsUserData* previews_user_data) {
  previews::PreviewsIOData* previews_io_data = io_data->previews_io_data();
  if (previews_io_data && previews_user_data) {
    std::vector<previews::PreviewsEligibilityReason> passed_reasons;
    if (previews_user_data->cache_control_no_transform_directive()) {
      previews_io_data->LogPreviewDecisionMade(
          previews::PreviewsEligibilityReason::CACHE_CONTROL_NO_TRANSFORM, url,
          base::Time::Now(), previews::PreviewsType::UNSPECIFIED,
          std::move(passed_reasons), previews_user_data->page_id());
    } else {
      previews_io_data->LogPreviewDecisionMade(
          previews::PreviewsEligibilityReason::COMMITTED, url,
          base::Time::Now(), previews_user_data->committed_previews_type(),
          std::move(passed_reasons), previews_user_data->page_id());
    }
  }
}

}  // namespace

ChromeResourceDispatcherHostDelegate::ChromeResourceDispatcherHostDelegate()
    : download_request_limiter_(g_browser_process->download_request_limiter()),
      safe_browsing_(g_browser_process->safe_browsing_service())
#if BUILDFLAG(ENABLE_EXTENSIONS)
      , user_script_listener_(new extensions::UserScriptListener())
#endif
      {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          content::ServiceWorkerContext::AddExcludedHeadersForFetchEvent,
          variations::GetVariationHeaderNames()));
}

ChromeResourceDispatcherHostDelegate::~ChromeResourceDispatcherHostDelegate() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  CHECK(stream_target_info_.empty());
#endif
}

void ChromeResourceDispatcherHostDelegate::RequestBeginning(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    content::AppCacheService* appcache_service,
    ResourceType resource_type,
    std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles) {
  if (safe_browsing_.get())
    safe_browsing_->OnResourceRequest(request);
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(resource_context);
  client_hints::RequestBeginning(request, io_data->GetCookieSettings());

  const ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(request);

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  // TODO(petewil): Unify the safe browsing request and the metrics observer
  // request if possible so we only have to cross to the main thread once.
  // http://crbug.com/712312.
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(&NotifyUIThreadOfRequestStarted,
                                         info->GetWebContentsGetterForRequest(),
                                         info->GetResourceType()));
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)

#if defined(OS_ANDROID)
  if (resource_type != content::RESOURCE_TYPE_MAIN_FRAME)
    InterceptNavigationDelegate::UpdateUserGestureCarryoverInfo(request);
#endif

#if defined(OS_CHROMEOS)
  // Check if we need to add merge session throttle. This throttle will postpone
  // loading of XHR requests.
  if (resource_type == content::RESOURCE_TYPE_XHR) {
    // Add interstitial page while merge session process (cookie
    // reconstruction from OAuth2 refresh token in ChromeOS login) is still in
    // progress while we are attempting to load a google property.
    if (!merge_session_throttling_utils::AreAllSessionMergedAlready() &&
        request->url().SchemeIsHTTPOrHTTPS()) {
      throttles->push_back(
          std::make_unique<MergeSessionResourceThrottle>(request));
    }
  }
#endif

  // Don't attempt to append headers to requests that have already started.
  // TODO(stevet): Remove this once the request ordering issues are resolved
  // in crbug.com/128048.
  if (!request->is_pending()) {
    net::HttpRequestHeaders headers;
    headers.CopyFrom(request->extra_request_headers());
    bool is_off_the_record = io_data->IsOffTheRecord();
    bool is_signed_in =
        !is_off_the_record &&
        !io_data->google_services_account_id()->GetValue().empty();
    variations::AppendVariationHeaders(
        request->url(),
        is_off_the_record ? variations::InIncognito::kYes
                          : variations::InIncognito::kNo,
        is_signed_in ? variations::SignedIn::kYes : variations::SignedIn::kNo,
        &headers);
    request->SetExtraRequestHeaders(headers);
  }

  if (io_data->policy_header_helper())
    io_data->policy_header_helper()->AddPolicyHeaders(request->url(), request);

  signin::FixAccountConsistencyRequestHeader(request, GURL() /* redirect_url */,
                                             io_data);

  AppendStandardResourceThrottles(request,
                                  resource_context,
                                  resource_type,
                                  throttles);
#if BUILDFLAG(ENABLE_NACL)
  AppendComponentUpdaterThrottles(request, *info, resource_context,
                                  resource_type, throttles);
#endif  // BUILDFLAG(ENABLE_NACL)

  if (io_data->loading_predictor_observer()) {
    io_data->loading_predictor_observer()->OnRequestStarted(
        request, resource_type, info->GetWebContentsGetterForRequest());
  }
}

void ChromeResourceDispatcherHostDelegate::DownloadStarting(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    bool is_content_initiated,
    bool must_download,
    bool is_new_request,
    std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles) {
  const content::ResourceRequestInfo* info =
        content::ResourceRequestInfo::ForRequest(request);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&NotifyDownloadInitiatedOnUI,
                     info->GetWebContentsGetterForRequest()));

  // If it's from the web, we don't trust it, so we push the throttle on.
  if (is_content_initiated) {
    throttles->push_back(std::make_unique<DownloadResourceThrottle>(
        download_request_limiter_, info->GetWebContentsGetterForRequest(),
        request->url(), request->method()));
  }

  // If this isn't a new request, the standard resource throttles have already
  // been added, so no need to add them again.
  if (is_new_request) {
    AppendStandardResourceThrottles(request,
                                    resource_context,
                                    content::RESOURCE_TYPE_MAIN_FRAME,
                                    throttles);
#if defined(OS_ANDROID)
    // On Android, forward text/html downloads to OfflinePages backend.
    throttles->push_back(
        std::make_unique<offline_pages::downloads::ResourceThrottle>(request));
#endif
  }

#if defined(OS_ANDROID)
  // Add the InterceptDownloadResourceThrottle after calling
  // AppendStandardResourceThrottles so the download will not bypass
  // safebrowsing checks.
  if (is_content_initiated) {
    throttles->push_back(std::make_unique<InterceptDownloadResourceThrottle>(
        request, info->GetWebContentsGetterForRequest()));
  }
#endif
}

void ChromeResourceDispatcherHostDelegate::AppendStandardResourceThrottles(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    ResourceType resource_type,
    std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles) {
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(resource_context);

  // Insert either safe browsing or data reduction proxy throttle at the front
  // of the list, so one of them gets to decide if the resource is safe.
  content::ResourceThrottle* first_throttle = NULL;
#if defined(OS_ANDROID)
  first_throttle = DataReductionProxyResourceThrottle::MaybeCreate(
      request, resource_context, resource_type, safe_browsing_.get());
#endif  // defined(OS_ANDROID)

#if defined(SAFE_BROWSING_DB_LOCAL) || defined(SAFE_BROWSING_DB_REMOTE)
  if (!first_throttle && io_data->safe_browsing_enabled()->GetValue() &&
      !base::FeatureList::IsEnabled(safe_browsing::kCheckByURLLoaderThrottle)) {
    first_throttle = MaybeCreateSafeBrowsingResourceThrottle(
        request, resource_type, safe_browsing_.get(), io_data);
  }
#endif  // defined(SAFE_BROWSING_DB_LOCAL) || defined(SAFE_BROWSING_DB_REMOTE)

  if (first_throttle)
    throttles->push_back(base::WrapUnique(first_throttle));

#if BUILDFLAG(ENABLE_EXTENSIONS)
  content::ResourceThrottle* wait_for_extensions_init_throttle =
      user_script_listener_->CreateResourceThrottle(request->url(),
                                                    resource_type);
  if (wait_for_extensions_init_throttle)
    throttles->push_back(base::WrapUnique(wait_for_extensions_init_throttle));

  extensions::ExtensionThrottleManager* extension_throttle_manager =
      io_data->GetExtensionThrottleManager();
  if (extension_throttle_manager) {
    std::unique_ptr<content::ResourceThrottle> extension_throttle =
        extension_throttle_manager->MaybeCreateThrottle(request);
    if (extension_throttle)
      throttles->push_back(std::move(extension_throttle));
  }
#endif

  const ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(request);
  if (info->IsPrerendering()) {
    // TODO(jam): remove this throttle once http://crbug.com/740130 is fixed and
    // PrerendererURLLoaderThrottle can be used for frame requests in the
    // network-service-disabled mode.
    if (!base::FeatureList::IsEnabled(network::features::kNetworkService) &&
        content::IsResourceTypeFrame(info->GetResourceType())) {
      throttles->push_back(
          std::make_unique<prerender::PrerenderResourceThrottle>(request));
    }
  }

  std::unique_ptr<PredictorResourceThrottle> predictor_throttle =
      PredictorResourceThrottle::MaybeCreate(request, io_data);
  if (predictor_throttle)
    throttles->push_back(std::move(predictor_throttle));
}

bool ChromeResourceDispatcherHostDelegate::ShouldInterceptResourceAsStream(
    net::URLRequest* request,
    const std::string& mime_type,
    GURL* origin,
    std::string* payload) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  const ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(request);
  std::string extension_id =
      PluginUtils::GetExtensionIdForMimeType(info->GetContext(), mime_type);
  if (!extension_id.empty()) {
    StreamTargetInfo target_info;
    *origin = Extension::GetBaseURLFromExtensionId(extension_id);
    target_info.extension_id = extension_id;
    target_info.view_id = base::GenerateGUID();
    *payload = target_info.view_id;
    stream_target_info_[request] = target_info;
    return true;
  }
#endif
  return false;
}

void ChromeResourceDispatcherHostDelegate::OnStreamCreated(
    net::URLRequest* request,
    std::unique_ptr<content::StreamInfo> stream) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  const ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(request);
  std::map<net::URLRequest*, StreamTargetInfo>::iterator ix =
      stream_target_info_.find(request);
  CHECK(ix != stream_target_info_.end());
  bool embedded = info->GetResourceType() != content::RESOURCE_TYPE_MAIN_FRAME;
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &extensions::StreamsPrivateAPI::SendExecuteMimeTypeHandlerEvent,
          request->GetExpectedContentSize(), ix->second.extension_id,
          ix->second.view_id, embedded, info->GetFrameTreeNodeId(),
          info->GetChildID(), info->GetRenderFrameID(), std::move(stream),
          nullptr /* transferrable_loader */, GURL()));
  stream_target_info_.erase(request);
#endif
}

void ChromeResourceDispatcherHostDelegate::OnResponseStarted(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    network::ResourceResponse* response) {
  const ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(request);
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(resource_context);

  signin::ProcessAccountConsistencyResponseHeaders(request, GURL(),
                                                   io_data->IsOffTheRecord());

  // Built-in additional protection for the chrome web store origin.
#if BUILDFLAG(ENABLE_EXTENSIONS)
  GURL webstore_url(extension_urls::GetWebstoreLaunchURL());
  if (request->url().SchemeIsHTTPOrHTTPS() &&
      request->url().DomainIs(webstore_url.host_piece())) {
    net::HttpResponseHeaders* response_headers = request->response_headers();
    if (response_headers &&
        !response_headers->HasHeaderValue("x-frame-options", "deny") &&
        !response_headers->HasHeaderValue("x-frame-options", "sameorigin")) {
      response_headers->RemoveHeader("x-frame-options");
      response_headers->AddHeader("x-frame-options: sameorigin");
    }
  }
#endif

  if (io_data->loading_predictor_observer())
    io_data->loading_predictor_observer()->OnResponseStarted(
        request, info->GetWebContentsGetterForRequest());

  // Update the PreviewsState for main frame response if needed.
  if (previews::HasEnabledPreviews(response->head.previews_state) &&
      info->GetResourceType() == content::RESOURCE_TYPE_MAIN_FRAME &&
      request->url().SchemeIsHTTPOrHTTPS()) {
    // Annotate request if no-transform directive found in response headers.
    if (request->response_headers() &&
        request->response_headers()->HasHeaderValue("cache-control",
                                                    "no-transform")) {
      previews::PreviewsUserData* previews_user_data =
          previews::PreviewsUserData::GetData(*request);
      if (previews_user_data)
        previews_user_data->SetCacheControlNoTransformDirective();
    }

    // Determine effective PreviewsState for this committed main frame response.
    content::PreviewsState committed_state = DetermineCommittedPreviews(
        request, io_data->previews_io_data(),
        static_cast<content::PreviewsState>(response->head.previews_state));

    // Update previews state in response to renderer.
    response->head.previews_state = static_cast<int>(committed_state);

    // Update previews state in nav data to UI.
    ChromeNavigationData* data =
        ChromeNavigationData::GetDataAndCreateIfNecessary(request);
    data->set_previews_state(committed_state);

    // Capture committed previews type, if any, in PreviewsUserData.
    // Note: this is for the subset of previews types that are decided upon
    // navigation commit. Previews types that are determined prior to
    // navigation (such as for offline pages or for redirecting to another
    // url), are not set here.
    previews::PreviewsType committed_type =
        previews::GetMainFramePreviewsType(committed_state);
    previews::PreviewsUserData* previews_user_data =
        previews::PreviewsUserData::GetData(*request);
    if (previews_user_data) {
      previews_user_data->SetCommittedPreviewsType(committed_type);
      LogCommittedPreviewsDecision(io_data, request->url(), previews_user_data);
    }
  }
}

void ChromeResourceDispatcherHostDelegate::OnRequestRedirected(
    const GURL& redirect_url,
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    network::ResourceResponse* response) {
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(resource_context);

  // Chrome tries to ensure that the identity is consistent between Chrome and
  // the content area.
  //
  // For example, on Android, for users that are signed in to Chrome, the
  // identity is mirrored into the content area. To do so, Chrome appends a
  // X-Chrome-Connected header to all Gaia requests from a connected profile so
  // Gaia could return a 204 response and let Chrome handle the action with
  // native UI.
  signin::FixAccountConsistencyRequestHeader(request, redirect_url, io_data);
  signin::ProcessAccountConsistencyResponseHeaders(request, redirect_url,
                                                   io_data->IsOffTheRecord());

  if (io_data->loading_predictor_observer()) {
    const ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(request);
    io_data->loading_predictor_observer()->OnRequestRedirected(
        request, redirect_url, info->GetWebContentsGetterForRequest());
  }

  if (io_data->policy_header_helper())
    io_data->policy_header_helper()->AddPolicyHeaders(redirect_url, request);
}

// Notification that a request has completed.
void ChromeResourceDispatcherHostDelegate::RequestComplete(
    net::URLRequest* url_request) {
  if (!url_request)
    return;
  // TODO(maksims): remove this and use net_error argument in RequestComplete
  // once ResourceDispatcherHostDelegate is modified.
  int net_error = url_request->status().error();
  const ResourceRequestInfo* info =
      ResourceRequestInfo::ForRequest(url_request);

  ProfileIOData* io_data =
      ProfileIOData::FromResourceContext(info->GetContext());
  data_reduction_proxy::DataReductionProxyIOData* data_reduction_proxy_io_data =
      io_data->data_reduction_proxy_io_data();
  data_reduction_proxy::LoFiDecider* lofi_decider = nullptr;
  if (data_reduction_proxy_io_data)
    lofi_decider = data_reduction_proxy_io_data->lofi_decider();

  data_reduction_proxy::DataReductionProxyData* data =
      data_reduction_proxy::DataReductionProxyData::GetData(*url_request);
  std::unique_ptr<data_reduction_proxy::DataReductionProxyData>
      data_reduction_proxy_data;
  if (data)
    data_reduction_proxy_data = data->DeepCopy();
  int64_t original_content_length =
      data && data->used_data_reduction_proxy()
          ? data_reduction_proxy::util::EstimateOriginalBodySize(*url_request,
                                                                 lofi_decider)
          : url_request->GetRawBodyBytes();

  net::HostPortPair request_host_port;
  // We want to get the IP address of the response if it was returned, and the
  // last endpoint that was checked if it failed.
  if (url_request->response_headers()) {
    request_host_port = url_request->GetSocketAddress();
  }
  if (request_host_port.IsEmpty()) {
    net::IPEndPoint request_ip_endpoint;
    bool was_successful = url_request->GetRemoteEndpoint(&request_ip_endpoint);
    if (was_successful) {
      request_host_port =
          net::HostPortPair::FromIPEndPoint(request_ip_endpoint);
    }
  }

  auto load_timing_info = std::make_unique<net::LoadTimingInfo>();
  url_request->GetLoadTimingInfo(load_timing_info.get());

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          &NotifyUIThreadOfRequestComplete,
          info->GetWebContentsGetterForRequest(),
          info->GetFrameTreeNodeIdGetterForRequest(), url_request->url(),
          request_host_port, info->GetGlobalRequestID(), info->GetChildID(),
          info->GetRenderFrameID(), info->GetResourceType(), info->IsDownload(),
          url_request->was_cached(), std::move(data_reduction_proxy_data),
          net_error, url_request->GetTotalReceivedBytes(),
          url_request->GetRawBodyBytes(), original_content_length,
          url_request->creation_time(),
          std::move(load_timing_info)));
}

content::PreviewsState
ChromeResourceDispatcherHostDelegate::DetermineEnabledPreviews(
    net::URLRequest* url_request,
    content::ResourceContext* resource_context,
    content::PreviewsState previews_to_allow) {
  ProfileIOData* io_data = ProfileIOData::FromResourceContext(resource_context);
  data_reduction_proxy::DataReductionProxyIOData* data_reduction_proxy_io_data =
      io_data->data_reduction_proxy_io_data();

  content::PreviewsState previews_state = content::PREVIEWS_UNSPECIFIED;

  previews::PreviewsIOData* previews_io_data = io_data->previews_io_data();
  if (data_reduction_proxy_io_data && previews_io_data) {
    previews::PreviewsUserData::Create(url_request,
                                       previews_io_data->GeneratePageId());
    if (data_reduction_proxy_io_data->ShouldAcceptServerPreview(
            *url_request, previews_io_data)) {
      previews_state |= content::SERVER_LOFI_ON;
      previews_state |= content::SERVER_LITE_PAGE_ON;
    }

    // Check for enabled client-side previews if data saver is enabled.
    if (data_reduction_proxy_io_data->IsEnabled()) {
      previews_state |= previews::DetermineEnabledClientPreviewsState(
          *url_request, previews_io_data);
    }
  }

  if (previews_state == content::PREVIEWS_UNSPECIFIED)
    return content::PREVIEWS_OFF;

  // If the allowed previews are limited, ensure we honor those limits.
  if (previews_to_allow != content::PREVIEWS_UNSPECIFIED &&
      previews_state != content::PREVIEWS_OFF &&
      previews_state != content::PREVIEWS_NO_TRANSFORM) {
    previews_state = previews_state & previews_to_allow;
    // If no valid previews are left, set the state explictly to PREVIEWS_OFF.
    if (previews_state == 0)
      previews_state = content::PREVIEWS_OFF;
  }
  return previews_state;
}

content::NavigationData*
ChromeResourceDispatcherHostDelegate::GetNavigationData(
    net::URLRequest* request) const {
  ChromeNavigationData* data =
      ChromeNavigationData::GetDataAndCreateIfNecessary(request);
  if (!request)
    return data;

  data_reduction_proxy::DataReductionProxyData* data_reduction_proxy_data =
      data_reduction_proxy::DataReductionProxyData::GetData(*request);
  // DeepCopy the DataReductionProxyData from the URLRequest to prevent the
  // URLRequest and DataReductionProxyData from both having ownership of the
  // same object. This copy will be shortlived as it will be deep copied again
  // when content makes a clone of NavigationData for the UI thread.
  if (data_reduction_proxy_data)
    data->SetDataReductionProxyData(data_reduction_proxy_data->DeepCopy());

  previews::PreviewsUserData* previews_user_data =
      previews::PreviewsUserData::GetData(*request);
  if (previews_user_data)
    data->set_previews_user_data(previews_user_data->DeepCopy());

  return data;
}

// static
content::PreviewsState
ChromeResourceDispatcherHostDelegate::DetermineCommittedPreviews(
    const net::URLRequest* request,
    const previews::PreviewsDecider* previews_decider,
    content::PreviewsState initial_state) {
  if (!previews::HasEnabledPreviews(initial_state))
    return content::PREVIEWS_OFF;

  content::PreviewsState previews_state =
      data_reduction_proxy::ContentLoFiDecider::
          DetermineCommittedServerPreviewsState(*request, initial_state);
  return previews::DetermineCommittedClientPreviewsState(
      *request, previews_state, previews_decider);
}
