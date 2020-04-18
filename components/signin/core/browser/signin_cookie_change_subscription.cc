// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/signin/core/browser/signin_cookie_change_subscription.h"

#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/cookies/cookie_change_dispatcher.h"
#include "net/cookies/cookie_store.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

SigninCookieChangeSubscription::SubscriptionHolder::SubscriptionHolder() =
    default;

SigninCookieChangeSubscription::SubscriptionHolder::~SubscriptionHolder() =
    default;

SigninCookieChangeSubscription::SigninCookieChangeSubscription(
    scoped_refptr<net::URLRequestContextGetter> context_getter,
    const GURL& url,
    const std::string& name,
    net::CookieChangeCallback callback)
    : context_getter_(std::move(context_getter)),
      subscription_holder_io_(std::make_unique<SubscriptionHolder>()),
      callback_(std::move(callback)) {
  RegisterForCookieChangeNotifications(url, name);
}

SigninCookieChangeSubscription::~SigninCookieChangeSubscription() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner =
      context_getter_->GetNetworkTaskRunner();
  if (network_task_runner->BelongsToCurrentThread()) {
    subscription_holder_io_.reset();
  } else {
    network_task_runner->DeleteSoon(FROM_HERE,
                                    subscription_holder_io_.release());
  }
}

void SigninCookieChangeSubscription::RegisterForCookieChangeNotifications(
    const GURL& url,
    const std::string& name) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // The cookie store can only be accessed from the context getter which lives
  // on the network thread. As |AddCookieChangeCallback| is called from the
  // main thread, a thread jump is needed to register for cookie changed
  // notifications.
  net::CookieChangeCallback run_on_current_thread_callback =
      base::BindRepeating(
          &SigninCookieChangeSubscription::RunAsyncOnCookieChange,
          base::ThreadTaskRunnerHandle::Get(), this->AsWeakPtr());
  base::OnceClosure register_closure =
      base::BindOnce(&RegisterForCookieChangesOnIOThread, context_getter_, url,
                     name, std::move(run_on_current_thread_callback),
                     base::Unretained(subscription_holder_io_.get()));
  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner =
      context_getter_->GetNetworkTaskRunner();
  if (network_task_runner->BelongsToCurrentThread()) {
    std::move(register_closure).Run();
  } else {
    network_task_runner->PostTask(FROM_HERE, std::move(register_closure));
  }
}

// static
void SigninCookieChangeSubscription::RegisterForCookieChangesOnIOThread(
    scoped_refptr<net::URLRequestContextGetter> context_getter,
    const GURL url,
    const std::string name,
    net::CookieChangeCallback callback,
    SigninCookieChangeSubscription::SubscriptionHolder*
        out_subscription_holder) {
  DCHECK(out_subscription_holder);
  net::CookieStore* cookie_store =
      context_getter->GetURLRequestContext()->cookie_store();
  DCHECK(cookie_store);
  out_subscription_holder->subscription =
      cookie_store->GetChangeDispatcher().AddCallbackForCookie(
          url, name, std::move(callback));
}

// static
void SigninCookieChangeSubscription::RunAsyncOnCookieChange(
    scoped_refptr<base::TaskRunner> proxy,
    base::WeakPtr<SigninCookieChangeSubscription> subscription,
    const net::CanonicalCookie& cookie,
    net::CookieChangeCause cause) {
  proxy->PostTask(
      FROM_HERE, base::BindOnce(&SigninCookieChangeSubscription::OnCookieChange,
                                subscription, cookie, cause));
}

void SigninCookieChangeSubscription::OnCookieChange(
    const net::CanonicalCookie& cookie,
    net::CookieChangeCause cause) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!callback_.is_null()) {
    callback_.Run(cookie, cause);
  }
}
