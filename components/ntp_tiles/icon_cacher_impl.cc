// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_tiles/icon_cacher_impl.h"

#include <utility>

#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_macros.h"
#include "components/favicon/core/favicon_server_fetcher_params.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon/core/favicon_util.h"
#include "components/favicon/core/large_icon_service.h"
#include "components/favicon_base/fallback_icon_style.h"
#include "components/favicon_base/favicon_types.h"
#include "components/favicon_base/favicon_util.h"
#include "components/image_fetcher/core/image_decoder.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "components/ntp_tiles/constants.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

namespace ntp_tiles {

namespace {

constexpr int kDesiredFrameSize = 128;

// TODO(jkrcal): Make the size in dip and the scale factor be passed as
// arguments from the UI so that we desire for the right size on a given device.
// See crbug.com/696563.
constexpr int kDefaultTileIconMinSizePx = 1;
constexpr int kDefaultTileIconDesiredSizePx = 96;

constexpr char kTileIconMinSizePxFieldParam[] = "min_size";
constexpr char kTileIconDesiredSizePxFieldParam[] = "desired_size";

favicon_base::IconType IconType(const PopularSites::Site& site) {
  return site.large_icon_url.is_valid() ? favicon_base::IconType::kTouchIcon
                                        : favicon_base::IconType::kFavicon;
}

const GURL& IconURL(const PopularSites::Site& site) {
  return site.large_icon_url.is_valid() ? site.large_icon_url
                                        : site.favicon_url;
}

bool HasResultDefaultBackgroundColor(
    const favicon_base::LargeIconResult& result) {
  if (!result.fallback_icon_style) {
    return false;
  }
  return result.fallback_icon_style->is_default_background_color;
}

int GetMinimumFetchingSizeForChromeSuggestionsFaviconsFromServer() {
  return base::GetFieldTrialParamByFeatureAsInt(
      kNtpMostLikelyFaviconsFromServerFeature, kTileIconMinSizePxFieldParam,
      kDefaultTileIconMinSizePx);
}

int GetDesiredFetchingSizeForChromeSuggestionsFaviconsFromServer() {
  return base::GetFieldTrialParamByFeatureAsInt(
      kNtpMostLikelyFaviconsFromServerFeature, kTileIconDesiredSizePxFieldParam,
      kDefaultTileIconDesiredSizePx);
}

}  // namespace

IconCacherImpl::IconCacherImpl(
    favicon::FaviconService* favicon_service,
    favicon::LargeIconService* large_icon_service,
    std::unique_ptr<image_fetcher::ImageFetcher> image_fetcher)
    : favicon_service_(favicon_service),
      large_icon_service_(large_icon_service),
      image_fetcher_(std::move(image_fetcher)),
      weak_ptr_factory_(this) {
  image_fetcher_->SetDataUseServiceName(
      data_use_measurement::DataUseUserData::NTP_TILES);
  // For images with multiple frames, prefer one of size 128x128px.
  image_fetcher_->SetDesiredImageFrameSize(
      gfx::Size(kDesiredFrameSize, kDesiredFrameSize));
}

IconCacherImpl::~IconCacherImpl() = default;

void IconCacherImpl::StartFetchPopularSites(
    PopularSites::Site site,
    const base::Closure& icon_available,
    const base::Closure& preliminary_icon_available) {
  // Copy values from |site| before it is moved.
  GURL site_url = site.url;
  if (!StartRequest(site_url, icon_available)) {
    return;
  }

  favicon_base::IconType icon_type = IconType(site);
  favicon::GetFaviconImageForPageURL(
      favicon_service_, site_url, icon_type,
      base::Bind(&IconCacherImpl::OnGetFaviconImageForPageURLFinished,
                 base::Unretained(this), std::move(site),
                 preliminary_icon_available),
      &tracker_);
}

void IconCacherImpl::OnGetFaviconImageForPageURLFinished(
    PopularSites::Site site,
    const base::Closure& preliminary_icon_available,
    const favicon_base::FaviconImageResult& result) {
  if (!result.image.IsEmpty()) {
    FinishRequestAndNotifyIconAvailable(site.url, /*newly_available=*/false);
    return;
  }

  std::unique_ptr<CancelableImageCallback> preliminary_callback =
      MaybeProvideDefaultIcon(site, preliminary_icon_available);

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("icon_cacher", R"(
        semantics {
          sender: "Popular Sites New Tab Fetch"
          description:
            "Chrome may display a list of regionally-popular web sites on the "
            "New Tab Page. This service fetches icons from those sites."
          trigger:
            "Whenever a popular site would be displayed, but its icon is not "
            "yet cached in the browser."
          data: "The URL for which to retrieve an icon."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting: "This feature cannot be disabled in settings."
          policy_exception_justification: "Not implemented."
        })");
  image_fetcher_->FetchImage(
      std::string(), IconURL(site),
      base::Bind(&IconCacherImpl::OnPopularSitesFaviconDownloaded,
                 base::Unretained(this), site,
                 base::Passed(std::move(preliminary_callback))),
      traffic_annotation);
}

void IconCacherImpl::OnPopularSitesFaviconDownloaded(
    PopularSites::Site site,
    std::unique_ptr<CancelableImageCallback> preliminary_callback,
    const std::string& id,
    const gfx::Image& fetched_image,
    const image_fetcher::RequestMetadata& metadata) {
  if (fetched_image.IsEmpty()) {
    FinishRequestAndNotifyIconAvailable(site.url, /*newly_available=*/false);
    UMA_HISTOGRAM_BOOLEAN("NewTabPage.TileFaviconFetchSuccess.Popular", false);
    return;
  }

  // Avoid invoking callback about preliminary icon to be triggered. The best
  // possible icon has already been downloaded.
  if (preliminary_callback) {
    preliminary_callback->Cancel();
  }
  SaveIconForSite(site, fetched_image);
  FinishRequestAndNotifyIconAvailable(site.url, /*newly_available=*/true);
  UMA_HISTOGRAM_BOOLEAN("NewTabPage.TileFaviconFetchSuccess.Popular", true);
}

void IconCacherImpl::SaveAndNotifyDefaultIconForSite(
    const PopularSites::Site& site,
    const base::Closure& preliminary_icon_available,
    const gfx::Image& image) {
  SaveIconForSite(site, image);
  if (preliminary_icon_available) {
    preliminary_icon_available.Run();
  }
}

void IconCacherImpl::SaveIconForSite(const PopularSites::Site& site,
                                     const gfx::Image& image) {
  // Although |SetFaviconColorSpace| affects OSX only, copies of gfx::Images are
  // just copies of the reference to the image and therefore cheap.
  gfx::Image img(image);
  favicon_base::SetFaviconColorSpace(&img);

  favicon_service_->SetFavicons({site.url}, IconURL(site), IconType(site),
                                std::move(img));
}

std::unique_ptr<IconCacherImpl::CancelableImageCallback>
IconCacherImpl::MaybeProvideDefaultIcon(
    const PopularSites::Site& site,
    const base::Closure& preliminary_icon_available) {
  if (site.default_icon_resource < 0) {
    return std::unique_ptr<CancelableImageCallback>();
  }
  std::unique_ptr<CancelableImageCallback> preliminary_callback(
      new CancelableImageCallback(base::Bind(
          &IconCacherImpl::SaveAndNotifyDefaultIconForSite,
          weak_ptr_factory_.GetWeakPtr(), site, preliminary_icon_available)));
  image_fetcher_->GetImageDecoder()->DecodeImage(
      ui::ResourceBundle::GetSharedInstance()
          .GetRawDataResource(site.default_icon_resource)
          .as_string(),
      gfx::Size(kDesiredFrameSize, kDesiredFrameSize),
      preliminary_callback->callback());
  return preliminary_callback;
}

void IconCacherImpl::StartFetchMostLikely(const GURL& page_url,
                                          const base::Closure& icon_available) {
  if (!StartRequest(page_url, icon_available)) {
    return;
  }

  // Desired size 0 means that we do not want the service to resize the image
  // (as we will not use it anyway).
  large_icon_service_->GetLargeIconOrFallbackStyle(
      page_url, GetMinimumFetchingSizeForChromeSuggestionsFaviconsFromServer(),
      /*desired_size_in_pixel=*/0,
      base::Bind(&IconCacherImpl::OnGetLargeIconOrFallbackStyleFinished,
                 weak_ptr_factory_.GetWeakPtr(), page_url),
      &tracker_);
}

void IconCacherImpl::OnGetLargeIconOrFallbackStyleFinished(
    const GURL& page_url,
    const favicon_base::LargeIconResult& result) {
  if (!HasResultDefaultBackgroundColor(result)) {
    // There is already an icon, there is nothing to do. (We should only fetch
    // for default "gray" tiles so that we never overwrite any favicon of any
    // size.)
    FinishRequestAndNotifyIconAvailable(page_url, /*newly_available=*/false);
    // Update the time when the icon was last requested - postpone thus the
    // automatic eviction of the favicon from the favicon database.
    large_icon_service_->TouchIconFromGoogleServer(result.bitmap.icon_url);
    return;
  }

  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("icon_catcher_get_large_icon", R"(
        semantics {
          sender: "Favicon Component"
          description:
            "Sends a request to a Google server to retrieve the favicon bitmap "
            "for a server-suggested most visited tile on the new tab page."
          trigger:
            "A request can be sent if Chrome does not have a favicon for a "
            "particular page and history sync is enabled."
          data: "Page URL and desired icon size."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting:
            "Users can disable this feature via 'History' setting under "
            "'Advanced sync settings'."
          chrome_policy {
            SyncDisabled {
              policy_options {mode: MANDATORY}
              SyncDisabled: true
            }
          }
        })");
  large_icon_service_
      ->GetLargeIconOrFallbackStyleFromGoogleServerSkippingLocalCache(
          favicon::FaviconServerFetcherParams::CreateForMobile(
              page_url,
              GetMinimumFetchingSizeForChromeSuggestionsFaviconsFromServer(),
              GetDesiredFetchingSizeForChromeSuggestionsFaviconsFromServer()),
          /*may_page_url_be_private=*/true, traffic_annotation,
          base::Bind(&IconCacherImpl::OnMostLikelyFaviconDownloaded,
                     weak_ptr_factory_.GetWeakPtr(), page_url));
}

void IconCacherImpl::OnMostLikelyFaviconDownloaded(
    const GURL& request_url,
    favicon_base::GoogleFaviconServerRequestStatus status) {
  UMA_HISTOGRAM_ENUMERATION(
      "NewTabPage.TileFaviconFetchStatus.Server", status,
      favicon_base::GoogleFaviconServerRequestStatus::COUNT);
  FinishRequestAndNotifyIconAvailable(
      request_url,
      status == favicon_base::GoogleFaviconServerRequestStatus::SUCCESS);
}

bool IconCacherImpl::StartRequest(const GURL& request_url,
                                  const base::Closure& icon_available) {
  bool in_flight = in_flight_requests_.count(request_url) > 0;
  in_flight_requests_[request_url].push_back(icon_available);
  return !in_flight;
}

void IconCacherImpl::FinishRequestAndNotifyIconAvailable(
    const GURL& request_url,
    bool newly_available) {
  std::vector<base::Closure> callbacks =
      std::move(in_flight_requests_[request_url]);
  in_flight_requests_.erase(request_url);
  if (!newly_available) {
    return;
  }
  for (const base::Closure& callback : callbacks) {
    if (callback) {
      callback.Run();
    }
  }
}

}  // namespace ntp_tiles
