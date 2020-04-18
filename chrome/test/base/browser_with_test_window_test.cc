// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/browser_with_test_window_test.h"

#include "base/location.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile_destroyer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/browser_navigator_params.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_side_navigation_test_utils.h"
#include "content/public/test/test_renderer_host.h"
#include "ui/base/page_transition_types.h"
#include "ui/views/test/test_views_delegate.h"

#if defined(TOOLKIT_VIEWS)
#include "chrome/browser/ui/views/chrome_constrained_window_views_client.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "components/constrained_window/constrained_window_views.h"

#if defined(OS_CHROMEOS)
#include "chrome/test/base/ash_test_environment_chrome.h"
#else
#include "ui/views/test/test_views_delegate.h"
#endif
#endif

using content::NavigationController;
using content::RenderFrameHost;
using content::RenderFrameHostTester;
using content::WebContents;

BrowserWithTestWindowTest::BrowserWithTestWindowTest()
    : BrowserWithTestWindowTest(Browser::TYPE_TABBED, false) {}

BrowserWithTestWindowTest::BrowserWithTestWindowTest(Browser::Type browser_type,
                                                     bool hosted_app)
    : browser_type_(browser_type), hosted_app_(hosted_app) {
#if defined(OS_CHROMEOS)
  ash_test_environment_ = std::make_unique<AshTestEnvironmentChrome>();
  ash_test_helper_ =
      std::make_unique<ash::AshTestHelper>(ash_test_environment_.get());
#endif
}

BrowserWithTestWindowTest::~BrowserWithTestWindowTest() {}

void BrowserWithTestWindowTest::SetUp() {
  testing::Test::SetUp();
#if defined(OS_CHROMEOS)
  ash_test_helper_->SetUp(true);
#elif defined(TOOLKIT_VIEWS)
  views_test_helper_.reset(new views::ScopedViewsTestHelper());
#endif
#if defined(TOOLKIT_VIEWS)
  SetConstrainedWindowViewsClient(CreateChromeConstrainedWindowViewsClient());

  test_views_delegate()->set_layout_provider(
      ChromeLayoutProvider::CreateLayoutProvider());
#endif

  content::BrowserSideNavigationSetUp();

  profile_manager_ = std::make_unique<TestingProfileManager>(
      TestingBrowserProcess::GetGlobal());
  ASSERT_TRUE(profile_manager_->SetUp());

  // Subclasses can provide their own Profile.
  profile_ = CreateProfile();
  // Subclasses can provide their own test BrowserWindow. If they return NULL
  // then Browser will create the a production BrowserWindow and the subclass
  // is responsible for cleaning it up (usually by NativeWidget destruction).
  window_.reset(CreateBrowserWindow());

  browser_.reset(
      CreateBrowser(profile(), browser_type_, hosted_app_, window_.get()));
}

void BrowserWithTestWindowTest::TearDown() {
  // Some tests end up posting tasks to the DB thread that must be completed
  // before the profile can be destroyed and the test safely shut down.
  base::RunLoop().RunUntilIdle();

  // Close the browser tabs and destroy the browser and window instances.
  if (browser_)
    browser_->tab_strip_model()->CloseAllTabs();
  browser_.reset();
  window_.reset();

  content::BrowserSideNavigationTearDown();

#if defined(TOOLKIT_VIEWS)
  constrained_window::SetConstrainedWindowViewsClient(nullptr);
#endif

  profile_manager_->DeleteAllTestingProfiles();
  profile_ = nullptr;
  profile_manager_.reset();

#if defined(OS_CHROMEOS)
  ash_test_helper_->TearDown();
#elif defined(TOOLKIT_VIEWS)
  views_test_helper_.reset();
#endif

  testing::Test::TearDown();

  // A Task is leaked if we don't destroy everything, then run the message loop.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::RunLoop::QuitCurrentWhenIdleClosureDeprecated());
  base::RunLoop().Run();
}

gfx::NativeWindow BrowserWithTestWindowTest::GetContext() {
#if defined(OS_CHROMEOS)
  return ash_test_helper_->CurrentContext();
#elif defined(TOOLKIT_VIEWS)
  return views_test_helper_->GetContext();
#else
  return nullptr;
#endif
}

void BrowserWithTestWindowTest::AddTab(Browser* browser, const GURL& url) {
  NavigateParams params(browser, url, ui::PAGE_TRANSITION_TYPED);
  params.tabstrip_index = 0;
  params.disposition = WindowOpenDisposition::NEW_FOREGROUND_TAB;
  Navigate(&params);
  CommitPendingLoad(&params.navigated_or_inserted_contents->GetController());
}

void BrowserWithTestWindowTest::CommitPendingLoad(
  NavigationController* controller) {
  if (!controller->GetPendingEntry())
    return;  // Nothing to commit.

  RenderFrameHostTester::CommitPendingLoad(controller);
}

void BrowserWithTestWindowTest::NavigateAndCommit(
    NavigationController* controller,
    const GURL& url) {
  controller->LoadURL(
      url, content::Referrer(), ui::PAGE_TRANSITION_LINK, std::string());
  CommitPendingLoad(controller);
}

void BrowserWithTestWindowTest::NavigateAndCommitActiveTab(const GURL& url) {
  NavigateAndCommit(&browser()->tab_strip_model()->GetActiveWebContents()->
                        GetController(),
                    url);
}

void BrowserWithTestWindowTest::NavigateAndCommitActiveTabWithTitle(
    Browser* navigating_browser,
    const GURL& url,
    const base::string16& title) {
  WebContents* contents =
      navigating_browser->tab_strip_model()->GetActiveWebContents();
  NavigationController* controller = &contents->GetController();
  NavigateAndCommit(controller, url);
  contents->UpdateTitleForEntry(controller->GetActiveEntry(), title);
}

TestingProfile* BrowserWithTestWindowTest::CreateProfile() {
  return profile_manager_->CreateTestingProfile(
      "testing_profile", nullptr, base::string16(), 0, std::string(),
      GetTestingFactories());
}

TestingProfile::TestingFactories
BrowserWithTestWindowTest::GetTestingFactories() {
  return {};
}

BrowserWindow* BrowserWithTestWindowTest::CreateBrowserWindow() {
  return new TestBrowserWindow();
}

Browser* BrowserWithTestWindowTest::CreateBrowser(
    Profile* profile,
    Browser::Type browser_type,
    bool hosted_app,
    BrowserWindow* browser_window) {
  Browser::CreateParams params(profile, true);
  if (hosted_app) {
    params = Browser::CreateParams::CreateForApp(
        "Test", true /* trusted_source */, gfx::Rect(), profile, true);
  } else {
    params.type = browser_type;
  }
  params.window = browser_window;
  return new Browser(params);
}
