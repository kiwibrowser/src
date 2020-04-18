// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <jni.h>

#include <string>
#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "chrome/browser/consent_auditor/consent_auditor_factory.h"
#include "chrome/browser/profiles/profile_android.h"
#include "components/consent_auditor/consent_auditor.h"
#include "jni/ConsentAuditorBridge_jni.h"

using base::android::JavaParamRef;

static void JNI_ConsentAuditorBridge_RecordConsent(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& j_profile,
    const JavaParamRef<jstring>& j_account_id,
    jint j_feature,
    const JavaParamRef<jintArray>& j_consent_description,
    jint j_consent_confirmation) {
  std::vector<int> consent_description;
  base::android::JavaIntArrayToIntVector(env, j_consent_description,
                                         &consent_description);
  ConsentAuditorFactory::GetForProfile(
      ProfileAndroid::FromProfileAndroid(j_profile))
      ->RecordGaiaConsent(ConvertJavaStringToUTF8(j_account_id),
                          static_cast<consent_auditor::Feature>(j_feature),
                          consent_description, j_consent_confirmation,
                          consent_auditor::ConsentStatus::GIVEN);
}
