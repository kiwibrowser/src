// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_EXPERIMENTAL_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_EXPERIMENTAL_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace extensions {

class Extension;

// This class creates navigation throttles that redirect URLs to Bookmark Apps
// if the URLs are in scope of the Bookmark App.
class BookmarkAppExperimentalNavigationThrottle
    : public content::NavigationThrottle {
 public:

  static std::unique_ptr<content::NavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* navigation_handle);

  explicit BookmarkAppExperimentalNavigationThrottle(
      content::NavigationHandle* navigation_handle);
  ~BookmarkAppExperimentalNavigationThrottle() override;

  // content::NavigationThrottle:
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;
  const char* GetNameForLogging() override;

 private:
  // Decides if the navigation should be opened in a new tab, in a new App
  // Window, or if the navigation should continue normally.
  content::NavigationThrottle::ThrottleCheckResult ProcessNavigation(
      bool is_redirect);

  // Opens the current target url in an App window. May post a task to do so if
  // opening the app synchronously could result in the app opening in the
  // background. Also closes the current tab if this is the first navigation.
  content::NavigationThrottle::ThrottleCheckResult
  OpenInAppWindowAndCloseTabIfNecessary(
      scoped_refptr<const Extension> target_app);

  void OpenBookmarkApp(scoped_refptr<const Extension> bookmark_app);
  void CloseWebContents();
  void OpenInNewTabAndCancel();
  void ReparentWebContentsAndResume(scoped_refptr<const Extension> target_app);

  base::WeakPtrFactory<BookmarkAppExperimentalNavigationThrottle>
      weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkAppExperimentalNavigationThrottle);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_EXPERIMENTAL_NAVIGATION_THROTTLE_H_
