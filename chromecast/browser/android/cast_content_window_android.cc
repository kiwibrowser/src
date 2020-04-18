// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/android/cast_content_window_android.h"

#include <memory>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/web_contents.h"
#include "jni/CastContentWindowAndroid_jni.h"
#include "ui/events/keycodes/keyboard_code_conversion_android.h"

namespace chromecast {

using base::android::ConvertUTF8ToJavaString;

namespace shell {

namespace {

base::android::ScopedJavaLocalRef<jobject> CreateJavaWindow(
    jlong native_window,
    bool is_headless,
    bool enable_touch_input) {
  JNIEnv* env = base::android::AttachCurrentThread();
  return Java_CastContentWindowAndroid_create(env, native_window, is_headless,
                                              enable_touch_input);
}

}  // namespace

// static
std::unique_ptr<CastContentWindow> CastContentWindow::Create(
    CastContentWindow::Delegate* delegate,
    bool is_headless,
    bool enable_touch_input) {
  return base::WrapUnique(
      new CastContentWindowAndroid(delegate, is_headless, enable_touch_input));
}

CastContentWindowAndroid::CastContentWindowAndroid(
    CastContentWindow::Delegate* delegate,
    bool is_headless,
    bool enable_touch_input)
    : delegate_(delegate),
      java_window_(CreateJavaWindow(reinterpret_cast<jlong>(this),
                                    is_headless,
                                    enable_touch_input)) {
  DCHECK(delegate_);
}

CastContentWindowAndroid::~CastContentWindowAndroid() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_CastContentWindowAndroid_onNativeDestroyed(env, java_window_);
}

void CastContentWindowAndroid::CreateWindowForWebContents(
    content::WebContents* web_contents,
    CastWindowManager* /* window_manager */,
    bool /* is_visible */,
    CastWindowManager::WindowId /* z_order */,
    VisibilityPriority visibility_priority) {
  DCHECK(web_contents);
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> java_web_contents =
      web_contents->GetJavaWebContents();

  Java_CastContentWindowAndroid_createWindowForWebContents(
      env, java_window_, java_web_contents,
      static_cast<int>(visibility_priority));
}

void CastContentWindowAndroid::EnableTouchInput(bool enabled) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_CastContentWindowAndroid_enableTouchInput(
      env, java_window_, static_cast<jboolean>(enabled));
}

void CastContentWindowAndroid::OnActivityStopped(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  delegate_->OnWindowDestroyed();
}

void CastContentWindowAndroid::OnKeyDown(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    int keycode) {
  ui::KeyEvent key_event(ui::ET_KEY_PRESSED,
                         ui::KeyboardCodeFromAndroidKeyCode(keycode),
                         ui::EF_NONE);
  delegate_->OnKeyEvent(key_event);
}

void CastContentWindowAndroid::RequestVisibility(
    VisibilityPriority visibility_priority) {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_CastContentWindowAndroid_requestVisibilityPriority(
      env, java_window_, static_cast<int>(visibility_priority));
}

void CastContentWindowAndroid::RequestMoveOut() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_CastContentWindowAndroid_requestMoveOut(env, java_window_);
}

bool CastContentWindowAndroid::ConsumeGesture(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    int gesture_type) {
  return delegate_->ConsumeGesture(static_cast<GestureType>(gesture_type));
}

void CastContentWindowAndroid::OnVisibilityChange(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller,
    int visibility_type) {
  delegate_->OnVisibilityChange(static_cast<VisibilityType>(visibility_type));
}

base::android::ScopedJavaLocalRef<jstring> CastContentWindowAndroid::GetId(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  return ConvertUTF8ToJavaString(env, delegate_->GetId());
}

}  // namespace shell
}  // namespace chromecast
