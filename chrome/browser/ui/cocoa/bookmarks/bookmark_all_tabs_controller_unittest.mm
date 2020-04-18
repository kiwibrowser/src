// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_all_tabs_controller.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using base::ASCIIToUTF16;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

@interface BookmarkAllTabsControllerOverride : BookmarkAllTabsController
@end

@implementation BookmarkAllTabsControllerOverride

- (void)UpdateActiveTabPairs {
  ActiveTabsNameURLPairVector* activeTabPairsVector =
      [self activeTabPairsVector];
  activeTabPairsVector->clear();
  activeTabPairsVector->push_back(
      ActiveTabNameURLPair(ASCIIToUTF16("at-0"), GURL("http://at-0.com")));
  activeTabPairsVector->push_back(
      ActiveTabNameURLPair(ASCIIToUTF16("at-1"), GURL("http://at-1.com")));
  activeTabPairsVector->push_back(
      ActiveTabNameURLPair(ASCIIToUTF16("at-2"), GURL("http://at-2.com")));
}

@end

class BookmarkAllTabsControllerTest : public CocoaProfileTest {
 public:
  const BookmarkNode* parent_node_;
  BookmarkAllTabsControllerOverride* controller_;
  const BookmarkNode* folder_a_;

  void CreateModel() {
    BookmarkModel* model =
        BookmarkModelFactory::GetForBrowserContext(profile());
    const BookmarkNode* root = model->bookmark_bar_node();
    folder_a_ = model->AddFolder(root, 0, ASCIIToUTF16("a"));
    model->AddURL(folder_a_, 0, ASCIIToUTF16("a-0"), GURL("http://a-0.com"));
    model->AddURL(folder_a_, 1, ASCIIToUTF16("a-1"), GURL("http://a-1.com"));
    model->AddURL(folder_a_, 2, ASCIIToUTF16("a-2"), GURL("http://a-2.com"));
  }

  virtual BookmarkAllTabsControllerOverride* CreateController() {
    return [[BookmarkAllTabsControllerOverride alloc]
            initWithParentWindow:test_window()
                         profile:profile()
                          parent:folder_a_
                             url:GURL()
                           title:base::string16()
                   configuration:BookmarkEditor::SHOW_TREE];
  }

  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(profile());

    CreateModel();
    controller_ = CreateController();
    [controller_ runAsModalSheet];
  }

  void TearDown() override {
    controller_ = NULL;
    CocoaProfileTest::TearDown();
  }
};

TEST_F(BookmarkAllTabsControllerTest, BookmarkAllTabs) {
  // OK button should always be enabled.
  EXPECT_TRUE([controller_ okButtonEnabled]);
  [controller_ selectTestNodeInBrowser:folder_a_];
  [controller_ setDisplayName:@"ALL MY TABS"];
  [controller_ ok:nil];
  EXPECT_EQ(4, folder_a_->child_count());
  const BookmarkNode* folderChild = folder_a_->GetChild(3);
  EXPECT_EQ(folderChild->GetTitle(), ASCIIToUTF16("ALL MY TABS"));
  EXPECT_EQ(3, folderChild->child_count());
}
