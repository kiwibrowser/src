// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/screen_orientation/screen_orientation_delegate_android.h"

#include "content/browser/android/content_view_core.h"
#include "content/browser/screen_orientation/screen_orientation_provider.h"
#include "jni/ScreenOrientationProvider_jni.h"
#include "ui/android/window_android.h"
#include "ui/gfx/native_widget_types.h"

namespace content {

ScreenOrientationDelegateAndroid::ScreenOrientationDelegateAndroid() {
  ScreenOrientationProvider::SetDelegate(this);
}

ScreenOrientationDelegateAndroid::~ScreenOrientationDelegateAndroid() {
  ScreenOrientationProvider::SetDelegate(nullptr);
}

bool ScreenOrientationDelegateAndroid::FullScreenRequired(
    WebContents* web_contents) {
  return true;
}

void ScreenOrientationDelegateAndroid::Lock(
    WebContents* web_contents,
    blink::WebScreenOrientationLockType lock_orientation) {
  gfx::NativeWindow window = web_contents->GetTopLevelNativeWindow();
  Java_ScreenOrientationProvider_lockOrientation(
      base::android::AttachCurrentThread(),
      window ? window->GetJavaObject() : nullptr,
      lock_orientation);
}

bool ScreenOrientationDelegateAndroid::ScreenOrientationProviderSupported() {
  // TODO(MLamouri): Consider moving isOrientationLockEnabled to a separate
  // function, so reported error messages can differentiate between the device
  // never supporting orientation or currently not support orientation.
  return Java_ScreenOrientationProvider_isOrientationLockEnabled(
      base::android::AttachCurrentThread());
}

void ScreenOrientationDelegateAndroid::Unlock(WebContents* web_contents) {
  gfx::NativeWindow window = web_contents->GetTopLevelNativeWindow();
  Java_ScreenOrientationProvider_unlockOrientation(
      base::android::AttachCurrentThread(),
      window ? window->GetJavaObject() : nullptr);
}

} // namespace content
