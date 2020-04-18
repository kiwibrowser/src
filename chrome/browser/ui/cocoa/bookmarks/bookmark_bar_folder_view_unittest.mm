// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/scoped_nsobject.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_view.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_folder_target.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "chrome/browser/ui/cocoa/url_drop_target.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "third_party/mozilla/NSPasteboard+Utils.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/ocmock_extensions.h"
#include "ui/base/clipboard/clipboard_util_mac.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

// Allows us to verify BookmarkBarFolderView.
@interface BookmarkBarFolderView(TestingAPI)

@property(readonly, nonatomic) BOOL dropIndicatorShown;
@property(readonly, nonatomic) CGFloat dropIndicatorPosition;

-(void)setController:(id<BookmarkButtonControllerProtocol>)controller;

@end

@implementation BookmarkBarFolderView(TestingAPI)

-(void)setController:(id<BookmarkButtonControllerProtocol>)controller {
  controller_ = controller;
}

-(BOOL)dropIndicatorShown {
  return dropIndicatorShown_;
}

-(CGFloat)dropIndicatorPosition {
  return NSMinY([dropIndicator_ frame]);
}

@end

namespace {

// Some values used for mocks and fakes.
const CGFloat kFakeIndicatorPos = 7.0;
const NSPoint kPoint = {10, 10};

class BookmarkBarFolderViewTest : public CocoaProfileTest {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();

    view_.reset([[BookmarkBarFolderView alloc] init]);

    mock_controller_.reset(GetMockController(
        YES, BookmarkModelFactory::GetForBrowserContext(profile())));

    mock_button_.reset(GetMockButton(mock_controller_.get()));
    [view_ awakeFromNib];
    [view_ setController:mock_controller_];
  }

  void TearDown() override {
    [mock_controller_ verify];
    CocoaProfileTest::TearDown();
  }

  id GetFakePasteboardForType(NSString* dataType) {
    id pasteboard = [OCMockObject mockForClass:[NSPasteboard class]];
    if ([dataType isEqualToString:ui::ClipboardUtil::UTIForPasteboardType(
                                      kBookmarkButtonDragType)]) {
      BookmarkButton* button = mock_button_.get();
      [[[pasteboard stub]
        andReturn:[NSData dataWithBytes:&button length:sizeof(button)]]
       dataForType:dataType];
    } else {
      [[[pasteboard stub] andReturn:[NSData data]] dataForType:dataType];
    }
    [[[pasteboard stub] andReturn:nil] dataForType:OCMOCK_ANY];
    [[[pasteboard stub] andReturnBool:YES]
        containsURLDataConvertingTextToURL:YES];
    [[pasteboard stub] getURLs:[OCMArg setTo:nil]
                     andTitles:[OCMArg setTo:nil]
           convertingFilenames:YES
           convertingTextToURL:YES];
    return pasteboard;
  }

  id GetFakeDragInfoForType(NSString* dataType) {
    // Need something non-nil to return as the draggingSource.
    id source = [OCMockObject mockForClass:[NSObject class]];
    id drag_info = [OCMockObject mockForProtocol:@protocol(NSDraggingInfo)];
    id pasteboard = GetFakePasteboardForType(dataType);
    [[[drag_info stub] andReturn:pasteboard] draggingPasteboard];
    [[[drag_info stub] andReturnNSPoint:kPoint] draggingLocation];
    [[[drag_info stub] andReturn:source] draggingSource];
    [[[drag_info stub]
      andReturnUnsignedInteger:NSDragOperationCopy | NSDragOperationMove]
     draggingSourceOperationMask];
    return drag_info;
  }

  id GetMockController(BOOL show_indicator, BookmarkModel* model) {
    id mock_controller =
        [OCMockObject mockForClass:[BookmarkBarFolderController class]];
    [[[mock_controller stub] andReturnBool:YES]
     draggingAllowed:OCMOCK_ANY];
    [[[mock_controller stub] andReturnBool:show_indicator]
     shouldShowIndicatorShownForPoint:kPoint];
    [[[mock_controller stub] andReturnCGFloat:kFakeIndicatorPos]
     indicatorPosForDragToPoint:kPoint];
    [[[mock_controller stub] andReturnValue:OCMOCK_VALUE(model)] bookmarkModel];
    return [mock_controller retain];
  }

  id GetMockButton(id mock_controller) {
    BookmarkModel* bookmark_model =
        BookmarkModelFactory::GetForBrowserContext(profile());
    const BookmarkNode* node =
        bookmark_model->AddURL(bookmark_model->bookmark_bar_node(),
                               0,
                               base::ASCIIToUTF16("Test Bookmark"),
                               GURL("http://www.exmaple.com"));

    id mock_button = [OCMockObject mockForClass:[BookmarkButton class]];
    [[[mock_button stub] andReturnValue:OCMOCK_VALUE(node)] bookmarkNode];
    return [mock_button retain];
  }

  base::scoped_nsobject<id> mock_controller_;
  base::scoped_nsobject<BookmarkBarFolderView> view_;
  base::scoped_nsobject<id> mock_button_;
};

// Disabled due to crash on Mac10.02 Tests bot (crbug.com/757063)
TEST_F(BookmarkBarFolderViewTest, DISABLED_BookmarkButtonDragAndDrop) {
  id drag_info = GetFakeDragInfoForType(
      ui::ClipboardUtil::UTIForPasteboardType(kBookmarkButtonDragType));
  [[[mock_controller_ expect] andReturnUnsignedInteger:NSDragOperationNone]
   draggingEntered:drag_info];
  [[[mock_controller_ expect] andReturnBool:NO] dragBookmarkData:drag_info];
  [[[mock_controller_ expect] andReturnBool:YES] dragButton:OCMOCK_ANY
                                                         to:kPoint
                                                       copy:NO];

  EXPECT_EQ([view_ draggingEntered:drag_info], NSDragOperationMove);
  EXPECT_TRUE([view_ performDragOperation:drag_info]);
}

// When dragging bookmarks across profiles, we should always copy, never move.
// Disabled due to crash on Mac10.02 Tests bot (crbug.com/757063)
TEST_F(BookmarkBarFolderViewTest,
       DISABLED_BookmarkButtonDragAndDropAcrossProfiles) {
  // |other_profile| is owned by the |testing_profile_manager|.
  TestingProfile* other_profile =
      testing_profile_manager()->CreateTestingProfile("other");
  other_profile->CreateBookmarkModel(true);
  bookmarks::test::WaitForBookmarkModelToLoad(
      BookmarkModelFactory::GetForBrowserContext(other_profile));

  mock_controller_.reset(GetMockController(
      YES, BookmarkModelFactory::GetForBrowserContext(other_profile)));
  [view_ setController:mock_controller_];

  id drag_info = GetFakeDragInfoForType(
      ui::ClipboardUtil::UTIForPasteboardType(kBookmarkButtonDragType));
  [[[mock_controller_ expect] andReturnUnsignedInteger:NSDragOperationNone]
   draggingEntered:drag_info];
  [[[mock_controller_ expect] andReturnBool:NO] dragBookmarkData:drag_info];
  [[[mock_controller_ expect] andReturnBool:YES] dragButton:OCMOCK_ANY
                                                         to:kPoint
                                                       copy:YES];

  EXPECT_EQ([view_ draggingEntered:drag_info], NSDragOperationMove);
  EXPECT_TRUE([view_ performDragOperation:drag_info]);
}

TEST_F(BookmarkBarFolderViewTest, URLDragAndDrop) {
  NSArray* dragTypes = [URLDropTargetHandler handledDragTypes];
  for (NSString* type in dragTypes) {
    id drag_info = GetFakeDragInfoForType(type);
    [[[mock_controller_ expect] andReturnUnsignedInteger:NSDragOperationNone]
     draggingEntered:drag_info];
    [[[mock_controller_ expect] andReturnBool:NO] dragBookmarkData:drag_info];
    [[[mock_controller_ expect] andReturnBool:YES] addURLs:OCMOCK_ANY
                                                withTitles:OCMOCK_ANY
                                                        at:kPoint];
    EXPECT_EQ([view_ draggingEntered:drag_info], NSDragOperationMove);
    EXPECT_TRUE([view_ performDragOperation:drag_info]);
    [mock_controller_ verify];
  }
}

TEST_F(BookmarkBarFolderViewTest, BookmarkButtonDropIndicator) {
  id drag_info = GetFakeDragInfoForType(
      ui::ClipboardUtil::UTIForPasteboardType(kBookmarkButtonDragType));
  [[[mock_controller_ expect] andReturnUnsignedInteger:NSDragOperationNone]
   draggingEntered:drag_info];
  EXPECT_EQ([view_ draggingEntered:drag_info], NSDragOperationMove);
  [mock_controller_ verify];
  EXPECT_TRUE([view_ dropIndicatorShown]);
  EXPECT_EQ([view_ dropIndicatorPosition], kFakeIndicatorPos);
  mock_controller_.reset(GetMockController(
      NO, BookmarkModelFactory::GetForBrowserContext(profile())));
  [view_ setController:mock_controller_];
  [[[mock_controller_ expect] andReturnUnsignedInteger:NSDragOperationNone]
   draggingEntered:drag_info];
  EXPECT_EQ([view_ draggingEntered:drag_info], NSDragOperationMove);
  EXPECT_FALSE([view_ dropIndicatorShown]);
}

}  // namespace
