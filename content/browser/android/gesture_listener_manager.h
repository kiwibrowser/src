// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ANDROID_GESTURE_LISTENER_MANAGER_H_
#define CONTENT_BROWSER_ANDROID_GESTURE_LISTENER_MANAGER_H_

#include "base/android/jni_weak_ref.h"
#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "content/browser/android/render_widget_host_connector.h"
#include "content/public/common/input_event_ack_state.h"

namespace blink {
class WebGestureEvent;
}

namespace gfx {
class SizeF;
class Vector2dF;
}  // namespace gfx

namespace content {

class NavigationHandle;
class WebContentsImpl;

// Native class for GestureListenerManagerImpl.
class GestureListenerManager : public RenderWidgetHostConnector {
 public:
  GestureListenerManager(JNIEnv* env,
                         const base::android::JavaParamRef<jobject>& obj,
                         WebContentsImpl* web_contents);
  ~GestureListenerManager() override;

  void Reset(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);
  void ResetGestureDetection(JNIEnv* env,
                             const base::android::JavaParamRef<jobject>& obj);
  void SetDoubleTapSupportEnabled(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jboolean enabled);
  void SetMultiTouchZoomSupportEnabled(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj,
      jboolean enabled);
  void GestureEventAck(const blink::WebGestureEvent& event,
                       InputEventAckState ack_result);
  void DidStopFlinging();
  bool FilterInputEvent(const blink::WebInputEvent& event);

  // All sizes and offsets are in CSS pixels (except |top_show_pix|)
  // as cached by the renderer.
  void UpdateScrollInfo(const gfx::Vector2dF& scroll_offset,
                        float page_scale_factor,
                        const float min_page_scale,
                        const float max_page_scale,
                        const gfx::SizeF& content,
                        const gfx::SizeF& viewport,
                        const float content_offset,
                        const float top_shown_pix,
                        bool top_changed);
  void UpdateOnTouchDown();

  // RendetWidgetHostConnector implementation.
  void UpdateRenderProcessConnection(
      RenderWidgetHostViewAndroid* old_rwhva,
      RenderWidgetHostViewAndroid* new_rhwva) override;

  void OnNavigationFinished(NavigationHandle* navigation_handle);
  void OnRenderProcessGone();

 private:
  class ResetScrollObserver;

  void ResetPopupsAndInput(bool render_process_gone);

  std::unique_ptr<ResetScrollObserver> reset_scroll_observer_;
  WebContentsImpl* web_contents_;
  RenderWidgetHostViewAndroid* rwhva_ = nullptr;

  // A weak reference to the Java GestureListenerManager object.
  JavaObjectWeakGlobalRef java_ref_;

  DISALLOW_COPY_AND_ASSIGN(GestureListenerManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ANDROID_GESTURE_LISTENER_MANAGER_H_
