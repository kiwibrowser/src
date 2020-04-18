// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CAPTIVE_PORTAL_CAPTIVE_PORTAL_TAB_HELPER_H_
#define CHROME_BROWSER_CAPTIVE_PORTAL_CAPTIVE_PORTAL_TAB_HELPER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/captive_portal/captive_portal_service.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/resource_type.h"

class Profile;

namespace content {
class NavigationHandle;
class WebContents;
}

namespace net {
class SSLInfo;
}

class CaptivePortalLoginDetector;
class CaptivePortalTabReloader;

// Along with the classes it owns, responsible for detecting page loads broken
// by a captive portal, triggering captive portal checks on navigation events
// that may indicate a captive portal is present, or has been removed / logged
// in to, and taking any correcting actions. Lives on the UI thread.
//
// It acts as a WebContentsObserver for its CaptivePortalLoginDetector and
// CaptivePortalTabReloader. It filters out non-main-frame navigations. It is
// also needed by CaptivePortalTabReloaders to inform the tab's
// CaptivePortalLoginDetector when the tab is at a captive portal's login page.
//
// The TabHelper assumes that a WebContents can only have one main frame
// navigation at a time. This assumption can be violated in rare cases, for
// example, a same-site navigation interrupted by a cross-process navigation
// started from the omnibox, may commit before it can be cancelled.  In these
// cases, this class may pass incorrect messages to the TabReloader, which
// will, at worst, result in not opening up a login tab until a second load
// fails or not automatically reloading a tab after logging in.
// TODO(clamy): See if this class can be made to handle these edge-cases
// following the refactor of navigation signaling to WebContentsObservers.
//
// For the design doc, see:
// https://docs.google.com/document/d/1k-gP2sswzYNvryu9NcgN7q5XrsMlUdlUdoW9WRaEmfM/edit
class CaptivePortalTabHelper
    : public content::WebContentsObserver,
      public content::NotificationObserver,
      public content::WebContentsUserData<CaptivePortalTabHelper> {
 public:
  ~CaptivePortalTabHelper() override;

  // content::WebContentsObserver:
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidRedirectNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidStopLoading() override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Called when a certificate interstitial error page is about to be shown.
  void OnSSLCertError(const net::SSLInfo& ssl_info);

  // A "Login Tab" is a tab that was originally at a captive portal login
  // page.  This is set to false when a captive portal is no longer detected.
  bool IsLoginTab() const;

  // Opens a login tab if the profile's active window doesn't have one already.
  static void OpenLoginTabForWebContents(content::WebContents* web_contents,
                                         bool focus);

 private:
  friend class CaptivePortalBrowserTest;
  friend class CaptivePortalTabHelperTest;

  friend class content::WebContentsUserData<CaptivePortalTabHelper>;
  explicit CaptivePortalTabHelper(content::WebContents* web_contents);

  // Called by Observe in response to the corresponding event.
  void OnCaptivePortalResults(
      captive_portal::CaptivePortalResult previous_result,
      captive_portal::CaptivePortalResult result);

  // Called to indicate a tab is at, or is navigating to, the captive portal
  // login page.
  void SetIsLoginTab();

  // |this| takes ownership of |tab_reloader|.
  void SetTabReloaderForTest(CaptivePortalTabReloader* tab_reloader);

  CaptivePortalTabReloader* GetTabReloaderForTest();

  Profile* profile_;

  // The current main frame navigation happening for the WebContents, or
  // nullptr if there is none. If there are two main frame navigations
  // happening at once, it's the one that started most recently.
  content::NavigationHandle* navigation_handle_;

  // Neither of these will ever be NULL.
  std::unique_ptr<CaptivePortalTabReloader> tab_reloader_;
  std::unique_ptr<CaptivePortalLoginDetector> login_detector_;

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(CaptivePortalTabHelper);
};

#endif  // CHROME_BROWSER_CAPTIVE_PORTAL_CAPTIVE_PORTAL_TAB_HELPER_H_
