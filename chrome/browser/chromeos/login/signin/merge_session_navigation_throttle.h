// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_MERGE_SESSION_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_MERGE_SESSION_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}

// Used to show an interstitial page while merge session process (cookie
// reconstruction from OAuth2 refresh token in ChromeOS login) is still in
// progress while we are attempting to load a google property.
class MergeSessionNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<content::NavigationThrottle> Create(
      content::NavigationHandle* handle);
  ~MergeSessionNavigationThrottle() override;

 private:
  explicit MergeSessionNavigationThrottle(content::NavigationHandle* handle);

  // content::NavigationThrottle implementation:
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;
  const char* GetNameForLogging() override;

  // MergeSessionLoadPage callback.
  void OnBlockingPageComplete();

  base::WeakPtrFactory<MergeSessionNavigationThrottle> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MergeSessionNavigationThrottle);
};

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_MERGE_SESSION_NAVIGATION_THROTTLE_H_
