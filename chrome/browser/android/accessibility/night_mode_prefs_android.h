// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_ACCESSIBILITY_NIGHT_MODE_PREFS_ANDROID_H_
#define CHROME_BROWSER_ANDROID_ACCESSIBILITY_NIGHT_MODE_PREFS_ANDROID_H_

#include <memory>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"

class PrefChangeRegistrar;
class PrefService;

/*
 * Native implementation of NightModePrefs. This class is used to get and set
 * NightModeFactor and ForceEnableZoom.
 */
class NightModePrefsAndroid {
 public:
  NightModePrefsAndroid(JNIEnv* env, jobject obj);
  ~NightModePrefsAndroid();

  void SetNightModeFactor(JNIEnv* env,
                          const base::android::JavaRef<jobject>& obj,
                          jfloat night);

  void SetNightModeEnabled(JNIEnv* env,
                          const base::android::JavaRef<jobject>& obj,
                          jboolean night);

  void SetNightModeGrayscaleEnabled(JNIEnv* env,
                          const base::android::JavaRef<jobject>& obj,
                          jboolean night);

  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;
  PrefService* const pref_service_;
  base::android::ScopedJavaGlobalRef<jobject> java_ref_;

  DISALLOW_COPY_AND_ASSIGN(NightModePrefsAndroid);
};

#endif  // CHROME_BROWSER_ANDROID_ACCESSIBILITY_NIGHT_MODE_PREFS_ANDROID_H_
