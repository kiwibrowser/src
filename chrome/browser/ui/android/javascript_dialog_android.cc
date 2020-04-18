// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/android/javascript_dialog_android.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/android/tab_android.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "jni/JavascriptTabModalDialog_jni.h"
#include "ui/android/window_android.h"

using base::android::AttachCurrentThread;
using base::android::ConvertUTF16ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;

// JavaScriptDialog:
JavaScriptDialog::JavaScriptDialog(content::WebContents* parent_web_contents) {
  parent_web_contents->GetDelegate()->ActivateContents(parent_web_contents);
}

JavaScriptDialog::~JavaScriptDialog() = default;

base::WeakPtr<JavaScriptDialog> JavaScriptDialog::Create(
    content::WebContents* parent_web_contents,
    content::WebContents* alerting_web_contents,
    const base::string16& title,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback dialog_callback) {
  return JavaScriptDialogAndroid::Create(
      parent_web_contents, alerting_web_contents, title, dialog_type,
      message_text, default_prompt_text, std::move(dialog_callback));
}

// JavaScriptDialogAndroid:
JavaScriptDialogAndroid::~JavaScriptDialogAndroid() {
  // In case the dialog is still displaying, tell it to close itself.
  // This can happen if you trigger a dialog but close the Tab before it's
  // shown, and then accept the dialog.
  if (!dialog_jobject_.is_null()) {
    Java_JavascriptTabModalDialog_dismiss(AttachCurrentThread(),
                                          dialog_jobject_);
  }
}

base::WeakPtr<JavaScriptDialogAndroid> JavaScriptDialogAndroid::Create(
    content::WebContents* parent_web_contents,
    content::WebContents* alerting_web_contents,
    const base::string16& title,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback dialog_callback) {
  return (new JavaScriptDialogAndroid(
              parent_web_contents, alerting_web_contents, title, dialog_type,
              message_text, default_prompt_text, std::move(dialog_callback)))
      ->weak_factory_.GetWeakPtr();
}

void JavaScriptDialogAndroid::CloseDialogWithoutCallback() {
  delete this;
}

base::string16 JavaScriptDialogAndroid::GetUserInput() {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> prompt =
      Java_JavascriptTabModalDialog_getUserInput(env, dialog_jobject_);
  return base::android::ConvertJavaStringToUTF16(env, prompt);
}

void JavaScriptDialogAndroid::Accept(JNIEnv* env,
                                     const JavaParamRef<jobject>&,
                                     const JavaParamRef<jstring>& prompt) {
  if (dialog_callback_) {
    base::string16 prompt_text =
        base::android::ConvertJavaStringToUTF16(env, prompt);
    std::move(dialog_callback_).Run(true, prompt_text);
    std::move(dialog_callback_).Reset();
  }
  delete this;
}

void JavaScriptDialogAndroid::Cancel(JNIEnv* env,
                                     const JavaParamRef<jobject>&) {
  if (dialog_callback_) {
    std::move(dialog_callback_).Run(false, base::string16());
    std::move(dialog_callback_).Reset();
  }
  delete this;
}

JavaScriptDialogAndroid::JavaScriptDialogAndroid(
    content::WebContents* parent_web_contents,
    content::WebContents* alerting_web_contents,
    const base::string16& title,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    content::JavaScriptDialogManager::DialogClosedCallback dialog_callback)
    : JavaScriptDialog(parent_web_contents),
      dialog_callback_(std::move(dialog_callback)),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  JNIEnv* env = AttachCurrentThread();
  jwindow_weak_ref_ = JavaObjectWeakGlobalRef(
      env,
      parent_web_contents->GetTopLevelNativeWindow()->GetJavaObject().obj());

  // Keep a strong ref to the parent window while we make the call to java to
  // display the dialog.
  ScopedJavaLocalRef<jobject> jwindow = jwindow_weak_ref_.get(env);

  ScopedJavaLocalRef<jobject> dialog_object;
  ScopedJavaLocalRef<jstring> title_ref = ConvertUTF16ToJavaString(env, title);
  ScopedJavaLocalRef<jstring> message_ref =
      ConvertUTF16ToJavaString(env, message_text);

  switch (dialog_type) {
    case content::JAVASCRIPT_DIALOG_TYPE_ALERT: {
      dialog_object = Java_JavascriptTabModalDialog_createAlertDialog(
          env, title_ref, message_ref);
      break;
    }
    case content::JAVASCRIPT_DIALOG_TYPE_CONFIRM: {
      dialog_object = Java_JavascriptTabModalDialog_createConfirmDialog(
          env, title_ref, message_ref);
      break;
    }
    case content::JAVASCRIPT_DIALOG_TYPE_PROMPT: {
      ScopedJavaLocalRef<jstring> default_prompt_text_ref =
          ConvertUTF16ToJavaString(env, default_prompt_text);
      dialog_object = Java_JavascriptTabModalDialog_createPromptDialog(
          env, title_ref, message_ref, default_prompt_text_ref);
      break;
    }
    default:
      NOTREACHED();
  }

  // Keep a ref to the java side object until we get accept or cancel.
  dialog_jobject_.Reset(dialog_object);

  Java_JavascriptTabModalDialog_showDialog(env, dialog_object, jwindow,
                                           reinterpret_cast<intptr_t>(this));
}
