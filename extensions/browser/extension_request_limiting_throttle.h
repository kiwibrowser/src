// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_REQUEST_LIMITING_THROTTLE_H_
#define EXTENSIONS_BROWSER_EXTENSION_REQUEST_LIMITING_THROTTLE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/resource_throttle.h"

namespace net {
struct RedirectInfo;
class URLRequest;
}

namespace extensions {

class ExtensionThrottleEntryInterface;
class ExtensionThrottleManager;

// This class monitors requests issued by extensions and throttles the request
// if there are too many requests made within a short time to urls with the same
// scheme, host, port and path. For the exact criteria for throttling, please
// also see extension_throttle_manager.cc.
class ExtensionRequestLimitingThrottle : public content::ResourceThrottle {
 public:
  ExtensionRequestLimitingThrottle(const net::URLRequest* request,
                                   ExtensionThrottleManager* manager);
  ~ExtensionRequestLimitingThrottle() override;

  // content::ResourceThrottle implementation (called on IO thread):
  void WillStartRequest(bool* defer) override;
  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override;
  void WillProcessResponse(bool* defer) override;

  const char* GetNameForLogging() const override;

 private:
  const net::URLRequest* request_;
  ExtensionThrottleManager* manager_;

  // This is used to supervise traffic and enforce exponential back-off.
  scoped_refptr<ExtensionThrottleEntryInterface> throttling_entry_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionRequestLimitingThrottle);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_REQUEST_LIMITING_THROTTLE_H_
