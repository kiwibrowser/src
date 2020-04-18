// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/select_popup.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/browser/web_contents/web_contents_view_android.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/menu_item.h"
#include "content/public/common/use_zoom_for_dsf_policy.h"
#include "jni/SelectPopup_jni.h"
#include "ui/android/window_android.h"
#include "ui/gfx/geometry/rect_f.h"

using base::android::AttachCurrentThread;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

namespace content {

namespace {

// Describes the type and enabled state of a select popup item.
//
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.content.browser.input
enum PopupItemType {
  // Popup item is of type group
  POPUP_ITEM_TYPE_GROUP,

  // Popup item is disabled
  POPUP_ITEM_TYPE_DISABLED,

  // Popup item is enabled
  POPUP_ITEM_TYPE_ENABLED,
};

}  // namespace

jlong JNI_SelectPopup_Init(JNIEnv* env,
                           const JavaParamRef<jobject>& obj,
                           const JavaParamRef<jobject>& jweb_contents) {
  WebContents* web_contents = WebContents::FromJavaWebContents(jweb_contents);
  DCHECK(web_contents);

  auto* wc_impl = static_cast<WebContentsImpl*>(web_contents);
  auto* wcv = static_cast<WebContentsViewAndroid*>(wc_impl->GetView());

  // Owned by |WebContentsViewAndroid|.
  auto select_popup = std::make_unique<SelectPopup>(env, obj, wc_impl);
  SelectPopup* select_popup_ptr = select_popup.get();
  wcv->SetSelectPopup(std::move(select_popup));
  return reinterpret_cast<intptr_t>(select_popup_ptr);
}

SelectPopup::SelectPopup(JNIEnv* env,
                         const JavaParamRef<jobject>& obj,
                         WebContentsImpl* web_contents)
    : web_contents_(web_contents), java_obj_(env, obj) {}

SelectPopup::~SelectPopup() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_obj = java_obj_.get(env);
  if (j_obj.is_null())
    return;
  Java_SelectPopup_destroy(env, j_obj);
}

void SelectPopup::ShowMenu(RenderFrameHost* frame,
                           const gfx::Rect& bounds,
                           const std::vector<MenuItem>& items,
                           int selected_item,
                           bool multiple,
                           bool right_aligned) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_obj = java_obj_.get(env);
  if (j_obj.is_null())
    return;

  // For multi-select list popups we find the list of previous selections by
  // iterating through the items. But for single selection popups we take the
  // given |selected_item| as is.
  ScopedJavaLocalRef<jintArray> selected_array;
  if (multiple) {
    std::unique_ptr<jint[]> native_selected_array(new jint[items.size()]);
    size_t selected_count = 0;
    for (size_t i = 0; i < items.size(); ++i) {
      if (items[i].checked)
        native_selected_array[selected_count++] = i;
    }

    selected_array =
        ScopedJavaLocalRef<jintArray>(env, env->NewIntArray(selected_count));
    env->SetIntArrayRegion(selected_array.obj(), 0, selected_count,
                           native_selected_array.get());
  } else {
    selected_array = ScopedJavaLocalRef<jintArray>(env, env->NewIntArray(1));
    jint value = selected_item;
    env->SetIntArrayRegion(selected_array.obj(), 0, 1, &value);
  }

  ScopedJavaLocalRef<jintArray> enabled_array(env,
                                              env->NewIntArray(items.size()));
  std::vector<base::string16> labels;
  labels.reserve(items.size());
  for (size_t i = 0; i < items.size(); ++i) {
    labels.push_back(items[i].label);
    jint enabled = (items[i].type == MenuItem::GROUP
                        ? POPUP_ITEM_TYPE_GROUP
                        : (items[i].enabled ? POPUP_ITEM_TYPE_ENABLED
                                            : POPUP_ITEM_TYPE_DISABLED));
    env->SetIntArrayRegion(enabled_array.obj(), i, 1, &enabled);
  }
  ScopedJavaLocalRef<jobjectArray> items_array(
      base::android::ToJavaArrayOfStrings(env, labels));
  ui::ViewAndroid* view = web_contents_->GetNativeView();
  popup_view_ = view->AcquireAnchorView();
  const ScopedJavaLocalRef<jobject> popup_view = popup_view_.view();
  if (popup_view.is_null())
    return;
  // |bounds| is in physical pixels if --use-zoom-for-dsf is enabled. Otherwise,
  // it is in DIP pixels.
  gfx::RectF bounds_dip = gfx::RectF(bounds);
  if (IsUseZoomForDSFEnabled())
    bounds_dip.Scale(1 / web_contents_->GetNativeView()->GetDipScale());
  view->SetAnchorRect(popup_view, bounds_dip);
  Java_SelectPopup_show(env, j_obj, popup_view,
                        reinterpret_cast<intptr_t>(frame), items_array,
                        enabled_array, multiple, selected_array, right_aligned);
}

void SelectPopup::HideMenu() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jobject> j_obj = java_obj_.get(env);
  if (!j_obj.is_null())
    Java_SelectPopup_hideWithoutCancel(env, j_obj);
  popup_view_.Reset();
}

void SelectPopup::SelectMenuItems(JNIEnv* env,
                                  const JavaParamRef<jobject>& obj,
                                  jlong selectPopupSourceFrame,
                                  const JavaParamRef<jintArray>& indices) {
  RenderFrameHostImpl* rfhi =
      reinterpret_cast<RenderFrameHostImpl*>(selectPopupSourceFrame);
  DCHECK(rfhi);
  if (indices == NULL) {
    rfhi->DidCancelPopupMenu();
    return;
  }

  int selected_count = env->GetArrayLength(indices);
  std::vector<int> selected_indices;
  jint* indices_ptr = env->GetIntArrayElements(indices, NULL);
  for (int i = 0; i < selected_count; ++i)
    selected_indices.push_back(indices_ptr[i]);
  env->ReleaseIntArrayElements(indices, indices_ptr, JNI_ABORT);
  rfhi->DidSelectPopupMenuItems(selected_indices);
}

base::android::ScopedJavaLocalRef<jobject> SelectPopup::GetWindowAndroid(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  auto* window = web_contents_->GetNativeView()->GetWindowAndroid();
  return window ? window->GetJavaObject() : ScopedJavaLocalRef<jobject>();
}

}  // namespace content
