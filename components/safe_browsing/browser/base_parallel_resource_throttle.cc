// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/browser/base_parallel_resource_throttle.h"

#include <utility>

#include "base/threading/thread_task_runner_handle.h"
#include "components/safe_browsing/browser/browser_url_loader_throttle.h"
#include "components/safe_browsing/browser/url_checker_delegate.h"
#include "content/public/browser/resource_request_info.h"
#include "net/http/http_request_headers.h"
#include "net/log/net_log_with_source.h"
#include "net/url_request/url_request.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_response.h"

namespace safe_browsing {

class BaseParallelResourceThrottle::URLLoaderThrottleHolder
    : public content::URLLoaderThrottle::Delegate {
 public:
  URLLoaderThrottleHolder(BaseParallelResourceThrottle* owner,
                          std::unique_ptr<BrowserURLLoaderThrottle> throttle)
      : owner_(owner), throttle_(std::move(throttle)) {
    throttle_->set_delegate(this);
  }
  ~URLLoaderThrottleHolder() override = default;

  BrowserURLLoaderThrottle* throttle() const { return throttle_.get(); }
  uint32_t inside_delegate_calls() const { return inside_delegate_calls_; }

  // content::URLLoaderThrottle::Delegate implementation:
  void CancelWithError(int error_code,
                       base::StringPiece custom_reason) override {
    if (!owner_)
      return;

    ScopedDelegateCall scoped_delegate_call(this);
    owner_->MayDeferCancelResourceLoad();
  }

  void Resume() override {
    if (!owner_)
      return;

    ScopedDelegateCall scoped_delegate_call(this);
    owner_->ResumeResourceLoad();
  }

  void Detach() {
    owner_ = nullptr;
  }

 private:
  class ScopedDelegateCall {
   public:
    explicit ScopedDelegateCall(URLLoaderThrottleHolder* holder)
        : holder_(holder) {
      holder_->inside_delegate_calls_++;
    }
    ~ScopedDelegateCall() { holder_->inside_delegate_calls_--; }

   private:
    URLLoaderThrottleHolder* const holder_;
    DISALLOW_COPY_AND_ASSIGN(ScopedDelegateCall);
  };

  BaseParallelResourceThrottle* owner_;
  uint32_t inside_delegate_calls_ = 0;
  std::unique_ptr<BrowserURLLoaderThrottle> throttle_;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderThrottleHolder);
};

BaseParallelResourceThrottle::BaseParallelResourceThrottle(
    const net::URLRequest* request,
    content::ResourceType resource_type,
    scoped_refptr<UrlCheckerDelegate> url_checker_delegate)
    : request_(request), resource_type_(resource_type) {
  const content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(request_);
  auto throttle = BrowserURLLoaderThrottle::MaybeCreate(
      std::move(url_checker_delegate), info->GetWebContentsGetterForRequest());
  url_loader_throttle_holder_ =
      std::make_unique<URLLoaderThrottleHolder>(this, std::move(throttle));
}

BaseParallelResourceThrottle::~BaseParallelResourceThrottle() {
  if (url_loader_throttle_holder_->inside_delegate_calls() > 0) {
    // The BrowserURLLoaderThrottle owned by |url_loader_throttle_holder_| is
    // calling into this object. In this case, delay destruction of
    // |url_loader_throttle_holder_|, so that the BrowserURLLoaderThrottle
    // doesn't need to worry about any delegate calls may destroy it
    // synchronously.
    url_loader_throttle_holder_->Detach();

    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(
        FROM_HERE, std::move(url_loader_throttle_holder_));
  }
}

void BaseParallelResourceThrottle::WillStartRequest(bool* defer) {
  throttle_in_band_ = true;
  if (should_cancel_on_notification_) {
    CancelResourceLoad();
    return;
  }

  network::ResourceRequest resource_request;

  net::HttpRequestHeaders full_headers;
  resource_request.headers = request_->GetFullRequestHeaders(&full_headers)
                                 ? full_headers
                                 : request_->extra_request_headers();

  resource_request.load_flags = request_->load_flags();
  resource_request.resource_type = resource_type_;

  const content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(request_);
  resource_request.has_user_gesture = info && info->HasUserGesture();

  resource_request.url = request_->url();
  resource_request.method = request_->method();

  url_loader_throttle_holder_->throttle()->WillStartRequest(&resource_request,
                                                            defer);
  DCHECK(!*defer);
  throttle_in_band_ = false;
}

void BaseParallelResourceThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    bool* defer) {
  throttle_in_band_ = true;
  if (should_cancel_on_notification_) {
    CancelResourceLoad();
    return;
  }

  // The safe browsing URLLoaderThrottle doesn't use ResourceResponse, so pass
  // in an empty struct to avoid changing ResourceThrottle signature.
  network::ResourceResponseHead resource_response;
  url_loader_throttle_holder_->throttle()->WillRedirectRequest(
      redirect_info, resource_response, defer);
  DCHECK(!*defer);
  throttle_in_band_ = false;
}

void BaseParallelResourceThrottle::WillProcessResponse(bool* defer) {
  throttle_in_band_ = true;
  if (should_cancel_on_notification_) {
    CancelResourceLoad();
    return;
  }

  url_loader_throttle_holder_->throttle()->WillProcessResponse(
      GURL(), network::ResourceResponseHead(), defer);
  if (!*defer)
    throttle_in_band_ = false;
}

const char* BaseParallelResourceThrottle::GetNameForLogging() const {
  return "BaseParallelResourceThrottle";
}

bool BaseParallelResourceThrottle::MustProcessResponseBeforeReadingBody() {
  // The response body should not be cached before SafeBrowsing confirms that it
  // is safe to do so.
  return true;
}

void BaseParallelResourceThrottle::CancelResourceLoad() {
  DCHECK(throttle_in_band_);
  throttle_in_band_ = false;
  Cancel();
}

void BaseParallelResourceThrottle::ResumeResourceLoad() {
  DCHECK(throttle_in_band_);
  throttle_in_band_ = false;
  Resume();
}

void BaseParallelResourceThrottle::MayDeferCancelResourceLoad() {
  if (!throttle_in_band_)
    should_cancel_on_notification_ = true;
  else
    CancelResourceLoad();
}

}  // namespace safe_browsing
