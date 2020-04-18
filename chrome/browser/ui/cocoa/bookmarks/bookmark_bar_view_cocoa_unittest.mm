// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/scoped_nsobject.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_view_cocoa.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button_cell.h"
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
#include "ui/base/clipboard/clipboard_util_mac.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace {

// Some values used for mocks and fakes.
const CGFloat kFakeIndicatorPos = 7.0;
const NSPoint kPoint = {10, 10};

}  // namespace

// Fake DraggingInfo, fake BookmarkBarController, fake NSPasteboard...
@interface FakeBookmarkDraggingInfo : BookmarkBarController {
 @public
  BOOL dragButtonToPong_;
  BOOL dragButtonToShouldCopy_;
  BOOL dragURLsPong_;
  BOOL dragBookmarkDataPong_;
  BOOL dropIndicatorShown_;
  BOOL draggingEnteredCalled_;
  // Only mock one type of drag data at a time.
  NSString* dragDataType_;
  BookmarkButton* button_;  // weak
  BookmarkModel* stubbedBookmarkModel_;  // weak
  id draggingSource_;
}
@property (nonatomic) BOOL dropIndicatorShown;
@property (nonatomic) BOOL draggingEnteredCalled;
@property (nonatomic, copy) NSString* dragDataType;
@property (nonatomic, assign) BookmarkButton* button;

@end

@implementation FakeBookmarkDraggingInfo

@synthesize dropIndicatorShown = dropIndicatorShown_;
@synthesize draggingEnteredCalled = draggingEnteredCalled_;
@synthesize dragDataType = dragDataType_;
@synthesize button = button_;

- (id)init {
  if ((self = [super init])) {
    dropIndicatorShown_ = YES;
  }
  return self;
}

- (void)dealloc {
  [dragDataType_ release];
  [super dealloc];
}

- (void)reset {
  [dragDataType_ release];
  dragDataType_ = nil;
  dragButtonToPong_ = NO;
  dragURLsPong_ = NO;
  dragBookmarkDataPong_ = NO;
  dropIndicatorShown_ = YES;
  draggingEnteredCalled_ = NO;
  draggingSource_ = self;
}

- (void)setDraggingSource:(id)draggingSource {
  draggingSource_ = draggingSource;
}

// NSDragInfo mocking functions.

- (id)draggingPasteboard {
  return self;
}

// So we can look local.
- (id)draggingSource {
  return draggingSource_;
}

- (NSDragOperation)draggingSourceOperationMask {
  return NSDragOperationCopy | NSDragOperationMove;
}

- (NSPoint)draggingLocation {
  return kPoint;
}

// NSPasteboard mocking functions.

- (BOOL)containsURLDataConvertingTextToURL:(BOOL)convertTextToURL {
  NSArray* urlTypes = [URLDropTargetHandler handledDragTypes];
  if (dragDataType_)
    return [urlTypes containsObject:dragDataType_];
  return NO;
}

- (NSData*)dataForType:(NSString*)type {
  if (dragDataType_ && [dragDataType_ isEqualToString:type]) {
    if (button_)
      return [NSData dataWithBytes:&button_ length:sizeof(button_)];
    else
      return [NSData data];  // Return something, anything.
  }
  return nil;
}

// Fake a controller for callback ponging

- (void)viewDidLoadImpl {
  // no-op
}

- (void)setBookmarkModel:(BookmarkModel*)model {
  stubbedBookmarkModel_ = model;
}

- (BookmarkModel*)bookmarkModel {
  return stubbedBookmarkModel_;
}

- (BOOL)dragButton:(BookmarkButton*)button to:(NSPoint)point copy:(BOOL)copy {
  dragButtonToPong_ = YES;
  dragButtonToShouldCopy_ = copy;
  return YES;
}

- (BOOL)addURLs:(NSArray*)urls withTitles:(NSArray*)titles at:(NSPoint)point {
  dragURLsPong_ = YES;
  return YES;
}

- (void)getURLs:(NSArray**)outUrls
              andTitles:(NSArray**)outTitles
    convertingFilenames:(BOOL)convertFilenames
    convertingTextToURL:(BOOL)convertTextToURL {
}

- (BOOL)dragBookmarkData:(id<NSDraggingInfo>)info {
  dragBookmarkDataPong_ = YES;
  return NO;
}

- (BOOL)canEditBookmarks {
  return YES;
}

// Confirm the pongs.

- (BOOL)dragButtonToPong {
  return dragButtonToPong_;
}

- (BOOL)dragButtonToShouldCopy {
  return dragButtonToShouldCopy_;
}

- (BOOL)dragURLsPong {
  return dragURLsPong_;
}

- (BOOL)dragBookmarkDataPong {
  return dragBookmarkDataPong_;
}

- (CGFloat)indicatorPosForDragToPoint:(NSPoint)point {
  return kFakeIndicatorPos;
}

- (BOOL)shouldShowIndicatorShownForPoint:(NSPoint)point {
  return dropIndicatorShown_;
}

- (BOOL)draggingAllowed:(id<NSDraggingInfo>)info {
  return YES;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)info {
  draggingEnteredCalled_ = YES;
  return NSDragOperationNone;
}

- (void)setDropInsertionPos:(CGFloat)where {
}

- (void)clearDropInsertionPos {
}

@end

namespace {

class BookmarkBarViewTestCocoa : public CocoaProfileTest {
 public:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    view_.reset([[BookmarkBarView alloc] init]);
  }

  base::scoped_nsobject<BookmarkBarView> view_;
};

TEST_F(BookmarkBarViewTestCocoa, CanDragWindow) {
  EXPECT_FALSE([view_ mouseDownCanMoveWindow]);
}

TEST_F(BookmarkBarViewTestCocoa, BookmarkButtonDragAndDrop) {
  base::scoped_nsobject<FakeBookmarkDraggingInfo> info(
      [[FakeBookmarkDraggingInfo alloc] init]);
  [view_ setController:info.get()];
  [info reset];

  BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* node =
      bookmark_model->AddURL(bookmark_model->bookmark_bar_node(),
                             0,
                             base::ASCIIToUTF16("Test Bookmark"),
                             GURL("http://www.exmaple.com"));

  base::scoped_nsobject<BookmarkButtonCell> button_cell(
      [[BookmarkButtonCell buttonCellForNode:node
                                        text:nil
                                       image:nil
                              menuController:nil] retain]);
  base::scoped_nsobject<BookmarkButton> dragged_button(
      [[BookmarkButton alloc] init]);
  [dragged_button setCell:button_cell];
  [info setDraggingSource:dragged_button.get()];
  [info setDragDataType:ui::ClipboardUtil::UTIForPasteboardType(
                            kBookmarkButtonDragType)];
  [info setButton:dragged_button.get()];
  [info setBookmarkModel:bookmark_model];
  EXPECT_EQ([view_ draggingEntered:(id)info.get()], NSDragOperationMove);
  EXPECT_TRUE([view_ performDragOperation:(id)info.get()]);
  EXPECT_TRUE([info dragButtonToPong]);
  EXPECT_FALSE([info dragButtonToShouldCopy]);
  EXPECT_FALSE([info dragURLsPong]);
  EXPECT_TRUE([info dragBookmarkDataPong]);
}

// When dragging bookmarks across profiles, we should always copy, never move.
TEST_F(BookmarkBarViewTestCocoa, BookmarkButtonDragAndDropAcrossProfiles) {
  base::scoped_nsobject<FakeBookmarkDraggingInfo> info(
      [[FakeBookmarkDraggingInfo alloc] init]);
  [view_ setController:info.get()];
  [info reset];

  // |other_profile| is owned by the |testing_profile_manager|.
  TestingProfile* other_profile =
      testing_profile_manager()->CreateTestingProfile("other");
  other_profile->CreateBookmarkModel(true);

  BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(profile());
  bookmarks::test::WaitForBookmarkModelToLoad(bookmark_model);

  const BookmarkNode* node =
      bookmark_model->AddURL(bookmark_model->bookmark_bar_node(),
                             0,
                             base::ASCIIToUTF16("Test Bookmark"),
                             GURL("http://www.exmaple.com"));

  base::scoped_nsobject<BookmarkButtonCell> button_cell(
      [[BookmarkButtonCell buttonCellForNode:node
                                        text:nil
                                       image:nil
                              menuController:nil] retain]);
  base::scoped_nsobject<BookmarkButton> dragged_button(
      [[BookmarkButton alloc] init]);
  [dragged_button setCell:button_cell];
  [info setDraggingSource:dragged_button.get()];
  [info setDragDataType:ui::ClipboardUtil::UTIForPasteboardType(
                            kBookmarkButtonDragType)];
  [info setButton:dragged_button.get()];
  [info setBookmarkModel:BookmarkModelFactory::GetForBrowserContext(
                             other_profile)];
  EXPECT_EQ([view_ draggingEntered:(id)info.get()], NSDragOperationMove);
  EXPECT_TRUE([view_ performDragOperation:(id)info.get()]);
  EXPECT_TRUE([info dragButtonToPong]);
  EXPECT_TRUE([info dragButtonToShouldCopy]);
  EXPECT_FALSE([info dragURLsPong]);
  EXPECT_TRUE([info dragBookmarkDataPong]);
}

TEST_F(BookmarkBarViewTestCocoa, URLDragAndDrop) {
  base::scoped_nsobject<FakeBookmarkDraggingInfo> info(
      [[FakeBookmarkDraggingInfo alloc] init]);
  [view_ setController:info.get()];
  [info reset];

  NSArray* dragTypes = [URLDropTargetHandler handledDragTypes];
  for (NSString* type in dragTypes) {
    [info setDragDataType:type];
    EXPECT_EQ([view_ draggingEntered:(id)info.get()], NSDragOperationCopy);
    EXPECT_TRUE([view_ performDragOperation:(id)info.get()]);
    EXPECT_FALSE([info dragButtonToPong]);
    EXPECT_TRUE([info dragURLsPong]);
    EXPECT_TRUE([info dragBookmarkDataPong]);
    [info reset];
  }
}

TEST_F(BookmarkBarViewTestCocoa, BookmarkButtonDropIndicator) {
  base::scoped_nsobject<FakeBookmarkDraggingInfo> info(
      [[FakeBookmarkDraggingInfo alloc] init]);
  [view_ setController:info.get()];
  [info reset];

  base::scoped_nsobject<BookmarkButton> dragged_button(
      [[BookmarkButton alloc] init]);
  [info setDraggingSource:dragged_button.get()];
  [info setDragDataType:ui::ClipboardUtil::UTIForPasteboardType(
                            kBookmarkButtonDragType)];
  EXPECT_FALSE([info draggingEnteredCalled]);
  EXPECT_EQ([view_ draggingEntered:(id)info.get()], NSDragOperationMove);
  EXPECT_TRUE([info draggingEnteredCalled]);  // Ensure controller pinged.
  EXPECT_TRUE([view_ dropIndicatorShown]);
  EXPECT_EQ([view_ dropIndicatorPosition], kFakeIndicatorPos);

  [info setDropIndicatorShown:NO];
  EXPECT_EQ([view_ draggingEntered:(id)info.get()], NSDragOperationMove);
  EXPECT_FALSE([view_ dropIndicatorShown]);
}

}  // namespace
