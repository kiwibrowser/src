// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/autofill/autofill_keyboard_accessory_view.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "chrome/browser/android/resource_mapper.h"
#include "chrome/browser/ui/android/view_android_helper.h"
#include "chrome/browser/ui/autofill/autofill_popup_controller.h"
#include "chrome/browser/ui/autofill/autofill_popup_layout_model.h"
#include "components/autofill/core/browser/popup_item_ids.h"
#include "components/autofill/core/browser/suggestion.h"
#include "jni/AutofillKeyboardAccessoryBridge_jni.h"
#include "ui/android/view_android.h"
#include "ui/android/window_android.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/rect.h"

using base::android::JavaParamRef;
using base::android::JavaRef;
using base::android::ScopedJavaLocalRef;

namespace autofill {

namespace {

void AddToJavaArray(const Suggestion& suggestion,
                    int icon_id,
                    JNIEnv* env,
                    const JavaRef<jobjectArray>& data_array,
                    size_t position,
                    bool deletable) {
  int android_icon_id = 0;
  if (!suggestion.icon.empty())
    android_icon_id = ResourceMapper::MapFromChromiumId(icon_id);

  Java_AutofillKeyboardAccessoryBridge_addToAutofillSuggestionArray(
      env, data_array, position,
      base::android::ConvertUTF16ToJavaString(env, suggestion.value),
      base::android::ConvertUTF16ToJavaString(env, suggestion.label),
      android_icon_id, suggestion.frontend_id, deletable);
}

}  // namespace

AutofillKeyboardAccessoryView::AutofillKeyboardAccessoryView(
    AutofillPopupController* controller,
    unsigned int animation_duration_millis,
    bool should_limit_label_width)
    : controller_(controller),
      animation_duration_millis_(animation_duration_millis),
      should_limit_label_width_(should_limit_label_width),
      deleting_index_(-1) {
  JNIEnv* env = base::android::AttachCurrentThread();
  java_object_.Reset(Java_AutofillKeyboardAccessoryBridge_create(env));
}

AutofillKeyboardAccessoryView::~AutofillKeyboardAccessoryView() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_AutofillKeyboardAccessoryBridge_resetNativeViewPointer(env,
                                                              java_object_);
}

void AutofillKeyboardAccessoryView::Show() {
  JNIEnv* env = base::android::AttachCurrentThread();
  ui::ViewAndroid* view_android = controller_->container_view();
  DCHECK(view_android);
  Java_AutofillKeyboardAccessoryBridge_init(
      env, java_object_, reinterpret_cast<intptr_t>(this),
      view_android->GetWindowAndroid()->GetJavaObject(),
      animation_duration_millis_, should_limit_label_width_);

  OnSuggestionsChanged();
}

void AutofillKeyboardAccessoryView::Hide() {
  controller_ = nullptr;
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_AutofillKeyboardAccessoryBridge_dismiss(env, java_object_);
}

void AutofillKeyboardAccessoryView::OnSelectedRowChanged(
    base::Optional<int> previous_row_selection,
    base::Optional<int> current_row_selection) {}

void AutofillKeyboardAccessoryView::OnSuggestionsChanged() {
  JNIEnv* env = base::android::AttachCurrentThread();
  size_t count = controller_->GetLineCount();
  ScopedJavaLocalRef<jobjectArray> data_array =
      Java_AutofillKeyboardAccessoryBridge_createAutofillSuggestionArray(env,
                                                                         count);
  positions_.resize(count);
  size_t position = 0;

  // Place "CLEAR FORM" and "CREATE HINT" items first in the list.
  // Both "CLEAR FORM" and "CREATE HINT" cannot be present in the list.
  for (size_t i = 0; i < count; ++i) {
    const Suggestion& suggestion = controller_->GetSuggestionAt(i);
    if (suggestion.frontend_id == POPUP_ITEM_ID_CLEAR_FORM ||
        suggestion.frontend_id == POPUP_ITEM_ID_CREATE_HINT) {
      AddToJavaArray(
          suggestion,
          controller_->layout_model().GetIconResourceID(suggestion.icon), env,
          data_array, position, false);
      positions_[position++] = i;
    }
  }

  DCHECK_LT(position, 2U);

  for (size_t i = 0; i < count; ++i) {
    const Suggestion& suggestion = controller_->GetSuggestionAt(i);
    if (suggestion.frontend_id != POPUP_ITEM_ID_CLEAR_FORM &&
        suggestion.frontend_id != POPUP_ITEM_ID_CREATE_HINT) {
      bool deletable =
          controller_->GetRemovalConfirmationText(i, nullptr, nullptr);
      AddToJavaArray(
          suggestion,
          controller_->layout_model().GetIconResourceID(suggestion.icon), env,
          data_array, position, deletable);
      positions_[position++] = i;
    }
  }

  Java_AutofillKeyboardAccessoryBridge_show(env, java_object_, data_array,
                                            controller_->IsRTL());
}

void AutofillKeyboardAccessoryView::SuggestionSelected(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint list_index) {
  // Race: Hide() may have already run.
  if (controller_)
    controller_->AcceptSuggestion(positions_[list_index]);
}

void AutofillKeyboardAccessoryView::DeletionRequested(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    jint list_index) {
  if (!controller_)
    return;

  base::string16 confirmation_title, confirmation_body;
  if (!controller_->GetRemovalConfirmationText(
          positions_[list_index], &confirmation_title, &confirmation_body)) {
    return;
  }

  deleting_index_ = positions_[list_index];
  Java_AutofillKeyboardAccessoryBridge_confirmDeletion(
      env, java_object_,
      base::android::ConvertUTF16ToJavaString(env, confirmation_title),
      base::android::ConvertUTF16ToJavaString(env, confirmation_body));
}

void AutofillKeyboardAccessoryView::DeletionConfirmed(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (!controller_)
    return;

  CHECK_GE(deleting_index_, 0);
  controller_->RemoveSuggestion(deleting_index_);
}

void AutofillKeyboardAccessoryView::ViewDismissed(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  if (controller_)
    controller_->ViewDestroyed();

  delete this;
}

}  // namespace autofill
