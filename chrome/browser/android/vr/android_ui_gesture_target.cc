// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr/android_ui_gesture_target.h"

#include <cmath>

#include "jni/AndroidUiGestureTarget_jni.h"
#include "third_party/blink/public/platform/web_gesture_event.h"
#include "third_party/blink/public/platform/web_input_event.h"
#include "third_party/blink/public/platform/web_mouse_event.h"

using base::android::JavaParamRef;
using base::android::JavaRef;
using content::MotionEventAction;

namespace vr {

AndroidUiGestureTarget::AndroidUiGestureTarget(JNIEnv* env,
                                               const JavaParamRef<jobject>& obj,
                                               float scale_factor,
                                               float scroll_ratio,
                                               int touch_slop)
    : scale_factor_(scale_factor),
      scroll_ratio_(scroll_ratio),
      touch_slop_(touch_slop),
      java_ref_(env, obj) {}

AndroidUiGestureTarget::~AndroidUiGestureTarget() = default;

void AndroidUiGestureTarget::DispatchWebInputEvent(
    std::unique_ptr<blink::WebInputEvent> event) {
  blink::WebMouseEvent* mouse;
  blink::WebGestureEvent* gesture;
  if (blink::WebInputEvent::IsMouseEventType(event->GetType())) {
    mouse = static_cast<blink::WebMouseEvent*>(event.get());
  } else {
    gesture = static_cast<blink::WebGestureEvent*>(event.get());
  }

  int64_t event_time_ms = event->TimeStamp().since_origin().InMilliseconds();
  switch (event->GetType()) {
    case blink::WebGestureEvent::kGestureScrollBegin: {
      DCHECK(gesture->data.scroll_begin.delta_hint_units ==
             blink::WebGestureEvent::ScrollUnits::kPrecisePixels);

      SetPointer(gesture->PositionInWidget().x, gesture->PositionInWidget().y);
      Inject(content::MOTION_EVENT_ACTION_START, event_time_ms);

      float xdiff = gesture->data.scroll_begin.delta_x_hint;
      float ydiff = gesture->data.scroll_begin.delta_y_hint;

      if (xdiff == 0 && ydiff == 0)
        ydiff = touch_slop_;
      double dist = std::sqrt((xdiff * xdiff) + (ydiff * ydiff));
      if (dist < touch_slop_) {
        xdiff *= touch_slop_ / dist;
        ydiff *= touch_slop_ / dist;
      }

      float xtarget = xdiff * scroll_ratio_ + gesture->PositionInWidget().x;
      float ytarget = ydiff * scroll_ratio_ + gesture->PositionInWidget().y;
      scroll_x_ = xtarget > 0 ? std::ceil(xtarget) : std::floor(xtarget);
      scroll_y_ = ytarget > 0 ? std::ceil(ytarget) : std::floor(ytarget);

      SetPointer(scroll_x_, scroll_y_);
      // Send a move immediately so that we can't accidentally trigger a click.
      Inject(content::MOTION_EVENT_ACTION_MOVE, event_time_ms);
      break;
    }
    case blink::WebGestureEvent::kGestureScrollEnd:
      SetPointer(scroll_x_, scroll_y_);
      Inject(content::MOTION_EVENT_ACTION_END, event_time_ms);
      break;
    case blink::WebGestureEvent::kGestureScrollUpdate:
      scroll_x_ += (scroll_ratio_ * gesture->data.scroll_update.delta_x);
      scroll_y_ += (scroll_ratio_ * gesture->data.scroll_update.delta_y);
      SetPointer(scroll_x_, scroll_y_);
      Inject(content::MOTION_EVENT_ACTION_MOVE, event_time_ms);
      break;
    case blink::WebGestureEvent::kGestureTapDown:
      SetPointer(gesture->PositionInWidget().x, gesture->PositionInWidget().y);
      Inject(content::MOTION_EVENT_ACTION_START, event_time_ms);
      Inject(content::MOTION_EVENT_ACTION_END, event_time_ms);
      break;
    case blink::WebGestureEvent::kGestureFlingCancel:
      Inject(content::MOTION_EVENT_ACTION_START, event_time_ms);
      Inject(content::MOTION_EVENT_ACTION_CANCEL, event_time_ms);
      break;
    case blink::WebMouseEvent::kMouseEnter:
      SetPointer(mouse->PositionInWidget().x, mouse->PositionInWidget().y);
      Inject(content::MOTION_EVENT_ACTION_HOVER_ENTER, event_time_ms);
      break;
    case blink::WebMouseEvent::kMouseMove:
    case blink::WebMouseEvent::kMouseLeave:
      // We don't need to inject MOTION_EVENT_ACTION_HOVER_EXIT as the platform
      // will generate it for us if the pointer is out of bounds.
      SetPointer(mouse->PositionInWidget().x, mouse->PositionInWidget().y);
      Inject(content::MOTION_EVENT_ACTION_HOVER_MOVE, event_time_ms);
      break;
    case blink::WebMouseEvent::kMouseDown:
      // Mouse down events are translated into touch events on Android anyways,
      // so we can just send touch events.
      // We intentionally don't support long press or drags/swipes with mouse
      // input as this could trigger long press and open 2D popups.
      SetPointer(mouse->PositionInWidget().x, mouse->PositionInWidget().y);
      Inject(content::MOTION_EVENT_ACTION_START, event_time_ms);
      Inject(content::MOTION_EVENT_ACTION_END, event_time_ms);
      break;
    case blink::WebMouseEvent::kMouseUp:
      // No need to do anything for mouseUp as mouseDown already handled up.
      break;
    default:
      NOTREACHED() << "Unsupported event type sent to Android UI.";
      break;
  }
}

void AndroidUiGestureTarget::Inject(MotionEventAction action, int64_t time_ms) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  Java_AndroidUiGestureTarget_inject(env, obj, action, time_ms);
}

void AndroidUiGestureTarget::SetPointer(int x, int y) {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj = java_ref_.get(env);
  if (obj.is_null())
    return;

  Java_AndroidUiGestureTarget_setPointer(env, obj, x * scale_factor_,
                                         y * scale_factor_);
}

// static
AndroidUiGestureTarget* AndroidUiGestureTarget::FromJavaObject(
    const JavaRef<jobject>& obj) {
  if (obj.is_null())
    return nullptr;

  JNIEnv* env = base::android::AttachCurrentThread();
  return reinterpret_cast<AndroidUiGestureTarget*>(
      Java_AndroidUiGestureTarget_getNativeObject(env, obj));
}

static jlong JNI_AndroidUiGestureTarget_Init(JNIEnv* env,
                                             const JavaParamRef<jobject>& obj,
                                             jfloat scale_factor,
                                             jfloat scroll_ratio,
                                             jint touch_slop) {
  return reinterpret_cast<intptr_t>(new AndroidUiGestureTarget(
      env, obj, scale_factor, scroll_ratio, touch_slop));
}

}  // namespace vr
