// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/amp_page_load_metrics_observer.h"

#include <string>

#include "base/optional.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "chrome/common/page_load_metrics/page_load_timing.h"
#include "components/google/core/browser/google_util.h"
#include "content/public/browser/navigation_handle.h"
#include "url/gurl.h"

namespace {

using AMPViewType = AMPPageLoadMetricsObserver::AMPViewType;

const char kHistogramPrefix[] = "PageLoad.Clients.AMP.";

const char kHistogramAMPDOMContentLoadedEventFired[] =
    "DocumentTiming.NavigationToDOMContentLoadedEventFired";
const char kHistogramAMPFirstLayout[] =
    "DocumentTiming.NavigationToFirstLayout";
const char kHistogramAMPLoadEventFired[] =
    "DocumentTiming.NavigationToLoadEventFired";
const char kHistogramAMPFirstContentfulPaint[] =
    "PaintTiming.NavigationToFirstContentfulPaint";
const char kHistogramAMPParseStart[] = "ParseTiming.NavigationToParseStart";
const char kHistogramAMPParseStartRedirect[] =
    "ParseTiming.NavigationToParseStart.RedirectToNonAmpPage";

// Host pattern for AMP Cache URLs.
// See https://developers.google.com/amp/cache/overview#amp-cache-url-format
// for a definition of the format of AMP Cache URLs.
const char kAmpCacheHostSuffix[] = "cdn.ampproject.org";

#define RECORD_HISTOGRAM_FOR_TYPE(name, amp_view_type, value)                 \
  do {                                                                        \
    PAGE_LOAD_HISTOGRAM(std::string(kHistogramPrefix).append(name), value);   \
    switch (amp_view_type) {                                                  \
      case AMPViewType::AMP_CACHE:                                            \
        PAGE_LOAD_HISTOGRAM(                                                  \
            std::string(kHistogramPrefix).append("AmpCache.").append(name),   \
            value);                                                           \
        break;                                                                \
      case AMPViewType::GOOGLE_SEARCH_AMP_VIEWER:                             \
        PAGE_LOAD_HISTOGRAM(std::string(kHistogramPrefix)                     \
                                .append("GoogleSearch.")                      \
                                .append(name),                                \
                            value);                                           \
        break;                                                                \
      case AMPViewType::GOOGLE_NEWS_AMP_VIEWER:                               \
        PAGE_LOAD_HISTOGRAM(                                                  \
            std::string(kHistogramPrefix).append("GoogleNews.").append(name), \
            value);                                                           \
        break;                                                                \
      case AMPViewType::NONE:                                                 \
        NOTREACHED();                                                         \
        break;                                                                \
      case AMPViewType::AMP_VIEW_TYPE_LAST:                                   \
        break;                                                                \
    }                                                                         \
  } while (false)

GURL GetCanonicalizedSameDocumentUrl(const GURL& url) {
  if (!url.has_ref())
    return url;

  // We're only interested in same document navigations where the full URL
  // changes, so we ignore the 'ref' or '#fragment' portion of the URL.
  GURL::Replacements replacements;
  replacements.ClearRef();
  return url.ReplaceComponents(replacements);
}

}  // namespace

AMPPageLoadMetricsObserver::AMPPageLoadMetricsObserver() {}

AMPPageLoadMetricsObserver::~AMPPageLoadMetricsObserver() {}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
AMPPageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  current_url_ = navigation_handle->GetURL();
  view_type_ = GetAMPViewType(current_url_);
  return CONTINUE_OBSERVING;
}

void AMPPageLoadMetricsObserver::OnCommitSameDocumentNavigation(
    content::NavigationHandle* navigation_handle) {
  const GURL url = GetCanonicalizedSameDocumentUrl(navigation_handle->GetURL());

  // Ignore same document navigations where the URL doesn't change.
  if (url == current_url_)
    return;
  current_url_ = url;

  AMPViewType same_document_view_type = GetAMPViewType(url);
  if (same_document_view_type == AMPViewType::NONE)
    return;

  // Though we're not currently able to track page load metrics such as FCP for
  // same-document navigations, we can count how often they happen, to better
  // understand the relative frequency of same-document vs new-document AMP
  // navigations.
  UMA_HISTOGRAM_ENUMERATION(
      std::string(kHistogramPrefix).append("SameDocumentView"),
      same_document_view_type, AMPViewType::AMP_VIEW_TYPE_LAST);
}

void AMPPageLoadMetricsObserver::OnDomContentLoadedEventStart(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (view_type_ == AMPViewType::NONE)
    return;

  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.document_timing->dom_content_loaded_event_start, info)) {
    return;
  }
  RECORD_HISTOGRAM_FOR_TYPE(
      kHistogramAMPDOMContentLoadedEventFired, view_type_,
      timing.document_timing->dom_content_loaded_event_start.value());
}

void AMPPageLoadMetricsObserver::OnLoadEventStart(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (view_type_ == AMPViewType::NONE)
    return;

  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.document_timing->load_event_start, info)) {
    return;
  }
  RECORD_HISTOGRAM_FOR_TYPE(kHistogramAMPLoadEventFired, view_type_,
                            timing.document_timing->load_event_start.value());
}

void AMPPageLoadMetricsObserver::OnFirstLayout(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (view_type_ == AMPViewType::NONE)
    return;

  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.document_timing->first_layout, info)) {
    return;
  }
  RECORD_HISTOGRAM_FOR_TYPE(kHistogramAMPFirstLayout, view_type_,
                            timing.document_timing->first_layout.value());
}

void AMPPageLoadMetricsObserver::OnFirstContentfulPaintInPage(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (view_type_ == AMPViewType::NONE)
    return;

  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.paint_timing->first_contentful_paint, info)) {
    return;
  }
  RECORD_HISTOGRAM_FOR_TYPE(
      kHistogramAMPFirstContentfulPaint, view_type_,
      timing.paint_timing->first_contentful_paint.value());
}

void AMPPageLoadMetricsObserver::OnParseStart(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  if (!WasStartedInForegroundOptionalEventInForeground(
          timing.parse_timing->parse_start, info)) {
    return;
  }

  if (view_type_ == AMPViewType::NONE) {
    // If we ended up on a non-AMP document, but the initial URL matched an AMP
    // document, record the time it took to get to this point. We encounter this
    // case in the Google News AMP viewer, for example: when a user loads a news
    // AMP URL in a non-same-document navigation context, the user is presented
    // with a redirect prompt which they must click through to continue to the
    // canoncial document on the non-AMP origin.
    AMPViewType initial_view_type = GetAMPViewType(info.start_url);
    if (initial_view_type != AMPViewType::NONE) {
      RECORD_HISTOGRAM_FOR_TYPE(kHistogramAMPParseStartRedirect,
                                initial_view_type,
                                timing.parse_timing->parse_start.value());
    }
    return;
  }

  RECORD_HISTOGRAM_FOR_TYPE(kHistogramAMPParseStart, view_type_,
                            timing.parse_timing->parse_start.value());
}

// static
AMPPageLoadMetricsObserver::AMPViewType
AMPPageLoadMetricsObserver::GetAMPViewType(const GURL& url) {
  const char kAmpViewerUrlPrefix[] = "/amp/";

  if (base::EndsWith(url.host(), kAmpCacheHostSuffix,
                     base::CompareCase::INSENSITIVE_ASCII)) {
    return AMPViewType::AMP_CACHE;
  }

  base::Optional<std::string> google_hostname_prefix =
      page_load_metrics::GetGoogleHostnamePrefix(url);
  if (!google_hostname_prefix.has_value())
    return AMPViewType::NONE;

  if (google_hostname_prefix.value() == "www" &&
      base::StartsWith(url.path_piece(), kAmpViewerUrlPrefix,
                       base::CompareCase::SENSITIVE) &&
      url.path_piece().length() > strlen(kAmpViewerUrlPrefix)) {
    return AMPViewType::GOOGLE_SEARCH_AMP_VIEWER;
  }

  if (google_hostname_prefix.value() == "news" &&
      url.path_piece() == "/news/amp" && !url.query_piece().empty()) {
    return AMPViewType::GOOGLE_NEWS_AMP_VIEWER;
  }
  return AMPViewType::NONE;
}
