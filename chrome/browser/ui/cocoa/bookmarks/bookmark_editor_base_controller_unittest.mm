// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/mac_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_editor_controller.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/test_browser_window.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/strings/grit/components_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"
#import "ui/base/cocoa/touch_bar_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

using base::ASCIIToUTF16;
using bookmarks::BookmarkExpandedStateTracker;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace {

// Touch bar identifier.
NSString* const kBookmarkEditDialogTouchBarId = @"bookmark-edit-dialog";

// Touch bar item identifiers.
NSString* const kNewFolderTouchBarId = @"NEW-FOLDER";
NSString* const kCancelTouchBarId = @"CANCEL";
NSString* const kSaveTouchBarId = @"SAVE";

}  // end namespace

class BookmarkEditorBaseControllerTest : public CocoaProfileTest {
 public:
  BookmarkEditorBaseController* controller_;  // weak
  const BookmarkNode* folder_a_;
  const BookmarkNode* folder_b_;
  const BookmarkNode* folder_b_0_;
  const BookmarkNode* folder_b_3_;
  const BookmarkNode* folder_c_;

  void CreateModel() {
    // Set up a small bookmark hierarchy, which will look as follows:
    //    a      b      c    d
    //     a-0    b-0    c-0
    //     a-1     b-00  c-1
    //     a-2    b-1    c-2
    //            b-2    c-3
    //            b-3
    //             b-30
    //             b-31
    //            b-4
    BookmarkModel* model =
        BookmarkModelFactory::GetForBrowserContext(profile());
    const BookmarkNode* root = model->bookmark_bar_node();
    folder_a_ = model->AddFolder(root, 0, ASCIIToUTF16("a"));
    model->AddURL(folder_a_, 0, ASCIIToUTF16("a-0"), GURL("http://a-0.com"));
    model->AddURL(folder_a_, 1, ASCIIToUTF16("a-1"), GURL("http://a-1.com"));
    model->AddURL(folder_a_, 2, ASCIIToUTF16("a-2"), GURL("http://a-2.com"));

    folder_b_ = model->AddFolder(root, 1, ASCIIToUTF16("b"));
    folder_b_0_ = model->AddFolder(folder_b_, 0, ASCIIToUTF16("b-0"));
    model->AddURL(folder_b_0_, 0, ASCIIToUTF16("bb-0"),
                  GURL("http://bb-0.com"));
    model->AddURL(folder_b_, 1, ASCIIToUTF16("b-1"), GURL("http://b-1.com"));
    model->AddURL(folder_b_, 2, ASCIIToUTF16("b-2"), GURL("http://b-2.com"));
    folder_b_3_ = model->AddFolder(folder_b_, 3, ASCIIToUTF16("b-3"));
    model->AddURL(folder_b_3_, 0, ASCIIToUTF16("b-30"),
                  GURL("http://b-30.com"));
    model->AddURL(folder_b_3_, 1, ASCIIToUTF16("b-31"),
                  GURL("http://b-31.com"));
    model->AddURL(folder_b_, 4, ASCIIToUTF16("b-4"), GURL("http://b-4.com"));

    folder_c_ = model->AddFolder(root, 2, ASCIIToUTF16("c"));
    model->AddURL(folder_c_, 0, ASCIIToUTF16("c-0"), GURL("http://c-0.com"));
    model->AddURL(folder_c_, 1, ASCIIToUTF16("c-1"), GURL("http://c-1.com"));
    model->AddURL(folder_c_, 2, ASCIIToUTF16("c-2"), GURL("http://c-2.com"));
    model->AddURL(folder_c_, 3, ASCIIToUTF16("c-3"), GURL("http://c-3.com"));
    model->AddFolder(folder_c_, 4, ASCIIToUTF16("c-4"));

    model->AddURL(root, 3, ASCIIToUTF16("d"), GURL("http://d-0.com"));
  }

  virtual BookmarkEditorBaseController* CreateController() {
    return [[BookmarkEditorBaseController alloc]
            initWithParentWindow:test_window()
                         nibName:@"BookmarkAllTabs"
                         profile:profile()
                          parent:folder_b_0_
                             url:GURL()
                           title:base::string16()
                   configuration:BookmarkEditor::SHOW_TREE];
  }

  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(profile());

    CreateModel();
    controller_ = CreateController();
    EXPECT_TRUE([controller_ window]);
    [controller_ runAsModalSheet];
  }

  void TearDown() override {
    controller_ = NULL;
    CocoaTest::TearDown();
  }

  Browser* CreateBrowser() override {
    Browser::CreateParams params(profile(), true);
    return CreateBrowserWithTestWindowForParams(&params).release();
  }
};

TEST_F(BookmarkEditorBaseControllerTest, VerifyBookmarkTestModel) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  EXPECT_EQ(4, root->child_count());
  // a
  const BookmarkNode* child = root->GetChild(0);
  EXPECT_EQ(3, child->child_count());
  const BookmarkNode* subchild = child->GetChild(0);
  EXPECT_EQ(0, subchild->child_count());
  subchild = child->GetChild(1);
  EXPECT_EQ(0, subchild->child_count());
  subchild = child->GetChild(2);
  EXPECT_EQ(0, subchild->child_count());
  // b
  child = root->GetChild(1);
  EXPECT_EQ(5, child->child_count());
  subchild = child->GetChild(0);
  EXPECT_EQ(1, subchild->child_count());
  const BookmarkNode* subsubchild = subchild->GetChild(0);
  EXPECT_EQ(0, subsubchild->child_count());
  subchild = child->GetChild(1);
  EXPECT_EQ(0, subchild->child_count());
  subchild = child->GetChild(2);
  EXPECT_EQ(0, subchild->child_count());
  subchild = child->GetChild(3);
  EXPECT_EQ(2, subchild->child_count());
  subsubchild = subchild->GetChild(0);
  EXPECT_EQ(0, subsubchild->child_count());
  subsubchild = subchild->GetChild(1);
  EXPECT_EQ(0, subsubchild->child_count());
  subchild = child->GetChild(4);
  EXPECT_EQ(0, subchild->child_count());
  // c
  child = root->GetChild(2);
  EXPECT_EQ(5, child->child_count());
  subchild = child->GetChild(0);
  EXPECT_EQ(0, subchild->child_count());
  subchild = child->GetChild(1);
  EXPECT_EQ(0, subchild->child_count());
  subchild = child->GetChild(2);
  EXPECT_EQ(0, subchild->child_count());
  subchild = child->GetChild(3);
  EXPECT_EQ(0, subchild->child_count());
  subchild = child->GetChild(4);
  EXPECT_EQ(0, subchild->child_count());
  // d
  child = root->GetChild(3);
  EXPECT_EQ(0, child->child_count());
  [controller_ cancel:nil];
}

TEST_F(BookmarkEditorBaseControllerTest, NodeSelection) {
  EXPECT_TRUE([controller_ folderTreeArray]);
  [controller_ selectTestNodeInBrowser:folder_b_3_];
  const BookmarkNode* node = [controller_ selectedNode];
  EXPECT_EQ(node, folder_b_3_);
  [controller_ cancel:nil];
}

TEST_F(BookmarkEditorBaseControllerTest, CreateFolder) {
  EXPECT_EQ(2, folder_b_3_->child_count());
  [controller_ selectTestNodeInBrowser:folder_b_3_];
  NSString* expectedName =
      l10n_util::GetNSStringWithFixup(IDS_BOOKMARK_EDITOR_NEW_FOLDER_NAME);
  [controller_ setDisplayName:expectedName];
  [controller_ newFolder:nil];
  NSArray* selectionPaths = [controller_ tableSelectionPaths];
  EXPECT_EQ(1U, [selectionPaths count]);
  NSIndexPath* selectionPath = [selectionPaths objectAtIndex:0];
  EXPECT_EQ(4U, [selectionPath length]);
  BookmarkFolderInfo* newFolderInfo = [controller_ selectedFolder];
  EXPECT_TRUE(newFolderInfo);
  NSString* newFolderName = [newFolderInfo folderName];
  EXPECT_NSEQ(expectedName, newFolderName);
  [controller_ createNewFolders];
  // Verify that the tab folder was added to the new folder.
  EXPECT_EQ(3, folder_b_3_->child_count());
  [controller_ cancel:nil];
}

TEST_F(BookmarkEditorBaseControllerTest, CreateTwoFolders) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* bar = model->bookmark_bar_node();
  // Create 2 folders which are children of the bar.
  [controller_ selectTestNodeInBrowser:bar];
  [controller_ newFolder:nil];
  [controller_ selectTestNodeInBrowser:bar];
  [controller_ newFolder:nil];
  // If we do NOT crash on createNewFolders, success!
  // (e.g. http://crbug.com/47877 is fixed).
  [controller_ createNewFolders];
  [controller_ cancel:nil];
}

TEST_F(BookmarkEditorBaseControllerTest, SelectedFolderDeleted) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  [controller_ selectTestNodeInBrowser:folder_b_3_];
  EXPECT_EQ(folder_b_3_, [controller_ selectedNode]);

  // Delete the selected node, and verify it's no longer selected:
  model->Remove(folder_b_->GetChild(3));
  EXPECT_NE(folder_b_3_, [controller_ selectedNode]);

  [controller_ cancel:nil];
}

TEST_F(BookmarkEditorBaseControllerTest, SelectedFoldersParentDeleted) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  [controller_ selectTestNodeInBrowser:folder_b_3_];
  EXPECT_EQ(folder_b_3_, [controller_ selectedNode]);

  // Delete the selected node's parent, and verify it's no longer selected:
  model->Remove(root->GetChild(1));
  EXPECT_NE(folder_b_3_, [controller_ selectedNode]);

  [controller_ cancel:nil];
}

TEST_F(BookmarkEditorBaseControllerTest, FolderAdded) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();

  // Add a folder node to the model, and verify it can be selected in the tree:
  const BookmarkNode* folder_added = model->AddFolder(
      root, 0, ASCIIToUTF16("added"));
  [controller_ selectTestNodeInBrowser:folder_added];
  EXPECT_EQ(folder_added, [controller_ selectedNode]);

  [controller_ cancel:nil];
}

// Verifies expandeNodes and getExpandedNodes.
TEST_F(BookmarkEditorBaseControllerTest, ExpandedState) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());

  // Sets up the state we're going to expand.
  BookmarkExpandedStateTracker::Nodes nodes;
  nodes.insert(model->bookmark_bar_node());
  nodes.insert(folder_b_);
  nodes.insert(folder_c_);

  // Initial state shouldn't match expected state, otherwise this test isn't
  // really going to test anything.
  BookmarkExpandedStateTracker::Nodes actual = [controller_ getExpandedNodes];
  EXPECT_NE(actual, nodes);

  [controller_ expandNodes:nodes];

  actual = [controller_ getExpandedNodes];

  EXPECT_EQ(nodes, actual);

  [controller_ cancel:nil];
}

// Verifies the dialog's touch bar.
TEST_F(BookmarkEditorBaseControllerTest, TouchBar) {
  if (@available(macOS 10.12.2, *)) {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(features::kDialogTouchBar);

    NSTouchBar* touch_bar = [controller_ makeTouchBar];
    NSArray* touch_bar_items = [touch_bar itemIdentifiers];
    EXPECT_TRUE([touch_bar_items
        containsObject:ui::GetTouchBarItemId(kBookmarkEditDialogTouchBarId,
                                             kNewFolderTouchBarId)]);
    EXPECT_TRUE([touch_bar_items
        containsObject:ui::GetTouchBarItemId(kBookmarkEditDialogTouchBarId,
                                             kCancelTouchBarId)]);
    EXPECT_TRUE([touch_bar_items
        containsObject:ui::GetTouchBarItemId(kBookmarkEditDialogTouchBarId,
                                             kSaveTouchBarId)]);
  }

  [controller_ cancel:nil];
}

class BookmarkFolderInfoTest : public CocoaTest { };

TEST_F(BookmarkFolderInfoTest, Construction) {
  NSMutableArray* children = [NSMutableArray arrayWithObject:@"child"];
  // We just need a pointer, and any pointer will do.
  const BookmarkNode* fakeNode =
      reinterpret_cast<const BookmarkNode*>(&children);
  BookmarkFolderInfo* info =
    [BookmarkFolderInfo bookmarkFolderInfoWithFolderName:@"name"
                                              folderNode:fakeNode
                                                children:children];
  EXPECT_TRUE(info);
  EXPECT_EQ([info folderName], @"name");
  EXPECT_EQ([info children], children);
  EXPECT_EQ([info folderNode], fakeNode);
}
