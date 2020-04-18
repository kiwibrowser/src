// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_INFOBARS_PERMISSION_INFOBAR_H_
#define CHROME_BROWSER_UI_ANDROID_INFOBARS_PERMISSION_INFOBAR_H_

#include <vector>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"

// TODO(timloh): Rename GroupedPermissionInfoBar to PermissionInfoBar and move
// these functions into it.
class PermissionInfoBar {
 public:
  static base::android::ScopedJavaLocalRef<jobject> CreateRenderInfoBarHelper(
      JNIEnv* env,
      int enumerated_icon_id,
      const base::android::JavaRef<jobject>& tab,
      const base::android::ScopedJavaLocalRef<jobject>& icon_bitmap,
      const base::string16& message_text,
      const base::string16& link_text,
      const base::string16& ok_button_text,
      const base::string16& cancel_button_text,
      std::vector<int>& content_settings);

 private:
  DISALLOW_COPY_AND_ASSIGN(PermissionInfoBar);
};

#endif  // CHROME_BROWSER_UI_ANDROID_INFOBARS_PERMISSION_INFOBAR_H_
