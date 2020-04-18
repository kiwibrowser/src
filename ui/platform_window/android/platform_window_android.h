// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_PLATFORM_WINDOW_ANDROID_PLATFORM_WINDOW_ANDROID_H_
#define UI_PLATFORM_WINDOW_ANDROID_PLATFORM_WINDOW_ANDROID_H_

#include "base/android/jni_weak_ref.h"
#include "base/macros.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/platform_window/android/android_window_export.h"
#include "ui/platform_window/android/platform_ime_controller_android.h"
#include "ui/platform_window/stub/stub_window.h"

struct ANativeWindow;

namespace ui {

class PlatformWindowDelegate;

// NOTE: This class extends StubWindow because it's very much a work in
// progress. If we make it real then it should subclass PlatformWindow directly.
class ANDROID_WINDOW_EXPORT PlatformWindowAndroid : public StubWindow {
 public:
  explicit PlatformWindowAndroid(PlatformWindowDelegate* delegate);
  ~PlatformWindowAndroid() override;

  void Destroy(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void SurfaceCreated(JNIEnv* env,
                      const base::android::JavaParamRef<jobject>& obj,
                      const base::android::JavaParamRef<jobject>& jsurface,
                      float device_pixel_ratio);
  void SurfaceDestroyed(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& obj);
  void SurfaceSetSize(JNIEnv* env,
                      const base::android::JavaParamRef<jobject>& obj,
                      jint width,
                      jint height,
                      jfloat density);
  bool TouchEvent(JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& obj,
                  jlong time_ms,
                  jint masked_action,
                  jint pointer_id,
                  jfloat x,
                  jfloat y,
                  jfloat pressure,
                  jfloat touch_major,
                  jfloat touch_minor,
                  jfloat orientation,
                  jfloat h_wheel,
                  jfloat v_wheel);
  bool KeyEvent(JNIEnv* env,
                const base::android::JavaParamRef<jobject>& obj,
                bool pressed,
                jint key_code,
                jint unicode_character);

 private:
  void ReleaseWindow();

  // Overridden from PlatformWindow:
  void Show() override;
  void Hide() override;
  void SetBounds(const gfx::Rect& bounds) override;
  gfx::Rect GetBounds() override;
  PlatformImeController* GetPlatformImeController() override;

  JavaObjectWeakGlobalRef java_platform_window_android_;
  ANativeWindow* window_;

  gfx::Size size_;  // Origin is always (0,0)

  PlatformImeControllerAndroid platform_ime_controller_;

  DISALLOW_COPY_AND_ASSIGN(PlatformWindowAndroid);
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_ANDROID_PLATFORM_WINDOW_ANDROID_H_
