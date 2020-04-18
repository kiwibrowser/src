// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_VIEW_ANDROID_HELPER_H_
#define CHROME_BROWSER_UI_ANDROID_VIEW_ANDROID_HELPER_H_

#include "base/macros.h"
#include "content/public/browser/web_contents_user_data.h"

// Per-tab class to provide access to ViewAndroid object.
class ViewAndroidHelper
    : public content::WebContentsUserData<ViewAndroidHelper> {
 public:
  ~ViewAndroidHelper() override;

  void SetViewAndroid(ui::ViewAndroid* view_android);
  ui::ViewAndroid* GetViewAndroid();

 private:
  explicit ViewAndroidHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<ViewAndroidHelper>;

  // The owning view that has a hold of the current window.
  ui::ViewAndroid* view_android_;

  DISALLOW_COPY_AND_ASSIGN(ViewAndroidHelper);
};

#endif  // CHROME_BROWSER_UI_ANDROID_VIEW_ANDROID_HELPER_H_
