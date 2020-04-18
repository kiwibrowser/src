// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#include <memory>

#include "base/strings/sys_string_conversions.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "ios/chrome/browser/browser_state/test_chrome_browser_state.h"
#include "ios/chrome/browser/content_settings/host_content_settings_map_factory.h"
#import "ios/chrome/browser/ui/collection_view/collection_view_controller_test.h"
#import "ios/chrome/browser/ui/settings/block_popups_collection_view_controller.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/testing/wait_util.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface SettingsRootCollectionViewController (ExposedForTesting)
- (void)editButtonPressed;
@end

namespace {

const char* kAllowedPattern = "[*.]example.com";
const char* kAllowedURL = "http://example.com";

class BlockPopupsCollectionViewControllerTest
    : public CollectionViewControllerTest {
 protected:
  void SetUp() override {
    CollectionViewControllerTest::SetUp();
    TestChromeBrowserState::Builder test_cbs_builder;
    chrome_browser_state_ = test_cbs_builder.Build();
  }

  CollectionViewController* InstantiateController() override {
    return [[BlockPopupsCollectionViewController alloc]
        initWithBrowserState:chrome_browser_state_.get()];
  }

  void SetDisallowPopups() {
    ios::HostContentSettingsMapFactory::GetForBrowserState(
        chrome_browser_state_.get())
        ->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS,
                                   CONTENT_SETTING_BLOCK);
  }

  void SetAllowPopups() {
    ios::HostContentSettingsMapFactory::GetForBrowserState(
        chrome_browser_state_.get())
        ->SetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS,
                                   CONTENT_SETTING_ALLOW);
  }

  void AddAllowedPattern(const std::string& pattern, const GURL& url) {
    ContentSettingsPattern allowed_pattern =
        ContentSettingsPattern::FromString(pattern);

    ios::HostContentSettingsMapFactory::GetForBrowserState(
        chrome_browser_state_.get())
        ->SetContentSettingCustomScope(
            allowed_pattern, ContentSettingsPattern::Wildcard(),
            CONTENT_SETTINGS_TYPE_POPUPS, std::string(), CONTENT_SETTING_ALLOW);
    EXPECT_EQ(CONTENT_SETTING_ALLOW,
              ios::HostContentSettingsMapFactory::GetForBrowserState(
                  chrome_browser_state_.get())
                  ->GetContentSetting(url, url, CONTENT_SETTINGS_TYPE_POPUPS,
                                      std::string()));
  }

  web::TestWebThreadBundle thread_bundle_;
  std::unique_ptr<TestChromeBrowserState> chrome_browser_state_;
};

TEST_F(BlockPopupsCollectionViewControllerTest, TestPopupsNotAllowed) {
  SetDisallowPopups();
  CreateController();
  CheckController();
  EXPECT_EQ(1, NumberOfSections());
}

TEST_F(BlockPopupsCollectionViewControllerTest, TestPopupsAllowed) {
  SetAllowPopups();
  CreateController();
  CheckController();
  EXPECT_EQ(1, NumberOfSections());
  EXPECT_FALSE([controller() navigationItem].rightBarButtonItem);
}

TEST_F(BlockPopupsCollectionViewControllerTest, TestPopupsAllowedWithOneItem) {
  // Ensure that even if there are 'allowed' patterns, if block popups is
  // turned off (popups are allowed), there is no list of patterns.
  AddAllowedPattern(kAllowedPattern, GURL(kAllowedURL));
  SetAllowPopups();

  CreateController();

  EXPECT_EQ(1, NumberOfSections());
  EXPECT_FALSE([controller() navigationItem].rightBarButtonItem);
}

TEST_F(BlockPopupsCollectionViewControllerTest, TestOneAllowedItem) {
  AddAllowedPattern(kAllowedPattern, GURL(kAllowedURL));

  CreateController();

  EXPECT_EQ(2, NumberOfSections());
  EXPECT_EQ(1, NumberOfItemsInSection(1));
  CheckSectionHeaderWithId(IDS_IOS_POPUPS_ALLOWED, 1);
  CheckTextCellTitle(base::SysUTF8ToNSString(kAllowedPattern), 1, 0);
  EXPECT_TRUE([controller() navigationItem].rightBarButtonItem);
}

TEST_F(BlockPopupsCollectionViewControllerTest, TestOneAllowedItemDeleted) {
  // Get the number of entries before testing, to ensure after adding and
  // deleting, the entries are the same.
  ContentSettingsForOneType initial_entries;
  ios::HostContentSettingsMapFactory::GetForBrowserState(
      chrome_browser_state_.get())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_POPUPS, std::string(),
                              &initial_entries);

  // Add the pattern to be deleted.
  AddAllowedPattern(kAllowedPattern, GURL(kAllowedURL));

  // Make sure adding the pattern changed the settings size.
  ContentSettingsForOneType added_entries;
  ios::HostContentSettingsMapFactory::GetForBrowserState(
      chrome_browser_state_.get())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_POPUPS, std::string(),
                              &added_entries);
  EXPECT_NE(initial_entries.size(), added_entries.size());

  CreateController();

  BlockPopupsCollectionViewController* popups_controller =
      static_cast<BlockPopupsCollectionViewController*>(controller());
  // Put the collectionView in 'edit' mode.
  [popups_controller editButtonPressed];
  // This is a bit of a shortcut, since actually clicking on the 'delete'
  // button would be tough.
  void (^delete_item_with_wait)(int, int) = ^(int i, int j) {
    __block BOOL completion_called = NO;
    this->DeleteItem(i, j, ^{
      completion_called = YES;
    });
    EXPECT_TRUE(testing::WaitUntilConditionOrTimeout(
        testing::kWaitForUIElementTimeout, ^bool() {
          return completion_called;
        }));
  };

  delete_item_with_wait(1, 0);
  // Exit 'edit' mode.
  [popups_controller editButtonPressed];

  // Verify the resulting UI.
  EXPECT_EQ(1, NumberOfSections());
  EXPECT_EQ(1, NumberOfItemsInSection(0));

  // Verify that there are no longer any allowed patterns in |profile_|.
  ContentSettingsForOneType final_entries;
  ios::HostContentSettingsMapFactory::GetForBrowserState(
      chrome_browser_state_.get())
      ->GetSettingsForOneType(CONTENT_SETTINGS_TYPE_POPUPS, std::string(),
                              &final_entries);
  EXPECT_EQ(initial_entries.size(), final_entries.size());
}

}  // namespace
