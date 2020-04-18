// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_THUMBNAIL_SOURCE_H_
#define CHROME_BROWSER_SEARCH_THUMBNAIL_SOURCE_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "components/image_fetcher/core/image_data_fetcher.h"
#include "content/public/browser/url_data_source.h"

class GURL;
class Profile;

namespace image_fetcher {
struct RequestMetadata;
}

namespace thumbnails {
class ThumbnailService;
}

// ThumbnailSource is the gateway between network-level chrome: requests for
// thumbnails and the history/top-sites backend that serves these.
class ThumbnailSource : public content::URLDataSource {
 public:
  ThumbnailSource(Profile* profile, bool capture_thumbnails);
  ~ThumbnailSource() override;

  // content::URLDataSource implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) const override;
  scoped_refptr<base::SingleThreadTaskRunner> TaskRunnerForRequestPath(
      const std::string& path) const override;
  bool AllowCaching() const override;
  bool ShouldServiceRequest(const GURL& url,
                            content::ResourceContext* resource_context,
                            int render_process_id) const override;

  // Extracts the |page_url| (e.g. cnn.com) and the |fallback_thumbnail_url|
  // fetchable from the server, if present, from the |path|. Visible for
  // testing.
  void ExtractPageAndThumbnailUrls(const std::string& path,
                                   GURL* page_url,
                                   GURL* fallback_thumbnail_url);

 private:
  void SendFetchedUrlImage(
      const content::URLDataSource::GotDataCallback& callback,
      const std::string& image_data,
      const image_fetcher::RequestMetadata& metadata);

  scoped_refptr<thumbnails::ThumbnailService> thumbnail_service_;

  // Indicate that, when a URL for which we don't have a thumbnail is requested
  // from this source, then Chrome should capture a thumbnail next time it
  // navigates to this URL. This is useful when the thumbnail URLs are generated
  // by an external service rather than TopSites, so Chrome can learn about the
  // URLs for which it should get thumbnails. Sources that capture thumbnails
  // are also be more lenient when matching thumbnail URLs by checking for
  // existing thumbnails in the database that contain a URL matching the prefix
  // of the requested URL.
  const bool capture_thumbnails_;

  image_fetcher::ImageDataFetcher image_data_fetcher_;

  base::WeakPtrFactory<ThumbnailSource> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThumbnailSource);
};

#endif  // CHROME_BROWSER_SEARCH_THUMBNAIL_SOURCE_H_
