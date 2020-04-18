// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_COOKIE_CHANGE_SUBSCRIPTION_H_
#define COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_COOKIE_CHANGE_SUBSCRIPTION_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "components/signin/core/browser/signin_client.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "net/url_request/url_request_context_getter.h"

// The subscription for cookie changes. This class lives on the main thread.
class SigninCookieChangeSubscription
    : public SigninClient::CookieChangeSubscription,
      public base::SupportsWeakPtr<SigninCookieChangeSubscription> {
 public:
  // Creates a cookie change subscription and registers for cookie changed
  // events.
  SigninCookieChangeSubscription(
      scoped_refptr<net::URLRequestContextGetter> context_getter,
      const GURL& url,
      const std::string& name,
      net::CookieChangeCallback callback);
  ~SigninCookieChangeSubscription() override;

 private:
  // Holder of a cookie store cookie changed subscription.
  struct SubscriptionHolder {
    std::unique_ptr<net::CookieChangeSubscription> subscription;
    SubscriptionHolder();
    ~SubscriptionHolder();
  };

  // Adds a callback for cookie changed events. This method is called on the
  // network thread, so it is safe to access the cookie store.
  static void RegisterForCookieChangesOnIOThread(
      scoped_refptr<net::URLRequestContextGetter> context_getter,
      const GURL url,
      const std::string name,
      net::CookieChangeCallback callback,
      SigninCookieChangeSubscription::SubscriptionHolder*
          out_subscription_holder);

  void RegisterForCookieChangeNotifications(const GURL& url,
                                            const std::string& name);

  // Posts a task on the |proxy| task runner that calls |OnCookieChange| on
  // |subscription|.
  // Note that this method is called on the network thread, so |subscription|
  // must not be used here, it is only passed around.
  static void RunAsyncOnCookieChange(
      scoped_refptr<base::TaskRunner> proxy,
      base::WeakPtr<SigninCookieChangeSubscription> subscription,
      const net::CanonicalCookie& cookie,
      net::CookieChangeCause cause);

  // Handler for cookie change events.
  void OnCookieChange(const net::CanonicalCookie& cookie,
                      net::CookieChangeCause cause);

  // The context getter.
  scoped_refptr<net::URLRequestContextGetter> context_getter_;

  // The holder of a cookie changed subscription. Must be destroyed on the
  // network thread.
  std::unique_ptr<SubscriptionHolder> subscription_holder_io_;

  // Callback to be run on cookie changed events.
  net::CookieChangeCallback callback_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(SigninCookieChangeSubscription);
};

#endif  // COMPONENTS_SIGNIN_CORE_BROWSER_SIGNIN_COOKIE_CHANGE_SUBSCRIPTION_H_
