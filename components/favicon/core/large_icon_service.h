// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FAVICON_CORE_LARGE_ICON_SERVICE_H_
#define COMPONENTS_FAVICON_CORE_LARGE_ICON_SERVICE_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/task/cancelable_task_tracker.h"
#include "components/favicon_base/favicon_callback.h"
#include "components/favicon_base/favicon_types.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

class GURL;

namespace image_fetcher {
class ImageFetcher;
}

namespace favicon {

class FaviconService;
class FaviconServerFetcherParams;

// The large icon service provides methods to access large icons. It relies on
// the favicon service.
class LargeIconService : public KeyedService {
 public:
  LargeIconService(
      FaviconService* favicon_service,
      std::unique_ptr<image_fetcher::ImageFetcher> image_fetcher);
  ~LargeIconService() override;

  // Requests the best large icon for the page at |page_url|.
  // Case 1. An icon exists whose size is >= MAX(|min_source_size_in_pixel|,
  // |desired_size_in_pixel|):
  // - If |desired_size_in_pixel| == 0: returns icon as is.
  // - Else: returns the icon resized to |desired_size_in_pixel|.
  // Case 2. An icon exists whose size is >= |min_source_size_in_pixel| and <
  // |desired_size_in_pixel|:
  // - Same as 1 with the biggest icon.
  // Case 4. An icon exists whose size is < |min_source_size_in_pixel|:
  // - Extracts dominant color of smaller image, returns a fallback icon style
  //   that has a matching background.
  // Case 5. No icon exists.
  // - Returns the default fallback icon style.
  // For cases 4 and 5, this function returns the style of the fallback icon
  // instead of rendering an icon so clients can render the icon themselves.
  // TODO(jkrcal): Rename to GetLargeIconRawBitmapOrFallbackStyle.
  base::CancelableTaskTracker::TaskId GetLargeIconOrFallbackStyle(
      const GURL& page_url,
      int min_source_size_in_pixel,
      int desired_size_in_pixel,
      const favicon_base::LargeIconCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Behaves the same as GetLargeIconOrFallbackStyle(), only returns the large
  // icon (if available) decoded.
  base::CancelableTaskTracker::TaskId GetLargeIconImageOrFallbackStyle(
      const GURL& page_url,
      int min_source_size_in_pixel,
      int desired_size_in_pixel,
      const favicon_base::LargeIconImageCallback& callback,
      base::CancelableTaskTracker* tracker);

  // Fetches the best large icon for the page at |page_url| from a Google
  // favicon server and stores the result in the FaviconService database
  // (implemented in HistoryService). The write will be a no-op if the local
  // favicon database contains an icon for |page_url|, so clients are
  // encouraged to use GetLargeIconOrFallbackStyle() first.
  //
  // A minimum size |min_source_size_in_pixel| can be specified as a constraint.
  // |desired_size_in_pixel| serves only as a hint to the service, no guarantees
  // on the fetched size are provided.
  //
  // Unless you are sure |page_url| is a public URL (known to Google Search),
  // set |may_page_url_be_private| to true. This slighty increases the chance of
  // a failure (e.g. if the URL _is_ private) but it makes sure Google servers
  // do not crawl a private URL as a result of this call.
  //
  // The callback is triggered when the operation finishes, where |success|
  // tells whether the fetch actually managed to database a new icon in the
  // FaviconService.
  //
  // WARNING: This function will share the |page_url| with a Google server. This
  // Can be used only for urls that are not privacy sensitive or for users that
  // sync their history with Google servers.
  // TODO(jkrcal): It is not clear from the name of this function, that it
  // actually adds the icon to the local cache. Maybe "StoreLargeIcon..."?
  void GetLargeIconOrFallbackStyleFromGoogleServerSkippingLocalCache(
      std::unique_ptr<FaviconServerFetcherParams> params,
      bool may_page_url_be_private,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      const favicon_base::GoogleFaviconServerCallback& callback);

  // Update the time that the icon at |icon_url| was requested. This should be
  // called after obtaining the icon by GetLargeIcon*OrFallbackStyle() for any
  // icon that _may_ originate from the Google favicon server (i.e. if the
  // caller uses
  // GetLargeIconOrFallbackStyleFromGoogleServerSkippingLocalCache()). This
  // postpones the automatic eviction of the favicon from the database.
  void TouchIconFromGoogleServer(const GURL& icon_url);

  // Extracts the organization-identifying domain from |url| which excludes
  // registrar portion (e.g. final ".com"). Used for logging UMA metrics.
  // Exposed publicly for testing.
  static std::string GetOrganizationNameForUma(const GURL& url);

 private:
  base::CancelableTaskTracker::TaskId GetLargeIconOrFallbackStyleImpl(
      const GURL& page_url,
      int min_source_size_in_pixel,
      int desired_size_in_pixel,
      const favicon_base::LargeIconCallback& raw_bitmap_callback,
      const favicon_base::LargeIconImageCallback& image_callback,
      base::CancelableTaskTracker* tracker);

  void OnCanSetOnDemandFaviconComplete(
      const GURL& server_request_url,
      const GURL& page_url,
      favicon_base::IconType icon_type,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      const favicon_base::GoogleFaviconServerCallback& callback,
      bool can_set_on_demand_favicon);

  FaviconService* favicon_service_;

  // A pre-populated list of icon types to consider when looking for large
  // icons. This is an optimization over populating an icon type vector on each
  // request.
  std::vector<favicon_base::IconTypeSet> large_icon_types_;

  std::unique_ptr<image_fetcher::ImageFetcher> image_fetcher_;

  base::WeakPtrFactory<LargeIconService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LargeIconService);
};

}  // namespace favicon

#endif  // COMPONENTS_FAVICON_CORE_LARGE_ICON_SERVICE_H_
