// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/invalidation/invalidation_service_factory_android.h"

#include "base/android/jni_android.h"
#include "chrome/browser/invalidation/profile_invalidation_provider_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "components/invalidation/impl/invalidation_service_android.h"
#include "components/invalidation/impl/profile_invalidation_provider.h"
#include "jni/InvalidationServiceFactory_jni.h"

using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

namespace invalidation {

ScopedJavaLocalRef<jobject> InvalidationServiceFactoryAndroid::GetForProfile(
    const JavaRef<jobject>& j_profile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile);
  invalidation::ProfileInvalidationProvider* provider =
      ProfileInvalidationProviderFactory::GetForProfile(profile);
  InvalidationServiceAndroid* service_android =
      static_cast<InvalidationServiceAndroid*>(
          provider->GetInvalidationService());
  return ScopedJavaLocalRef<jobject>(service_android->java_ref_);
}

ScopedJavaLocalRef<jobject> InvalidationServiceFactoryAndroid::GetForTest() {
  InvalidationServiceAndroid* service_android =
      new InvalidationServiceAndroid();
  return ScopedJavaLocalRef<jobject>(service_android->java_ref_);
}

ScopedJavaLocalRef<jobject> JNI_InvalidationServiceFactory_GetForProfile(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jobject>& j_profile) {
  return InvalidationServiceFactoryAndroid::GetForProfile(j_profile);
}

ScopedJavaLocalRef<jobject> JNI_InvalidationServiceFactory_GetForTest(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz) {
  return InvalidationServiceFactoryAndroid::GetForTest();
}

}  // namespace invalidation
