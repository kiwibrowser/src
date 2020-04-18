// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/init_native_callback.h"

#include "android_webview/browser/android_protocol_handler.h"
#include "android_webview/browser/net/aw_url_request_job_factory.h"
#include "base/callback.h"
#include "base/single_thread_task_runner.h"
#include "net/url_request/url_request_interceptor.h"

namespace android_webview {

void PostTaskToCookieStoreTaskRunner(base::OnceClosure task) {
  GetCookieStoreTaskRunner()->PostTask(FROM_HERE, std::move(task));
}

std::unique_ptr<net::URLRequestInterceptor>
CreateAndroidAssetFileRequestInterceptor() {
  return CreateAssetFileRequestInterceptor();
}

std::unique_ptr<net::URLRequestInterceptor>
CreateAndroidContentRequestInterceptor() {
  return CreateContentSchemeRequestInterceptor();
}

}  // namespace android_webview
