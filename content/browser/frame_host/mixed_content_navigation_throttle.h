// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_MIXED_CONTENT_NAVIGATION_THROTTLE_H_
#define CONTENT_BROWSER_FRAME_HOST_MIXED_CONTENT_NAVIGATION_THROTTLE_H_

#include <set>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/common/request_context_type.h"
#include "third_party/blink/public/platform/web_mixed_content_context_type.h"

namespace content {

class FrameTreeNode;
struct WebPreferences;

// Responsible for browser-process-side mixed content security checks. It is
// only enabled if PlzNavigate is and checks only for frame-level resource loads
// (aka navigation loads). Sub-resources fetches are checked in the renderer
// process by MixedContentChecker. Changes to this class might need to be
// reflected on its renderer counterpart.
//
// Current mixed content W3C draft that drives this implementation:
// https://w3c.github.io/webappsec-mixed-content/
class MixedContentNavigationThrottle : public NavigationThrottle {
 public:
  static std::unique_ptr<NavigationThrottle> CreateThrottleForNavigation(
      NavigationHandle* navigation_handle);

  MixedContentNavigationThrottle(NavigationHandle* navigation_handle);
  ~MixedContentNavigationThrottle() override;

  // NavigationThrottle overrides.
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  ThrottleCheckResult WillProcessResponse() override;
  const char* GetNameForLogging() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(MixedContentNavigationThrottleTest, IsMixedContent);

  // Copy of mixed content related values from the blink::UseCounter enum that
  // are needed to report feature usage during browser side checks.
  enum UseCounterFeature {
    MIXED_CONTENT_PRESENT = 609,
    MIXED_CONTENT_BLOCKABLE = 610,
    MIXED_CONTENT_INTERNAL = 615,
    MIXED_CONTENT_PREFETCH = 617,
    MIXED_CONTENT_IN_NON_HTTPS_FRAME_THAT_RESTRICTS_MIXED_CONTENT = 661,
    MIXED_CONTENT_IN_SECURE_FRAME_THAT_DOES_NOT_RESTRICT_MIXED_CONTENT = 662,
    MIXED_CONTENT_BLOCKABLE_ALLOWED = 896,
  };

  // Checks if a navigation should be blocked or not due to mixed content.
  bool ShouldBlockNavigation(bool for_redirect);

  // Returns the parent frame where mixed content exists for the provided data
  // or nullptr if there is no mixed content.
  FrameTreeNode* InWhichFrameIsContentMixed(FrameTreeNode* node,
                                            const GURL& url);

  // Updates the renderer about any Blink feature usage.
  void MaybeSendBlinkFeatureUsageReport();

  // Records basic mixed content "feature" usage when any kind of mixed content
  // is found.
  void ReportBasicMixedContentFeatures(
      RequestContextType request_context_type,
      blink::WebMixedContentContextType mixed_content_context_type,
      const WebPreferences& prefs);

  static bool CONTENT_EXPORT IsMixedContentForTesting(const GURL& origin_url,
                                                      const GURL& url);

  // Keeps track of mixed content features (as defined in blink::UseCounter)
  // encountered while running one of the navigation throttling steps. These
  // values are reported to the respective renderer process after each mixed
  // content check is finished.
  std::set<int> mixed_content_features_;

  DISALLOW_COPY_AND_ASSIGN(MixedContentNavigationThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_MIXED_CONTENT_NAVIGATION_THROTTLE_H_
