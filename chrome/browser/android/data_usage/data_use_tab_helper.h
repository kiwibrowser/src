// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_TAB_HELPER_H_
#define CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_TAB_HELPER_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class RenderFrameHost;
class NavigationHandle;
}

// Per-tab class to manage tab closures and navigations, so that data usage per
// tab can be properly recorded..
class DataUseTabHelper : public content::WebContentsObserver,
                         public content::WebContentsUserData<DataUseTabHelper> {
 public:
  ~DataUseTabHelper() override;

 private:
  friend class content::WebContentsUserData<DataUseTabHelper>;

  explicit DataUseTabHelper(content::WebContents* web_contents);

  // Overridden from content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void FrameDeleted(content::RenderFrameHost* render_frame_host) override;

  DISALLOW_COPY_AND_ASSIGN(DataUseTabHelper);
};

#endif  // CHROME_BROWSER_ANDROID_DATA_USAGE_DATA_USE_TAB_HELPER_H_
