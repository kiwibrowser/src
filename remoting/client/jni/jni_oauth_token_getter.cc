// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/jni/jni_oauth_token_getter.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "jni/JniOAuthTokenGetter_jni.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::JavaParamRef;

namespace remoting {

static void JNI_JniOAuthTokenGetter_ResolveOAuthTokenCallback(
    JNIEnv* env,
    const JavaParamRef<jclass>& clazz,
    jlong callback_ptr,
    jint jni_status,
    const JavaParamRef<jstring>& user_email,
    const JavaParamRef<jstring>& token) {
  auto* callback =
      reinterpret_cast<OAuthTokenGetter::TokenCallback*>(callback_ptr);
  OAuthTokenGetter::Status status;
  switch (static_cast<JniOAuthTokenGetter::JniStatus>(jni_status)) {
    case JniOAuthTokenGetter::JNI_STATUS_SUCCESS:
      status = OAuthTokenGetter::SUCCESS;
      break;
    case JniOAuthTokenGetter::JNI_STATUS_NETWORK_ERROR:
      status = OAuthTokenGetter::NETWORK_ERROR;
      break;
    case JniOAuthTokenGetter::JNI_STATUS_AUTH_ERROR:
      status = OAuthTokenGetter::AUTH_ERROR;
      break;
    default:
      NOTREACHED();
      return;
  }
  callback->Run(status,
                user_email.is_null() ? "" : ConvertJavaStringToUTF8(user_email),
                token.is_null() ? "" : ConvertJavaStringToUTF8(token));
  delete callback;
}

JniOAuthTokenGetter::JniOAuthTokenGetter() : weak_factory_(this) {
  DETACH_FROM_THREAD(thread_checker_);
  weak_ptr_ = weak_factory_.GetWeakPtr();
}

JniOAuthTokenGetter::~JniOAuthTokenGetter() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void JniOAuthTokenGetter::CallWithToken(const TokenCallback& on_access_token) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  JNIEnv* env = base::android::AttachCurrentThread();
  TokenCallback* callback_copy = new TokenCallback(on_access_token);
  Java_JniOAuthTokenGetter_fetchAuthToken(
      env, reinterpret_cast<intptr_t>(callback_copy));
}

void JniOAuthTokenGetter::InvalidateCache() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_JniOAuthTokenGetter_invalidateCache(env);
}

base::WeakPtr<JniOAuthTokenGetter> JniOAuthTokenGetter::GetWeakPtr() {
  return weak_ptr_;
}

}  // namespace remoting
