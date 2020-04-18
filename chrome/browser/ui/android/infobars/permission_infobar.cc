// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/infobars/permission_infobar.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "chrome/browser/android/tab_android.h"
#include "chrome/browser/ui/android/infobars/confirm_infobar.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "jni/PermissionInfoBar_jni.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/image/image.h"

using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

ScopedJavaLocalRef<jobject> PermissionInfoBar::CreateRenderInfoBarHelper(
    JNIEnv* env,
    int enumerated_icon_id,
    const JavaRef<jobject>& tab,
    const ScopedJavaLocalRef<jobject>& icon_bitmap,
    const base::string16& message_text,
    const base::string16& link_text,
    const base::string16& ok_button_text,
    const base::string16& cancel_button_text,
    std::vector<int>& content_settings) {
  ScopedJavaLocalRef<jstring> message_text_java =
      base::android::ConvertUTF16ToJavaString(env, message_text);
  ScopedJavaLocalRef<jstring> link_text_java =
      base::android::ConvertUTF16ToJavaString(env, link_text);
  ScopedJavaLocalRef<jstring> ok_button_text_java =
      base::android::ConvertUTF16ToJavaString(env, ok_button_text);
  ScopedJavaLocalRef<jstring> cancel_button_text_java =
      base::android::ConvertUTF16ToJavaString(env, cancel_button_text);

  ScopedJavaLocalRef<jintArray> content_settings_types =
      base::android::ToJavaIntArray(env, content_settings);
  return Java_PermissionInfoBar_create(
      env, tab, enumerated_icon_id, icon_bitmap, message_text_java,
      link_text_java, ok_button_text_java, cancel_button_text_java,
      content_settings_types);
}
