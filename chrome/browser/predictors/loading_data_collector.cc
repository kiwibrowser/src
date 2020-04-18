// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "chrome/browser/predictors/loading_data_collector.h"
#include "chrome/browser/predictors/loading_stats_collector.h"
#include "chrome/browser/predictors/resource_prefetch_predictor.h"
#include "chrome/browser/predictors/resource_prefetch_predictor_tables.h"
#include "chrome/browser/profiles/profile.h"
#include "components/history/core/browser/history_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/common/resource_type.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "third_party/blink/public/common/mime_util/mime_util.h"

using content::BrowserThread;

namespace predictors {

namespace {

bool g_allow_port_in_urls = true;

// Sorted by decreasing likelihood according to HTTP archive.
const char* const kFontMimeTypes[] = {"font/woff2",
                                      "application/x-font-woff",
                                      "application/font-woff",
                                      "application/font-woff2",
                                      "font/x-woff",
                                      "application/x-font-ttf",
                                      "font/woff",
                                      "font/ttf",
                                      "application/x-font-otf",
                                      "x-font/woff",
                                      "application/font-sfnt",
                                      "application/font-ttf"};

bool IsNoStore(const net::URLRequest& response) {
  if (response.was_cached())
    return false;

  const net::HttpResponseInfo& response_info = response.response_info();
  if (!response_info.headers.get())
    return false;
  return response_info.headers->HasHeaderValue("cache-control", "no-store");
}

}  // namespace

OriginRequestSummary::OriginRequestSummary() = default;
OriginRequestSummary::OriginRequestSummary(const OriginRequestSummary& other) =
    default;
OriginRequestSummary::~OriginRequestSummary() = default;

URLRequestSummary::URLRequestSummary() = default;
URLRequestSummary::URLRequestSummary(const URLRequestSummary& other) = default;
URLRequestSummary::~URLRequestSummary() = default;

// static
bool URLRequestSummary::SummarizeResponse(const net::URLRequest& request,
                                          URLRequestSummary* summary) {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(&request);
  if (!request_info)
    return false;

  summary->request_url = request.url();
  content::ResourceType resource_type_from_request =
      request_info->GetResourceType();
  std::string mime_type;
  request.GetMimeType(&mime_type);
  summary->resource_type = LoadingDataCollector::GetResourceType(
      resource_type_from_request, mime_type);

  scoped_refptr<net::HttpResponseHeaders> headers =
      request.response_info().headers;
  if (headers.get()) {
    // RFC 2616, section 14.9.
    summary->always_revalidate =
        headers->HasHeaderValue("cache-control", "no-cache") ||
        headers->HasHeaderValue("pragma", "no-cache") ||
        headers->HasHeaderValue("vary", "*");
    summary->is_no_store = IsNoStore(request);
  }
  summary->network_accessed = request.response_info().network_accessed;

  return true;
}

PageRequestSummary::PageRequestSummary(const GURL& i_main_frame_url)
    : main_frame_url(i_main_frame_url),
      initial_url(i_main_frame_url),
      first_contentful_paint(base::TimeTicks::Max()) {}

PageRequestSummary::PageRequestSummary(const PageRequestSummary& other) =
    default;

void PageRequestSummary::UpdateOrAddToOrigins(
    const URLRequestSummary& request_summary) {
  GURL origin = request_summary.request_url.GetOrigin();
  DCHECK(origin.is_valid());
  if (!origin.is_valid())
    return;

  auto it = origins.find(origin);
  if (it == origins.end()) {
    OriginRequestSummary summary;
    summary.origin = origin;
    summary.first_occurrence = origins.size();
    it = origins.insert({origin, summary}).first;
  }

  it->second.always_access_network |=
      request_summary.always_revalidate || request_summary.is_no_store;
  it->second.accessed_network |= request_summary.network_accessed;
}

PageRequestSummary::~PageRequestSummary() = default;

// static
content::ResourceType LoadingDataCollector::GetResourceTypeFromMimeType(
    const std::string& mime_type,
    content::ResourceType fallback) {
  if (mime_type.empty()) {
    return fallback;
  } else if (blink::IsSupportedImageMimeType(mime_type)) {
    return content::RESOURCE_TYPE_IMAGE;
  } else if (blink::IsSupportedJavascriptMimeType(mime_type)) {
    return content::RESOURCE_TYPE_SCRIPT;
  } else if (net::MatchesMimeType("text/css", mime_type)) {
    return content::RESOURCE_TYPE_STYLESHEET;
  } else {
    bool found =
        std::any_of(std::begin(kFontMimeTypes), std::end(kFontMimeTypes),
                    [&mime_type](const std::string& mime) {
                      return net::MatchesMimeType(mime, mime_type);
                    });
    if (found)
      return content::RESOURCE_TYPE_FONT_RESOURCE;
  }
  return fallback;
}

// static
content::ResourceType LoadingDataCollector::GetResourceType(
    content::ResourceType resource_type,
    const std::string& mime_type) {
  // Restricts content::RESOURCE_TYPE_{PREFETCH,SUB_RESOURCE,XHR} to a small set
  // of mime types, because these resource types don't communicate how the
  // resources will be used.
  if (resource_type == content::RESOURCE_TYPE_PREFETCH ||
      resource_type == content::RESOURCE_TYPE_SUB_RESOURCE ||
      resource_type == content::RESOURCE_TYPE_XHR) {
    return GetResourceTypeFromMimeType(mime_type,
                                       content::RESOURCE_TYPE_LAST_TYPE);
  }
  return resource_type;
}

// static
bool LoadingDataCollector::ShouldRecordRequest(
    net::URLRequest* request,
    content::ResourceType resource_type) {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(request);
  if (!request_info)
    return false;

  if (!request_info->IsMainFrame())
    return false;

  return resource_type == content::RESOURCE_TYPE_MAIN_FRAME &&
         IsHandledMainPage(request);
}

// static
bool LoadingDataCollector::ShouldRecordResponse(net::URLRequest* response) {
  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(response);
  if (!request_info)
    return false;

  if (!request_info->IsMainFrame())
    return false;

  content::ResourceType resource_type = request_info->GetResourceType();
  return resource_type == content::RESOURCE_TYPE_MAIN_FRAME
             ? IsHandledMainPage(response)
             : IsHandledSubresource(response, resource_type);
}

// static
bool LoadingDataCollector::ShouldRecordRedirect(net::URLRequest* response) {
  return ShouldRecordResponse(response);
}

// static
bool LoadingDataCollector::ShouldRecordResourceFromMemoryCache(
    const GURL& url,
    content::ResourceType resource_type,
    const std::string& mime_type) {
  return IsHandledUrl(url) && IsHandledResourceType(resource_type, mime_type);
}

// static
bool LoadingDataCollector::IsHandledMainPage(net::URLRequest* request) {
  const GURL& url = request->url();
  bool bad_port = !g_allow_port_in_urls && url.has_port();
  return url.SchemeIsHTTPOrHTTPS() && !bad_port;
}

// static
bool LoadingDataCollector::IsHandledSubresource(
    net::URLRequest* response,
    content::ResourceType resource_type) {
  if (!response->site_for_cookies().SchemeIsHTTPOrHTTPS())
    return false;

  std::string mime_type;
  response->GetMimeType(&mime_type);
  if (!IsHandledResourceType(resource_type, mime_type))
    return false;

  if (response->method() != "GET")
    return false;

  if (!IsHandledUrl(response->url()) ||
      !IsHandledUrl(response->original_url())) {
    return false;
  }

  if (!response->response_info().headers.get())
    return false;

  return true;
}

// static
bool LoadingDataCollector::IsHandledResourceType(
    content::ResourceType resource_type,
    const std::string& mime_type) {
  content::ResourceType actual_resource_type =
      GetResourceType(resource_type, mime_type);
  return actual_resource_type == content::RESOURCE_TYPE_STYLESHEET ||
         actual_resource_type == content::RESOURCE_TYPE_SCRIPT ||
         actual_resource_type == content::RESOURCE_TYPE_IMAGE ||
         actual_resource_type == content::RESOURCE_TYPE_FONT_RESOURCE;
}

// static
bool LoadingDataCollector::IsHandledUrl(const GURL& url) {
  bool bad_port = !g_allow_port_in_urls && url.has_port();
  if (!url.is_valid() || !url.SchemeIsHTTPOrHTTPS() || bad_port)
    return false;

  if (url.spec().length() > ResourcePrefetchPredictorTables::kMaxStringLength)
    return false;

  return true;
}

// static
void LoadingDataCollector::SetAllowPortInUrlsForTesting(bool state) {
  g_allow_port_in_urls = state;
}

LoadingDataCollector::LoadingDataCollector(
    ResourcePrefetchPredictor* predictor,
    predictors::LoadingStatsCollector* stats_collector,
    const LoadingPredictorConfig& config)
    : predictor_(predictor),
      stats_collector_(stats_collector),
      config_(config) {}

LoadingDataCollector::~LoadingDataCollector() = default;

void LoadingDataCollector::RecordURLRequest(const URLRequestSummary& request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(request.resource_type, content::RESOURCE_TYPE_MAIN_FRAME);

  CleanupAbandonedNavigations(request.navigation_id);

  // New empty navigation entry.
  const GURL& main_frame_url = request.navigation_id.main_frame_url;
  inflight_navigations_.emplace(
      request.navigation_id,
      std::make_unique<PageRequestSummary>(main_frame_url));
}

void LoadingDataCollector::RecordURLResponse(
    const URLRequestSummary& response) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  NavigationMap::const_iterator nav_it =
      inflight_navigations_.find(response.navigation_id);
  if (nav_it == inflight_navigations_.end())
    return;
  auto& page_request_summary = *nav_it->second;

  if (config_.is_origin_learning_enabled)
    page_request_summary.UpdateOrAddToOrigins(response);
}

void LoadingDataCollector::RecordURLRedirect(
    const URLRequestSummary& response) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (response.resource_type == content::RESOURCE_TYPE_MAIN_FRAME)
    OnMainFrameRedirect(response);
  else
    OnSubresourceRedirect(response);
}

void LoadingDataCollector::RecordMainFrameLoadComplete(
    const NavigationID& navigation_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // WebContents can return an empty URL if the navigation entry corresponding
  // to the navigation has not been created yet.
  if (navigation_id.main_frame_url.is_empty())
    return;

  // Initialize |predictor_| no matter whether the |navigation_id| is present in
  // |inflight_navigations_|. This is the case for NTP and about:blank pages,
  // for example.
  if (predictor_)
    predictor_->StartInitialization();

  NavigationMap::iterator nav_it = inflight_navigations_.find(navigation_id);
  if (nav_it == inflight_navigations_.end())
    return;

  // Remove the navigation from the inflight navigations.
  std::unique_ptr<PageRequestSummary> summary = std::move(nav_it->second);
  inflight_navigations_.erase(nav_it);

  if (stats_collector_)
    stats_collector_->RecordPageRequestSummary(*summary);

  if (predictor_)
    predictor_->RecordPageRequestSummary(std::move(summary));
}

void LoadingDataCollector::RecordFirstContentfulPaint(
    const NavigationID& navigation_id,
    const base::TimeTicks& first_contentful_paint) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  NavigationMap::iterator nav_it = inflight_navigations_.find(navigation_id);
  if (nav_it != inflight_navigations_.end())
    nav_it->second->first_contentful_paint = first_contentful_paint;
}

void LoadingDataCollector::OnMainFrameRedirect(
    const URLRequestSummary& response) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const GURL& main_frame_url = response.navigation_id.main_frame_url;
  std::unique_ptr<PageRequestSummary> summary;
  NavigationMap::iterator nav_it =
      inflight_navigations_.find(response.navigation_id);
  if (nav_it != inflight_navigations_.end()) {
    summary = std::move(nav_it->second);
    inflight_navigations_.erase(nav_it);
  }

  // The redirect url may be empty if the URL was invalid.
  if (response.redirect_url.is_empty())
    return;

  // If we lost the information about the first hop for some reason.
  if (!summary) {
    summary = std::make_unique<PageRequestSummary>(main_frame_url);
  }

  // A redirect will not lead to another OnMainFrameRequest call, so record the
  // redirect url as a new navigation id and save the initial url.
  NavigationID navigation_id(response.navigation_id);
  navigation_id.main_frame_url = response.redirect_url;
  summary->main_frame_url = response.redirect_url;
  inflight_navigations_.emplace(navigation_id, std::move(summary));
}

void LoadingDataCollector::OnSubresourceRedirect(
    const URLRequestSummary& response) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!config_.is_origin_learning_enabled)
    return;

  NavigationMap::const_iterator nav_it =
      inflight_navigations_.find(response.navigation_id);
  if (nav_it == inflight_navigations_.end())
    return;
  auto& page_request_summary = *nav_it->second;
  page_request_summary.UpdateOrAddToOrigins(response);
}

void LoadingDataCollector::CleanupAbandonedNavigations(
    const NavigationID& navigation_id) {
  if (stats_collector_)
    stats_collector_->CleanupAbandonedStats();

  static const base::TimeDelta max_navigation_age =
      base::TimeDelta::FromSeconds(config_.max_navigation_lifetime_seconds);

  base::TimeTicks time_now = base::TimeTicks::Now();
  for (NavigationMap::iterator it = inflight_navigations_.begin();
       it != inflight_navigations_.end();) {
    if ((it->first.tab_id == navigation_id.tab_id) ||
        (time_now - it->first.creation_time > max_navigation_age)) {
      inflight_navigations_.erase(it++);
    } else {
      ++it;
    }
  }
}

}  // namespace predictors
