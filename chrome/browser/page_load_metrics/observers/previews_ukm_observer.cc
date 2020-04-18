// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/previews_ukm_observer.h"

#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/loader/chrome_navigation_data.h"
#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings.h"
#include "chrome/browser/net/spdyproxy/data_reduction_proxy_chrome_settings_factory.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_observer.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "chrome/browser/previews/previews_infobar_delegate.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/page_load_metrics/page_load_timing.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_settings.h"
#include "components/previews/content/previews_content_util.h"
#include "components/ukm/ukm_source.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/previews_state.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace previews {

PreviewsUKMObserver::PreviewsUKMObserver() {}

PreviewsUKMObserver::~PreviewsUKMObserver() {}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
PreviewsUKMObserver::OnCommit(content::NavigationHandle* navigation_handle,
                              ukm::SourceId source_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  save_data_enabled_ = IsDataSaverEnabled(navigation_handle);

  // As documented in content/public/browser/navigation_handle.h, this
  // NavigationData is a clone of the NavigationData instance returned from
  // ResourceDispatcherHostDelegate::GetNavigationData during commit.
  // Because ChromeResourceDispatcherHostDelegate always returns a
  // ChromeNavigationData, it is safe to static_cast here.
  ChromeNavigationData* chrome_navigation_data =
      static_cast<ChromeNavigationData*>(
          navigation_handle->GetNavigationData());
  if (!chrome_navigation_data)
    return STOP_OBSERVING;
  data_reduction_proxy::DataReductionProxyData* data =
      chrome_navigation_data->GetDataReductionProxyData();
  if (data && data->used_data_reduction_proxy() && data->lite_page_received()) {
    lite_page_seen_ = true;
  }
  content::PreviewsState previews_state =
      chrome_navigation_data->previews_state();
  if (previews_state && previews::GetMainFramePreviewsType(previews_state) ==
                            previews::PreviewsType::NOSCRIPT) {
    noscript_seen_ = true;
  }
  previews::PreviewsUserData* previews_user_data =
      chrome_navigation_data->previews_user_data();
  if (previews_user_data &&
      previews_user_data->cache_control_no_transform_directive()) {
    origin_opt_out_occurred_ = true;
  }

  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
PreviewsUKMObserver::OnStart(content::NavigationHandle* navigation_handle,
                             const GURL& currently_committed_url,
                             bool started_in_foreground) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!started_in_foreground)
    return STOP_OBSERVING;
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
PreviewsUKMObserver::FlushMetricsOnAppEnterBackground(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RecordPreviewsTypes(info);
  return STOP_OBSERVING;
}

void PreviewsUKMObserver::OnComplete(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  RecordPreviewsTypes(info);
}

void PreviewsUKMObserver::RecordPreviewsTypes(
    const page_load_metrics::PageLoadExtraInfo& info) {
  // Only record previews types when they are active.
  if (!server_lofi_seen_ && !client_lofi_seen_ && !lite_page_seen_ &&
      !noscript_seen_ && !origin_opt_out_occurred_ && !save_data_enabled_) {
    return;
  }

  ukm::builders::Previews builder(info.source_id);
  if (server_lofi_seen_)
    builder.Setserver_lofi(1);
  if (client_lofi_seen_)
    builder.Setclient_lofi(1);
  if (lite_page_seen_)
    builder.Setlite_page(1);
  if (noscript_seen_)
    builder.Setnoscript(1);
  if (opt_out_occurred_)
    builder.Setopt_out(1);
  if (origin_opt_out_occurred_)
    builder.Setorigin_opt_out(1);
  if (save_data_enabled_)
    builder.Setsave_data_enabled(1);
  builder.Record(ukm::UkmRecorder::Get());
}

void PreviewsUKMObserver::OnLoadedResource(
    const page_load_metrics::ExtraRequestCompleteInfo&
        extra_request_complete_info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (extra_request_complete_info.data_reduction_proxy_data) {
    if (extra_request_complete_info.data_reduction_proxy_data
            ->lofi_received()) {
      server_lofi_seen_ = true;
    }
    if (extra_request_complete_info.data_reduction_proxy_data
            ->client_lofi_requested()) {
      client_lofi_seen_ = true;
    }
  }
}

void PreviewsUKMObserver::OnEventOccurred(const void* const event_key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (event_key == PreviewsInfoBarDelegate::OptOutEventKey())
    opt_out_occurred_ = true;
}

bool PreviewsUKMObserver::IsDataSaverEnabled(
    content::NavigationHandle* navigation_handle) const {
  Profile* profile = Profile::FromBrowserContext(
      navigation_handle->GetWebContents()->GetBrowserContext());

  data_reduction_proxy::DataReductionProxySettings*
      data_reduction_proxy_settings =
          DataReductionProxyChromeSettingsFactory::GetForBrowserContext(
              profile);
  if (!data_reduction_proxy_settings) {
    DCHECK(profile->IsOffTheRecord());
    return false;
  }

  return data_reduction_proxy_settings->IsDataReductionProxyEnabled();
}

}  // namespace previews
