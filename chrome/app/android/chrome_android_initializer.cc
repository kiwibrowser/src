// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/android/chrome_android_initializer.h"

#include "base/android/library_loader/library_loader_hooks.h"
#include "chrome/app/android/chrome_main_delegate_android.h"
#include "components/version_info/version_info.h"
#include "content/public/app/content_main.h"

bool RunChrome() {
  // Pass the library version number to content so that we can check it from the
  // Java side before continuing initialization
  base::android::SetVersionNumber(version_info::GetVersionNumber().c_str());
  content::SetContentMainDelegate(ChromeMainDelegateAndroid::Create());

  return true;
}
