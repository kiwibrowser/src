// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_WEBAPPS_SINGLE_TAB_MODE_TAB_HELPER_H_
#define CHROME_BROWSER_ANDROID_WEBAPPS_SINGLE_TAB_MODE_TAB_HELPER_H_

#include "base/macros.h"
#include "chrome/browser/ui/blocked_content/blocked_window_params.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

// Tracks tabs in single tab mode, which are disallowed from opening new windows
// via ChromeContentBrowserClient::CanCreateWindow().
class SingleTabModeTabHelper
    : public content::WebContentsUserData<SingleTabModeTabHelper> {
 public:
  ~SingleTabModeTabHelper() override;

  bool block_all_new_windows() const { return block_all_new_windows_; }

  // Permanently block this WebContents from creating new windows -- there is no
  // current need to allow toggling this flag on or off.
  void PermanentlyBlockAllNewWindows();

 private:
  explicit SingleTabModeTabHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<SingleTabModeTabHelper>;

  bool block_all_new_windows_ = false;

  DISALLOW_COPY_AND_ASSIGN(SingleTabModeTabHelper);
};

#endif  // CHROME_BROWSER_ANDROID_WEBAPPS_SINGLE_TAB_MODE_TAB_HELPER_H_
