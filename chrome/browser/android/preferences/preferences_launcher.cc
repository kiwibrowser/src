// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/preferences/preferences_launcher.h"

#include "base/android/jni_android.h"
#include "chrome/browser/android/tab_android.h"
#include "content/public/browser/web_contents.h"
#include "jni/PreferencesLauncher_jni.h"

namespace chrome {
namespace android {

void PreferencesLauncher::ShowAutofillSettings(
    content::WebContents* web_contents) {
  Java_PreferencesLauncher_showAutofillSettings(
      base::android::AttachCurrentThread(), web_contents->GetJavaWebContents());
}

void PreferencesLauncher::ShowPasswordSettings() {
  Java_PreferencesLauncher_showPasswordSettings(
      base::android::AttachCurrentThread());
}

}  // namespace android
}  // namespace chrome
