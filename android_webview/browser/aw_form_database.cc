// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_browser_context.h"
#include "android_webview/browser/aw_content_browser_client.h"
#include "android_webview/browser/aw_form_database_service.h"
#include "base/android/jni_android.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "jni/AwFormDatabase_jni.h"

using base::android::JavaParamRef;

namespace android_webview {

namespace {

AwFormDatabaseService* GetFormDatabaseService() {
  AwBrowserContext* context = AwContentBrowserClient::GetAwBrowserContext();
  AwFormDatabaseService* service = context->GetFormDatabaseService();
  return service;
}

}  // anonymous namespace

// static
jboolean JNI_AwFormDatabase_HasFormData(JNIEnv*, const JavaParamRef<jclass>&) {
  return GetFormDatabaseService()->HasFormData();
}

// static
void JNI_AwFormDatabase_ClearFormData(JNIEnv*, const JavaParamRef<jclass>&) {
  GetFormDatabaseService()->ClearFormData();
}

}  // namespace android_webview
