// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_BASE_BROWSER_WITH_TEST_WINDOW_TEST_H_
#define CHROME_TEST_BASE_BROWSER_WITH_TEST_WINDOW_TEST_H_

#include "base/at_exit.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(TOOLKIT_VIEWS)
#if defined(OS_CHROMEOS)
#include "ash/test/ash_test_helper.h"
#include "ash/test/ash_test_views_delegate.h"
#include "chrome/browser/chromeos/login/users/scoped_test_user_manager.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#else
#include "ui/views/test/scoped_views_test_helper.h"
#endif
#endif

#if defined(OS_WIN)
#include "ui/base/win/scoped_ole_initializer.h"
#endif

class GURL;

#if defined(TOOLKIT_VIEWS)
namespace views {
class TestViewsDelegate;
}
#if defined(OS_CHROMEOS)
namespace ash {
namespace test {
class AshTestEnvironment;
}
}
#endif
#endif

namespace content {
class NavigationController;
}

class TestingProfileManager;

// Base class for browser based unit tests. BrowserWithTestWindowTest creates a
// Browser with a TestingProfile and TestBrowserWindow. To add a tab use
// AddTab. For example, the following adds a tab and navigates to
// two URLs that target the TestWebContents:
//
//   // Add a new tab and navigate it. This will be at index 0.
//   AddTab(browser(), GURL("http://foo/1"));
//   NavigationController* controller =
//       &browser()->tab_strip_model()->GetWebContentsAt(0)->GetController();
//
//   // Navigate somewhere else.
//   GURL url2("http://foo/2");
//   NavigateAndCommit(controller, url2);
//
//   // This is equivalent to the above, and lets you test pending navigations.
//   browser()->OpenURL(OpenURLParams(
//       GURL("http://foo/2"), GURL(), WindowOpenDisposition::CURRENT_TAB,
//       ui::PAGE_TRANSITION_TYPED, false));
//   CommitPendingLoad(controller);
//
// Subclasses must invoke BrowserWithTestWindowTest::SetUp as it is responsible
// for creating the various objects of this class.
class BrowserWithTestWindowTest : public testing::Test {
 public:
  // Creates a BrowserWithTestWindowTest for which the initial window will be
  // a tabbed browser created on the native desktop, which is not a hosted app.
  BrowserWithTestWindowTest();

  // Creates a BrowserWithTestWindowTest for which the initial window will be
  // the specified type.
  BrowserWithTestWindowTest(Browser::Type browser_type, bool hosted_app);

  ~BrowserWithTestWindowTest() override;

  void SetUp() override;
  void TearDown() override;

 protected:
  BrowserWindow* window() const { return window_.get(); }

  Browser* browser() const { return browser_.get(); }
  void set_browser(Browser* browser) {
    browser_.reset(browser);
  }
  Browser* release_browser() WARN_UNUSED_RESULT {
    return browser_.release();
  }

  TestingProfile* profile() const { return profile_; }

  TestingProfile* GetProfile() { return profile_; }

  TestingProfileManager* profile_manager() { return profile_manager_.get(); }

  BrowserWindow* release_browser_window() WARN_UNUSED_RESULT {
    return window_.release();
  }

#if defined(OS_CHROMEOS)
  ash::AshTestHelper* ash_test_helper() { return ash_test_helper_.get(); }
#endif

  // The context to help determine desktop type when creating new Widgets.
  gfx::NativeWindow GetContext();

  // Adds a tab to |browser| with the given URL and commits the load.
  // This is a convenience function. The new tab will be added at index 0.
  void AddTab(Browser* browser, const GURL& url);

  // Commits the pending load on the given controller. It will keep the
  // URL of the pending load. If there is no pending load, this does nothing.
  void CommitPendingLoad(content::NavigationController* controller);

  // Creates a pending navigation on the given navigation controller to the
  // given URL with the default parameters and the commits the load with a page
  // ID one larger than any seen. This emulates what happens on a new
  // navigation.
  void NavigateAndCommit(content::NavigationController* controller,
                         const GURL& url);

  // Navigates the current tab. This is a wrapper around NavigateAndCommit.
  void NavigateAndCommitActiveTab(const GURL& url);

  // Set the |title| of the current tab.
  void NavigateAndCommitActiveTabWithTitle(Browser* browser,
                                           const GURL& url,
                                           const base::string16& title);

  // Creates the profile used by this test. The caller owns the return value.
  virtual TestingProfile* CreateProfile();

  // Returns a vector of testing factories to be used when creating the profile.
  // This is only used by CreateProfile(), and will be irrelevant if that
  // method is overridden.
  virtual TestingProfile::TestingFactories GetTestingFactories();

  // Creates the BrowserWindow used by this test. The caller owns the return
  // value. Can return NULL to use the default window created by Browser.
  virtual BrowserWindow* CreateBrowserWindow();

  // Creates the browser given |profile|, |browser_type|, |hosted_app|, and
  // |browser_window|. The caller owns the return value.
  virtual Browser* CreateBrowser(Profile* profile,
                                 Browser::Type browser_type,
                                 bool hosted_app,
                                 BrowserWindow* browser_window);

#if defined(TOOLKIT_VIEWS)
  views::TestViewsDelegate* test_views_delegate() {
#if defined(OS_CHROMEOS)
    return ash_test_helper_->test_views_delegate();
#else
    return views_test_helper_->test_views_delegate();
#endif
  }
#endif

 private:
  // We need to create a MessageLoop, otherwise a bunch of things fails.
  content::TestBrowserThreadBundle thread_bundle_;
  base::ShadowingAtExitManager at_exit_manager_;

#if defined(OS_CHROMEOS)
  chromeos::ScopedTestDeviceSettingsService test_device_settings_service_;
  chromeos::ScopedTestCrosSettings test_cros_settings_;
  chromeos::ScopedTestUserManager test_user_manager_;
#endif

  TestingProfile* profile_;

  std::unique_ptr<TestingProfileManager> profile_manager_;
  std::unique_ptr<BrowserWindow> window_;  // Usually a TestBrowserWindow.
  std::unique_ptr<Browser> browser_;

  // The existence of this object enables tests via
  // RenderViewHostTester.
  content::RenderViewHostTestEnabler rvh_test_enabler_;

#if defined(OS_CHROMEOS)
  std::unique_ptr<ash::AshTestEnvironment> ash_test_environment_;
  std::unique_ptr<ash::AshTestHelper> ash_test_helper_;
#elif defined(TOOLKIT_VIEWS)
  std::unique_ptr<views::ScopedViewsTestHelper> views_test_helper_;
#endif

#if defined(OS_WIN)
  ui::ScopedOleInitializer ole_initializer_;
#endif

  // The type of browser to create (tabbed or popup).
  Browser::Type browser_type_;

  // Whether the browser is part of a hosted app.
  bool hosted_app_;

  DISALLOW_COPY_AND_ASSIGN(BrowserWithTestWindowTest);
};

#endif  // CHROME_TEST_BASE_BROWSER_WITH_TEST_WINDOW_TEST_H_
