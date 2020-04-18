// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/bookmark_app_navigation_throttle.h"

#include <memory>

#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/extensions/bookmark_app_navigation_throttle_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/web_applications/web_app.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

using content::BrowserThread;

namespace extensions {

// static
std::unique_ptr<content::NavigationThrottle>
BookmarkAppNavigationThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  DVLOG(1) << "Considering URL for interception: "
           << navigation_handle->GetURL().spec();
  if (!navigation_handle->IsInMainFrame()) {
    DVLOG(1) << "Don't intercept: Navigation is not in main frame.";
    return nullptr;
  }

  content::BrowserContext* browser_context =
      navigation_handle->GetWebContents()->GetBrowserContext();
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (profile->GetProfileType() == Profile::INCOGNITO_PROFILE) {
    DVLOG(1) << "Don't intercept: Navigation is in incognito.";
    return nullptr;
  }

  DVLOG(1) << "Attaching Bookmark App Navigation Throttle.";
  return std::make_unique<extensions::BookmarkAppNavigationThrottle>(
      navigation_handle);
}

BookmarkAppNavigationThrottle::BookmarkAppNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {}

BookmarkAppNavigationThrottle::~BookmarkAppNavigationThrottle() {}

const char* BookmarkAppNavigationThrottle::GetNameForLogging() {
  return "BookmarkAppNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
BookmarkAppNavigationThrottle::WillStartRequest() {
  return OpenForegroundTabIfOutOfScope(false /* is_redirect */);
}

content::NavigationThrottle::ThrottleCheckResult
BookmarkAppNavigationThrottle::WillRedirectRequest() {
  return OpenForegroundTabIfOutOfScope(true /* is_redirect */);
}

content::NavigationThrottle::ThrottleCheckResult
BookmarkAppNavigationThrottle::OpenForegroundTabIfOutOfScope(bool is_redirect) {
  content::WebContents* source = navigation_handle()->GetWebContents();
  scoped_refptr<const Extension> app_for_window = GetAppForWindow(source);

  // When launching an app, if the page redirects to an out-of-scope URL, then
  // continue the navigation in a regular browser window. (Launching an app
  // results in an AUTO_BOOKMARK transition).
  //
  // Note that for non-redirecting app launches, GetAppForWindow() might return
  // null, because the navigation's WebContents might not be attached to a
  // window yet.
  if (is_redirect &&
      PageTransitionCoreTypeIs(navigation_handle()->GetPageTransition(),
                               ui::PAGE_TRANSITION_AUTO_BOOKMARK)) {
    // If GetAppForWindow returned nullptr, we are already in the browser, so
    // don't open a new tab.
    if (app_for_window &&
        app_for_window != GetTargetApp(source, navigation_handle()->GetURL())) {
      DVLOG(1) << "Out-of-scope navigation during launch. Opening in Chrome.";
      RecordBookmarkAppNavigationThrottleResult(
          BookmarkAppNavigationThrottleResult::
              kOpenInChromeProceedOutOfScopeLaunch);
      Browser* browser = chrome::FindBrowserWithWebContents(source);
      DCHECK(browser);
      chrome::OpenInChrome(browser);
      return content::NavigationThrottle::PROCEED;
    }
  }

  if (!app_for_window)
    return content::NavigationThrottle::PROCEED;

  if (app_for_window == GetTargetApp(source, navigation_handle()->GetURL())) {
    DVLOG(1) << "Don't intercept: The target URL is in the same scope as the "
             << "current app.";
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::kProceedInAppSameScope);

    return content::NavigationThrottle::PROCEED;
  }

  if (source->GetController().IsInitialNavigation()) {
    DVLOG(1) << "In-app initial navigation to out-of-scope URL. "
             << "Opening in popup.";
    RecordBookmarkAppNavigationThrottleResult(
        BookmarkAppNavigationThrottleResult::
            kReparentIntoPopupProceedOutOfScopeInitialNavigation);
    ReparentIntoPopup(source, navigation_handle()->HasUserGesture());
    return content::NavigationThrottle::PROCEED;
  }

  DVLOG(1) << "Open in new tab.";
  RecordBookmarkAppNavigationThrottleResult(
      BookmarkAppNavigationThrottleResult::kDeferOpenNewTabInAppOutOfScope);
  OpenNewForegroundTab(navigation_handle());

  return content::NavigationThrottle::CANCEL_AND_IGNORE;
}

}  // namespace extensions
