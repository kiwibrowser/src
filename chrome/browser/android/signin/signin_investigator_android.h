// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_SIGNIN_SIGNIN_INVESTIGATOR_ANDROID_H_
#define CHROME_BROWSER_ANDROID_SIGNIN_SIGNIN_INVESTIGATOR_ANDROID_H_

#include <jni.h>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"

// Bridge to invoke shared signin manager logic from Android. Java class name is
// SigninInvestigator.
class SigninInvestigatorAndroid {
 public:
  // Investigates the current signin, and returns an int corresponding to the
  // scenario we are currently in.
  static jint Investigate(
      JNIEnv* env,
      const base::android::JavaParamRef<jclass>& jcaller,
      const base::android::JavaParamRef<jstring>& current_email);

 private:
  DISALLOW_COPY_AND_ASSIGN(SigninInvestigatorAndroid);
};

#endif  // CHROME_BROWSER_ANDROID_SIGNIN_SIGNIN_INVESTIGATOR_ANDROID_H_
