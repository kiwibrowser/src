// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREVIEWS_PREVIEWS_INFOBAR_TAB_HELPER_H_
#define CHROME_BROWSER_PREVIEWS_PREVIEWS_INFOBAR_TAB_HELPER_H_

#include "base/macros.h"
#include "base/optional.h"
#include "components/previews/core/previews_user_data.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

// Tracks whether a previews infobar has been shown for a page. Handles showing
// the infobar when the main frame response indicates a Lite Page.
class PreviewsInfoBarTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<PreviewsInfoBarTabHelper> {
 public:
  ~PreviewsInfoBarTabHelper() override;

  // Indicates whether the InfoBar for a preview has been shown for the page.
  bool displayed_preview_infobar() const {
    return displayed_preview_infobar_;
  }

  // Sets whether the InfoBar for a preview has been shown for the page.
  // |displayed_preview_infobar_| is reset to false on
  // DidStartProvisionalLoadForFrame for the main frame.
  void set_displayed_preview_infobar(bool displayed) {
    displayed_preview_infobar_ = displayed;
  }

  // Sets whether the timestamp on the InfoBar for a preview has been shown for
  // the page. |displayed_preview_timestamp_| is reset to false on
  // DidStartProvisionalLoadForFrame for the main frame.
  void set_displayed_preview_timestamp(bool displayed_preview_timestamp) {
    displayed_preview_timestamp_ = displayed_preview_timestamp;
  }

  // The Previews information related to the navigation that was most recently
  // finished.
  previews::PreviewsUserData* previews_user_data() const {
    return previews_user_data_.get();
  }

 private:
  friend class content::WebContentsUserData<PreviewsInfoBarTabHelper>;
  friend class PreviewsInfoBarTabHelperUnitTest;

  explicit PreviewsInfoBarTabHelper(content::WebContents* web_contents);

  // Overridden from content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // True if the InfoBar for a preview has been shown for the page.
  bool displayed_preview_infobar_;

  // True if the InfoBar with a timestamp was shown for the page.
  bool displayed_preview_timestamp_;

  // The Previews information related to the navigation that was most recently
  // finished.
  std::unique_ptr<previews::PreviewsUserData> previews_user_data_;

  DISALLOW_COPY_AND_ASSIGN(PreviewsInfoBarTabHelper);
};

#endif  // CHROME_BROWSER_PREVIEWS_PREVIEWS_INFOBAR_TAB_HELPER_H_
