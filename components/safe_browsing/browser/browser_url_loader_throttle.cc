// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/browser/browser_url_loader_throttle.h"

#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "components/safe_browsing/browser/safe_browsing_url_checker_impl.h"
#include "components/safe_browsing/browser/url_checker_delegate.h"
#include "components/safe_browsing/common/safebrowsing_constants.h"
#include "components/safe_browsing/common/utils.h"
#include "net/log/net_log_event_type.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_response.h"

namespace safe_browsing {

// static
std::unique_ptr<BrowserURLLoaderThrottle> BrowserURLLoaderThrottle::MaybeCreate(
    scoped_refptr<UrlCheckerDelegate> url_checker_delegate,
    const base::Callback<content::WebContents*()>& web_contents_getter) {
  if (!url_checker_delegate ||
      !url_checker_delegate->GetDatabaseManager()->IsSupported()) {
    return nullptr;
  }

  return base::WrapUnique<BrowserURLLoaderThrottle>(
      new BrowserURLLoaderThrottle(std::move(url_checker_delegate),
                                   web_contents_getter));
}

BrowserURLLoaderThrottle::BrowserURLLoaderThrottle(
    scoped_refptr<UrlCheckerDelegate> url_checker_delegate,
    const base::Callback<content::WebContents*()>& web_contents_getter)
    : url_checker_delegate_(std::move(url_checker_delegate)),
      web_contents_getter_(web_contents_getter) {}

BrowserURLLoaderThrottle::~BrowserURLLoaderThrottle() {
  if (deferred_)
    TRACE_EVENT_ASYNC_END0("safe_browsing", "Deferred", this);

  if (!user_action_involved_)
    LogNoUserActionResourceLoadingDelay(total_delay_);
}

void BrowserURLLoaderThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  DCHECK_EQ(0u, pending_checks_);
  DCHECK(!blocked_);
  DCHECK(!url_checker_);

  original_url_ = request->url;
  pending_checks_++;
  url_checker_ = std::make_unique<SafeBrowsingUrlCheckerImpl>(
      request->headers, request->load_flags,
      static_cast<content::ResourceType>(request->resource_type),
      request->has_user_gesture, std::move(url_checker_delegate_),
      web_contents_getter_);

  url_checker_->CheckUrl(
      request->url, request->method,
      base::BindOnce(&BrowserURLLoaderThrottle::OnCheckUrlResult,
                     base::Unretained(this)));
}

void BrowserURLLoaderThrottle::WillRedirectRequest(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseHead& response_head,
    bool* defer) {
  if (blocked_) {
    // OnCheckUrlResult() has set |blocked_| to true and called
    // |delegate_->CancelWithError|, but this method is called before the
    // request is actually cancelled. In that case, simply defer the request.
    *defer = true;
    return;
  }

  pending_checks_++;
  url_checker_->CheckUrl(
      redirect_info.new_url, redirect_info.new_method,
      base::BindOnce(&BrowserURLLoaderThrottle::OnCheckUrlResult,
                     base::Unretained(this)));
}

void BrowserURLLoaderThrottle::WillProcessResponse(
    const GURL& response_url,
    const network::ResourceResponseHead& response_head,
    bool* defer) {
  if (blocked_) {
    // OnCheckUrlResult() has set |blocked_| to true and called
    // |delegate_->CancelWithError|, but this method is called before the
    // request is actually cancelled. In that case, simply defer the request.
    *defer = true;
    return;
  }

  if (pending_checks_ == 0)
    return;

  DCHECK(!deferred_);
  deferred_ = true;
  defer_start_time_ = base::TimeTicks::Now();
  *defer = true;
  TRACE_EVENT_ASYNC_BEGIN1("safe_browsing", "Deferred", this, "original_url",
                           original_url_.spec());
}

void BrowserURLLoaderThrottle::OnCompleteCheck(bool slow_check,
                                               bool proceed,
                                               bool showed_interstitial) {
  DCHECK(!blocked_);

  DCHECK_LT(0u, pending_checks_);
  pending_checks_--;

  if (slow_check) {
    DCHECK_LT(0u, pending_slow_checks_);
    pending_slow_checks_--;
  }

  user_action_involved_ = user_action_involved_ || showed_interstitial;
  // If the resource load is currently deferred and is going to exit that state
  // (either being cancelled or resumed), record the total delay.
  if (deferred_ && (!proceed || pending_checks_ == 0))
    total_delay_ = base::TimeTicks::Now() - defer_start_time_;

  if (proceed) {
    if (pending_slow_checks_ == 0 && slow_check)
      delegate_->ResumeReadingBodyFromNet();

    if (pending_checks_ == 0 && deferred_) {
      deferred_ = false;
      TRACE_EVENT_ASYNC_END0("safe_browsing", "Deferred", this);
      delegate_->Resume();
    }
  } else {
    blocked_ = true;

    url_checker_.reset();
    pending_checks_ = 0;
    pending_slow_checks_ = 0;
    delegate_->CancelWithError(net::ERR_ABORTED,
                               kCustomCancelReasonForURLLoader);
  }
}

void BrowserURLLoaderThrottle::OnCheckUrlResult(
    NativeUrlCheckNotifier* slow_check_notifier,
    bool proceed,
    bool showed_interstitial) {
  DCHECK(!blocked_);

  if (!slow_check_notifier) {
    OnCompleteCheck(false, proceed, showed_interstitial);
    return;
  }

  pending_slow_checks_++;
  // Pending slow checks indicate that the resource may be unsafe. In that case,
  // pause reading response body from network to minimize the chance of
  // processing unsafe contents (e.g., writing unsafe contents into cache),
  // until we get the results. According to the results, we may resume reading
  // or cancel the resource load.
  if (pending_slow_checks_ == 1)
    delegate_->PauseReadingBodyFromNet();

  // In this case |proceed| and |showed_interstitial| should be ignored. The
  // result will be returned by calling |*slow_check_notifier| callback.
  *slow_check_notifier =
      base::BindOnce(&BrowserURLLoaderThrottle::OnCompleteCheck,
                     base::Unretained(this), true /* slow_check */);
}

}  // namespace safe_browsing
