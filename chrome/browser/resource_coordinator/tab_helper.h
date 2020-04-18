// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_HELPER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_HELPER_H_

#include <memory>

#include "base/macros.h"
#include "base/time/time.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "url/gurl.h"

namespace resource_coordinator {

class PageResourceCoordinator;

class ResourceCoordinatorTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<ResourceCoordinatorTabHelper> {
 public:
  ~ResourceCoordinatorTabHelper() override;

  static bool ukm_recorder_initialized;

  static bool IsEnabled();

  resource_coordinator::PageResourceCoordinator* page_resource_coordinator() {
    return page_resource_coordinator_.get();
  }

  // WebContentsObserver implementation.
  void DidStartLoading() override;
  void DidReceiveResponse() override;
  void DidStopLoading() override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code,
                   const base::string16& error_description) override;
  void OnVisibilityChanged(content::Visibility visibility) override;
  void WebContentsDestroyed() override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void TitleWasSet(content::NavigationEntry* entry) override;
  void DidUpdateFaviconURL(
      const std::vector<content::FaviconURL>& candidates) override;

  void UpdateUkmRecorder(int64_t navigation_id);
  ukm::SourceId ukm_source_id() const { return ukm_source_id_; }
  void SetUkmSourceIdForTest(ukm::SourceId id) { ukm_source_id_ = id; }

 private:
  explicit ResourceCoordinatorTabHelper(content::WebContents* web_contents);
  // Favicon, title are set the first time a page is loaded, thus we want to
  // ignore the very first update, and reset the flags when a non same-document
  // navigation finished in main frame.
  void ResetFlag();

  friend class content::WebContentsUserData<ResourceCoordinatorTabHelper>;

  std::unique_ptr<resource_coordinator::PageResourceCoordinator>
      page_resource_coordinator_;
  ukm::SourceId ukm_source_id_ = ukm::kInvalidSourceId;

  // Favicon and title are set when a page is loaded, we only want to send
  // signals to GRC about title and favicon update from the previous title and
  // favicon, thus we want to ignore the very first update since it is always
  // supposed to happen.
  bool first_time_favicon_set_ = false;
  bool first_time_title_set_ = false;

  DISALLOW_COPY_AND_ASSIGN(ResourceCoordinatorTabHelper);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_TAB_HELPER_H_
