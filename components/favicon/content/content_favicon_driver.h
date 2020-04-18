// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FAVICON_CONTENT_CONTENT_FAVICON_DRIVER_H_
#define COMPONENTS_FAVICON_CONTENT_CONTENT_FAVICON_DRIVER_H_

#include <vector>

#include "base/macros.h"
#include "base/optional.h"
#include "components/favicon/core/favicon_driver_impl.h"
#include "content/public/browser/reload_type.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/favicon_url.h"
#include "url/gurl.h"

namespace content {
struct FaviconURL;
class WebContents;
}

namespace favicon {

// ContentFaviconDriver is an implementation of FaviconDriver that listens to
// WebContents events to start download of favicons and to get informed when the
// favicon download has completed.
class ContentFaviconDriver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<ContentFaviconDriver>,
      public FaviconDriverImpl {
 public:
  ~ContentFaviconDriver() override;

  static void CreateForWebContents(content::WebContents* web_contents,
                                   FaviconService* favicon_service,
                                   history::HistoryService* history_service);

  // Returns the current tab's favicon URLs. If this is empty,
  // DidUpdateFaviconURL has not yet been called for the current navigation.
  std::vector<content::FaviconURL> favicon_urls() const {
    return favicon_urls_.value_or(std::vector<content::FaviconURL>());
  }

  // Saves the favicon for the last committed navigation entry to the thumbnail
  // database.
  void SaveFaviconEvenIfInIncognito();

  // FaviconDriver implementation.
  gfx::Image GetFavicon() const override;
  bool FaviconIsValid() const override;
  GURL GetActiveURL() override;

 protected:
  ContentFaviconDriver(content::WebContents* web_contents,
                       FaviconService* favicon_service,
                       history::HistoryService* history_service);

 private:
  friend class content::WebContentsUserData<ContentFaviconDriver>;

  // FaviconHandler::Delegate implementation.
  int DownloadImage(const GURL& url,
                    int max_image_size,
                    ImageDownloadCallback callback) override;
  void DownloadManifest(const GURL& url,
                        ManifestDownloadCallback callback) override;
  bool IsOffTheRecord() override;
  void OnFaviconUpdated(const GURL& page_url,
                        FaviconDriverObserver::NotificationIconType icon_type,
                        const GURL& icon_url,
                        bool icon_url_changed,
                        const gfx::Image& image) override;
  void OnFaviconDeleted(const GURL& page_url,
                        FaviconDriverObserver::NotificationIconType
                            notification_icon_type) override;

  // content::WebContentsObserver implementation.
  void DidUpdateFaviconURL(
      const std::vector<content::FaviconURL>& candidates) override;
  void DidUpdateWebManifestURL(
      const base::Optional<GURL>& manifest_url) override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentOnLoadCompletedInMainFrame() override;

  bool document_on_load_completed_;
  GURL bypass_cache_page_url_;
  // nullopt until the actual list is reported via DidUpdateFaviconURL().
  base::Optional<std::vector<content::FaviconURL>> favicon_urls_;
  // Web Manifest URL or empty URL if none.
  GURL manifest_url_;

  DISALLOW_COPY_AND_ASSIGN(ContentFaviconDriver);
};

}  // namespace favicon

#endif  // COMPONENTS_FAVICON_CONTENT_CONTENT_FAVICON_DRIVER_H_
