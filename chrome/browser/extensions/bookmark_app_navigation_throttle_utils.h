// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_THROTTLE_UTILS_H_
#define CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_THROTTLE_UTILS_H_

#include "base/memory/ref_counted.h"

class GURL;

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace extensions {

class Extension;

// Used to record the result of a navigation.
enum class BookmarkAppNavigationThrottleResult {
  kProceedStartedFromContextMenu,
  kProceedTransitionTyped,
  kProceedTransitionAutoBookmark,
  kProceedTransitionAutoSubframe,
  kProceedTransitionManualSubframe,
  kProceedTransitionGenerated,
  kProceedTransitionAutoToplevel,
  kProceedTransitionReload,
  kProceedTransitionKeyword,
  kProceedTransitionKeywordGenerated,
  kProceedTransitionForwardBack,
  kProceedTransitionFromAddressBar,
  kOpenInChromeProceedOutOfScopeLaunch,
  kProceedInAppSameScope,
  kProceedInBrowserFormSubmission,
  kProceedInBrowserSameScope,
  kCancelPrerenderContents,
  kDeferMovingContentsToNewAppWindow,
  kCancelOpenedApp,
  kDeferOpenNewTabInAppOutOfScope,
  kProceedDispositionSingletonTab,
  kProceedDispositionNewBackgroundTab,
  kProceedDispositionNewPopup,
  kProceedDispositionSwitchToTab,
  kReparentIntoPopupProceedOutOfScopeInitialNavigation,
  // Add BookmarkAppNavigationThrottle and
  // BookmarkAppExperimentalNavigationThrottle results immediately above this
  // line. Also update the enum list in tools/metrics/enums.xml accordingly.
  kCount,
};

// Set of common functions between BookmarkAppNavigationThrottle and
// BookmarkAppExperimentalNavigationThrottle.

void RecordBookmarkAppNavigationThrottleResult(
    BookmarkAppNavigationThrottleResult result);

// Retrieves the Bookmark App corresponding to |source|'s window only
// if the app is for an installable website.
scoped_refptr<const Extension> GetAppForWindow(content::WebContents* source);

// Retrieves the target Bookmark App for |target_url|.
scoped_refptr<const Extension> GetTargetApp(content::WebContents* source,
                                            const GURL& target_url);

scoped_refptr<const Extension> GetAppForMainFrameURL(
    content::WebContents* source);

void OpenNewForegroundTab(content::NavigationHandle* navigation_handle);

void ReparentIntoPopup(content::WebContents* source, bool has_user_gesture);

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_THROTTLE_UTILS_H_
