// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/startup/session_crashed_infobar_delegate.h"

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/sessions/session_service_factory.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/browser_with_test_window_test.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_registry_simple.h"

using SessionCrashedInfoBarDelegateUnitTest = BrowserWithTestWindowTest;

TEST_F(SessionCrashedInfoBarDelegateUnitTest, DetachingTabWithCrashedInfoBar) {
  SessionServiceFactory::SetForTestProfile(
      browser()->profile(),
      base::WrapUnique(static_cast<SessionService*>(
          SessionServiceFactory::GetInstance()->BuildServiceInstanceFor(
              browser()->profile()))));

  // Create a browser which we can close during the test.
  Browser::CreateParams params(browser()->profile(), true);
  std::unique_ptr<Browser> first_browser(
      CreateBrowserWithTestWindowForParams(&params));
  AddTab(first_browser.get(), GURL(chrome::kChromeUINewTabURL));

  // Attach the crashed infobar to it.
  SessionCrashedInfoBarDelegate::Create(first_browser.get());

  TabStripModel* tab_strip = first_browser->tab_strip_model();
  ASSERT_EQ(1, tab_strip->count());
  content::WebContents* web_contents = tab_strip->GetWebContentsAt(0);
  InfoBarService* infobar_service =
      InfoBarService::FromWebContents(web_contents);
  EXPECT_EQ(1U, infobar_service->infobar_count());
  ConfirmInfoBarDelegate* infobar =
      infobar_service->infobar_at(0)->delegate()->AsConfirmInfoBarDelegate();
  ASSERT_TRUE(infobar);

  // Open another browser.
  std::unique_ptr<Browser> opened_browser(
      CreateBrowserWithTestWindowForParams(&params));

  // Move the tab which is destroying the crash info bar to the new browser.
  std::unique_ptr<content::WebContents> owned_web_contents =
      tab_strip->DetachWebContentsAt(0);
  tab_strip = opened_browser->tab_strip_model();
  tab_strip->AppendWebContents(std::move(owned_web_contents), true);

  // Close the original browser.
  first_browser->window()->Close();
  first_browser.reset();

  ASSERT_EQ(1, tab_strip->count());
  infobar_service =
      InfoBarService::FromWebContents(tab_strip->GetWebContentsAt(0));
  EXPECT_EQ(1U, infobar_service->infobar_count());

  // This used to crash.
  infobar->Accept();

  // Ramp down the test.
  tab_strip->CloseWebContentsAt(0, TabStripModel::CLOSE_NONE);
}
