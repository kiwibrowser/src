// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/page_capping_page_load_metrics_observer.h"

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/optional.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/data_use_measurement/page_load_capping/chrome_page_load_capping_features.h"
#include "chrome/browser/data_use_measurement/page_load_capping/page_load_capping_infobar_delegate.h"
#include "content/public/browser/navigation_handle.h"

namespace {

const char kMediaPageCap[] = "MediaPageCapMB";
const char kPageCap[] = "PageCapMB";

// The page load capping bytes threshold for the page. There are seperate
// thresholds for media and non-media pages. Returns empty optional if the page
// should not be capped.
base::Optional<int64_t> GetPageLoadCappingBytesThreshold(bool media_page_load) {
  if (!base::FeatureList::IsEnabled(data_use_measurement::page_load_capping::
                                        features::kDetectingHeavyPages)) {
    return base::nullopt;
  }
  // Defaults are 15 MB for media and 5 MB for non-media.
  int64_t default_cap_mb = media_page_load ? 15 : 5;
  return base::GetFieldTrialParamByFeatureAsInt(
             data_use_measurement::page_load_capping::features::
                 kDetectingHeavyPages,
             (media_page_load ? kMediaPageCap : kPageCap), default_cap_mb) *
         1024 * 1024;
}

}  // namespace

PageCappingPageLoadMetricsObserver::PageCappingPageLoadMetricsObserver() =
    default;
PageCappingPageLoadMetricsObserver::~PageCappingPageLoadMetricsObserver() =
    default;

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
PageCappingPageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  web_contents_ = navigation_handle->GetWebContents();
  page_cap_ = GetPageLoadCappingBytesThreshold(false /* media_page_load */);
  // TODO(ryansturm) Check a blacklist of eligible pages.
  // https://crbug.com/797981
  return page_load_metrics::PageLoadMetricsObserver::CONTINUE_OBSERVING;
}

void PageCappingPageLoadMetricsObserver::OnLoadedResource(
    const page_load_metrics::ExtraRequestCompleteInfo&
        extra_request_complete_info) {
  if (extra_request_complete_info.was_cached)
    return;
  network_bytes_ += extra_request_complete_info.raw_body_bytes;
  MaybeCreate();
}

void PageCappingPageLoadMetricsObserver::MaybeCreate() {
  // If the infobar has already been shown for the page, don't show an infobar.
  if (displayed_infobar_)
    return;

  // If the page has not committed, don't show an infobar.
  if (!web_contents_)
    return;

  // If there is no capping threshold, the threshold or the threshold is not
  // met, do not show an infobar.
  if (!page_cap_ || network_bytes_ < page_cap_.value())
    return;

  displayed_infobar_ =
      PageLoadCappingInfoBarDelegate::Create(page_cap_.value(), web_contents_);
}

void PageCappingPageLoadMetricsObserver::MediaStartedPlaying(
    const content::WebContentsObserver::MediaPlayerInfo& video_type,
    bool is_in_main_frame) {
  page_cap_ = GetPageLoadCappingBytesThreshold(true /* media_page_load */);
}
