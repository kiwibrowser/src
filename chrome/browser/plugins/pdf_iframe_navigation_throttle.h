// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGINS_PDF_IFRAME_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_PLUGINS_PDF_IFRAME_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}  // namespace content

class PDFIFrameNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<content::NavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* handle);

  explicit PDFIFrameNavigationThrottle(content::NavigationHandle* handle);
  ~PDFIFrameNavigationThrottle() override;

  // content::NavigationThrottle:
  ThrottleCheckResult WillProcessResponse() override;
  const char* GetNameForLogging() override;
};

#endif  // CHROME_BROWSER_PLUGINS_PDF_IFRAME_NAVIGATION_THROTTLE_H_
