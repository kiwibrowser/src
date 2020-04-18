// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_PLATFORM_WINDOW_ANDROID_PLATFORM_IME_CONTROLLER_ANDROID_H_
#define UI_PLATFORM_WINDOW_ANDROID_PLATFORM_IME_CONTROLLER_ANDROID_H_

#include "base/android/jni_weak_ref.h"
#include "base/macros.h"
#include "ui/platform_window/android/android_window_export.h"
#include "ui/platform_window/platform_ime_controller.h"

namespace ui {

class ANDROID_WINDOW_EXPORT PlatformImeControllerAndroid :
    public PlatformImeController {
 public:
  PlatformImeControllerAndroid();
  ~PlatformImeControllerAndroid() override;

  // Native methods called by Java code.
  void Init(JNIEnv* env, const base::android::JavaParamRef<jobject>& jobj);

 private:
  // Overridden from PlatformImeController:
  void UpdateTextInputState(const TextInputState& state) override;
  void SetImeVisibility(bool visible) override;

  JavaObjectWeakGlobalRef java_platform_ime_controller_android_;

  DISALLOW_COPY_AND_ASSIGN(PlatformImeControllerAndroid);
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_ANDROID_PLATFORM_IME_CONTROLLER_ANDROID_H_
