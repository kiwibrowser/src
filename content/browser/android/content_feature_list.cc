// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/content_feature_list.h"

#include "base/android/jni_string.h"
#include "base/feature_list.h"
#include "base/macros.h"
#include "content/public/common/content_features.h"
#include "jni/ContentFeatureList_jni.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::JavaParamRef;

namespace content {
namespace android {

namespace {

// Array of features exposed through the Java ContentFeatureList API. Entries in
// this array may either refer to features defined in the header of this file or
// in other locations in the code base (e.g. content_features.h).
const base::Feature* kFeaturesExposedToJava[] = {
    &kEnhancedSelectionInsertionHandle,
};

const base::Feature* FindFeatureExposedToJava(const std::string& feature_name) {
  for (size_t i = 0; i < arraysize(kFeaturesExposedToJava); ++i) {
    if (kFeaturesExposedToJava[i]->name == feature_name)
      return kFeaturesExposedToJava[i];
  }
  NOTREACHED() << "Queried feature cannot be found in ContentFeatureList: "
               << feature_name;
  return nullptr;
}

}  // namespace

// Alphabetical:
const base::Feature kEnhancedSelectionInsertionHandle{
    "EnhancedSelectionInsertionHandle", base::FEATURE_ENABLED_BY_DEFAULT};
const base::Feature kRequestUnbufferedDispatch{
    "RequestUnbufferedDispatch", base::FEATURE_ENABLED_BY_DEFAULT};

static jboolean JNI_ContentFeatureList_IsEnabled(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jstring>& jfeature_name) {
  const base::Feature* feature =
      FindFeatureExposedToJava(ConvertJavaStringToUTF8(env, jfeature_name));
  return base::FeatureList::IsEnabled(*feature);
}

}  // namespace android
}  // namespace content
