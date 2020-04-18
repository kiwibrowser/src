// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/voice/voice_search_navigations_tab_helper.h"

#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/test/web_test_with_web_state.h"
#import "ios/web/public/web_state/web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test fixture for VoiceSearchNavigations.
class VoiceSearchNavigationsTest : public web::WebTestWithWebState {
 public:
  void SetUp() override {
    web::WebTestWithWebState::SetUp();
    VoiceSearchNavigationTabHelper::CreateForWebState(web_state());
  }

  VoiceSearchNavigationTabHelper* navigations() {
    return VoiceSearchNavigationTabHelper::FromWebState(web_state());
  }
};

// Tests that a NavigationItem is not marked as a voice search if
// WillLoadVoiceSearchResult() was not called.
TEST_F(VoiceSearchNavigationsTest, NotVoiceSearchNavigation) {
  LoadHtml(@"<html></html>");
  web::NavigationItem* item =
      web_state()->GetNavigationManager()->GetLastCommittedItem();
  EXPECT_FALSE(navigations()->IsNavigationFromVoiceSearch(item));
}

// Tests that a pending NavigationItem is recorded as a voice search navigation
// if it is added after calling WillLoadVoiceSearchResult().
TEST_F(VoiceSearchNavigationsTest, PendingVoiceSearchNavigation) {
  navigations()->WillLoadVoiceSearchResult();
  const GURL kPendingUrl("http://pending.test");
  AddPendingItem(kPendingUrl, ui::PAGE_TRANSITION_LINK);
  web::NavigationItem* item =
      web_state()->GetNavigationManager()->GetPendingItem();
  EXPECT_TRUE(navigations()->IsNavigationFromVoiceSearch(item));
}

// Tests that a committed NavigationItem is recordored as a voice search
// navigation if it occurs after calling WillLoadVoiceSearchResult().
TEST_F(VoiceSearchNavigationsTest, CommittedVoiceSearchNavigation) {
  navigations()->WillLoadVoiceSearchResult();
  LoadHtml(@"<html></html>");
  web::NavigationItem* item =
      web_state()->GetNavigationManager()->GetLastCommittedItem();
  EXPECT_TRUE(navigations()->IsNavigationFromVoiceSearch(item));
}

// Tests that navigations that occur after a voice search navigation are not
// marked as voice search navigations.
TEST_F(VoiceSearchNavigationsTest, NavigationAfterVoiceSearch) {
  navigations()->WillLoadVoiceSearchResult();
  const GURL kVoiceSearchUrl("http://voice.test");
  LoadHtml(@"<html></html>", kVoiceSearchUrl);
  web::NavigationItem* item =
      web_state()->GetNavigationManager()->GetLastCommittedItem();
  EXPECT_TRUE(navigations()->IsNavigationFromVoiceSearch(item));
  // Load another page without calling WillLoadVoiceSearchResult().
  const GURL kNonVoiceSearchUrl("http://not-voice.test");
  LoadHtml(@"<html></html>", kNonVoiceSearchUrl);
  item = web_state()->GetNavigationManager()->GetLastCommittedItem();
  EXPECT_FALSE(navigations()->IsNavigationFromVoiceSearch(item));
}

// Tests that transient NavigationItems are handled the same as pending items
TEST_F(VoiceSearchNavigationsTest, TransientNavigations) {
  LoadHtml(@"<html></html>", GURL("http://committed_url.test"));
  const GURL kTransientURL("http://transient.test");
  AddTransientItem(kTransientURL);
  web::NavigationItem* item =
      web_state()->GetNavigationManager()->GetTransientItem();
  EXPECT_FALSE(navigations()->IsNavigationFromVoiceSearch(item));
  navigations()->WillLoadVoiceSearchResult();
  EXPECT_TRUE(navigations()->IsNavigationFromVoiceSearch(item));
}
