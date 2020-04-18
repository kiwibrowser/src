// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/aw_cookie_change_dispatcher_wrapper.h"

#include "android_webview/browser/net/init_native_callback.h"
#include "base/bind.h"
#include "base/memory/ref_counted_delete_on_sequence.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_store.h"
#include "url/gurl.h"

namespace android_webview {

namespace {

using CookieChangeCallbackList =
    base::CallbackList<void(const net::CanonicalCookie& cookie,
                            net::CookieChangeCause cause)>;

class AwCookieChangeSubscription : public net::CookieChangeSubscription {
 public:
  explicit AwCookieChangeSubscription(
      std::unique_ptr<CookieChangeCallbackList::Subscription> subscription)
      : subscription_(std::move(subscription)) {}

 private:
  std::unique_ptr<CookieChangeCallbackList::Subscription> subscription_;

  DISALLOW_COPY_AND_ASSIGN(AwCookieChangeSubscription);
};

// Wraps a subscription to cookie change notifications for the global
// CookieStore for a consumer that lives on another thread. Handles passing
// messages between threads, and destroys itself when the consumer unsubscribes.
// Must be created on the consumer's thread. Each instance only supports a
// single subscription.
class SubscriptionWrapper {
 public:
  SubscriptionWrapper() : weak_factory_(this) {}

  std::unique_ptr<net::CookieChangeSubscription> Subscribe(
      const GURL& url,
      const std::string& name,
      net::CookieChangeCallback callback) {
    // This class is only intended to be used for a single subscription.
    DCHECK(callback_list_.empty());

    nested_subscription_ =
        new NestedSubscription(url, name, weak_factory_.GetWeakPtr());
    return std::make_unique<AwCookieChangeSubscription>(
        callback_list_.Add(std::move(callback)));
  }

 private:
  // The NestedSubscription is responsible for creating and managing the
  // underlying subscription to the real CookieStore, and posting notifications
  // back to |callback_list_|.
  class NestedSubscription
      : public base::RefCountedDeleteOnSequence<NestedSubscription> {
   public:
    NestedSubscription(const GURL& url,
                       const std::string& name,
                       base::WeakPtr<SubscriptionWrapper> subscription_wrapper)
        : base::RefCountedDeleteOnSequence<NestedSubscription>(
              GetCookieStoreTaskRunner()),
          subscription_wrapper_(subscription_wrapper),
          client_task_runner_(base::ThreadTaskRunnerHandle::Get()) {
      PostTaskToCookieStoreTaskRunner(
          base::BindOnce(&NestedSubscription::Subscribe, this, url, name));
    }

   private:
    friend class base::RefCountedDeleteOnSequence<NestedSubscription>;
    friend class base::DeleteHelper<NestedSubscription>;

    ~NestedSubscription() {}

    void Subscribe(const GURL& url, const std::string& name) {
      subscription_ =
          GetCookieStore()->GetChangeDispatcher().AddCallbackForCookie(
              url, name,
              base::BindRepeating(&NestedSubscription::OnChanged, this));
    }

    void OnChanged(const net::CanonicalCookie& cookie,
                   net::CookieChangeCause cause) {
      client_task_runner_->PostTask(
          FROM_HERE, base::BindOnce(&SubscriptionWrapper::OnChanged,
                                    subscription_wrapper_, cookie, cause));
    }

    base::WeakPtr<SubscriptionWrapper> subscription_wrapper_;
    scoped_refptr<base::TaskRunner> client_task_runner_;

    std::unique_ptr<net::CookieChangeSubscription> subscription_;

    DISALLOW_COPY_AND_ASSIGN(NestedSubscription);
  };

  void OnChanged(const net::CanonicalCookie& cookie,
                 net::CookieChangeCause cause) {
    callback_list_.Notify(cookie, cause);
  }

  // The "list" only had one entry, so can just clean up now.
  void OnUnsubscribe() { delete this; }

  scoped_refptr<NestedSubscription> nested_subscription_;
  CookieChangeCallbackList callback_list_;
  base::WeakPtrFactory<SubscriptionWrapper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SubscriptionWrapper);
};

}  // anonymous namespace

AwCookieChangeDispatcherWrapper::AwCookieChangeDispatcherWrapper()
#if DCHECK_IS_ON()
    : client_task_runner_(base::ThreadTaskRunnerHandle::Get())
#endif  // DCHECK_IS_ON()
{
}

AwCookieChangeDispatcherWrapper::~AwCookieChangeDispatcherWrapper() = default;

std::unique_ptr<net::CookieChangeSubscription>
AwCookieChangeDispatcherWrapper::AddCallbackForCookie(
    const GURL& url,
    const std::string& name,
    net::CookieChangeCallback callback) {
#if DCHECK_IS_ON()
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
#endif  // DCHECK_IS_ON()

  // The SubscriptionWrapper is owned by the subscription itself, and has no
  // connection to the AwCookieStoreWrapper after creation. Other CookieStore
  // implementations DCHECK if a subscription outlasts the cookie store,
  // unfortunately, this design makes DCHECKing if there's an outstanding
  // subscription when the AwCookieStoreWrapper is destroyed a bit ugly.
  // TODO(mmenke):  Still worth adding a DCHECK?
  SubscriptionWrapper* subscription = new SubscriptionWrapper();
  return subscription->Subscribe(url, name, std::move(callback));
}

std::unique_ptr<net::CookieChangeSubscription>
AwCookieChangeDispatcherWrapper::AddCallbackForUrl(
    const GURL& url,
    net::CookieChangeCallback callback) {
  // Implement when needed by Android Webview consumers.
  NOTIMPLEMENTED();
  return nullptr;
}

std::unique_ptr<net::CookieChangeSubscription>
AwCookieChangeDispatcherWrapper::AddCallbackForAllChanges(
    net::CookieChangeCallback callback) {
  // Implement when needed by Android Webview consumers.
  NOTIMPLEMENTED();
  return nullptr;
}

}  // namespace android_webview
