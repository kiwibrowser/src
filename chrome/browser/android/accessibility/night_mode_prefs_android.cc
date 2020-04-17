// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/accessibility/night_mode_prefs_android.h"

#include "base/observer_list.h"
#include "chrome/browser/profiles/profile_android.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "jni/NightModePrefs_jni.h"

using base::android::JavaParamRef;
using base::android::JavaRef;

NightModePrefsAndroid::NightModePrefsAndroid(JNIEnv* env, jobject obj)
    : pref_service_(ProfileManager::GetActiveUserProfile()->GetPrefs()) {
  java_ref_.Reset(env, obj);
  pref_change_registrar_.reset(new PrefChangeRegistrar);
  pref_change_registrar_->Init(pref_service_);
}

NightModePrefsAndroid::~NightModePrefsAndroid() {
}

jlong JNI_NightModePrefs_Init(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  NightModePrefsAndroid* night_mode_prefs_android =
      new NightModePrefsAndroid(env, obj);
  return reinterpret_cast<intptr_t>(night_mode_prefs_android);
}

void NightModePrefsAndroid::SetNightModeFactor(JNIEnv* env,
                                              const JavaRef<jobject>& obj,
                                              jfloat night_mode) {
  pref_service_->SetDouble(prefs::kWebKitNightModeFactor,
                           static_cast<double>(night_mode));
}

void NightModePrefsAndroid::SetNightModeEnabled(JNIEnv* env,
                                              const JavaRef<jobject>& obj,
                                              jboolean night_mode) {
  pref_service_->SetBoolean(prefs::kWebKitNightModeEnabled, night_mode);
}

void NightModePrefsAndroid::SetNightModeGrayscaleEnabled(JNIEnv* env,
                                              const JavaRef<jobject>& obj,
                                              jboolean night_mode) {
  pref_service_->SetBoolean(prefs::kWebKitNightModeGrayscaleEnabled, night_mode);
}
