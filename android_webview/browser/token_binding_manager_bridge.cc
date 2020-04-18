// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/token_binding_manager.h"
#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/ec_private_key.h"
#include "jni/AwTokenBindingManager_jni.h"
#include "net/base/net_errors.h"

using base::android::ConvertJavaStringToUTF8;
using base::android::JavaParamRef;
using base::android::ScopedJavaGlobalRef;
using base::android::ScopedJavaLocalRef;
using content::BrowserThread;

namespace android_webview {

namespace {

// Provides the key to the Webview client.
void OnKeyReady(const ScopedJavaGlobalRef<jobject>& callback,
                int status,
                crypto::ECPrivateKey* key) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  JNIEnv* env = base::android::AttachCurrentThread();

  if (status != net::OK || !key) {
    Java_AwTokenBindingManager_onKeyReady(env, callback, nullptr, nullptr);
    return;
  }

  std::vector<uint8_t> private_key;
  key->ExportPrivateKey(&private_key);
  ScopedJavaLocalRef<jbyteArray> jprivate_key = base::android::ToJavaByteArray(
      env, private_key.data(), private_key.size());

  std::vector<uint8_t> public_key;
  key->ExportPublicKey(&public_key);
  ScopedJavaLocalRef<jbyteArray> jpublic_key =
      base::android::ToJavaByteArray(env, public_key.data(), public_key.size());

  Java_AwTokenBindingManager_onKeyReady(env, callback, jprivate_key,
                                        jpublic_key);
}

// Indicates webview client that key deletion is complete.
void OnDeletionComplete(const ScopedJavaGlobalRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (callback.is_null())
    return;
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_AwTokenBindingManager_onDeletionComplete(env, callback);
}

}  // namespace

static void JNI_AwTokenBindingManager_EnableTokenBinding(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj) {
  // This needs to be called before starting chromium engine, so no thread
  // checks for UI.
  TokenBindingManager::GetInstance()->enable_token_binding();
}

static void JNI_AwTokenBindingManager_GetTokenBindingKey(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& host,
    const JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Store the Java callback in a scoped object and give the ownership to
  // base::Callback
  ScopedJavaGlobalRef<jobject> j_callback;
  j_callback.Reset(env, callback);

  TokenBindingManager::KeyReadyCallback key_callback =
      base::BindOnce(&OnKeyReady, j_callback);
  TokenBindingManager::GetInstance()->GetKey(ConvertJavaStringToUTF8(env, host),
                                             std::move(key_callback));
}

static void JNI_AwTokenBindingManager_DeleteTokenBindingKey(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jstring>& host,
    const JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Store the Java callback in a scoped object and give the ownership to
  // base::Callback
  ScopedJavaGlobalRef<jobject> j_callback;
  j_callback.Reset(env, callback);
  TokenBindingManager::DeletionCompleteCallback complete_callback =
      base::BindOnce(&OnDeletionComplete, j_callback);
  TokenBindingManager::GetInstance()->DeleteKey(
      ConvertJavaStringToUTF8(env, host), std::move(complete_callback));
}

static void JNI_AwTokenBindingManager_DeleteAllTokenBindingKeys(
    JNIEnv* env,
    const JavaParamRef<jobject>& obj,
    const JavaParamRef<jobject>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // Store the Java callback in a scoped object and give the ownership to
  // base::Callback
  ScopedJavaGlobalRef<jobject> j_callback;
  j_callback.Reset(env, callback);
  TokenBindingManager::DeletionCompleteCallback complete_callback =
      base::BindOnce(&OnDeletionComplete, j_callback);
  TokenBindingManager::GetInstance()->DeleteAllKeys(
      std::move(complete_callback));
}

}  // namespace android_webview
