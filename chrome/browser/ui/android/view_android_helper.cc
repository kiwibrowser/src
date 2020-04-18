// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/view_android_helper.h"
#include "content/public/browser/web_contents.h"
#include "ui/android/view_android.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(ViewAndroidHelper);

ViewAndroidHelper::ViewAndroidHelper(content::WebContents* web_contents)
    : view_android_(nullptr) {
}

ViewAndroidHelper::~ViewAndroidHelper() {
}

void ViewAndroidHelper::SetViewAndroid(ui::ViewAndroid* view_android) {
  view_android_ = view_android;
}

ui::ViewAndroid* ViewAndroidHelper::GetViewAndroid() {
  return view_android_;
}
