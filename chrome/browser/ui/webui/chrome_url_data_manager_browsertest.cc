// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"

namespace {

class NavigationNotificationObserver : public content::NotificationObserver {
 public:
  NavigationNotificationObserver()
      : got_navigation_(false),
        http_status_code_(0) {
    registrar_.Add(this, content::NOTIFICATION_NAV_ENTRY_COMMITTED,
                   content::NotificationService::AllSources());
  }

  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    DCHECK_EQ(content::NOTIFICATION_NAV_ENTRY_COMMITTED, type);
    got_navigation_ = true;
    http_status_code_ =
        content::Details<content::LoadCommittedDetails>(details)->
        http_status_code;
  }

  int http_status_code() const { return http_status_code_; }
  bool got_navigation() const { return got_navigation_; }

 private:
  content::NotificationRegistrar registrar_;
  int got_navigation_;
  int http_status_code_;

  DISALLOW_COPY_AND_ASSIGN(NavigationNotificationObserver);
};

class NavigationObserver : public content::WebContentsObserver {
public:
  enum NavigationResult {
    NOT_FINISHED,
    ERROR_PAGE,
    SUCCESS,
  };

  explicit NavigationObserver(content::WebContents* web_contents)
      : WebContentsObserver(web_contents), navigation_result_(NOT_FINISHED) {}
  ~NavigationObserver() override = default;

  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    navigation_result_ =
        navigation_handle->IsErrorPage() ? ERROR_PAGE : SUCCESS;
  }

  NavigationResult navigation_result() const { return navigation_result_; }

  void Reset() { navigation_result_ = NOT_FINISHED; }

 private:
  NavigationResult navigation_result_;

  DISALLOW_COPY_AND_ASSIGN(NavigationObserver);
};

}  // namespace

typedef InProcessBrowserTest ChromeURLDataManagerTest;

// Makes sure navigating to the new tab page results in a http status code
// of 200.
IN_PROC_BROWSER_TEST_F(ChromeURLDataManagerTest, 200) {
  NavigationNotificationObserver observer;
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUINewTabURL));
  EXPECT_TRUE(observer.got_navigation());
  EXPECT_EQ(200, observer.http_status_code());
}

// Makes sure browser does not crash when navigating to an unknown resource.
IN_PROC_BROWSER_TEST_F(ChromeURLDataManagerTest, UnknownResource) {
  // Known resource
  NavigationObserver observer(
      browser()->tab_strip_model()->GetActiveWebContents());
  ui_test_utils::NavigateToURL(
      browser(), GURL("chrome://theme/IDR_SETTINGS_FAVICON"));
  EXPECT_EQ(NavigationObserver::SUCCESS, observer.navigation_result());

  // Unknown resource
  observer.Reset();
  ui_test_utils::NavigateToURL(
      browser(), GURL("chrome://theme/IDR_ASDFGHJKL"));
  EXPECT_EQ(NavigationObserver::ERROR_PAGE, observer.navigation_result());
}

// Makes sure browser does not crash when the resource scale is very large.
IN_PROC_BROWSER_TEST_F(ChromeURLDataManagerTest, LargeResourceScale) {
  // Valid scale
  NavigationObserver observer(
      browser()->tab_strip_model()->GetActiveWebContents());
  ui_test_utils::NavigateToURL(
      browser(), GURL("chrome://theme/IDR_SETTINGS_FAVICON@2x"));
  EXPECT_EQ(NavigationObserver::SUCCESS, observer.navigation_result());

  // Unreasonably large scale
  observer.Reset();
  ui_test_utils::NavigateToURL(
      browser(), GURL("chrome://theme/IDR_SETTINGS_FAVICON@99999x"));
  EXPECT_EQ(NavigationObserver::ERROR_PAGE, observer.navigation_result());
}
