// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_BROWSER_BASE_PARALLEL_RESOURCE_THROTTLE_H_
#define COMPONENTS_SAFE_BROWSING_BROWSER_BASE_PARALLEL_RESOURCE_THROTTLE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/resource_throttle.h"
#include "content/public/common/resource_type.h"
#include "content/public/common/url_loader_throttle.h"

namespace net {
class URLRequest;
}

namespace safe_browsing {

class BrowserURLLoaderThrottle;
class UrlCheckerDelegate;

// A thin wrapper around BrowserURLLoaderThrottle to adapt to the
// content::ResourceThrottle interface.
class BaseParallelResourceThrottle : public content::ResourceThrottle {
 protected:
  BaseParallelResourceThrottle(
      const net::URLRequest* request,
      content::ResourceType resource_type,
      scoped_refptr<UrlCheckerDelegate> url_checker_delegate);

  ~BaseParallelResourceThrottle() override;

  // content::ResourceThrottle implementation:
  void WillStartRequest(bool* defer) override;
  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override;
  void WillProcessResponse(bool* defer) override;
  const char* GetNameForLogging() const override;
  bool MustProcessResponseBeforeReadingBody() override;

  // Cancels the resource load. This calls ResourceThrottle::Cancel() but also
  // maintains internal state. It may be overridden in a subclass. The override
  // in subclass should call this base implementation for cancellation, instead
  // of calling ResourceThrottle::Cancel() directly.
  virtual void CancelResourceLoad();

 private:
  class URLLoaderThrottleHolder;

  void ResumeResourceLoad();
  void MayDeferCancelResourceLoad();

  const net::URLRequest* const request_;
  const content::ResourceType resource_type_;
  // Set to true if the throttle is currently either inside a ResourceThrottle
  // notification call or responsible for deferring the request.
  bool throttle_in_band_ = false;
  // Whether we should directly cancel the request on subsequent
  // ResourceThrottle notification calls.
  bool should_cancel_on_notification_ = false;

  std::unique_ptr<URLLoaderThrottleHolder> url_loader_throttle_holder_;

  DISALLOW_COPY_AND_ASSIGN(BaseParallelResourceThrottle);
};

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_BROWSER_BASE_PARALLEL_RESOURCE_THROTTLE_H_
