// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/autofill/password_generation_popup_view_android.h"

#include <jni.h>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/logging.h"
#include "chrome/browser/ui/android/view_android_helper.h"
#include "chrome/browser/ui/autofill/password_generation_popup_controller.h"
#include "jni/PasswordGenerationPopupBridge_jni.h"
#include "ui/android/view_android.h"
#include "ui/android/window_android.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/range/range.h"

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace autofill {

PasswordGenerationPopupViewAndroid::PasswordGenerationPopupViewAndroid(
    PasswordGenerationPopupController* controller)
    : controller_(controller) {}

void PasswordGenerationPopupViewAndroid::SavedPasswordsLinkClicked(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (controller_)
    controller_->OnSavedPasswordsLinkClicked();
}

void PasswordGenerationPopupViewAndroid::Dismissed(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (controller_)
    controller_->ViewDestroyed();

  delete this;
}

void PasswordGenerationPopupViewAndroid::PasswordSelected(
    JNIEnv* env,
    const JavaParamRef<jobject>& object) {
  if (controller_)
    controller_->PasswordAccepted();
}

PasswordGenerationPopupViewAndroid::~PasswordGenerationPopupViewAndroid() {}

void PasswordGenerationPopupViewAndroid::Show() {
  ui::ViewAndroid* view_android = controller_->container_view();

  DCHECK(view_android);

  popup_ = view_android->AcquireAnchorView();
  const ScopedJavaLocalRef<jobject> view = popup_.view();
  if (view.is_null())
    return;
  JNIEnv* env = base::android::AttachCurrentThread();
  java_object_.Reset(Java_PasswordGenerationPopupBridge_create(
      env, view, reinterpret_cast<intptr_t>(this),
      view_android->GetWindowAndroid()->GetJavaObject()));

  UpdateBoundsAndRedrawPopup();
}

void PasswordGenerationPopupViewAndroid::Hide() {
  controller_ = NULL;
  JNIEnv* env = base::android::AttachCurrentThread();
  if (!java_object_.is_null()) {
    Java_PasswordGenerationPopupBridge_hide(env, java_object_);
  } else {
    // Hide() should delete |this| either via Java dismiss or directly.
    delete this;
  }
}

gfx::Size PasswordGenerationPopupViewAndroid::GetPreferredSizeOfPasswordView() {
  static const int kUnusedSize = 0;
  return gfx::Size(kUnusedSize, kUnusedSize);
}

void PasswordGenerationPopupViewAndroid::UpdateBoundsAndRedrawPopup() {
  if (java_object_.is_null())
    return;

  const ScopedJavaLocalRef<jobject> view = popup_.view();
  if (view.is_null())
    return;

  ui::ViewAndroid* view_android = controller_->container_view();

  DCHECK(view_android);
  view_android->SetAnchorRect(view, controller_->element_bounds());
  JNIEnv* env = base::android::AttachCurrentThread();
  ScopedJavaLocalRef<jstring> password =
      base::android::ConvertUTF16ToJavaString(env, controller_->password());
  ScopedJavaLocalRef<jstring> suggestion =
      base::android::ConvertUTF16ToJavaString(
          env, controller_->SuggestedText());
  ScopedJavaLocalRef<jstring> help =
      base::android::ConvertUTF16ToJavaString(env, controller_->HelpText());

  Java_PasswordGenerationPopupBridge_show(
      env, java_object_, controller_->IsRTL(), controller_->display_password(),
      password, suggestion, help, controller_->HelpTextLinkRange().start(),
      controller_->HelpTextLinkRange().end());
}

void PasswordGenerationPopupViewAndroid::PasswordSelectionUpdated() {}

bool PasswordGenerationPopupViewAndroid::IsPointInPasswordBounds(
        const gfx::Point& point) {
  NOTREACHED();
  return false;
}

// static
PasswordGenerationPopupView* PasswordGenerationPopupView::Create(
    PasswordGenerationPopupController* controller) {
  return new PasswordGenerationPopupViewAndroid(controller);
}

}  // namespace autofill
