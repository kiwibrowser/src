// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_ANDROID_UI_GESTURE_TARGET_H_
#define CHROME_BROWSER_ANDROID_VR_ANDROID_UI_GESTURE_TARGET_H_

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "content/public/browser/android/motion_event_action.h"

namespace blink {
class WebInputEvent;
}

namespace vr {

// Used to forward events to MotionEventSynthesizer. Owned by VrShell.
class AndroidUiGestureTarget {
 public:
  AndroidUiGestureTarget(JNIEnv* env,
                         const base::android::JavaParamRef<jobject>& obj,
                         float scale_factor,
                         float scroll_ratio,
                         int touch_slop);
  ~AndroidUiGestureTarget();

  static AndroidUiGestureTarget* FromJavaObject(
      const base::android::JavaRef<jobject>& obj);

  void DispatchWebInputEvent(std::unique_ptr<blink::WebInputEvent> event);

 private:
  void Inject(content::MotionEventAction action, int64_t time_ms);
  void SetPointer(int x, int y);

  int scroll_x_ = 0;
  int scroll_y_ = 0;
  float scale_factor_;
  float scroll_ratio_;
  int touch_slop_;

  JavaObjectWeakGlobalRef java_ref_;

  DISALLOW_COPY_AND_ASSIGN(AndroidUiGestureTarget);
};

}  // namespace vr

#endif  // CHROME_BROWSER_ANDROID_VR_ANDROID_UI_GESTURE_TARGET_H_
