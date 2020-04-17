// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/ads_page_load_metrics_observer.h"

#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "chrome/common/chrome_features.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace {

#define ADS_HISTOGRAM(suffix, hist_macro, ad_type, value)                  \
  switch (ad_type) {                                                       \
    case AdsPageLoadMetricsObserver::AD_TYPE_GOOGLE:                       \
      hist_macro("PageLoad.Clients.Ads.Google." suffix, value);            \
      break;                                                               \
    case AdsPageLoadMetricsObserver::AD_TYPE_SUBRESOURCE_FILTER:           \
      hist_macro("PageLoad.Clients.Ads.SubresourceFilter." suffix, value); \
      break;                                                               \
    case AdsPageLoadMetricsObserver::AD_TYPE_ALL:                          \
      hist_macro("PageLoad.Clients.Ads.All." suffix, value);               \
      break;                                                               \
  }

// Finds the RenderFrameHost for the handle, possibly using the FrameTreeNode
// ID directly if the the handle has not been committed.
// NOTE: Unsafe with respect to security privileges.
content::RenderFrameHost* FindFrameMaybeUnsafe(
    content::NavigationHandle* handle) {
  return handle->HasCommitted()
             ? handle->GetRenderFrameHost()
             : handle->GetWebContents()->UnsafeFindFrameByFrameTreeNodeId(
                   handle->GetFrameTreeNodeId());
}

bool DetectGoogleAd(content::NavigationHandle* navigation_handle) {
  // Because sub-resource filtering isn't always enabled, and doesn't work
  // well in monitoring mode (no CSS enforcement), it's difficult to identify
  // ads. Google ads are prevalent and easy to track, so we'll start by
  // tracking those. Note that the frame name can be very large, so be careful
  // to avoid full string searches if possible.
  // TODO(jkarlin): Track other ad networks that are easy to identify.

  // In case the navigation aborted, look up the RFH by the Frame Tree Node
  // ID. It returns the committed frame host or the initial frame host for the
  // frame if no committed host exists. Using a previous host is fine because
  // once a frame has an ad we always consider it to have an ad.
  // NOTE: Just used for measuring bytes, does not grant security privileges.
  content::RenderFrameHost* current_frame_host =
      FindFrameMaybeUnsafe(navigation_handle);
  if (current_frame_host) {
    const std::string& frame_name = current_frame_host->GetFrameName();
    if (base::StartsWith(frame_name, "google_ads_iframe",
                         base::CompareCase::SENSITIVE) ||
        base::StartsWith(frame_name, "google_ads_frame",
                         base::CompareCase::SENSITIVE)) {
      return true;
    }
  }

  const GURL& url = navigation_handle->GetURL();
  return url.host_piece() == "tpc.googlesyndication.com" &&
         base::StartsWith(url.path_piece(), "/safeframe",
                          base::CompareCase::SENSITIVE);
}

void RecordParentExistsForSubFrame(
    bool parent_exists,
    const AdsPageLoadMetricsObserver::AdTypes& ad_types) {
  ADS_HISTOGRAM("ParentExistsForSubFrame", UMA_HISTOGRAM_BOOLEAN,
                AdsPageLoadMetricsObserver::AD_TYPE_ALL, parent_exists);
}

}  // namespace

AdsPageLoadMetricsObserver::AdFrameData::AdFrameData(
    FrameTreeNodeId frame_tree_node_id,
    AdTypes ad_types,
    AdOriginStatus origin_status,
    bool frame_navigated)
    : frame_bytes(0u),
      frame_bytes_uncached(0u),
      frame_tree_node_id(frame_tree_node_id),
      ad_types(ad_types),
      origin_status(origin_status),
      frame_navigated(frame_navigated) {}

// static
std::unique_ptr<AdsPageLoadMetricsObserver>
AdsPageLoadMetricsObserver::CreateIfNeeded() {
  if (!base::FeatureList::IsEnabled(features::kAdsFeature))
    return nullptr;
  return std::make_unique<AdsPageLoadMetricsObserver>();
}

AdsPageLoadMetricsObserver::AdsPageLoadMetricsObserver()
    : subresource_observer_(this) {}

AdsPageLoadMetricsObserver::~AdsPageLoadMetricsObserver() = default;

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
AdsPageLoadMetricsObserver::OnStart(
    content::NavigationHandle* navigation_handle,
    const GURL& currently_committed_url,
    bool started_in_foreground) {
  auto* observer_manager =
      subresource_filter::SubresourceFilterObserverManager::FromWebContents(
          navigation_handle->GetWebContents());
  // |observer_manager| isn't constructed if the feature for subresource
  // filtering isn't enabled.
  if (observer_manager)
    subresource_observer_.Add(observer_manager);
  return CONTINUE_OBSERVING;
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
AdsPageLoadMetricsObserver::OnCommit(
    content::NavigationHandle* navigation_handle,
    ukm::SourceId source_id) {
  DCHECK(ad_frames_data_.empty());

  committed_ = true;

  // The main frame is never considered an ad.
  ad_frames_data_[navigation_handle->GetFrameTreeNodeId()] = nullptr;
  ProcessOngoingNavigationResource(navigation_handle->GetFrameTreeNodeId());
  return CONTINUE_OBSERVING;
}

// Given an ad being triggered for a frame or navigation, get its AdFrameData
// and record it into the appropriate data structures.
void AdsPageLoadMetricsObserver::RecordAdFrameData(
    FrameTreeNodeId ad_id,
    AdTypes ad_types,
    content::RenderFrameHost* ad_host,
    bool frame_navigated) {
  // If an existing subframe is navigating and it was an ad previously that
  // hasn't navigated yet, then we need to update it.
  const auto& id_and_data = ad_frames_data_.find(ad_id);
  AdFrameData* previous_data = nullptr;
  if (id_and_data != ad_frames_data_.end() && id_and_data->second) {
    DCHECK(frame_navigated);
    if (id_and_data->second->frame_navigated) {
      // We need to update the types with any new types that triggered it.
      id_and_data->second->ad_types |= ad_types;
      ProcessOngoingNavigationResource(ad_id);
      return;
    }
    previous_data = id_and_data->second;
  }

  // Determine who the parent frame's ad ancestor is.  If we don't know who it
  // is, return, such as with a frame from a previous navigation.
  content::RenderFrameHost* parent_frame_host =
      ad_host ? ad_host->GetParent() : nullptr;
  const auto& parent_id_and_data =
      parent_frame_host
          ? ad_frames_data_.find(parent_frame_host->GetFrameTreeNodeId())
          : ad_frames_data_.end();
  bool parent_exists = parent_id_and_data != ad_frames_data_.end();
  RecordParentExistsForSubFrame(parent_exists, ad_types);
  if (!parent_exists)
    return;

  // This frame is not nested within an ad frame but is itself an ad.
  AdFrameData* ad_data = parent_id_and_data->second;
  if (!ad_data && ad_types.any()) {
    AdOriginStatus origin_status = AdOriginStatus::kUnknown;
    if (ad_host) {
      content::RenderFrameHost* main_host =
          content::WebContents::FromRenderFrameHost(ad_host)->GetMainFrame();
      // For ads triggered on render, their origin is their parent's origin.
      if (!frame_navigated)
        ad_host = ad_host->GetParent();
      origin_status = main_host->GetLastCommittedOrigin().IsSameOriginWith(
                          ad_host->GetLastCommittedOrigin())
                          ? AdOriginStatus::kSame
                          : AdOriginStatus::kCross;
    }
    // If data existed already, update it and exit, otherwise, add it.
    if (previous_data) {
      previous_data->ad_types |= ad_types;
      previous_data->origin_status = origin_status;
      previous_data->frame_navigated = frame_navigated;
      return;
    }
    ad_frames_data_storage_.emplace_back(ad_id, ad_types, origin_status,
                                         frame_navigated);
    ad_data = &ad_frames_data_storage_.back();
  }

  // If there was previous data, then we don't want to overwrite this frame.
  if (!previous_data)
    ad_frames_data_[ad_id] = ad_data;
}

// Determine if the frame is part of an existing ad, the root of a new ad, or a
// non-ad frame. Once a frame is labeled as an ad, it is always considered an
// ad, even if it navigates to a non-ad page. This function labels all of a
// page's frames, even those that fail to commit.
void AdsPageLoadMetricsObserver::OnDidFinishSubFrameNavigation(
    content::NavigationHandle* navigation_handle) {
  FrameTreeNodeId frame_tree_node_id = navigation_handle->GetFrameTreeNodeId();
  AdTypes ad_types = DetectAds(navigation_handle);
  // NOTE: Frame look-up only used for determining cross-origin status, not
  // granting security permissions.
  content::RenderFrameHost* ad_host = FindFrameMaybeUnsafe(navigation_handle);

  RecordAdFrameData(frame_tree_node_id, ad_types, ad_host,
                    /*frame_navigated=*/true);
  ProcessOngoingNavigationResource(frame_tree_node_id);
}

page_load_metrics::PageLoadMetricsObserver::ObservePolicy
AdsPageLoadMetricsObserver::FlushMetricsOnAppEnterBackground(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  // The browser may come back, but there is no guarantee. To be safe, record
  // what we have now and ignore future changes to this navigation.
  if (extra_info.did_commit)
    RecordHistograms();

  return STOP_OBSERVING;
}

void AdsPageLoadMetricsObserver::OnLoadedResource(
    const page_load_metrics::ExtraRequestCompleteInfo& extra_request_info) {
  ProcessLoadedResource(extra_request_info);
}

void AdsPageLoadMetricsObserver::OnComplete(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  RecordHistograms();
}

void AdsPageLoadMetricsObserver::OnSubframeNavigationEvaluated(
    content::NavigationHandle* navigation_handle,
    subresource_filter::LoadPolicy load_policy,
    bool is_ad_subframe) {
  // We don't track DISALLOW frames because their resources won't be loaded
  // and therefore would provide bad histogram data. Note that WOULD_DISALLOW
  // is only seen in dry runs.
  if (is_ad_subframe &&
      load_policy != subresource_filter::LoadPolicy::DISALLOW) {
    unfinished_subresource_ad_frames_.insert(
        navigation_handle->GetFrameTreeNodeId());
  }
}

void AdsPageLoadMetricsObserver::OnAdSubframeDetected(
    content::RenderFrameHost* render_frame_host) {
  AdTypes ad_types;
  ad_types.set(AD_TYPE_SUBRESOURCE_FILTER);
  FrameTreeNodeId frame_tree_node_id = render_frame_host->GetFrameTreeNodeId();
  RecordAdFrameData(frame_tree_node_id, ad_types, render_frame_host,
                    /*frame_navigated=*/false);
}

void AdsPageLoadMetricsObserver::OnSubresourceFilterGoingAway() {
  subresource_observer_.RemoveAll();
}

bool AdsPageLoadMetricsObserver::DetectSubresourceFilterAd(
    FrameTreeNodeId frame_tree_node_id) {
  return unfinished_subresource_ad_frames_.erase(frame_tree_node_id);
}

AdsPageLoadMetricsObserver::AdTypes AdsPageLoadMetricsObserver::DetectAds(
    content::NavigationHandle* navigation_handle) {
  AdTypes ad_types;

  if (DetectGoogleAd(navigation_handle))
    ad_types.set(AD_TYPE_GOOGLE);

  if (DetectSubresourceFilterAd(navigation_handle->GetFrameTreeNodeId()))
    ad_types.set(AD_TYPE_SUBRESOURCE_FILTER);

  return ad_types;
}

void AdsPageLoadMetricsObserver::ProcessLoadedResource(
    const page_load_metrics::ExtraRequestCompleteInfo& extra_request_info) {
  const auto& id_and_data =
      ad_frames_data_.find(extra_request_info.frame_tree_node_id);
  if (id_and_data == ad_frames_data_.end()) {
    if (extra_request_info.resource_type == content::RESOURCE_TYPE_MAIN_FRAME ||
        extra_request_info.resource_type == content::RESOURCE_TYPE_SUB_FRAME) {
      // This resource request is the primary resource load for a frame that
      // hasn't yet finished navigating. Hang onto the request info and replay
      // it once the frame finishes navigating.
      ongoing_navigation_resources_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(extra_request_info.frame_tree_node_id),
          std::forward_as_tuple(
              extra_request_info.url, extra_request_info.host_port_pair,
              extra_request_info.frame_tree_node_id,
              extra_request_info.was_cached, extra_request_info.raw_body_bytes,
              extra_request_info.original_network_content_length, nullptr,
              extra_request_info.resource_type, extra_request_info.net_error,
              extra_request_info.load_timing_info
                  ? std::make_unique<net::LoadTimingInfo>(
                        *extra_request_info.load_timing_info)
                  : nullptr));
    } else {
      // This is unexpected, it could be:
      // 1. a resource from a previous navigation that started its resource
      //    load after this page started navigation.
      // 2. possibly a resource from a document.written frame whose frame
      //    failure message has yet to arrive. (uncertain of this)
    }
    if (committed_) {
      UMA_HISTOGRAM_ENUMERATION(
          "PageLoad.Clients.Ads.All.ResourceTypeWhenNoFrameFound",
          extra_request_info.resource_type, content::RESOURCE_TYPE_LAST_TYPE);
    }

    return;
  }

  page_bytes_ += extra_request_info.raw_body_bytes;
  if (!extra_request_info.was_cached)
    uncached_page_bytes_ += extra_request_info.raw_body_bytes;

  // Determine if the frame (or its ancestor) is an ad, if so attribute the
  // bytes to the highest ad ancestor.
  AdFrameData* ancestor_data = id_and_data->second;

  if (ancestor_data) {
    ancestor_data->frame_bytes += extra_request_info.raw_body_bytes;
    if (!extra_request_info.was_cached) {
      ancestor_data->frame_bytes_uncached += extra_request_info.raw_body_bytes;
    }
  }
}

void AdsPageLoadMetricsObserver::RecordHistograms() {
  RecordHistogramsForType(AD_TYPE_GOOGLE);
  RecordHistogramsForType(AD_TYPE_SUBRESOURCE_FILTER);
  RecordHistogramsForType(AD_TYPE_ALL);
}

void AdsPageLoadMetricsObserver::RecordHistogramsForType(int ad_type) {
  if (page_bytes_ == 0)
    return;

  int non_zero_ad_frames = 0;
  size_t total_ad_frame_bytes = 0;
  size_t uncached_ad_frame_bytes = 0;

  for (const AdFrameData& ad_frame_data : ad_frames_data_storage_) {
    if (ad_frame_data.frame_bytes == 0)
      continue;

    // If this isn't the type of ad we're looking for, move on to the next.
    if (ad_type != AD_TYPE_ALL && !ad_frame_data.ad_types.test(ad_type))
      continue;

    non_zero_ad_frames += 1;
    total_ad_frame_bytes += ad_frame_data.frame_bytes;

    uncached_ad_frame_bytes += ad_frame_data.frame_bytes_uncached;
    ADS_HISTOGRAM("Bytes.AdFrames.PerFrame.Total", PAGE_BYTES_HISTOGRAM,
                  ad_type, ad_frame_data.frame_bytes);
    ADS_HISTOGRAM("Bytes.AdFrames.PerFrame.Network", PAGE_BYTES_HISTOGRAM,
                  ad_type, ad_frame_data.frame_bytes_uncached);
    ADS_HISTOGRAM(
        "Bytes.AdFrames.PerFrame.PercentNetwork", UMA_HISTOGRAM_PERCENTAGE,
        ad_type,
        ad_frame_data.frame_bytes_uncached * 100 / ad_frame_data.frame_bytes);
    ADS_HISTOGRAM("FrameCounts.AdFrames.PerFrame.OriginStatus",
                  UMA_HISTOGRAM_ENUMERATION, ad_type,
                  ad_frame_data.origin_status);
  }

  // TODO(ericrobinson): Consider renaming this to match
  //   'FrameCounts.AdFrames.PerFrame.OriginStatus'.
  ADS_HISTOGRAM("FrameCounts.AnyParentFrame.AdFrames",
                UMA_HISTOGRAM_COUNTS_1000, ad_type, non_zero_ad_frames);

  // Don't post UMA for pages that don't have ads.
  if (non_zero_ad_frames == 0)
    return;

  ADS_HISTOGRAM("Bytes.NonAdFrames.Aggregate.Total", PAGE_BYTES_HISTOGRAM,
                ad_type, page_bytes_ - total_ad_frame_bytes);

  ADS_HISTOGRAM("Bytes.FullPage.Total", PAGE_BYTES_HISTOGRAM, ad_type,
                page_bytes_);
  ADS_HISTOGRAM("Bytes.FullPage.Network", PAGE_BYTES_HISTOGRAM, ad_type,
                uncached_page_bytes_);

  if (page_bytes_) {
    ADS_HISTOGRAM("Bytes.FullPage.Total.PercentAds", UMA_HISTOGRAM_PERCENTAGE,
                  ad_type, total_ad_frame_bytes * 100 / page_bytes_);
  }
  if (uncached_page_bytes_ > 0) {
    ADS_HISTOGRAM("Bytes.FullPage.Network.PercentAds", UMA_HISTOGRAM_PERCENTAGE,
                  ad_type,
                  uncached_ad_frame_bytes * 100 / uncached_page_bytes_);
  }

  ADS_HISTOGRAM("Bytes.AdFrames.Aggregate.Total", PAGE_BYTES_HISTOGRAM, ad_type,
                total_ad_frame_bytes);
  ADS_HISTOGRAM("Bytes.AdFrames.Aggregate.Network", PAGE_BYTES_HISTOGRAM,
                ad_type, uncached_ad_frame_bytes);

  if (total_ad_frame_bytes) {
    ADS_HISTOGRAM("Bytes.AdFrames.Aggregate.PercentNetwork",
                  UMA_HISTOGRAM_PERCENTAGE, ad_type,
                  uncached_ad_frame_bytes * 100 / total_ad_frame_bytes);
  }
}

void AdsPageLoadMetricsObserver::ProcessOngoingNavigationResource(
    FrameTreeNodeId frame_tree_node_id) {
  const auto& frame_id_and_request =
      ongoing_navigation_resources_.find(frame_tree_node_id);
  if (frame_id_and_request == ongoing_navigation_resources_.end())
    return;

  ProcessLoadedResource(frame_id_and_request->second);
  ongoing_navigation_resources_.erase(frame_id_and_request);
}
