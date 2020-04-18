// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_BACKGROUND_SYNC_LAUNCHER_ANDROID_H_
#define CHROME_BROWSER_ANDROID_BACKGROUND_SYNC_LAUNCHER_ANDROID_H_

#include <stdint.h>

#include <set>

#include "base/android/jni_android.h"
#include "base/lazy_instance.h"
#include "base/macros.h"

// The BackgroundSyncLauncherAndroid singleton owns the Java
// BackgroundSyncLauncher object and is used to register interest in starting
// the browser the next time the device goes online. This class runs on the UI
// thread.
class BackgroundSyncLauncherAndroid {
 public:
  static BackgroundSyncLauncherAndroid* Get();

  static void LaunchBrowserIfStopped(bool launch_when_next_online,
                                     int64_t min_delay_ms);

  static bool ShouldDisableBackgroundSync();

  // TODO(iclelland): Remove this once the bots have their play services package
  // updated before every test run. (https://crbug.com/514449)
  static void SetPlayServicesVersionCheckDisabledForTests(bool disabled);

 private:
  friend struct base::LazyInstanceTraitsBase<BackgroundSyncLauncherAndroid>;

  // Constructor and destructor marked private to enforce singleton
  BackgroundSyncLauncherAndroid();
  ~BackgroundSyncLauncherAndroid();

  void LaunchBrowserIfStoppedImpl(bool launch_when_next_online,
                                  int64_t min_delay_ms);

  base::android::ScopedJavaGlobalRef<jobject> java_launcher_;
  DISALLOW_COPY_AND_ASSIGN(BackgroundSyncLauncherAndroid);
};

#endif  // CHROME_BROWSER_ANDROID_BACKGROUND_SYNC_LAUNCHER_ANDROID_H_
