// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INVALIDATION_INVALIDATION_SERVICE_FACTORY_ANDROID_H_
#define CHROME_BROWSER_INVALIDATION_INVALIDATION_SERVICE_FACTORY_ANDROID_H_

#include "base/android/scoped_java_ref.h"

namespace invalidation {

// This class should not be used except from the Java class
// InvalidationServiceFactory.
class InvalidationServiceFactoryAndroid {
 public:
  static base::android::ScopedJavaLocalRef<jobject> GetForProfile(
      const base::android::JavaRef<jobject>& j_profile);

  static base::android::ScopedJavaLocalRef<jobject> GetForTest();
};

}  // namespace invalidation

#endif  // CHROME_BROWSER_INVALIDATION_INVALIDATION_SERVICE_FACTORY_ANDROID_H_
