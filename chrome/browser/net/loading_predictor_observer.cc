// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/loading_predictor_observer.h"

#include <memory>
#include <string>
#include <utility>

#include "base/metrics/histogram_macros.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

using content::BrowserThread;
using predictors::LoadingPredictor;
using predictors::LoadingDataCollector;
using predictors::URLRequestSummary;

namespace {

// Enum for measuring statistics pertaining to observed request, responses and
// redirects.
enum RequestStats {
  REQUEST_STATS_TOTAL_RESPONSES = 0,
  REQUEST_STATS_TOTAL_PROCESSED_RESPONSES = 1,
  REQUEST_STATS_NO_RESOURCE_REQUEST_INFO = 2,  // Not recorded (never was).
  REQUEST_STATS_NO_RENDER_FRAME_ID_FROM_REQUEST_INFO = 3,  // Not recorded.
  REQUEST_STATS_MAX = 4,
};

// Specific to main frame requests.
enum MainFrameRequestStats {
  MAIN_FRAME_REQUEST_STATS_TOTAL_REQUESTS = 0,
  MAIN_FRAME_REQUEST_STATS_PROCESSED_REQUESTS = 1,
  MAIN_FRAME_REQUEST_STATS_TOTAL_REDIRECTS = 2,
  MAIN_FRAME_REQUEST_STATS_PROCESSED_REDIRECTS = 3,
  MAIN_FRAME_REQUEST_STATS_TOTAL_RESPONSES = 4,
  MAIN_FRAME_REQUEST_STATS_PROCESSED_RESPONSES = 5,
  MAIN_FRAME_REQUEST_STATS_MAX = 6,
};

void ReportRequestStats(RequestStats stat) {
  UMA_HISTOGRAM_ENUMERATION("ResourcePrefetchPredictor.RequestStats", stat,
                            REQUEST_STATS_MAX);
}

void ReportMainFrameRequestStats(MainFrameRequestStats stat) {
  UMA_HISTOGRAM_ENUMERATION("ResourcePrefetchPredictor.MainFrameRequestStats",
                            stat, MAIN_FRAME_REQUEST_STATS_MAX);
}

bool TryToFillNavigationID(
    predictors::NavigationID* navigation_id,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    const GURL& main_frame_url,
    const base::TimeTicks& creation_time) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return false;
  *navigation_id =
      predictors::NavigationID(web_contents, main_frame_url, creation_time);
  // A WebContents might be associated with something that is not a tab.
  // In this case tab_id will be -1 and is_valid() will return false.
  return navigation_id->is_valid();
}

}  // namespace

namespace chrome_browser_net {

LoadingPredictorObserver::LoadingPredictorObserver(LoadingPredictor* predictor)
    : predictor_(predictor->GetWeakPtr()) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

LoadingPredictorObserver::~LoadingPredictorObserver() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI) ||
        BrowserThread::CurrentlyOn(BrowserThread::IO));
}

void LoadingPredictorObserver::OnRequestStarted(
    net::URLRequest* request,
    content::ResourceType resource_type,
    const content::ResourceRequestInfo::WebContentsGetter&
        web_contents_getter) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (resource_type == content::RESOURCE_TYPE_MAIN_FRAME)
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_TOTAL_REQUESTS);

  if (!LoadingDataCollector::ShouldRecordRequest(request, resource_type))
    return;

  auto summary = std::make_unique<URLRequestSummary>();
  summary->resource_type = resource_type;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&LoadingPredictorObserver::OnRequestStartedOnUIThread,
                     base::Unretained(this), std::move(summary),
                     web_contents_getter, request->site_for_cookies(),
                     request->creation_time()));

  if (resource_type == content::RESOURCE_TYPE_MAIN_FRAME)
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_PROCESSED_REQUESTS);
}

void LoadingPredictorObserver::OnRequestRedirected(
    net::URLRequest* request,
    const GURL& redirect_url,
    const content::ResourceRequestInfo::WebContentsGetter&
        web_contents_getter) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(request);
  if (request_info &&
      request_info->GetResourceType() == content::RESOURCE_TYPE_MAIN_FRAME) {
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_TOTAL_REDIRECTS);
  }

  if (!LoadingDataCollector::ShouldRecordRedirect(request))
    return;

  auto summary = std::make_unique<URLRequestSummary>();
  if (!URLRequestSummary::SummarizeResponse(*request, summary.get())) {
    return;
  }
  summary->redirect_url = redirect_url;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&LoadingPredictorObserver::OnRequestRedirectedOnUIThread,
                     base::Unretained(this), std::move(summary),
                     web_contents_getter, request->site_for_cookies(),
                     request->creation_time()));

  if (request_info &&
      request_info->GetResourceType() == content::RESOURCE_TYPE_MAIN_FRAME) {
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_PROCESSED_REDIRECTS);
  }
}

void LoadingPredictorObserver::OnResponseStarted(
    net::URLRequest* request,
    const content::ResourceRequestInfo::WebContentsGetter&
        web_contents_getter) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ReportRequestStats(REQUEST_STATS_TOTAL_RESPONSES);

  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(request);
  if (request_info &&
      request_info->GetResourceType() == content::RESOURCE_TYPE_MAIN_FRAME) {
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_TOTAL_RESPONSES);
  }

  if (!LoadingDataCollector::ShouldRecordResponse(request))
    return;
  auto summary = std::make_unique<URLRequestSummary>();
  if (!URLRequestSummary::SummarizeResponse(*request, summary.get())) {
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&LoadingPredictorObserver::OnResponseStartedOnUIThread,
                     base::Unretained(this), std::move(summary),
                     web_contents_getter, request->site_for_cookies(),
                     request->creation_time()));

  ReportRequestStats(REQUEST_STATS_TOTAL_PROCESSED_RESPONSES);
  if (request_info &&
      request_info->GetResourceType() == content::RESOURCE_TYPE_MAIN_FRAME) {
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_PROCESSED_RESPONSES);
  }
}

void LoadingPredictorObserver::OnRequestStartedOnUIThread(
    std::unique_ptr<URLRequestSummary> summary,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    const GURL& main_frame_url,
    const base::TimeTicks& creation_time) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!TryToFillNavigationID(&summary->navigation_id, web_contents_getter,
                             main_frame_url, creation_time)) {
    return;
  }
  if (summary->resource_type == content::RESOURCE_TYPE_MAIN_FRAME)
    predictor_->OnMainFrameRequest(*summary);
  predictor_->loading_data_collector()->RecordURLRequest(*summary);
}

void LoadingPredictorObserver::OnRequestRedirectedOnUIThread(
    std::unique_ptr<URLRequestSummary> summary,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    const GURL& main_frame_url,
    const base::TimeTicks& creation_time) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!TryToFillNavigationID(&summary->navigation_id, web_contents_getter,
                             main_frame_url, creation_time)) {
    return;
  }
  if (summary->resource_type == content::RESOURCE_TYPE_MAIN_FRAME)
    predictor_->OnMainFrameRedirect(*summary);
  predictor_->loading_data_collector()->RecordURLRedirect(*summary);
}

void LoadingPredictorObserver::OnResponseStartedOnUIThread(
    std::unique_ptr<URLRequestSummary> summary,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    const GURL& main_frame_url,
    const base::TimeTicks& creation_time) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!TryToFillNavigationID(&summary->navigation_id, web_contents_getter,
                             main_frame_url, creation_time)) {
    return;
  }
  if (summary->resource_type == content::RESOURCE_TYPE_MAIN_FRAME)
    predictor_->OnMainFrameResponse(*summary);
  predictor_->loading_data_collector()->RecordURLResponse(*summary);
}

}  // namespace chrome_browser_net
