// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_ANDROID_H_
#define CHROME_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_ANDROID_H_

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "components/search_engines/template_url.h"

// Android wrapper of the TemplateUrl which provides access for the Java
// layer.
class TemplateUrlAndroid;

base::android::ScopedJavaLocalRef<jobject> CreateTemplateUrlAndroid(
    JNIEnv* env,
    const TemplateURL* template_url);

#endif  // CHROME_BROWSER_SEARCH_ENGINES_TEMPLATE_URL_ANDROID_H_
