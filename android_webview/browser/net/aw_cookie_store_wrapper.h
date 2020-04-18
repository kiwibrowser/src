// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_NET_AW_COOKIE_STORE_WRAPPER_H_
#define ANDROID_WEBVIEW_BROWSER_NET_AW_COOKIE_STORE_WRAPPER_H_

#include <string>

#include "android_webview/browser/net/aw_cookie_change_dispatcher_wrapper.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner.h"
#include "base/time/time.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "net/cookies/cookie_constants.h"
#include "net/cookies/cookie_store.h"

namespace android_webview {

// A cross-threaded cookie store implementation that wraps the CookieManager's
// CookieStore. It posts tasks to the CookieStore's thread, and invokes
// callbacks on the originating thread. Deleting it cancels pending callbacks.
// This is needed to allow Webview to run the CookieStore on its own thread, to
// enable synchronous calls into the store on the IO Thread from Java.
//
// AwCookieStoreWrapper will only grab the CookieStore pointer from the
// CookieManager when it's needed, allowing for lazy creation of the
// CookieStore.
//
// AwCookieStoreWrapper may only be called from the thread on which it's
// created.
//
// The CookieManager must outlive the AwCookieStoreWrapper.
class AwCookieStoreWrapper : public net::CookieStore {
 public:
  AwCookieStoreWrapper();
  ~AwCookieStoreWrapper() override;

  // CookieStore implementation:
  void SetCookieWithOptionsAsync(const GURL& url,
                                 const std::string& cookie_line,
                                 const net::CookieOptions& options,
                                 SetCookiesCallback callback) override;
  void SetCanonicalCookieAsync(std::unique_ptr<net::CanonicalCookie> cookie,
                               bool secure_source,
                               bool modify_http_only,
                               SetCookiesCallback callback) override;
  void GetCookieListWithOptionsAsync(const GURL& url,
                                     const net::CookieOptions& options,
                                     GetCookieListCallback callback) override;
  void GetAllCookiesAsync(GetCookieListCallback callback) override;
  void DeleteCookieAsync(const GURL& url,
                         const std::string& cookie_name,
                         base::OnceClosure callback) override;
  void DeleteCanonicalCookieAsync(const net::CanonicalCookie& cookie,
                                  DeleteCallback callback) override;
  void DeleteAllCreatedInTimeRangeAsync(
      const net::CookieDeletionInfo::TimeRange& creation_range,
      DeleteCallback callback) override;
  void DeleteAllMatchingInfoAsync(net::CookieDeletionInfo delete_info,
                                  DeleteCallback callback) override;
  void DeleteSessionCookiesAsync(DeleteCallback callback) override;
  void FlushStore(base::OnceClosure callback) override;
  void SetForceKeepSessionState() override;
  net::CookieChangeDispatcher& GetChangeDispatcher() override;
  bool IsEphemeral() override;

 private:
  // Used by CreateWrappedCallback below. Takes an argument of Type and posts
  // a task to |task_runner| to invoke |callback| with that argument. If
  // |weak_cookie_store| is deleted before the task is run, the task will not
  // be run.
  template <class Type>
  static void RunCallbackOnClientThread(
      base::TaskRunner* task_runner,
      base::WeakPtr<AwCookieStoreWrapper> weak_cookie_store,
      base::OnceCallback<void(Type)> callback,
      Type argument) {
    task_runner->PostTask(
        FROM_HERE,
        base::BindOnce(&AwCookieStoreWrapper::RunClosureCallback,
                       weak_cookie_store,
                       base::BindOnce(std::move(callback), argument)));
  }

  // Returns a base::Callback that takes an argument of Type and posts a task to
  // the |client_task_runner_| to invoke |callback| with that argument.
  template <class Type>
  base::OnceCallback<void(Type)> CreateWrappedCallback(
      base::OnceCallback<void(Type)> callback) {
    if (callback.is_null())
      return std::move(callback);
    return base::BindOnce(
        &AwCookieStoreWrapper::RunCallbackOnClientThread<Type>,
        base::RetainedRef(client_task_runner_), weak_factory_.GetWeakPtr(),
        std::move(callback));
  }

  // Returns a base::OnceClosure that posts a task to the |client_task_runner_|
  // to invoke |callback|.
  base::OnceClosure CreateWrappedClosureCallback(base::OnceClosure callback);

  // Runs |callback|. Used to prevent callbacks from being invoked after the
  // AwCookieStoreWrapper has been destroyed.
  void RunClosureCallback(base::OnceClosure callback);

  scoped_refptr<base::SingleThreadTaskRunner> client_task_runner_;

  AwCookieChangeDispatcherWrapper change_dispatcher_;

  base::WeakPtrFactory<AwCookieStoreWrapper> weak_factory_;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_NET_AW_COOKIE_STORE_WRAPPER_H_
