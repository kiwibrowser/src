// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_APP_HOOKS_H_
#define CHROME_BROWSER_ANDROID_APP_HOOKS_H_

#include <jni.h>
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"

namespace chrome {
namespace android {

class AppHooks {
 public:
  static base::android::ScopedJavaLocalRef<jobject>
  GetOfflinePagesCCTRequestDoneCallback();

 private:
  AppHooks();
  ~AppHooks();

  DISALLOW_COPY_AND_ASSIGN(AppHooks);
};

}  // namespace android
}  // namespace chrome

#endif  // CHROME_BROWSER_ANDROID_APP_HOOKS_H_
