// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEB_RESTRICTION_WEB_RESTRICTION_RESOURCE_THROTTLE_H_
#define COMPONENTS_WEB_RESTRICTION_WEB_RESTRICTION_RESOURCE_THROTTLE_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/resource_throttle.h"
#include "url/gurl.h"

namespace web_restrictions {

class WebRestrictionsClient;

class WebRestrictionsResourceThrottle : public content::ResourceThrottle {
 public:
  WebRestrictionsResourceThrottle(WebRestrictionsClient* provider,
                                  const GURL& request_url,
                                  bool is_main_frame);
  ~WebRestrictionsResourceThrottle() override;

  // content::ResourceThrottle implementation:
  void WillStartRequest(bool* defer) override;
  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override;
  const char* GetNameForLogging() const override;

 private:
  bool ShouldDefer(const GURL& url);

  void OnCheckResult(bool should_proceed);

  WebRestrictionsClient* provider_;  // Not owned.

  const GURL request_url_;
  bool is_main_frame_;
  base::WeakPtrFactory<WebRestrictionsResourceThrottle> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebRestrictionsResourceThrottle);
};

}  // namespace web_restrictions

#endif  // COMPONENTS_WEB_RESTRICTION_WEB_RESTRICTION_RESOURCE_THROTTLE_H_
