// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONTENT_SETTINGS_MIXED_CONTENT_SETTINGS_TAB_HELPER_H_
#define CHROME_BROWSER_CONTENT_SETTINGS_MIXED_CONTENT_SETTINGS_TAB_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class SiteInstance;
}

// Controls mixed content related settings for the associated WebContents,
// working as the browser version of the mixed content state kept by
// ContentSettingsObserver in the renderer.
class MixedContentSettingsTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<MixedContentSettingsTabHelper> {
 public:
  ~MixedContentSettingsTabHelper() override;

  // Enables running active mixed content resources in the associated
  // WebContents/tab.
  void AllowRunningOfInsecureContent();

  bool is_running_insecure_content_allowed() {
    return is_running_insecure_content_allowed_;
  }

 private:
  friend class content::WebContentsUserData<MixedContentSettingsTabHelper>;

  explicit MixedContentSettingsTabHelper(content::WebContents* tab);

  // content::WebContentsObserver
  void RenderFrameCreated(content::RenderFrameHost* render_frame_host) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // Members to control mixed content settings for the tab. They will be reset
  // on cross-site main frame navigations.
  content::SiteInstance* insecure_content_site_instance_ = nullptr;
  bool is_running_insecure_content_allowed_ = false;

  DISALLOW_COPY_AND_ASSIGN(MixedContentSettingsTabHelper);
};

#endif  // CHROME_BROWSER_CONTENT_SETTINGS_MIXED_CONTENT_SETTINGS_TAB_HELPER_H_
