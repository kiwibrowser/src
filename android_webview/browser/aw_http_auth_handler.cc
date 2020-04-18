// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_http_auth_handler.h"

#include "android_webview/browser/aw_contents.h"
#include "android_webview/browser/aw_login_delegate.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "jni/AwHttpAuthHandler_jni.h"
#include "net/base/auth.h"

using base::android::ConvertJavaStringToUTF16;
using base::android::JavaParamRef;
using content::BrowserThread;

namespace android_webview {

AwHttpAuthHandler::AwHttpAuthHandler(AwLoginDelegate* login_delegate,
                                     net::AuthChallengeInfo* auth_info,
                                     bool first_auth_attempt)
    : login_delegate_(login_delegate),
      host_(auth_info->challenger.host()),
      realm_(auth_info->realm) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  JNIEnv* env = base::android::AttachCurrentThread();
  http_auth_handler_.Reset(Java_AwHttpAuthHandler_create(
      env, reinterpret_cast<intptr_t>(this), first_auth_attempt));
}

AwHttpAuthHandler::~AwHttpAuthHandler() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  Java_AwHttpAuthHandler_handlerDestroyed(base::android::AttachCurrentThread(),
                                          http_auth_handler_);
}

void AwHttpAuthHandler::Proceed(JNIEnv* env,
                                const JavaParamRef<jobject>& obj,
                                const JavaParamRef<jstring>& user,
                                const JavaParamRef<jstring>& password) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (login_delegate_.get()) {
    login_delegate_->Proceed(ConvertJavaStringToUTF16(env, user),
                             ConvertJavaStringToUTF16(env, password));
    login_delegate_ = NULL;
  }
}

void AwHttpAuthHandler::Cancel(JNIEnv* env, const JavaParamRef<jobject>& obj) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (login_delegate_.get()) {
    login_delegate_->Cancel();
    login_delegate_ = NULL;
  }
}

bool AwHttpAuthHandler::HandleOnUIThread(content::WebContents* web_contents) {
  DCHECK(web_contents);
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  AwContents* aw_contents = AwContents::FromWebContents(web_contents);

  return aw_contents->OnReceivedHttpAuthRequest(http_auth_handler_, host_,
                                                realm_);
}

}  // namespace android_webview
