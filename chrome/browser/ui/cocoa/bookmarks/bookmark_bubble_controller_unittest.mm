// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/bookmarks/bookmark_bubble_observer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bubble_controller.h"
#include "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_touch_bar.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/notification_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "ui/base/cocoa/touch_bar_forward_declarations.h"
#import "ui/base/cocoa/touch_bar_util.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/ui_base_features.h"

using base::ASCIIToUTF16;
using bookmarks::BookmarkBubbleObserver;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using content::WebContents;

namespace {

// URL of the test bookmark.
const char kTestBookmarkURL[] = "http://www.google.com";

// Touch bar identifier.
NSString* const kBookmarkBubbleTouchBarId = @"bookmark-bubble";

// Touch bar item identifiers.
NSString* const kRemoveTouchBarId = @"REMOVE";
NSString* const kEditTouchBarId = @"EDIT";
NSString* const kDoneTouchBarId = @"DONE";

class TestBookmarkBubbleObserver : public BookmarkBubbleObserver {
 public:
  TestBookmarkBubbleObserver() {}
  ~TestBookmarkBubbleObserver() override {}

  // bookmarks::BookmarkBubbleObserver.
  void OnBookmarkBubbleShown(const BookmarkNode* node) override {
    ++shown_count_;
  }
  void OnBookmarkBubbleHidden() override { ++hidden_count_; }

  int shown_count() { return shown_count_; }
  int hidden_count() { return hidden_count_; }

 private:
  int shown_count_ = 0;
  int hidden_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestBookmarkBubbleObserver);
};

class BookmarkBubbleControllerTest : public CocoaProfileTest {
 public:
  static int edits_;
  BookmarkBubbleController* controller_;
  TestBookmarkBubbleObserver test_observer_;

  BookmarkBubbleControllerTest() : controller_(nil) {
    edits_ = 0;
  }

  // CocoaProfileTest:
  void SetUp() override {
    // This file only tests Cocoa UI and can be deleted when kSecondaryUiMd is
    // default.
    scoped_feature_list_.InitAndDisableFeature(features::kSecondaryUiMd);
    CocoaProfileTest::SetUp();
  }

  void TearDown() override {
    [controller_ close];
    CocoaProfileTest::TearDown();
  }

  // Returns a controller but ownership not transferred.
  // Only one of these will be valid at a time.
  BookmarkBubbleController* ControllerForNode(const BookmarkNode* node) {
    if (controller_ && !IsWindowClosing()) {
      [controller_ close];
      controller_ = nil;
    }
    BookmarkModel* model =
        BookmarkModelFactory::GetForBrowserContext(profile());
    bookmarks::ManagedBookmarkService* managed =
        ManagedBookmarkServiceFactory::GetForProfile(profile());
    controller_ = [[BookmarkBubbleController alloc]
        initWithParentWindow:browser()->window()->GetNativeWindow()
              bubbleObserver:&test_observer_
                     managed:managed
                       model:model
                        node:node
           alreadyBookmarked:YES];
    EXPECT_TRUE([controller_ window]);
    // The window must be gone or we'll fail a unit test with windows left open.
    [static_cast<InfoBubbleWindow*>([controller_ window])
        setAllowedAnimations:info_bubble::kAnimateNone];
    [controller_ showWindow:nil];
    return controller_;
  }

  BookmarkModel* GetBookmarkModel() {
    return BookmarkModelFactory::GetForBrowserContext(profile());
  }

  const BookmarkNode* CreateTestBookmark() {
    BookmarkModel* model = GetBookmarkModel();
    return model->AddURL(model->bookmark_bar_node(),
                         0,
                         ASCIIToUTF16("Bookie markie title"),
                         GURL(kTestBookmarkURL));
  }

  bool IsWindowClosing() {
    return [static_cast<InfoBubbleWindow*>([controller_ window]) isClosing];
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkBubbleControllerTest);
};

// static
int BookmarkBubbleControllerTest::edits_;

// Confirm basics about the bubble window (e.g. that its frame fits inside, or
// almost completely fits inside, its parent window's frame).
TEST_F(BookmarkBubbleControllerTest, TestBubbleWindow) {
  const BookmarkNode* node = CreateTestBookmark();
  BookmarkBubbleController* controller = ControllerForNode(node);
  EXPECT_TRUE(controller);
  NSWindow* window = [controller window];
  EXPECT_TRUE(window);
  NSRect browser_window_frame = [browser()->window()->GetNativeWindow() frame];
  // In this test case the bookmarks bubble's window frame extendeds slightly
  // beyond its parent window's frame.
  browser_window_frame.size.width += 1;
  EXPECT_TRUE(NSContainsRect(browser_window_frame, [window frame]));
}

// Test that we can handle closing the parent window
TEST_F(BookmarkBubbleControllerTest, TestClosingParentWindow) {
  const BookmarkNode* node = CreateTestBookmark();
  BookmarkBubbleController* controller = ControllerForNode(node);
  EXPECT_TRUE(controller);
  NSWindow* window = [controller window];
  EXPECT_TRUE(window);
  base::mac::ScopedNSAutoreleasePool pool;
  [browser()->window()->GetNativeWindow() performClose:NSApp];
}


// Confirm population of folder list
TEST_F(BookmarkBubbleControllerTest, TestFillInFolder) {
  // Create some folders, including a nested folder
  BookmarkModel* model = GetBookmarkModel();
  EXPECT_TRUE(model);
  const BookmarkNode* bookmarkBarNode = model->bookmark_bar_node();
  EXPECT_TRUE(bookmarkBarNode);
  const BookmarkNode* node1 = model->AddFolder(bookmarkBarNode, 0,
                                               ASCIIToUTF16("one"));
  EXPECT_TRUE(node1);
  const BookmarkNode* node2 = model->AddFolder(bookmarkBarNode, 1,
                                               ASCIIToUTF16("two"));
  EXPECT_TRUE(node2);
  const BookmarkNode* node3 = model->AddFolder(bookmarkBarNode, 2,
                                               ASCIIToUTF16("three"));
  EXPECT_TRUE(node3);
  const BookmarkNode* node4 = model->AddFolder(node2, 0, ASCIIToUTF16("sub"));
  EXPECT_TRUE(node4);
  const BookmarkNode* node5 = model->AddURL(node1, 0, ASCIIToUTF16("title1"),
                                            GURL(kTestBookmarkURL));
  EXPECT_TRUE(node5);
  const BookmarkNode* node6 = model->AddURL(node3, 0, ASCIIToUTF16("title2"),
                                            GURL(kTestBookmarkURL));
  EXPECT_TRUE(node6);
  const BookmarkNode* node7 = model->AddURL(
      node4, 0, ASCIIToUTF16("title3"), GURL("http://www.google.com/reader"));
  EXPECT_TRUE(node7);

  BookmarkBubbleController* controller = ControllerForNode(node4);
  EXPECT_TRUE(controller);

  NSArray* titles =
      [[[controller folderPopUpButton] itemArray] valueForKey:@"title"];
  EXPECT_TRUE([titles containsObject:@"one"]);
  EXPECT_TRUE([titles containsObject:@"two"]);
  EXPECT_TRUE([titles containsObject:@"three"]);
  EXPECT_TRUE([titles containsObject:@"sub"]);
  EXPECT_FALSE([titles containsObject:@"title1"]);
  EXPECT_FALSE([titles containsObject:@"title2"]);


  // Verify that the top level folders are displayed correctly.
  EXPECT_TRUE([titles containsObject:@"Other Bookmarks"]);
  EXPECT_TRUE([titles containsObject:@"Bookmarks Bar"]);
  if (model->mobile_node()->IsVisible()) {
    EXPECT_TRUE([titles containsObject:@"Mobile Bookmarks"]);
  } else {
    EXPECT_FALSE([titles containsObject:@"Mobile Bookmarks"]);
  }
}

// Confirm ability to handle folders with blank name.
TEST_F(BookmarkBubbleControllerTest, TestFolderWithBlankName) {
  // Create some folders, including a nested folder
  BookmarkModel* model = GetBookmarkModel();
  EXPECT_TRUE(model);
  const BookmarkNode* bookmarkBarNode = model->bookmark_bar_node();
  EXPECT_TRUE(bookmarkBarNode);
  const BookmarkNode* node1 = model->AddFolder(bookmarkBarNode, 0,
                                               ASCIIToUTF16("one"));
  EXPECT_TRUE(node1);
  const BookmarkNode* node2 = model->AddFolder(bookmarkBarNode, 1,
                                               base::string16());
  EXPECT_TRUE(node2);
  const BookmarkNode* node3 = model->AddFolder(bookmarkBarNode, 2,
                                               ASCIIToUTF16("three"));
  EXPECT_TRUE(node3);
  const BookmarkNode* node2_1 = model->AddURL(node2, 0, ASCIIToUTF16("title1"),
                                              GURL(kTestBookmarkURL));
  EXPECT_TRUE(node2_1);

  BookmarkBubbleController* controller = ControllerForNode(node1);
  EXPECT_TRUE(controller);

  // One of the items should be blank and its node should be node2.
  NSArray* items = [[controller folderPopUpButton] itemArray];
  EXPECT_GT([items count], 4U);
  BOOL blankFolderFound = NO;
  for (NSMenuItem* item in [[controller folderPopUpButton] itemArray]) {
    if ([[item title] length] == 0 &&
        static_cast<const BookmarkNode*>([[item representedObject]
                                          pointerValue]) == node2) {
      blankFolderFound = YES;
      break;
    }
  }
  EXPECT_TRUE(blankFolderFound);
}

// Click on edit; bubble gets closed.
TEST_F(BookmarkBubbleControllerTest, TestEdit) {
  const BookmarkNode* node = CreateTestBookmark();
  BookmarkBubbleController* controller = ControllerForNode(node);
  EXPECT_TRUE(controller);

  EXPECT_EQ(edits_, 0);
  EXPECT_FALSE(IsWindowClosing());
  [controller edit:controller];
  EXPECT_EQ(edits_, 1);
  EXPECT_TRUE(IsWindowClosing());
}

// CallClose; bubble gets closed.
// Also confirm bubble notifications get sent.
TEST_F(BookmarkBubbleControllerTest, TestClose) {
  const BookmarkNode* node = CreateTestBookmark();
  EXPECT_EQ(edits_, 0);

  BookmarkBubbleController* controller = ControllerForNode(node);
  EXPECT_TRUE(controller.bookmarkBubbleObserver);
  EXPECT_EQ(1, test_observer_.shown_count());
  EXPECT_EQ(0, test_observer_.hidden_count());
  EXPECT_TRUE(controller);
  EXPECT_FALSE(IsWindowClosing());
  [controller ok:controller];
  EXPECT_EQ(edits_, 0);
  EXPECT_TRUE(IsWindowClosing());
  EXPECT_FALSE(controller.bookmarkBubbleObserver);
  EXPECT_EQ(1, test_observer_.hidden_count());
}

// User changes title and parent folder in the UI
TEST_F(BookmarkBubbleControllerTest, TestUserEdit) {
  BookmarkModel* model = GetBookmarkModel();
  EXPECT_TRUE(model);
  const BookmarkNode* bookmarkBarNode = model->bookmark_bar_node();
  EXPECT_TRUE(bookmarkBarNode);
  const BookmarkNode* node = model->AddURL(bookmarkBarNode,
                                           0,
                                           ASCIIToUTF16("short-title"),
                                           GURL(kTestBookmarkURL));
  const BookmarkNode* grandma = model->AddFolder(bookmarkBarNode, 0,
                                                 ASCIIToUTF16("grandma"));
  EXPECT_TRUE(grandma);
  const BookmarkNode* grandpa = model->AddFolder(bookmarkBarNode, 0,
                                                 ASCIIToUTF16("grandpa"));
  EXPECT_TRUE(grandpa);

  BookmarkBubbleController* controller = ControllerForNode(node);
  EXPECT_TRUE(controller);

  // simulate a user edit
  [controller setTitle:@"oops" parentFolder:grandma];
  [controller edit:controller];

  // Make sure bookmark has changed
  EXPECT_EQ(node->GetTitle(), ASCIIToUTF16("oops"));
  EXPECT_EQ(node->parent()->GetTitle(), ASCIIToUTF16("grandma"));
}

// Confirm happiness with parent nodes that have the same name.
TEST_F(BookmarkBubbleControllerTest, TestNewParentSameName) {
  BookmarkModel* model = GetBookmarkModel();
  EXPECT_TRUE(model);
  const BookmarkNode* bookmarkBarNode = model->bookmark_bar_node();
  EXPECT_TRUE(bookmarkBarNode);
  for (int i=0; i<2; i++) {
    const BookmarkNode* node = model->AddURL(bookmarkBarNode,
                                             0,
                                             ASCIIToUTF16("short-title"),
                                             GURL(kTestBookmarkURL));
    EXPECT_TRUE(node);
    const BookmarkNode* folder = model->AddFolder(bookmarkBarNode, 0,
                                                 ASCIIToUTF16("NAME"));
    EXPECT_TRUE(folder);
    folder = model->AddFolder(bookmarkBarNode, 0, ASCIIToUTF16("NAME"));
    EXPECT_TRUE(folder);
    folder = model->AddFolder(bookmarkBarNode, 0, ASCIIToUTF16("NAME"));
    EXPECT_TRUE(folder);
    BookmarkBubbleController* controller = ControllerForNode(node);
    EXPECT_TRUE(controller);

    // simulate a user edit
    [controller setParentFolderSelection:bookmarkBarNode->GetChild(i)];
    [controller edit:controller];

    // Make sure bookmark has changed, and that the parent is what we
    // expect.  This proves nobody did searching based on name.
    EXPECT_EQ(node->parent(), bookmarkBarNode->GetChild(i));
  }
}

// Confirm happiness with nodes with the same Name
TEST_F(BookmarkBubbleControllerTest, TestDuplicateNodeNames) {
  BookmarkModel* model = GetBookmarkModel();
  const BookmarkNode* bookmarkBarNode = model->bookmark_bar_node();
  EXPECT_TRUE(bookmarkBarNode);
  const BookmarkNode* node1 = model->AddFolder(bookmarkBarNode, 0,
                                               ASCIIToUTF16("NAME"));
  EXPECT_TRUE(node1);
  const BookmarkNode* node2 = model->AddFolder(bookmarkBarNode, 0,
                                               ASCIIToUTF16("NAME"));
  EXPECT_TRUE(node2);
  BookmarkBubbleController* controller = ControllerForNode(bookmarkBarNode);
  EXPECT_TRUE(controller);

  NSPopUpButton* button = [controller folderPopUpButton];
  [controller setParentFolderSelection:node1];
  NSMenuItem* item = [button selectedItem];
  id itemObject = [item representedObject];
  EXPECT_NSEQ([NSValue valueWithPointer:node1], itemObject);
  [controller setParentFolderSelection:node2];
  item = [button selectedItem];
  itemObject = [item representedObject];
  EXPECT_NSEQ([NSValue valueWithPointer:node2], itemObject);
}

// Click the "remove" button
TEST_F(BookmarkBubbleControllerTest, TestRemove) {
  const BookmarkNode* node = CreateTestBookmark();
  BookmarkBubbleController* controller = ControllerForNode(node);
  EXPECT_TRUE(controller);

  BookmarkModel* model = GetBookmarkModel();
  EXPECT_TRUE(model->IsBookmarked(GURL(kTestBookmarkURL)));

  [controller remove:controller];
  EXPECT_FALSE(model->IsBookmarked(GURL(kTestBookmarkURL)));
  EXPECT_TRUE(IsWindowClosing());
}

// Confirm picking "choose another folder" caused edit: to be called.
TEST_F(BookmarkBubbleControllerTest, PopUpSelectionChanged) {
  BookmarkModel* model = GetBookmarkModel();
  const BookmarkNode* node = model->AddURL(model->bookmark_bar_node(),
                                           0, ASCIIToUTF16("super-title"),
                                           GURL(kTestBookmarkURL));
  BookmarkBubbleController* controller = ControllerForNode(node);
  EXPECT_TRUE(controller);

  NSPopUpButton* button = [controller folderPopUpButton];
  [button selectItemWithTitle:[[controller class] chooseAnotherFolderString]];
  EXPECT_EQ(edits_, 0);
  [button sendAction:[button action] to:[button target]];
  EXPECT_EQ(edits_, 1);
}

// Create a controller that simulates the bookmark just now being created by
// the user clicking the star, then sending the "cancel" command to represent
// them pressing escape. The bookmark should not be there.
TEST_F(BookmarkBubbleControllerTest, EscapeRemovesNewBookmark) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  bookmarks::ManagedBookmarkService* managed =
      ManagedBookmarkServiceFactory::GetForProfile(profile());
  const BookmarkNode* node = CreateTestBookmark();
  BookmarkBubbleController* controller = [[BookmarkBubbleController alloc]
      initWithParentWindow:browser()->window()->GetNativeWindow()
            bubbleObserver:&test_observer_
                   managed:managed
                     model:model
                      node:node
         alreadyBookmarked:NO];  // The last param is the key difference.
  EXPECT_TRUE([controller window]);
  // Calls release on controller.
  [controller cancel:nil];
  EXPECT_FALSE(model->IsBookmarked(GURL(kTestBookmarkURL)));
}

// Create a controller where the bookmark already existed prior to clicking
// the star and test that sending a cancel command doesn't change the state
// of the bookmark.
TEST_F(BookmarkBubbleControllerTest, EscapeDoesntTouchExistingBookmark) {
  const BookmarkNode* node = CreateTestBookmark();
  BookmarkBubbleController* controller = ControllerForNode(node);
  EXPECT_TRUE(controller);

  [(id)controller cancel:nil];
  EXPECT_TRUE(GetBookmarkModel()->IsBookmarked(GURL(kTestBookmarkURL)));
}

// Confirm indentation of items in pop-up menu
TEST_F(BookmarkBubbleControllerTest, TestMenuIndentation) {
  // Create some folders, including a nested folder
  BookmarkModel* model = GetBookmarkModel();
  EXPECT_TRUE(model);
  const BookmarkNode* bookmarkBarNode = model->bookmark_bar_node();
  EXPECT_TRUE(bookmarkBarNode);
  const BookmarkNode* node1 = model->AddFolder(bookmarkBarNode, 0,
                                               ASCIIToUTF16("one"));
  EXPECT_TRUE(node1);
  const BookmarkNode* node2 = model->AddFolder(bookmarkBarNode, 1,
                                               ASCIIToUTF16("two"));
  EXPECT_TRUE(node2);
  const BookmarkNode* node2_1 = model->AddFolder(node2, 0,
                                                 ASCIIToUTF16("two dot one"));
  EXPECT_TRUE(node2_1);
  const BookmarkNode* node3 = model->AddFolder(bookmarkBarNode, 2,
                                               ASCIIToUTF16("three"));
  EXPECT_TRUE(node3);

  BookmarkBubbleController* controller = ControllerForNode(node1);
  EXPECT_TRUE(controller);

  // Compare the menu item indents against expectations.
  static const int kExpectedIndent[] = {0, 1, 1, 2, 1, 0};
  NSArray* items = [[controller folderPopUpButton] itemArray];
  ASSERT_GE([items count], 6U);
  for(int itemNo = 0; itemNo < 6; itemNo++) {
    NSMenuItem* item = [items objectAtIndex:itemNo];
    EXPECT_EQ(kExpectedIndent[itemNo], [item indentationLevel])
        << "Unexpected indent for menu item #" << itemNo;
  }
}

// Confirm that the sync promo is displayed when the user is not signed in.
TEST_F(BookmarkBubbleControllerTest, SyncPromoNotSignedIn) {
  const BookmarkNode* node = CreateTestBookmark();
  BookmarkBubbleController* controller = ControllerForNode(node);

  EXPECT_EQ(1u, [[controller.syncPromoPlaceholder subviews] count]);
}

// Confirm that the sync promo is not displayed when the user is signed in.
TEST_F(BookmarkBubbleControllerTest, SyncPromoSignedIn) {
  SigninManager* signin = SigninManagerFactory::GetForProfile(profile());
  signin->SetAuthenticatedAccountInfo("fake_username", "fake_username");

  const BookmarkNode* node = CreateTestBookmark();
  BookmarkBubbleController* controller = ControllerForNode(node);

  EXPECT_EQ(0u, [[controller.syncPromoPlaceholder subviews] count]);
}

// Tests to see if setting the title textfield multiple times will crash.
// See crbug.com/731284.
TEST_F(BookmarkBubbleControllerTest, TextfieldChanges) {
  BookmarkModel* model = GetBookmarkModel();
  EXPECT_TRUE(model);
  const BookmarkNode* bookmark_bar_node = model->bookmark_bar_node();
  EXPECT_TRUE(bookmark_bar_node);
  const BookmarkNode* node =
      model->AddURL(bookmark_bar_node, 0, ASCIIToUTF16("short-title"),
                    GURL(kTestBookmarkURL));

  BookmarkBubbleController* controller = ControllerForNode(node);
  EXPECT_TRUE(controller);

  const BookmarkNode* parent = node->parent();
  EXPECT_TRUE(parent);

  [controller setTitle:@"test" parentFolder:parent];
  [controller setTitle:@"" parentFolder:parent];
  [controller setTitle:@"             " parentFolder:parent];
  [controller setTitle:@"       test 2      " parentFolder:parent];
}

// Verifies the bubble's touch bar.
TEST_F(BookmarkBubbleControllerTest, TouchBar) {
  if (@available(macOS 10.12.2, *)) {
    base::test::ScopedFeatureList feature_list;
    feature_list.InitAndEnableFeature(features::kDialogTouchBar);

    const BookmarkNode* node = CreateTestBookmark();
    NSTouchBar* touch_bar = [ControllerForNode(node) makeTouchBar];
    NSArray* touch_bar_items = [touch_bar itemIdentifiers];
    EXPECT_TRUE([touch_bar_items
        containsObject:ui::GetTouchBarItemId(kBookmarkBubbleTouchBarId,
                                             kRemoveTouchBarId)]);
    EXPECT_TRUE([touch_bar_items
        containsObject:ui::GetTouchBarItemId(kBookmarkBubbleTouchBarId,
                                             kEditTouchBarId)]);
    EXPECT_TRUE([touch_bar_items
        containsObject:ui::GetTouchBarItemId(kBookmarkBubbleTouchBarId,
                                             kDoneTouchBarId)]);
  }
}

}  // namespace

@implementation NSApplication (BookmarkBubbleUnitTest)
// Add handler for the editBookmarkNode: action to NSApp for testing purposes.
// Normally this would be sent up the responder tree correctly, but since
// tests run in the background, key window and main window are never set on
// NSApplication. Adding it to NSApplication directly removes the need for
// worrying about what the current window with focus is.
- (void)editBookmarkNode:(id)sender {
  EXPECT_TRUE([sender respondsToSelector:@selector(node)]);
  BookmarkBubbleControllerTest::edits_++;
}

@end
