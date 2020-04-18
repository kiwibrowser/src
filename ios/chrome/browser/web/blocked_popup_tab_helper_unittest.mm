// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/blocked_popup_tab_helper.h"

#include "base/memory/ptr_util.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_manager.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "ios/chrome/browser/infobars/infobar_manager_impl.h"
#import "ios/chrome/browser/web/chrome_web_test.h"
#import "ios/web/public/test/fakes/test_navigation_manager.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#import "ios/web/public/test/fakes/test_web_state_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using web::WebState;

// Test fixture for BlockedPopupTabHelper class.
class BlockedPopupTabHelperTest : public ChromeWebTest {
 protected:
  void SetUp() override {
    ChromeWebTest::SetUp();
    web_state()->SetDelegate(&web_state_delegate_);
    BlockedPopupTabHelper::CreateForWebState(web_state());
    InfoBarManagerImpl::CreateForWebState(web_state());
  }

  // Returns true if InfoBarManager is being observed.
  bool IsObservingSources() {
    return GetBlockedPopupTabHelper()->scoped_observer_.IsObservingSources();
  }

  // Returns BlockedPopupTabHelper that is being tested.
  BlockedPopupTabHelper* GetBlockedPopupTabHelper() {
    return BlockedPopupTabHelper::FromWebState(web_state());
  }

  // Returns InfoBarManager attached to |web_state()|.
  infobars::InfoBarManager* GetInfobarManager() {
    return InfoBarManagerImpl::FromWebState(web_state());
  }

  web::TestWebStateDelegate web_state_delegate_;
};

// Tests ShouldBlockPopup method. This test changes content settings without
// restoring them back, which is fine because changes do not persist across test
// runs.
TEST_F(BlockedPopupTabHelperTest, ShouldBlockPopup) {
  const GURL source_url1("https://source-url1");
  EXPECT_TRUE(GetBlockedPopupTabHelper()->ShouldBlockPopup(source_url1));

  // Allow popups for |source_url1|.
  scoped_refptr<HostContentSettingsMap> settings_map(
      ios::HostContentSettingsMapFactory::GetForBrowserState(
          chrome_browser_state_.get()));
  settings_map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromURL(source_url1),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_POPUPS,
      std::string(), CONTENT_SETTING_ALLOW);

  EXPECT_FALSE(GetBlockedPopupTabHelper()->ShouldBlockPopup(source_url1));
  const GURL source_url2("https://source-url2");
  EXPECT_TRUE(GetBlockedPopupTabHelper()->ShouldBlockPopup(source_url2));

  // Allow all popups.
  settings_map->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS,
                                         CONTENT_SETTING_ALLOW);

  EXPECT_FALSE(GetBlockedPopupTabHelper()->ShouldBlockPopup(source_url1));
  EXPECT_FALSE(GetBlockedPopupTabHelper()->ShouldBlockPopup(source_url2));
}

// Tests that allowing blocked popup opens a child window and allows future
// popups for the source url.
TEST_F(BlockedPopupTabHelperTest, AllowBlockedPopup) {
  const GURL source_url("https://source-url");
  ASSERT_TRUE(GetBlockedPopupTabHelper()->ShouldBlockPopup(source_url));

  // Block popup.
  const GURL target_url("https://target-url");
  web::Referrer referrer(source_url, web::ReferrerPolicyDefault);
  GetBlockedPopupTabHelper()->HandlePopup(target_url, referrer);

  // Allow blocked popup.
  ASSERT_EQ(1U, GetInfobarManager()->infobar_count());
  infobars::InfoBar* infobar = GetInfobarManager()->infobar_at(0);
  auto* delegate = infobar->delegate()->AsConfirmInfoBarDelegate();
  ASSERT_TRUE(delegate);
  ASSERT_FALSE(web_state_delegate_.last_open_url_request());
  delegate->Accept();

  // Verify that popups are allowed for |test_url|.
  EXPECT_FALSE(GetBlockedPopupTabHelper()->ShouldBlockPopup(source_url));

  // Verify that child window was open.
  auto* open_url_request = web_state_delegate_.last_open_url_request();
  ASSERT_TRUE(open_url_request);
  EXPECT_EQ(web_state(), open_url_request->web_state);
  WebState::OpenURLParams params = open_url_request->params;
  EXPECT_EQ(target_url, params.url);
  EXPECT_EQ(source_url, params.referrer.url);
  EXPECT_EQ(web::ReferrerPolicyDefault, params.referrer.policy);
  EXPECT_EQ(WindowOpenDisposition::NEW_POPUP, params.disposition);
  EXPECT_TRUE(
      PageTransitionCoreTypeIs(params.transition, ui::PAGE_TRANSITION_LINK));
  EXPECT_TRUE(params.is_renderer_initiated);
}

// Tests that destroying WebState while Infobar is presented does not crash.
TEST_F(BlockedPopupTabHelperTest, DestroyWebState) {
  const GURL source_url("https://source-url");
  ASSERT_TRUE(GetBlockedPopupTabHelper()->ShouldBlockPopup(source_url));

  // Block popup.
  const GURL target_url("https://target-url");
  web::Referrer referrer(source_url, web::ReferrerPolicyDefault);
  GetBlockedPopupTabHelper()->HandlePopup(target_url, referrer);

  // Verify that destroying WebState does not crash.
  DestroyWebState();
}

// Tests that an infobar is added to the infobar manager when
// BlockedPopupTabHelper::HandlePopup() is called.
TEST_F(BlockedPopupTabHelperTest, ShowAndDismissInfoBar) {
  // Check that there are no infobars showing and no registered observers.
  EXPECT_EQ(0U, GetInfobarManager()->infobar_count());
  EXPECT_FALSE(IsObservingSources());

  // Call |HandlePopup| to show an infobar.
  const GURL test_url("https://popups.example.com");
  GetBlockedPopupTabHelper()->HandlePopup(test_url, web::Referrer());
  ASSERT_EQ(1U, GetInfobarManager()->infobar_count());
  EXPECT_TRUE(IsObservingSources());

  // Dismiss the infobar and check that the tab helper no longer has any
  // registered observers.
  GetInfobarManager()->infobar_at(0)->RemoveSelf();
  EXPECT_EQ(0U, GetInfobarManager()->infobar_count());
  EXPECT_FALSE(IsObservingSources());
}
