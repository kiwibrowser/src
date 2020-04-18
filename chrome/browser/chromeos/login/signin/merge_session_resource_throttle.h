// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_MERGE_SESSION_RESOURCE_THROTTLE_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_MERGE_SESSION_RESOURCE_THROTTLE_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/resource_throttle.h"

class GURL;

namespace net {
class URLRequest;
}

class MergeSessionResourceThrottle : public content::ResourceThrottle {
 public:
  explicit MergeSessionResourceThrottle(net::URLRequest* request);
  ~MergeSessionResourceThrottle() override;

 private:
  // Returns true if the resource loading for the given url should be deferred.
  bool MaybeDeferLoading(const GURL& url);

  // content::ResourceThrottle implementation:
  void WillStartRequest(bool* defer) override;
  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override;
  const char* GetNameForLogging() const override;

  // MergeSessionXHRRequestWaiter callback.
  void OnBlockingPageComplete();

  // Not owned.
  net::URLRequest* request_;

  base::WeakPtrFactory<MergeSessionResourceThrottle> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MergeSessionResourceThrottle);
};

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_MERGE_SESSION_RESOURCE_THROTTLE_H_
