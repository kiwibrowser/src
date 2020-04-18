// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_COMPOSITOR_RESOURCES_RESOURCE_FACTORY_H_
#define CHROME_BROWSER_ANDROID_COMPOSITOR_RESOURCES_RESOURCE_FACTORY_H_

#include "base/android/jni_android.h"
#include "jni/ResourceFactory_jni.h"

using base::android::JavaParamRef;

namespace android {

jlong CreateToolbarContainerResource(JNIEnv* env,
                                     const JavaParamRef<jclass>& clazz,
                                     jint toolbar_left,
                                     jint toolbar_top,
                                     jint toolbar_right,
                                     jint toolbar_bottom,
                                     jint location_bar_left,
                                     jint location_bar_top,
                                     jint location_bar_right,
                                     jint location_bar_bottom,
                                     jint shadow_height);
}  // namespace android

#endif  // CHROME_BROWSER_ANDROID_COMPOSITOR_RESOURCES_RESOURCE_FACTORY_H_
