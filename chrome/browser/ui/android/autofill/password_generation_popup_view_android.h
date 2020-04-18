// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ANDROID_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_ANDROID_H_
#define CHROME_BROWSER_UI_ANDROID_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_ANDROID_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/ui/autofill/password_generation_popup_view.h"
#include "ui/android/view_android.h"

namespace autofill {

class PasswordGenerationPopupController;

// The android implementation of the password generation UI.
class PasswordGenerationPopupViewAndroid : public PasswordGenerationPopupView {
 public:
  // Builds the UI for the |controller|.
  explicit PasswordGenerationPopupViewAndroid(
      PasswordGenerationPopupController* controller);

  // Called from JNI when the "saved passwords" link was clicked.
  void SavedPasswordsLinkClicked(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& obj);

  // Called from JNI when the popup was dismissed.
  void Dismissed(JNIEnv* env, const base::android::JavaParamRef<jobject>& obj);

  // Called from JNI when the suggested password was selected.
  void PasswordSelected(JNIEnv* env,
                        const base::android::JavaParamRef<jobject>& object);

 private:
  // The popup owns itself.
  virtual ~PasswordGenerationPopupViewAndroid();

  // PasswordGenerationPopupView implementation.
  void Show() override;
  void Hide() override;
  gfx::Size GetPreferredSizeOfPasswordView() override;
  void UpdateBoundsAndRedrawPopup() override;
  void PasswordSelectionUpdated() override;
  bool IsPointInPasswordBounds(const gfx::Point& point) override;

  // Weak pointer to the controller.
  PasswordGenerationPopupController* controller_;

  // The corresponding java object.
  base::android::ScopedJavaGlobalRef<jobject> java_object_;

  // Popup view to be anchored to the container.
  ui::ViewAndroid::ScopedAnchorView popup_;

  DISALLOW_COPY_AND_ASSIGN(PasswordGenerationPopupViewAndroid);
};

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_ANDROID_AUTOFILL_PASSWORD_GENERATION_POPUP_VIEW_ANDROID_H_
