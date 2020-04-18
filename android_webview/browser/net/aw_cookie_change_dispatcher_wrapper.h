// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_NET_AW_COOKIE_CHANGE_DISPATCHER_WRAPPER_H_
#define ANDROID_WEBVIEW_BROWSER_NET_AW_COOKIE_CHANGE_DISPATCHER_WRAPPER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/single_thread_task_runner.h"
#include "net/cookies/cookie_change_dispatcher.h"

class GURL;

namespace android_webview {

// CookieChangeDispatcher implementation used by AwCookieStoreWrapper.
//
// AwCookieStoreWrapper wraps a CookieStore that lives on a different thread and
// proxies calls and callbacks between the originating thread and the
// CookieStore's thread via task posting.
//
// By extension, AwCookieChangeDispatcherWrapper proxies cookie change
// notifications between the CookieStore's thread and the thread that subscribes
// to the notifications.
class AwCookieChangeDispatcherWrapper : public net::CookieChangeDispatcher {
 public:
  AwCookieChangeDispatcherWrapper();
  ~AwCookieChangeDispatcherWrapper() override;

  // net::CookieChangeDispatcher
  std::unique_ptr<net::CookieChangeSubscription> AddCallbackForCookie(
      const GURL& url,
      const std::string& name,
      net::CookieChangeCallback callback) override WARN_UNUSED_RESULT;
  std::unique_ptr<net::CookieChangeSubscription> AddCallbackForUrl(
      const GURL& url,
      net::CookieChangeCallback callback) override WARN_UNUSED_RESULT;
  std::unique_ptr<net::CookieChangeSubscription> AddCallbackForAllChanges(
      net::CookieChangeCallback callback) override WARN_UNUSED_RESULT;

 private:
#if DCHECK_IS_ON()
  scoped_refptr<base::TaskRunner> client_task_runner_;
#endif  // DCHECK_IS_ON()

  DISALLOW_COPY_AND_ASSIGN(AwCookieChangeDispatcherWrapper);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_NET_AW_COOKIE_CHANGE_DISPATCHER_WRAPPER_H_
