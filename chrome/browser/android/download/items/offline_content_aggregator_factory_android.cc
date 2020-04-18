// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "chrome/browser/offline_items_collection/offline_content_aggregator_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_android.h"
#include "components/offline_items_collection/core/android/offline_content_aggregator_bridge.h"
#include "components/offline_items_collection/core/offline_content_aggregator.h"
#include "jni/OfflineContentAggregatorFactory_jni.h"

using base::android::JavaParamRef;

// Takes a Java Profile and returns a Java OfflineContentAggregatorBridge.
static base::android::ScopedJavaLocalRef<jobject>
JNI_OfflineContentAggregatorFactory_GetOfflineContentAggregatorForProfile(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    const JavaParamRef<jobject>& jprofile) {
  Profile* profile = ProfileAndroid::FromProfileAndroid(jprofile);
  DCHECK(profile);
  offline_items_collection::OfflineContentAggregator* aggregator =
      OfflineContentAggregatorFactory::GetInstance()->GetForBrowserContext(
          profile);
  return offline_items_collection::android::OfflineContentAggregatorBridge::
      GetBridgeForOfflineContentAggregator(aggregator);
}
