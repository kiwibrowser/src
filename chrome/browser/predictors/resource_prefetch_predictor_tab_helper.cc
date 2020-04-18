// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/resource_prefetch_predictor_tab_helper.h"

#include <string>

#include "chrome/browser/predictors/loading_predictor.h"
#include "chrome/browser/predictors/loading_predictor_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    predictors::ResourcePrefetchPredictorTabHelper);

using content::BrowserThread;

namespace predictors {

ResourcePrefetchPredictorTabHelper::ResourcePrefetchPredictorTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
}

ResourcePrefetchPredictorTabHelper::~ResourcePrefetchPredictorTabHelper() {
}

void ResourcePrefetchPredictorTabHelper::DocumentOnLoadCompletedInMainFrame() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto* loading_predictor = LoadingPredictorFactory::GetForProfile(
      Profile::FromBrowserContext(web_contents()->GetBrowserContext()));
  if (!loading_predictor)
    return;

  auto* collector = loading_predictor->loading_data_collector();
  NavigationID navigation_id(web_contents());
  collector->RecordMainFrameLoadComplete(navigation_id);
}

void ResourcePrefetchPredictorTabHelper::DidLoadResourceFromMemoryCache(
    const GURL& url,
    const std::string& mime_type,
    content::ResourceType resource_type) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto* loading_predictor = LoadingPredictorFactory::GetForProfile(
      Profile::FromBrowserContext(web_contents()->GetBrowserContext()));
  if (!loading_predictor)
    return;

  if (!LoadingDataCollector::ShouldRecordResourceFromMemoryCache(
          url, resource_type, mime_type)) {
    return;
  }

  URLRequestSummary summary;
  summary.navigation_id = NavigationID(web_contents());
  summary.request_url = url;
  summary.resource_type =
      LoadingDataCollector::GetResourceType(resource_type, mime_type);
  auto* collector = loading_predictor->loading_data_collector();
  collector->RecordURLResponse(summary);
}

}  // namespace predictors
