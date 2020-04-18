// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>

#include "base/mac/scoped_nsobject.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_constants.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_button_cell.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_unittest_helper.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "chrome/browser/ui/cocoa/view_resizer_pong.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "ui/base/cocoa/animation_utils.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/material_design/material_design_controller.h"

using base::ASCIIToUTF16;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace {

const int kLotsOfNodesCount = 150;

// Deletes the bookmark corresponding to |button|.
void DeleteBookmark(BookmarkButton* button, Profile* profile) {
  const BookmarkNode* node = [button bookmarkNode];
  if (node) {
    BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile);
    model->Remove(node);
  }
}

}  // namespace

// Add a redirect to make testing easier.
@interface BookmarkBarFolderController(MakeTestingEasier)
- (void)validateMenuSpacing;
@end

@implementation BookmarkBarFolderController(MakeTestingEasier)

// Utility function to verify that the buttons in this folder are all
// evenly spaced in a progressive manner.
- (void)validateMenuSpacing {
  BOOL firstButton = YES;
  CGFloat lastVerticalOffset = 0.0;
  for (BookmarkButton* button in [self buttons]) {
    if (firstButton) {
      firstButton = NO;
      lastVerticalOffset = [button frame].origin.y;
    } else {
      CGFloat nextVerticalOffset = [button frame].origin.y;
      EXPECT_CGFLOAT_EQ(lastVerticalOffset -
                            bookmarks::kBookmarkFolderButtonHeight,
                        nextVerticalOffset);
      lastVerticalOffset = nextVerticalOffset;
    }
  }
}
@end

// Don't use a high window level when running unit tests -- it'll
// interfere with anything else you are working on.
// For testing.
@interface BookmarkBarFolderControllerNoLevel : BookmarkBarFolderController
@end

@implementation BookmarkBarFolderControllerNoLevel
- (void)configureWindowLevel {
  // Intentionally empty.
}
@end

@interface BookmarkBarFolderControllerPong : BookmarkBarFolderController {
  BOOL childFolderWillShow_;
  BOOL childFolderWillClose_;
}
@property (nonatomic, readonly) BOOL childFolderWillShow;
@property (nonatomic, readonly) BOOL childFolderWillClose;
@end

@implementation BookmarkBarFolderControllerPong
@synthesize childFolderWillShow = childFolderWillShow_;
@synthesize childFolderWillClose = childFolderWillClose_;

- (void)childFolderWillShow:(id<BookmarkButtonControllerProtocol>)child {
  childFolderWillShow_ = YES;
}

- (void)childFolderWillClose:(id<BookmarkButtonControllerProtocol>)child {
  childFolderWillClose_ = YES;
}

// We don't have a real BookmarkBarController as our parent root so
// we fake this one out.
- (void)closeAllBookmarkFolders {
  [self closeBookmarkFolder:self];
}

@end

// Redirect certain calls so they can be seen by tests.

@interface BookmarkBarControllerChildFolderRedirect : BookmarkBarController {
  BookmarkBarFolderController* childFolderDelegate_;
}
@property (nonatomic, assign) BookmarkBarFolderController* childFolderDelegate;
@end

@implementation BookmarkBarControllerChildFolderRedirect

@synthesize childFolderDelegate = childFolderDelegate_;

- (void)childFolderWillShow:(id<BookmarkButtonControllerProtocol>)child {
  [childFolderDelegate_ childFolderWillShow:child];
}

- (void)childFolderWillClose:(id<BookmarkButtonControllerProtocol>)child {
  [childFolderDelegate_ childFolderWillClose:child];
}

@end


class BookmarkBarFolderControllerTest : public CocoaProfileTest {
 public:
  base::scoped_nsobject<BookmarkBarControllerChildFolderRedirect> bar_;
  const BookmarkNode* folderA_;  // Owned by model.
  const BookmarkNode* longTitleNode_;  // Owned by model.

  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(profile());

    CreateModel();
  }

  void CreateModel() {
    BookmarkModel* model =
        BookmarkModelFactory::GetForBrowserContext(profile());
    const BookmarkNode* parent = model->bookmark_bar_node();
    const BookmarkNode* folderA = model->AddFolder(parent,
                                                   parent->child_count(),
                                                   ASCIIToUTF16("folder"));
    folderA_ = folderA;
    model->AddFolder(parent, parent->child_count(),
                     ASCIIToUTF16("sibbling folder"));
    const BookmarkNode* folderB = model->AddFolder(folderA,
                                                   folderA->child_count(),
                                                   ASCIIToUTF16("subfolder 1"));
    model->AddFolder(folderA,
                     folderA->child_count(),
                     ASCIIToUTF16("subfolder 2"));
    model->AddURL(folderA, folderA->child_count(), ASCIIToUTF16("title a"),
                  GURL("http://www.google.com/a"));
    longTitleNode_ = model->AddURL(
      folderA, folderA->child_count(),
      ASCIIToUTF16("title super duper long long whoa momma title you betcha"),
      GURL("http://www.google.com/b"));
    model->AddURL(folderB, folderB->child_count(), ASCIIToUTF16("t"),
                  GURL("http://www.google.com/c"));

    bar_.reset([[BookmarkBarControllerChildFolderRedirect alloc]
        initWithBrowser:browser()
           initialWidth:300
               delegate:nil]);
    [bar_ loaded:model];
    // Make parent frame for bookmark bar then open it.
    NSRect frame = [[test_window() contentView] frame];
    frame = NSMakeRect(
        frame.origin.x,
        frame.size.height - GetCocoaLayoutConstant(BOOKMARK_BAR_NTP_HEIGHT),
        frame.size.width, GetCocoaLayoutConstant(BOOKMARK_BAR_NTP_HEIGHT));
    NSView* fakeToolbarView = [[[NSView alloc] initWithFrame:frame]
                                autorelease];
    [[test_window() contentView] addSubview:fakeToolbarView];
    [fakeToolbarView addSubview:[bar_ view]];
    [bar_ setBookmarkBarEnabled:YES];
  }

  // Remove the bookmark with the long title.
  void RemoveLongTitleNode() {
    BookmarkModel* model =
        BookmarkModelFactory::GetForBrowserContext(profile());
    model->Remove(longTitleNode_);
  }

  // Add LOTS of nodes to our model if needed (e.g. scrolling).
  // Returns the number of nodes added.
  int AddLotsOfNodes() {
    BookmarkModel* model =
        BookmarkModelFactory::GetForBrowserContext(profile());
    for (int i = 0; i < kLotsOfNodesCount; i++) {
      model->AddURL(folderA_, folderA_->child_count(),
                    ASCIIToUTF16("repeated title"),
                    GURL("http://www.google.com/repeated/url"));
    }
    return kLotsOfNodesCount;
  }

  // Return a simple BookmarkBarFolderController.
  BookmarkBarFolderControllerPong* SimpleBookmarkBarFolderController() {
    BookmarkButton* parentButton = [[bar_ buttons] objectAtIndex:0];
    BookmarkBarFolderControllerPong* c =
      [[BookmarkBarFolderControllerPong alloc]
               initWithParentButton:parentButton
                   parentController:nil
                      barController:bar_
                            profile:profile()];
    [c window];  // Force nib load.
    return c;
  }
};

TEST_F(BookmarkBarFolderControllerTest, InitCreateAndDelete) {
  base::scoped_nsobject<BookmarkBarFolderController> bbfc;
  bbfc.reset(SimpleBookmarkBarFolderController());

  // Make sure none of the buttons overlap, that all are inside
  // the content frame, and their cells are of the proper class.
  NSArray* buttons = [bbfc buttons];
  EXPECT_TRUE([buttons count]);
  for (unsigned int i = 0; i < ([buttons count]-1); i++) {
    EXPECT_FALSE(NSContainsRect([[buttons objectAtIndex:i] frame],
                              [[buttons objectAtIndex:i+1] frame]));
  }
  Class cellClass = [BookmarkBarFolderButtonCell class];
  for (BookmarkButton* button in buttons) {
    NSRect r = [[bbfc folderView] convertRect:[button frame] fromView:button];
    // TODO(jrg): remove this adjustment.
    NSRect bigger = NSInsetRect([[bbfc folderView] frame], -2, 0);
    EXPECT_TRUE(NSContainsRect(bigger, r));
    EXPECT_TRUE([[button cell] isKindOfClass:cellClass]);
  }
}

// Make sure closing of the window releases the controller.
// (e.g. valgrind shouldn't complain if we do this).
TEST_F(BookmarkBarFolderControllerTest, ReleaseOnClose) {
  base::scoped_nsobject<BookmarkBarFolderController> bbfc;
  bbfc.reset(SimpleBookmarkBarFolderController());
  EXPECT_TRUE(bbfc.get());

  [bbfc retain];  // stop the scoped_nsobject from doing anything
  [[bbfc window] close];  // trigger an autorelease of bbfc.get()
}

TEST_F(BookmarkBarFolderControllerTest, BasicPosition) {
  BookmarkButton* parentButton = [[bar_ buttons] objectAtIndex:0];
  EXPECT_TRUE(parentButton);

  // If parent is a BookmarkBarController, grow down.
  base::scoped_nsobject<BookmarkBarFolderController> bbfc;
  bbfc.reset([[BookmarkBarFolderController alloc]
               initWithParentButton:parentButton
                   parentController:nil
                      barController:bar_
                            profile:profile()]);
  [bbfc window];
  NSPoint pt = [bbfc windowTopLeftForWidth:0 height:100];  // screen coords
  NSPoint buttonOriginInWindow =
      [parentButton convertRect:[parentButton bounds]
                         toView:nil].origin;
  NSPoint buttonOriginInScreen = ui::ConvertPointFromWindowToScreen(
      [parentButton window], buttonOriginInWindow);
  // Within margin
  EXPECT_LE(std::abs(pt.x - buttonOriginInScreen.x),
            bookmarks::kBookmarkMenuOverlap + 1);
  EXPECT_LE(std::abs(pt.y - buttonOriginInScreen.y),
            bookmarks::kBookmarkMenuOverlap + 1);

  // Make sure we see the window shift left if it spills off the screen
  pt = [bbfc windowTopLeftForWidth:0 height:100];
  NSPoint shifted = [bbfc windowTopLeftForWidth:9999999 height:100];
  EXPECT_LT(shifted.x, pt.x);

  // If parent is a BookmarkBarFolderController, grow right.
  base::scoped_nsobject<BookmarkBarFolderController> bbfc2;
  bbfc2.reset([[BookmarkBarFolderController alloc]
                initWithParentButton:[[bbfc buttons] objectAtIndex:0]
                    parentController:bbfc.get()
                       barController:bar_
                             profile:profile()]);
  [bbfc2 window];
  pt = [bbfc2 windowTopLeftForWidth:0 height:100];
  // We're now overlapping the window a bit.
  EXPECT_EQ(pt.x, NSMaxX([[bbfc.get() window] frame]) -
            bookmarks::kBookmarkMenuOverlap);
}

// Confirm we grow right until end of screen, then start growing left
// until end of screen again, then right.
TEST_F(BookmarkBarFolderControllerTest, PositionRightLeftRight) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* parent = model->bookmark_bar_node();
  const BookmarkNode* folder = parent;

  const int count = 100;
  int i;
  // Make some super duper deeply nested folders.
  for (i = 0; i < count; i++) {
    folder = model->AddFolder(folder, 0, ASCIIToUTF16("nested folder"));
  }

  // Setup initial state for opening all folders.
  folder = parent;
  BookmarkButton* parentButton = [[bar_ buttons] objectAtIndex:0];
  BookmarkBarFolderController* parentController = nil;
  EXPECT_TRUE(parentButton);

  // Open them all.
  base::scoped_nsobject<NSMutableArray> folder_controller_array;
  folder_controller_array.reset([[NSMutableArray array] retain]);
  for (i=0; i<count; i++) {
    BookmarkBarFolderControllerNoLevel* bbfcl =
        [[BookmarkBarFolderControllerNoLevel alloc]
          initWithParentButton:parentButton
              parentController:parentController
                 barController:bar_
                       profile:profile()];
    [folder_controller_array addObject:bbfcl];
    [bbfcl autorelease];
    [bbfcl window];
    parentController = bbfcl;
    parentButton = [[bbfcl buttons] objectAtIndex:0];
  }

  // Make vector of all x positions.
  std::vector<CGFloat> leftPositions;
  for (i=0; i<count; i++) {
    CGFloat x = [[[folder_controller_array objectAtIndex:i] window]
                  frame].origin.x;
    leftPositions.push_back(x);
  }

  // Make sure the first few grow right.
  for (i=0; i<3; i++)
    EXPECT_TRUE(leftPositions[i+1] > leftPositions[i]);

  // Look for the first "grow left".
  while (leftPositions[i] > leftPositions[i-1])
    i++;
  // Confirm the next few also grow left.
  int j;
  for (j=i; j<i+3; j++)
    EXPECT_TRUE(leftPositions[j+1] < leftPositions[j]);
  i = j;

  // Finally, confirm we see a "grow right" once more.
  while (leftPositions[i] < leftPositions[i-1])
    i++;
  // (No need to EXPECT a final "grow right"; if we didn't find one
  // we'd get a C++ array bounds exception).
}

TEST_F(BookmarkBarFolderControllerTest, DropDestination) {
  base::scoped_nsobject<BookmarkBarFolderController> bbfc;
  bbfc.reset(SimpleBookmarkBarFolderController());
  EXPECT_TRUE(bbfc.get());

  // Confirm "off the top" and "off the bottom" match no buttons.
  NSPoint p = NSMakePoint(NSMidX([[bbfc folderView] frame]), 10000);
  EXPECT_FALSE([bbfc buttonForDroppingOnAtPoint:p]);
  EXPECT_TRUE([bbfc shouldShowIndicatorShownForPoint:p]);
  p = NSMakePoint(NSMidX([[bbfc folderView] frame]), -1);
  EXPECT_FALSE([bbfc buttonForDroppingOnAtPoint:p]);
  EXPECT_TRUE([bbfc shouldShowIndicatorShownForPoint:p]);

  // Confirm "right in the center" (give or take a pixel) is a match,
  // and confirm "just barely in the button" is not.  Anything more
  // specific seems likely to be tweaked.  We don't loop over all
  // buttons because the scroll view makes them not visible.
  for (BookmarkButton* button in [bbfc buttons]) {
    CGFloat x = NSMidX([button frame]);
    CGFloat y = NSMidY([button frame]);
    // Somewhere near the center: a match (but only if a folder!)
    if ([button isFolder]) {
      EXPECT_EQ(button,
                [bbfc buttonForDroppingOnAtPoint:NSMakePoint(x-1, y+1)]);
      EXPECT_EQ(button,
                [bbfc buttonForDroppingOnAtPoint:NSMakePoint(x+1, y-1)]);
      EXPECT_FALSE([bbfc shouldShowIndicatorShownForPoint:NSMakePoint(x, y)]);;
    } else {
      // If not a folder we don't drop into it.
      EXPECT_FALSE([bbfc buttonForDroppingOnAtPoint:NSMakePoint(x-1, y+1)]);
      EXPECT_FALSE([bbfc buttonForDroppingOnAtPoint:NSMakePoint(x+1, y-1)]);
      EXPECT_TRUE([bbfc shouldShowIndicatorShownForPoint:NSMakePoint(x, y)]);;
    }
  }
}

TEST_F(BookmarkBarFolderControllerTest, OpenFolder) {
  base::scoped_nsobject<BookmarkBarFolderController> bbfc;
  bbfc.reset(SimpleBookmarkBarFolderController());
  EXPECT_TRUE(bbfc.get());

  EXPECT_FALSE([bbfc folderController]);
  BookmarkButton* button = [[bbfc buttons] objectAtIndex:0];
  [bbfc openBookmarkFolderFromButton:button];
  id controller = [bbfc folderController];
  EXPECT_TRUE(controller);
  EXPECT_EQ([controller parentButton], button);

  // Click the same one --> it gets closed.
  [bbfc openBookmarkFolderFromButton:[[bbfc buttons] objectAtIndex:0]];
  EXPECT_FALSE([bbfc folderController]);

  // Open a new one --> change.
  [bbfc openBookmarkFolderFromButton:[[bbfc buttons] objectAtIndex:1]];
  EXPECT_NE(controller, [bbfc folderController]);
  EXPECT_NE([[bbfc folderController] parentButton], button);

  // Close it --> all gone!
  [bbfc closeBookmarkFolder:nil];
  EXPECT_FALSE([bbfc folderController]);
}

TEST_F(BookmarkBarFolderControllerTest, DeleteOpenFolder) {
  base::scoped_nsobject<BookmarkBarFolderController> parent_controller(
      SimpleBookmarkBarFolderController());

  // Open a folder.
  EXPECT_FALSE([parent_controller folderController]);
  BookmarkButton* button = [[parent_controller buttons] objectAtIndex:0];
  [parent_controller openBookmarkFolderFromButton:button];
  EXPECT_EQ([[parent_controller folderController] parentButton], button);

  // Delete the folder's button - the folder should close.
  [parent_controller removeButton:0 animate:NO];
  EXPECT_FALSE([parent_controller folderController]);
}

TEST_F(BookmarkBarFolderControllerTest, ChildFolderCallbacks) {
  base::scoped_nsobject<BookmarkBarFolderControllerPong> bbfc;
  bbfc.reset(SimpleBookmarkBarFolderController());
  EXPECT_TRUE(bbfc.get());
  [bar_ setChildFolderDelegate:bbfc.get()];

  EXPECT_FALSE([bbfc childFolderWillShow]);
  [bbfc openBookmarkFolderFromButton:[[bbfc buttons] objectAtIndex:0]];
  EXPECT_TRUE([bbfc childFolderWillShow]);

  EXPECT_FALSE([bbfc childFolderWillClose]);
  [bbfc closeBookmarkFolder:nil];
  EXPECT_TRUE([bbfc childFolderWillClose]);

  [bar_ setChildFolderDelegate:nil];
}

// Make sure bookmark folders have variable widths.
TEST_F(BookmarkBarFolderControllerTest, ChildFolderWidth) {
  base::scoped_nsobject<BookmarkBarFolderController> bbfc;

  bbfc.reset(SimpleBookmarkBarFolderController());
  EXPECT_TRUE(bbfc.get());
  [bbfc showWindow:bbfc.get()];
  CGFloat wideWidth = NSWidth([[bbfc window] frame]);

  RemoveLongTitleNode();
  bbfc.reset(SimpleBookmarkBarFolderController());
  EXPECT_TRUE(bbfc.get());
  CGFloat thinWidth = NSWidth([[bbfc window] frame]);

  // Make sure window size changed as expected.
  EXPECT_GT(wideWidth, thinWidth);
}

// Scrolling (in this case using keyboard up/down buttons) should
// not be a cause of item hovering change where the mouse is pointing to.
// Here we are simulating scrolling by calling -performOneScroll: which
// indirectly is called by keyboard up/down buttons.
TEST_F(BookmarkBarFolderControllerTest, ScrollingBehaviorAndMouseMovement){
  base::scoped_nsobject<BookmarkBarFolderController> bbfc;
  AddLotsOfNodes();
  bbfc.reset(SimpleBookmarkBarFolderController());
  [bbfc showWindow:bbfc.get()];
  // We should be able to scroll-up otherwise the rest of the test is pointless.
  ASSERT_TRUE([bbfc canScrollUp]);
  NSArray* buttons = [bbfc buttons];
  BookmarkButton* currentButton = [bbfc buttonThatMouseIsIn];
  // Mouse cursor is not pointing to any button.
  EXPECT_FALSE(currentButton);
  BookmarkButton* firstButton = [buttons objectAtIndex:0];
  [bbfc mouseEnteredButton:firstButton event:nil];
  // Mouse cursor should be over the first button.
  EXPECT_EQ(firstButton, [bbfc buttonThatMouseIsIn]);
  while ([bbfc canScrollUp]) {
    [bbfc performOneScroll:200 updateMouseSelection:NO];
    [bbfc mouseEnteredButton:[buttons objectAtIndex:2] event:nil];
    // -buttonThatMouseIsIn: must return firstButton because we
    // are still scrolling. i.e. should not be changed when
    // -mouseEnteredButton: is called.
    EXPECT_EQ(firstButton,[bbfc buttonThatMouseIsIn]);
    // We are scrolling unless mouse movement happens.
    EXPECT_TRUE([bbfc isScrolling]);
  }
  [bbfc mouseExitedButton:firstButton event:nil];
  // If mouse exit from a button scrolling should be stopped.
  EXPECT_FALSE([bbfc isScrolling]);
  [bbfc mouseEnteredButton:nil event:nil];
}

// Simple scrolling tests.
// Currently flaky due to a changed definition of the correct menu boundaries.
TEST_F(BookmarkBarFolderControllerTest, DISABLED_SimpleScroll) {
  base::scoped_nsobject<BookmarkBarFolderController> bbfc;
  NSRect screenFrame = [[NSScreen mainScreen] visibleFrame];
  CGFloat screenHeight = NSHeight(screenFrame);
  int nodecount = AddLotsOfNodes();
  bbfc.reset(SimpleBookmarkBarFolderController());
  EXPECT_TRUE(bbfc.get());
  [bbfc showWindow:bbfc.get()];
  NSWindow* window = [bbfc window];

  // The window should be shorter than the screen but reach exactly to the
  // bottom of the screen since it's scrollable.
  EXPECT_LT(NSHeight([window frame]), screenHeight);
  EXPECT_CGFLOAT_EQ(0.0, [window frame].origin.y);

  // Initially, should show scroll-up but not scroll-down.
  EXPECT_TRUE([bbfc canScrollUp]);
  EXPECT_FALSE([bbfc canScrollDown]);

  // Scroll up a bit.  Make sure the window has gotten bigger each time.
  // Also, for each scroll, make sure our hit test finds a new button
  // (to confirm the content area changed).
  NSView* savedHit = nil;
  NSScrollView* scrollView = [bbfc scrollView];

  // Find the next-to-last button showing at the bottom of the window and
  // use its center for hit testing.
  BookmarkButton* targetButton = nil;
  NSPoint scrollPoint = [scrollView documentVisibleRect].origin;
  for (BookmarkButton* button in [bbfc buttons]) {
    NSRect buttonFrame = [button frame];
    buttonFrame.origin.y -= scrollPoint.y;
    if (buttonFrame.origin.y < 0.0)
      break;
    targetButton = button;
  }
  EXPECT_TRUE(targetButton != nil);
  NSPoint hitPoint = [targetButton frame].origin;
  hitPoint.x += 50.0;
  hitPoint.y += (bookmarks::kBookmarkFolderButtonHeight / 2.0) - scrollPoint.y;
  hitPoint = [targetButton convertPoint:hitPoint toView:scrollView];

  for (int i = 0; i < 3; i++) {
    CGFloat height = NSHeight([window frame]);
    [bbfc performOneScroll:60 updateMouseSelection:NO];
    EXPECT_GT(NSHeight([window frame]), height);
    NSView* hit = [scrollView hitTest:hitPoint];
    // We should hit a bookmark button.
    EXPECT_TRUE([[hit className] isEqualToString:@"BookmarkButton"]);
    EXPECT_NE(hit, savedHit);
    savedHit = hit;
  }

  // Keep scrolling up; make sure we never get bigger than the screen.
  // Also confirm we never scroll the window off the screen.
  bool bothAtOnce = false;
  while ([bbfc canScrollUp]) {
    [bbfc performOneScroll:60 updateMouseSelection:NO];
    EXPECT_TRUE(NSContainsRect([[NSScreen mainScreen] frame], [window frame]));
    // Make sure, sometime during our scroll, we have the ability to
    // scroll in either direction.
    if ([bbfc canScrollUp] &&
        [bbfc canScrollDown])
      bothAtOnce = true;
  }
  EXPECT_TRUE(bothAtOnce);

  // Once we've scrolled to the end, our only option should be to scroll back.
  EXPECT_FALSE([bbfc canScrollUp]);
  EXPECT_TRUE([bbfc canScrollDown]);

  NSRect wholeScreenRect = [[NSScreen mainScreen] frame];

  // Now scroll down and make sure the window size does not change.
  // Also confirm we never scroll the window off the screen the other
  // way.
  for (int i = 0; i < nodecount+50; ++i) {
    [bbfc performOneScroll:-60 updateMouseSelection:NO];
    // Once we can no longer scroll down the window height changes.
    if (![bbfc canScrollDown])
      break;
    EXPECT_TRUE(NSContainsRect(wholeScreenRect, [window frame]));
  }

  EXPECT_GT(NSHeight(wholeScreenRect), NSHeight([window frame]));
  EXPECT_TRUE(NSContainsRect(wholeScreenRect, [window frame]));
}

// Folder menu sizing and placement while deleting bookmarks
// and scrolling tests.
TEST_F(BookmarkBarFolderControllerTest, MenuPlacementWhileScrollingDeleting) {
  base::scoped_nsobject<BookmarkBarFolderController> bbfc;
  AddLotsOfNodes();
  bbfc.reset(SimpleBookmarkBarFolderController());
  [bbfc showWindow:bbfc.get()];
  NSWindow* menuWindow = [bbfc window];
  BookmarkBarFolderController* folder = [bar_ folderController];
  NSArray* buttons = [folder buttons];

  // Before scrolling any, delete a bookmark and make sure the window top has
  // not moved. Pick a button which is near the top and visible.
  CGFloat oldTop = [menuWindow frame].origin.y + NSHeight([menuWindow frame]);
  BookmarkButton* button = [buttons objectAtIndex:3];
  DeleteBookmark(button, profile());
  CGFloat newTop = [menuWindow frame].origin.y + NSHeight([menuWindow frame]);
  EXPECT_CGFLOAT_EQ(oldTop, newTop);

  // Scroll so that both the top and bottom scroll arrows show, make sure
  // the top of the window has moved up, then delete a visible button and
  // make sure the top has not moved.
  oldTop = newTop;
  const CGFloat scrollOneBookmark = bookmarks::kBookmarkFolderButtonHeight +
      bookmarks::kBookmarkVerticalPadding;
  NSUInteger buttonCounter = 0;
  NSUInteger extraButtonLimit = 3;
  while (![bbfc canScrollDown] || extraButtonLimit > 0) {
    [bbfc performOneScroll:scrollOneBookmark updateMouseSelection:NO];
    ++buttonCounter;
    if ([bbfc canScrollDown])
      --extraButtonLimit;
  }
  newTop = [menuWindow frame].origin.y + NSHeight([menuWindow frame]);
  EXPECT_NE(oldTop, newTop);
  oldTop = newTop;
  button = [buttons objectAtIndex:buttonCounter + 3];
  DeleteBookmark(button, profile());
  newTop = [menuWindow frame].origin.y + NSHeight([menuWindow frame]);
  EXPECT_CGFLOAT_EQ(oldTop, newTop);

  // Scroll so that the top scroll arrow is no longer showing, make sure
  // the top of the window has not moved, then delete a visible button and
  // make sure the top has not moved.
  while ([bbfc canScrollDown]) {
    [bbfc performOneScroll:-scrollOneBookmark updateMouseSelection:NO];
    --buttonCounter;
  }
  button = [buttons objectAtIndex:buttonCounter + 3];
  DeleteBookmark(button, profile());
  newTop = [menuWindow frame].origin.y + NSHeight([menuWindow frame]);
  EXPECT_CGFLOAT_EQ(oldTop - bookmarks::kScrollWindowVerticalMargin, newTop);
}

// Make sure that we return the correct browser window.
TEST_F(BookmarkBarFolderControllerTest, BrowserWindow) {
  base::scoped_nsobject<BookmarkBarFolderController> controller(
      SimpleBookmarkBarFolderController());
  EXPECT_EQ([bar_ browserWindow], [controller browserWindow]);
}

@interface FakedDragInfo : NSObject {
@public
  NSPoint dropLocation_;
  NSDragOperation sourceMask_;
}
@property (nonatomic, assign) NSPoint dropLocation;
- (void)setDraggingSourceOperationMask:(NSDragOperation)mask;
@end

@implementation FakedDragInfo

@synthesize dropLocation = dropLocation_;

- (id)init {
  if ((self = [super init])) {
    dropLocation_ = NSZeroPoint;
    sourceMask_ = NSDragOperationMove;
  }
  return self;
}

// NSDraggingInfo protocol functions.

- (id)draggingPasteboard {
  return self;
}

- (id)draggingSource {
  return self;
}

- (NSDragOperation)draggingSourceOperationMask {
  return sourceMask_;
}

- (NSPoint)draggingLocation {
  return dropLocation_;
}

// Other functions.

- (void)setDraggingSourceOperationMask:(NSDragOperation)mask {
  sourceMask_ = mask;
}

@end


class BookmarkBarFolderControllerMenuTest : public CocoaProfileTest {
 public:
  base::scoped_nsobject<NSView> parent_view_;
  base::scoped_nsobject<ViewResizerPong> resizeDelegate_;
  base::scoped_nsobject<BookmarkBarController> bar_;

  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    resizeDelegate_.reset([[ViewResizerPong alloc] init]);
    NSRect parent_frame = NSMakeRect(0, 0, 800, 50);
    parent_view_.reset([[NSView alloc] initWithFrame:parent_frame]);
    [parent_view_ setHidden:YES];
    bar_.reset([[BookmarkBarController alloc]
        initWithBrowser:browser()
           initialWidth:NSWidth(parent_frame)
               delegate:nil]);
    InstallAndToggleBar(bar_.get());
  }

  void InstallAndToggleBar(BookmarkBarController* bar) {
    // Forces loading of the nib.
    [[bar controlledView] setResizeDelegate:resizeDelegate_];
    // Awkwardness to look like we've been installed.
    [parent_view_ addSubview:[bar view]];
    NSRect frame = [[[bar view] superview] frame];
    frame.origin.y = 400;
    [[[bar view] superview] setFrame:frame];

    // Make sure it's on in a window so viewDidMoveToWindow is called
    [[test_window() contentView] addSubview:parent_view_];

    // Make sure it's open so certain things aren't no-ops.
    [bar updateState:BookmarkBar::SHOW
          changeType:BookmarkBar::DONT_ANIMATE_STATE_CHANGE];
  }
};

TEST_F(BookmarkBarFolderControllerMenuTest, DragMoveBarBookmarkToFolder) {
  WithNoAnimation at_all;
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b "
      "2f2f3b ] 2f3b ] 3b 4f:[ 4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f2f:[ 4f2f1b "
      "4f2f2b 4f2f3b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] 5b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Pop up a folder menu and drag in a button from the bar.
  BookmarkButton* toFolder = [bar_ buttonWithTitleEqualTo:@"2f"];
  NSRect oldToFolderFrame = [toFolder frame];
  [[toFolder target] performSelector:@selector(openBookmarkFolderFromButton:)
                          withObject:toFolder];
  BookmarkBarFolderController* folderController = [bar_ folderController];
  EXPECT_TRUE(folderController);
  NSWindow* toWindow = [folderController window];
  EXPECT_TRUE(toWindow);
  NSRect oldToWindowFrame = [toWindow frame];
  // Drag a bar button onto a bookmark (i.e. not a folder) in a folder
  // so it should end up below the target bookmark.
  BookmarkButton* draggedButton = [bar_ buttonWithTitleEqualTo:@"1b"];
  ASSERT_TRUE(draggedButton);
  CGFloat horizontalShift =
      NSWidth([draggedButton frame]) + bookmarks::kBookmarkHorizontalPadding;
  BookmarkButton* targetButton =
      [folderController buttonWithTitleEqualTo:@"2f1b"];
  ASSERT_TRUE(targetButton);
  [folderController dragButton:draggedButton
                            to:[targetButton center]
                          copy:NO];
  // The button should have landed just after "2f1b".
  const std::string expected_string("2f:[ 2f1b 1b 2f2f:[ 2f2f1b "
      "2f2f2b 2f2f3b ] 2f3b ] 3b 4f:[ 4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f2f:[ "
      "4f2f1b 4f2f2b 4f2f3b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] 5b ");
  EXPECT_EQ(expected_string, bookmarks::test::ModelStringFromNode(root));

  // Reopen the window.
  [[toFolder target] performSelector:@selector(openBookmarkFolderFromButton:)
                          withObject:toFolder];
  EXPECT_TRUE([bar_ folderController]);
  folderController = [bar_ folderController];

  // Gather the new frames.
  NSRect newToFolderFrame = [toFolder frame];
  NSRect newToWindowFrame = [toWindow frame];
  // The toFolder should have shifted left horizontally but not vertically.
  NSRect expectedToFolderFrame =
      NSOffsetRect(oldToFolderFrame, -horizontalShift, 0);
  EXPECT_NSRECT_EQ(expectedToFolderFrame, newToFolderFrame);
  // The toWindow should have shifted left horizontally, down vertically,
  // and grown vertically.
  NSRect expectedToWindowFrame = oldToWindowFrame;
  expectedToWindowFrame.origin.x -= horizontalShift;
  expectedToWindowFrame.origin.y -= bookmarks::kBookmarkFolderButtonHeight;
  expectedToWindowFrame.size.height += bookmarks::kBookmarkFolderButtonHeight;
  EXPECT_NSRECT_EQ(expectedToWindowFrame, newToWindowFrame);

  // Check button spacing.
  [folderController validateMenuSpacing];

  // Move the button back to the bar at the beginning.
  draggedButton = [folderController buttonWithTitleEqualTo:@"1b"];
  ASSERT_TRUE(draggedButton);
  targetButton = [bar_ buttonWithTitleEqualTo:@"2f"];
  ASSERT_TRUE(targetButton);
  [bar_ dragButton:draggedButton
                to:[targetButton left]
              copy:NO];
  EXPECT_EQ(model_string, bookmarks::test::ModelStringFromNode(root));
  // Don't check the folder window since it's not supposed to be showing.
}

TEST_F(BookmarkBarFolderControllerMenuTest, DragCopyBarBookmarkToFolder) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b "
      "2f2f3b ] 2f3b ] 3b 4f:[ 4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f2f:[ 4f2f1b "
      "4f2f2b 4f2f3b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] 5b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Pop up a folder menu and copy in a button from the bar.
  BookmarkButton* toFolder = [bar_ buttonWithTitleEqualTo:@"2f"];
  ASSERT_TRUE(toFolder);
  NSRect oldToFolderFrame = [toFolder frame];
  [[toFolder target] performSelector:@selector(openBookmarkFolderFromButton:)
                          withObject:toFolder];
  BookmarkBarFolderController* folderController = [bar_ folderController];
  EXPECT_TRUE(folderController);
  NSWindow* toWindow = [folderController window];
  EXPECT_TRUE(toWindow);
  NSRect oldToWindowFrame = [toWindow frame];
  // Drag a bar button onto a bookmark (i.e. not a folder) in a folder
  // so it should end up below the target bookmark.
  BookmarkButton* draggedButton = [bar_ buttonWithTitleEqualTo:@"1b"];
  ASSERT_TRUE(draggedButton);
  BookmarkButton* targetButton =
      [folderController buttonWithTitleEqualTo:@"2f1b"];
  ASSERT_TRUE(targetButton);
  [folderController dragButton:draggedButton
                            to:[targetButton center]
                          copy:YES];
  // The button should have landed just after "2f1b".
  const std::string expected_1("1b 2f:[ 2f1b 1b 2f2f:[ 2f2f1b "
    "2f2f2b 2f2f3b ] 2f3b ] 3b 4f:[ 4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f2f:[ "
    "4f2f1b 4f2f2b 4f2f3b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] 5b ");
  EXPECT_EQ(expected_1, bookmarks::test::ModelStringFromNode(root));

  // Gather the new frames.
  NSRect newToFolderFrame = [toFolder frame];
  NSRect newToWindowFrame = [toWindow frame];
  // The toFolder should have shifted.
  EXPECT_NSRECT_EQ(oldToFolderFrame, newToFolderFrame);
  // The toWindow should have shifted down vertically and grown vertically.
  NSRect expectedToWindowFrame = oldToWindowFrame;
  expectedToWindowFrame.origin.y -= bookmarks::kBookmarkFolderButtonHeight;
  expectedToWindowFrame.size.height += bookmarks::kBookmarkFolderButtonHeight;
  EXPECT_NSRECT_EQ(expectedToWindowFrame, newToWindowFrame);

  // Copy the button back to the bar after "3b".
  draggedButton = [folderController buttonWithTitleEqualTo:@"1b"];
  ASSERT_TRUE(draggedButton);
  targetButton = [bar_ buttonWithTitleEqualTo:@"4f"];
  ASSERT_TRUE(targetButton);
  [bar_ dragButton:draggedButton
                to:[targetButton left]
              copy:YES];
  const std::string expected_2("1b 2f:[ 2f1b 1b 2f2f:[ 2f2f1b "
      "2f2f2b 2f2f3b ] 2f3b ] 3b 1b 4f:[ 4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f2f:[ "
      "4f2f1b 4f2f2b 4f2f3b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] 5b ");
  EXPECT_EQ(expected_2, bookmarks::test::ModelStringFromNode(root));
}

TEST_F(BookmarkBarFolderControllerMenuTest, DragMoveBarBookmarkToSubfolder) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b "
      "2f2f3b ] 2f3b ] 3b 4f:[ 4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f2f:[ 4f2f1b "
      "4f2f2b 4f2f3b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] 5b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Pop up a folder menu and a subfolder menu.
  BookmarkButton* toFolder = [bar_ buttonWithTitleEqualTo:@"4f"];
  ASSERT_TRUE(toFolder);
  [[toFolder target] performSelector:@selector(openBookmarkFolderFromButton:)
                          withObject:toFolder];
  BookmarkBarFolderController* folderController = [bar_ folderController];
  EXPECT_TRUE(folderController);
  NSWindow* toWindow = [folderController window];
  EXPECT_TRUE(toWindow);
  NSRect oldToWindowFrame = [toWindow frame];
  BookmarkButton* toSubfolder =
      [folderController buttonWithTitleEqualTo:@"4f2f"];
  ASSERT_TRUE(toSubfolder);
  [[toSubfolder target] performSelector:@selector(openBookmarkFolderFromButton:)
                             withObject:toSubfolder];
  BookmarkBarFolderController* subfolderController =
      [folderController folderController];
  EXPECT_TRUE(subfolderController);
  NSWindow* toSubwindow = [subfolderController window];
  EXPECT_TRUE(toSubwindow);
  NSRect oldToSubwindowFrame = [toSubwindow frame];
  // Drag a bar button onto a bookmark (i.e. not a folder) in a folder
  // so it should end up below the target bookmark.
  BookmarkButton* draggedButton = [bar_ buttonWithTitleEqualTo:@"5b"];
  ASSERT_TRUE(draggedButton);
  BookmarkButton* targetButton =
      [subfolderController buttonWithTitleEqualTo:@"4f2f3b"];
  ASSERT_TRUE(targetButton);
  [subfolderController dragButton:draggedButton
                               to:[targetButton center]
                             copy:NO];
  // The button should have landed just after "2f".
  const std::string expected_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b "
      "2f2f2b 2f2f3b ] 2f3b ] 3b 4f:[ 4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f2f:[ "
      "4f2f1b 4f2f2b 4f2f3b 5b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] ");
  EXPECT_EQ(expected_string, bookmarks::test::ModelStringFromNode(root));

  // Check button spacing.
  [folderController validateMenuSpacing];
  [subfolderController validateMenuSpacing];

  // Check the window layouts. The folder window should not have changed,
  // but the subfolder window should have shifted vertically and grown.
  NSRect newToWindowFrame = [toWindow frame];
  EXPECT_NSRECT_EQ(oldToWindowFrame, newToWindowFrame);
  NSRect newToSubwindowFrame = [toSubwindow frame];
  NSRect expectedToSubwindowFrame = oldToSubwindowFrame;
  expectedToSubwindowFrame.origin.y -= bookmarks::kBookmarkFolderButtonHeight;
  expectedToSubwindowFrame.size.height +=
      bookmarks::kBookmarkFolderButtonHeight;
  EXPECT_NSRECT_EQ(expectedToSubwindowFrame, newToSubwindowFrame);
}

TEST_F(BookmarkBarFolderControllerMenuTest, DragMoveWithinFolder) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b "
      "2f2f3b ] 2f3b ] 3b 4f:[ 4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f2f:[ 4f2f1b "
      "4f2f2b 4f2f3b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] 5b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Pop up a folder menu.
  BookmarkButton* toFolder = [bar_ buttonWithTitleEqualTo:@"4f"];
  ASSERT_TRUE(toFolder);
  [[toFolder target] performSelector:@selector(openBookmarkFolderFromButton:)
                          withObject:toFolder];
  BookmarkBarFolderController* folderController = [bar_ folderController];
  EXPECT_TRUE(folderController);
  NSWindow* toWindow = [folderController window];
  EXPECT_TRUE(toWindow);
  NSRect oldToWindowFrame = [toWindow frame];
  // Drag a folder button to the top within the same parent.
  BookmarkButton* draggedButton =
      [folderController buttonWithTitleEqualTo:@"4f2f"];
  ASSERT_TRUE(draggedButton);
  BookmarkButton* targetButton =
      [folderController buttonWithTitleEqualTo:@"4f1f"];
  ASSERT_TRUE(targetButton);
  [folderController dragButton:draggedButton
                            to:[targetButton top]
                          copy:NO];
  // The button should have landed above "4f1f".
  const std::string expected_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b "
      "2f2f2b 2f2f3b ] 2f3b ] 3b 4f:[ 4f2f:[ 4f2f1b 4f2f2b 4f2f3b ] "
      "4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] 5b ");
  EXPECT_EQ(expected_string, bookmarks::test::ModelStringFromNode(root));

  // The window should not have gone away.
  EXPECT_TRUE([bar_ folderController]);

  // The folder window should not have changed.
  NSRect newToWindowFrame = [toWindow frame];
  EXPECT_NSRECT_EQ(oldToWindowFrame, newToWindowFrame);

  // Check button spacing.
  [folderController validateMenuSpacing];
}

TEST_F(BookmarkBarFolderControllerMenuTest, DragParentOntoChild) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b "
      "2f2f3b ] 2f3b ] 3b 4f:[ 4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f2f:[ 4f2f1b "
      "4f2f2b 4f2f3b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] 5b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Pop up a folder menu.
  BookmarkButton* toFolder = [bar_ buttonWithTitleEqualTo:@"4f"];
  ASSERT_TRUE(toFolder);
  [[toFolder target] performSelector:@selector(openBookmarkFolderFromButton:)
                          withObject:toFolder];
  BookmarkBarFolderController* folderController = [bar_ folderController];
  EXPECT_TRUE(folderController);
  NSWindow* toWindow = [folderController window];
  EXPECT_TRUE(toWindow);
  // Drag a folder button to one of its children.
  BookmarkButton* draggedButton = [bar_ buttonWithTitleEqualTo:@"4f"];
  ASSERT_TRUE(draggedButton);
  BookmarkButton* targetButton =
      [folderController buttonWithTitleEqualTo:@"4f3f"];
  ASSERT_TRUE(targetButton);
  [folderController dragButton:draggedButton
                            to:[targetButton top]
                          copy:NO];
  // The model should not have changed.
  EXPECT_EQ(model_string, bookmarks::test::ModelStringFromNode(root));

  // Check button spacing.
  [folderController validateMenuSpacing];
}

TEST_F(BookmarkBarFolderControllerMenuTest, DragMoveChildToParent) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b "
      "2f2f3b ] 2f3b ] 3b 4f:[ 4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f2f:[ 4f2f1b "
      "4f2f2b 4f2f3b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] 5b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Pop up a folder menu and a subfolder menu.
  BookmarkButton* toFolder = [bar_ buttonWithTitleEqualTo:@"4f"];
  ASSERT_TRUE(toFolder);
  [[toFolder target] performSelector:@selector(openBookmarkFolderFromButton:)
                          withObject:toFolder];
  BookmarkBarFolderController* folderController = [bar_ folderController];
  EXPECT_TRUE(folderController);
  BookmarkButton* toSubfolder =
      [folderController buttonWithTitleEqualTo:@"4f2f"];
  ASSERT_TRUE(toSubfolder);
  [[toSubfolder target] performSelector:@selector(openBookmarkFolderFromButton:)
                             withObject:toSubfolder];
  BookmarkBarFolderController* subfolderController =
      [folderController folderController];
  EXPECT_TRUE(subfolderController);

  // Drag a subfolder bookmark to the parent folder.
  BookmarkButton* draggedButton =
      [subfolderController buttonWithTitleEqualTo:@"4f2f3b"];
  ASSERT_TRUE(draggedButton);
  BookmarkButton* targetButton =
      [folderController buttonWithTitleEqualTo:@"4f2f"];
  ASSERT_TRUE(targetButton);
  [folderController dragButton:draggedButton
                            to:[targetButton top]
                          copy:NO];
  // The button should have landed above "4f2f".
  const std::string expected_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b "
      "2f2f3b ] 2f3b ] 3b 4f:[ 4f1f:[ 4f1f1b 4f1f2b 4f1f3b ] 4f2f3b 4f2f:[ "
      "4f2f1b 4f2f2b ] 4f3f:[ 4f3f1b 4f3f2b 4f3f3b ] ] 5b ");
  EXPECT_EQ(expected_string, bookmarks::test::ModelStringFromNode(root));

  // Check button spacing.
  [folderController validateMenuSpacing];
  // The window should not have gone away.
  EXPECT_TRUE([bar_ folderController]);
  // The subfolder should have gone away.
  EXPECT_FALSE([folderController folderController]);
}

TEST_F(BookmarkBarFolderControllerMenuTest, DragWindowResizing) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string(
      "a b:[ b1 b2 b3 ] reallyReallyLongBookmarkName c ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Pop up a folder menu.
  BookmarkButton* toFolder = [bar_ buttonWithTitleEqualTo:@"b"];
  ASSERT_TRUE(toFolder);
  [[toFolder target] performSelector:@selector(openBookmarkFolderFromButton:)
                          withObject:toFolder];
  BookmarkBarFolderController* folderController = [bar_ folderController];
  EXPECT_TRUE(folderController);
  NSWindow* toWindow = [folderController window];
  EXPECT_TRUE(toWindow);
  CGFloat oldWidth = NSWidth([toWindow frame]);
  // Drag the bookmark with a long name to the folder.
  BookmarkButton* draggedButton =
      [bar_ buttonWithTitleEqualTo:@"reallyReallyLongBookmarkName"];
  ASSERT_TRUE(draggedButton);
  BookmarkButton* targetButton =
      [folderController buttonWithTitleEqualTo:@"b1"];
  ASSERT_TRUE(targetButton);
  [folderController dragButton:draggedButton
                            to:[targetButton center]
                          copy:NO];
  // Verify the model change.
  const std::string expected_string(
      "a b:[ b1 reallyReallyLongBookmarkName b2 b3 ] c ");
  EXPECT_EQ(expected_string, bookmarks::test::ModelStringFromNode(root));
  // Verify the window grew. Just test a reasonable width gain.
  CGFloat newWidth = NSWidth([toWindow frame]);
  EXPECT_LT(oldWidth + 30.0, newWidth);
}

TEST_F(BookmarkBarFolderControllerMenuTest, MoveRemoveAddButtons) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2b 2f3b ] 3b 4b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Pop up a folder menu.
  BookmarkButton* toFolder = [bar_ buttonWithTitleEqualTo:@"2f"];
  ASSERT_TRUE(toFolder);
  [[toFolder target] performSelector:@selector(openBookmarkFolderFromButton:)
                          withObject:toFolder];
  BookmarkBarFolderController* folder = [bar_ folderController];
  EXPECT_TRUE(folder);

  // Remember how many buttons are showing.
  NSArray* buttons = [folder buttons];
  NSUInteger oldDisplayedButtons = [buttons count];

  // Move a button around a bit.
  [folder moveButtonFromIndex:0 toIndex:2];
  EXPECT_NSEQ(@"2f2b", [[buttons objectAtIndex:0] title]);
  EXPECT_NSEQ(@"2f3b", [[buttons objectAtIndex:1] title]);
  EXPECT_NSEQ(@"2f1b", [[buttons objectAtIndex:2] title]);
  EXPECT_EQ(oldDisplayedButtons, [buttons count]);
  [folder moveButtonFromIndex:2 toIndex:0];
  EXPECT_NSEQ(@"2f1b", [[buttons objectAtIndex:0] title]);
  EXPECT_NSEQ(@"2f2b", [[buttons objectAtIndex:1] title]);
  EXPECT_NSEQ(@"2f3b", [[buttons objectAtIndex:2] title]);
  EXPECT_EQ(oldDisplayedButtons, [buttons count]);

  // Add a couple of buttons.
  const BookmarkNode* node = root->GetChild(2); // Purloin an existing node.
  [folder addButtonForNode:node atIndex:0];
  EXPECT_NSEQ(@"3b", [[buttons objectAtIndex:0] title]);
  EXPECT_NSEQ(@"2f1b", [[buttons objectAtIndex:1] title]);
  EXPECT_NSEQ(@"2f2b", [[buttons objectAtIndex:2] title]);
  EXPECT_NSEQ(@"2f3b", [[buttons objectAtIndex:3] title]);
  EXPECT_EQ(oldDisplayedButtons + 1, [buttons count]);
  node = root->GetChild(3);
  [folder addButtonForNode:node atIndex:-1];
  EXPECT_NSEQ(@"3b", [[buttons objectAtIndex:0] title]);
  EXPECT_NSEQ(@"2f1b", [[buttons objectAtIndex:1] title]);
  EXPECT_NSEQ(@"2f2b", [[buttons objectAtIndex:2] title]);
  EXPECT_NSEQ(@"2f3b", [[buttons objectAtIndex:3] title]);
  EXPECT_NSEQ(@"4b", [[buttons objectAtIndex:4] title]);
  EXPECT_EQ(oldDisplayedButtons + 2, [buttons count]);

  // Remove a couple of buttons.
  [folder removeButton:4 animate:NO];
  [folder removeButton:1 animate:NO];
  EXPECT_NSEQ(@"3b", [[buttons objectAtIndex:0] title]);
  EXPECT_NSEQ(@"2f2b", [[buttons objectAtIndex:1] title]);
  EXPECT_NSEQ(@"2f3b", [[buttons objectAtIndex:2] title]);
  EXPECT_EQ(oldDisplayedButtons, [buttons count]);

  // Check button spacing.
  [folder validateMenuSpacing];
}

TEST_F(BookmarkBarFolderControllerMenuTest, RemoveLastButtonOtherBookmarks) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* otherBookmarks = model->other_node();

  BookmarkButton* otherButton = [bar_ otherBookmarksButton];
  ASSERT_TRUE(otherButton);

  // Open the folder to get the folderController_.
  [[otherButton target] openBookmarkFolderFromButton:otherButton];
  BookmarkBarFolderController* folder = [bar_ folderController];
  EXPECT_TRUE(folder);

  // Initially there is only (empty) placeholder button, hence buttonCount
  // should be 1.
  NSArray* buttons = [folder buttons];
  EXPECT_TRUE(buttons);
  EXPECT_EQ(1U, [buttons count]);

  // Add a new bookmark into 'Other bookmarks' folder.
  model->AddURL(otherBookmarks, otherBookmarks->child_count(),
                ASCIIToUTF16("TheOther"),
                GURL("http://www.other.com"));

  // buttonCount still should be 1, as we remove the (empty) placeholder button
  // when adding a new button to an empty folder.
  EXPECT_EQ(1U, [buttons count]);

  // Now we have only 1 button; remove it so that 'Other bookmarks' folder
  // is hidden.
  [folder removeButton:0 animate:NO];
  EXPECT_EQ(0U, [buttons count]);

  // 'Other bookmarks' folder gets closed once we remove the last button. Hence
  // folderController_ should be NULL.
  EXPECT_FALSE([bar_ folderController]);
}

TEST_F(BookmarkBarFolderControllerMenuTest, ControllerForNode) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2b ] 3b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Find the main bar controller.
  const void* expectedController = bar_;
  const void* actualController = [bar_ controllerForNode:root];
  EXPECT_EQ(expectedController, actualController);

  // Pop up the folder menu.
  BookmarkButton* targetFolder = [bar_ buttonWithTitleEqualTo:@"2f"];
  ASSERT_TRUE(targetFolder);
  [[targetFolder target]
   performSelector:@selector(openBookmarkFolderFromButton:)
   withObject:targetFolder];
  BookmarkBarFolderController* folder = [bar_ folderController];
  EXPECT_TRUE(folder);

  // Find the folder controller using the folder controller.
  const BookmarkNode* targetNode = root->GetChild(1);
  expectedController = folder;
  actualController = [bar_ controllerForNode:targetNode];
  EXPECT_EQ(expectedController, actualController);

  // Find the folder controller from the bar.
  actualController = [folder controllerForNode:targetNode];
  EXPECT_EQ(expectedController, actualController);
}

TEST_F(BookmarkBarFolderControllerMenuTest, MenuSizingAndScrollArrows) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2b 3b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  const BookmarkNode* parent = model->bookmark_bar_node();
  const BookmarkNode* folder = model->AddFolder(parent,
                                                parent->child_count(),
                                                ASCIIToUTF16("BIG"));

  // Pop open the new folder window and verify it has one (empty) item.
  BookmarkButton* button = [bar_ buttonWithTitleEqualTo:@"BIG"];
  [[button target] performSelector:@selector(openBookmarkFolderFromButton:)
                        withObject:button];
  BookmarkBarFolderController* folderController = [bar_ folderController];
  EXPECT_TRUE(folderController);
  NSWindow* folderWindow = [folderController window];
  EXPECT_TRUE(folderWindow);
  CGFloat expectedHeight =
      (CGFloat)bookmarks::kBookmarkFolderButtonHeight +
      bookmarks::kBookmarkTopVerticalPadding +
      bookmarks::kBookmarkBottomVerticalPadding;
  NSRect windowFrame = [folderWindow frame];
  CGFloat windowHeight = NSHeight(windowFrame);
  EXPECT_CGFLOAT_EQ(expectedHeight, windowHeight);
  EXPECT_FALSE([folderController canScrollUp]);
  EXPECT_FALSE([folderController canScrollDown]);

  // Now add a real bookmark and reopen.
  model->AddURL(folder, folder->child_count(), ASCIIToUTF16("a"),
                GURL("http://a.com/"));
  folderController = [bar_ folderController];
  EXPECT_TRUE(folderController);
  NSView* folderView = [folderController folderView];
  EXPECT_TRUE(folderView);
  NSRect menuFrame = [folderView frame];
  NSView* visibleView = [folderController visibleView];
  NSRect visibleFrame = [visibleView frame];
  NSScrollView* scrollView = [folderController scrollView];
  NSRect scrollFrame = [scrollView frame];

  // Determine the margins between the scroll frame and the visible frame.
  CGFloat widthDelta = NSWidth(visibleFrame) - NSWidth(scrollFrame);

  CGFloat menuHeight = NSHeight(menuFrame);
  EXPECT_CGFLOAT_EQ(expectedHeight, menuHeight);
  CGFloat scrollerWidth = NSWidth(scrollFrame);
  button = [folderController buttonWithTitleEqualTo:@"a"];
  CGFloat buttonWidth = NSWidth([button frame]);
  EXPECT_CGFLOAT_EQ(scrollerWidth, buttonWidth);
  CGFloat visibleWidth = NSWidth(visibleFrame);
  EXPECT_CGFLOAT_EQ(visibleWidth - widthDelta, buttonWidth);
  EXPECT_CGFLOAT_EQ(scrollerWidth, NSWidth([folderView frame]));

  // Add a wider bookmark and make sure the button widths match.
  int reallyWideButtonNumber = folder->child_count();
  model->AddURL(folder, reallyWideButtonNumber,
                ASCIIToUTF16("A really, really, really, really, really, "
                            "really long name"),
                GURL("http://www.google.com/a"));
  BookmarkButton* bigButton =
      [folderController buttonWithTitleEqualTo:
       @"A really, really, really, really, really, really long name"];
  EXPECT_TRUE(bigButton);
  CGFloat buttonWidthB = NSWidth([bigButton frame]);
  EXPECT_LT(buttonWidth, buttonWidthB);
  // Add a bunch of bookmarks until the window becomes scrollable, then check
  // for a scroll up arrow.
  NSUInteger tripWire = 0;  // Prevent a runaway.
  while (![folderController canScrollUp] && ++tripWire < 1000) {
    model->AddURL(folder, folder->child_count(), ASCIIToUTF16("B"),
                  GURL("http://b.com/"));
  }
  EXPECT_TRUE([folderController canScrollUp]);

  // Remove one bookmark and make sure the scroll down arrow has been removed.
  // We'll remove the really long node so we can see if the buttons get resized.
  scrollerWidth = NSWidth([folderView frame]);
  buttonWidth = NSWidth([button frame]);
  model->Remove(folder->GetChild(reallyWideButtonNumber));
  EXPECT_FALSE([folderController canScrollUp]);
  EXPECT_FALSE([folderController canScrollDown]);

  // Check the size. It should have reduced.
  EXPECT_GT(scrollerWidth, NSWidth([folderView frame]));
  EXPECT_GT(buttonWidth, NSWidth([button frame]));

  // Check button spacing.
  [folderController validateMenuSpacing];
}

// See http://crbug.com/46101
TEST_F(BookmarkBarFolderControllerMenuTest, HoverThenDeleteBookmark) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const BookmarkNode* folder = model->AddFolder(root,
                                                root->child_count(),
                                                ASCIIToUTF16("BIG"));
  for (int i = 0; i < kLotsOfNodesCount; i++)
    model->AddURL(folder, folder->child_count(), ASCIIToUTF16("kid"),
                  GURL("http://kid.com/smile"));

  // Pop open the new folder window and hover one of its kids.
  BookmarkButton* button = [bar_ buttonWithTitleEqualTo:@"BIG"];
  [[button target] performSelector:@selector(openBookmarkFolderFromButton:)
                        withObject:button];
  BookmarkBarFolderController* bbfc = [bar_ folderController];
  NSArray* buttons = [bbfc buttons];

  // Hover over a button and verify that it is now known.
  button = [buttons objectAtIndex:3];
  BookmarkButton* buttonThatMouseIsIn = [bbfc buttonThatMouseIsIn];
  EXPECT_FALSE(buttonThatMouseIsIn);
  [bbfc mouseEnteredButton:button event:nil];
  buttonThatMouseIsIn = [bbfc buttonThatMouseIsIn];
  EXPECT_EQ(button, buttonThatMouseIsIn);

  // Delete the bookmark and verify that it is now not known.
  model->Remove(folder->GetChild(3));
  buttonThatMouseIsIn = [bbfc buttonThatMouseIsIn];
  EXPECT_FALSE(buttonThatMouseIsIn);
}

// Tests that sending keyboard events to the (empty) button in an empty
// folder menu does not crash. https://crbug.com/690424
TEST_F(BookmarkBarFolderControllerMenuTest, ActOnEmptyMenu) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const BookmarkNode* folder =
      model->AddFolder(root, root->child_count(), ASCIIToUTF16("empty"));
  ASSERT_TRUE(folder);

  BookmarkButton* button = [bar_ buttonWithTitleEqualTo:@"empty"];
  [[button target] performSelector:@selector(openBookmarkFolderFromButton:)
                        withObject:button];

  BookmarkBarFolderController* bbfc = [bar_ folderController];
  NSArray* buttons = [bbfc buttons];
  ASSERT_EQ(1u, [buttons count]);

  button = [buttons objectAtIndex:0];
  EXPECT_TRUE([button isEmpty]);

  [bbfc mouseEnteredButton:button event:nil];

  EXPECT_TRUE([bbfc handleInputText:@" "]);
}

// Tests that sending enter key event to the nested folder opens
// submenu for it. https://crbug.com/791962
TEST_F(BookmarkBarFolderControllerMenuTest, ActOnNestedFolder) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const BookmarkNode* folder =
      model->AddFolder(root, root->child_count(), ASCIIToUTF16("folder"));
  ASSERT_TRUE(folder);

  const BookmarkNode* nested_folder =
      model->AddFolder(folder, folder->child_count(), ASCIIToUTF16("nested"));
  ASSERT_TRUE(nested_folder);

  BookmarkButton* button = [bar_ buttonWithTitleEqualTo:@"folder"];
  [[button target] performSelector:@selector(openBookmarkFolderFromButton:)
                        withObject:button];

  BookmarkBarFolderController* bbfc = [bar_ folderController];
  NSArray* buttons = [bbfc buttons];
  ASSERT_EQ(1u, [buttons count]);

  button = [buttons objectAtIndex:0];
  [bbfc mouseEnteredButton:button event:nil];

  // It musn't be closed.
  EXPECT_FALSE([bbfc handleInputText:@" "]);
}

// Just like a BookmarkBarFolderController but intercedes when providing
// pasteboard drag data.
@interface BookmarkBarFolderControllerDragData : BookmarkBarFolderController {
  const BookmarkNode* dragDataNode_;  // Weak
}
- (void)setDragDataNode:(const BookmarkNode*)node;
@end

@implementation BookmarkBarFolderControllerDragData

- (id)initWithParentButton:(BookmarkButton*)button
          parentController:(BookmarkBarFolderController*)parentController
             barController:(BookmarkBarController*)barController
                   profile:(Profile*)profile {
  if ((self = [super initWithParentButton:button
                         parentController:parentController
                            barController:barController
                                  profile:profile])) {
    dragDataNode_ = NULL;
  }
  return self;
}

- (void)setDragDataNode:(const BookmarkNode*)node {
  dragDataNode_ = node;
}

- (std::vector<const BookmarkNode*>)retrieveBookmarkNodeData {
  std::vector<const BookmarkNode*> dragDataNodes;
  if(dragDataNode_) {
    dragDataNodes.push_back(dragDataNode_);
  }
  return dragDataNodes;
}

@end

TEST_F(BookmarkBarFolderControllerMenuTest, DragBookmarkData) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                                 "2f3b ] 3b 4b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);
  const BookmarkNode* other = model->other_node();
  const std::string other_string("O1b O2b O3f:[ O3f1b O3f2f ] "
                                 "O4f:[ O4f1b O4f2f ] 05b ");
  bookmarks::test::AddNodesFromModelString(model, other, other_string);

  // Validate initial model.
  std::string actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actual);
  actual = bookmarks::test::ModelStringFromNode(other);
  EXPECT_EQ(other_string, actual);

  // Pop open a folder.
  BookmarkButton* button = [bar_ buttonWithTitleEqualTo:@"2f"];
  base::scoped_nsobject<BookmarkBarFolderControllerDragData> folderController;
  folderController.reset([[BookmarkBarFolderControllerDragData alloc]
                          initWithParentButton:button
                              parentController:nil
                                 barController:bar_
                                       profile:profile()]);
  BookmarkButton* targetButton =
      [folderController buttonWithTitleEqualTo:@"2f1b"];
  ASSERT_TRUE(targetButton);

  // Gen up some dragging data.
  const BookmarkNode* newNode = other->GetChild(2);
  [folderController setDragDataNode:newNode];
  base::scoped_nsobject<FakedDragInfo> dragInfo([[FakedDragInfo alloc] init]);
  [dragInfo setDropLocation:[targetButton top]];
  [folderController dragBookmarkData:(id<NSDraggingInfo>)dragInfo.get()];

  // Verify the model.
  const std::string expected("1b 2f:[ O3f:[ O3f1b O3f2f ] 2f1b 2f2f:[ 2f2f1b "
                             "2f2f2b 2f2f3b ] 2f3b ] 3b 4b ");
  actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(expected, actual);

  // Now drag over a folder button.
  targetButton = [folderController buttonWithTitleEqualTo:@"2f2f"];
  ASSERT_TRUE(targetButton);
  newNode = other->GetChild(2);  // Should be O4f.
  EXPECT_EQ(newNode->GetTitle(), ASCIIToUTF16("O4f"));
  [folderController setDragDataNode:newNode];
  [dragInfo setDropLocation:[targetButton center]];
  [folderController dragBookmarkData:(id<NSDraggingInfo>)dragInfo.get()];

  // Verify the model.
  const std::string expectedA("1b 2f:[ O3f:[ O3f1b O3f2f ] 2f1b 2f2f:[ "
                              "2f2f1b 2f2f2b 2f2f3b O4f:[ O4f1b O4f2f ] ] "
                              "2f3b ] 3b 4b ");
  actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(expectedA, actual);

  // Check button spacing.
  [folderController validateMenuSpacing];
}

TEST_F(BookmarkBarFolderControllerMenuTest, DragBookmarkDataToTrash) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                                 "2f3b ] 3b 4b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actual);

  const BookmarkNode* folderNode = root->GetChild(1);
  int oldFolderChildCount = folderNode->child_count();

  // Pop open a folder.
  BookmarkButton* button = [bar_ buttonWithTitleEqualTo:@"2f"];
  base::scoped_nsobject<BookmarkBarFolderControllerDragData> folderController;
  folderController.reset([[BookmarkBarFolderControllerDragData alloc]
                          initWithParentButton:button
                              parentController:nil
                                 barController:bar_
                                       profile:profile()]);

  // Drag a button to the trash.
  BookmarkButton* buttonToDelete =
      [folderController buttonWithTitleEqualTo:@"2f1b"];
  ASSERT_TRUE(buttonToDelete);
  EXPECT_TRUE([folderController canDragBookmarkButtonToTrash:buttonToDelete]);
  [folderController didDragBookmarkToTrash:buttonToDelete];

  // There should be one less button in the folder.
  int newFolderChildCount = folderNode->child_count();
  EXPECT_EQ(oldFolderChildCount - 1, newFolderChildCount);
  // Verify the model.
  const std::string expected("1b 2f:[ 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                             "2f3b ] 3b 4b ");
  actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(expected, actual);

  // Check button spacing.
  [folderController validateMenuSpacing];
}

TEST_F(BookmarkBarFolderControllerMenuTest, AddURLs) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                                 "2f3b ] 3b 4b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actual);

  // Pop open a folder.
  BookmarkButton* button = [bar_ buttonWithTitleEqualTo:@"2f"];
  [[button target] performSelector:@selector(openBookmarkFolderFromButton:)
                        withObject:button];
  BookmarkBarFolderController* folderController = [bar_ folderController];
  EXPECT_TRUE(folderController);
  NSArray* buttons = [folderController buttons];
  EXPECT_TRUE(buttons);

  // Remember how many buttons are showing.
  int oldDisplayedButtons = [buttons count];

  BookmarkButton* targetButton =
      [folderController buttonWithTitleEqualTo:@"2f1b"];
  ASSERT_TRUE(targetButton);

  NSArray* urls = [NSArray arrayWithObjects: @"http://www.a.com/",
                   @"http://www.b.com/", nil];
  NSArray* titles = [NSArray arrayWithObjects: @"SiteA", @"SiteB", nil];
  [folderController addURLs:urls withTitles:titles at:[targetButton top]];

  // There should two more buttons in the folder.
  int newDisplayedButtons = [buttons count];
  EXPECT_EQ(oldDisplayedButtons + 2, newDisplayedButtons);
  // Verify the model.
  const std::string expected("1b 2f:[ SiteA SiteB 2f1b 2f2f:[ 2f2f1b 2f2f2b "
                             "2f2f3b ] 2f3b ] 3b 4b ");
  actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(expected, actual);

  // Check button spacing.
  [folderController validateMenuSpacing];
}

TEST_F(BookmarkBarFolderControllerMenuTest, DropPositionIndicator) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b 2f2f3b ] "
                                 "2f3b ] 3b 4b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actual = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actual);

  // Pop open the folder.
  BookmarkButton* button = [bar_ buttonWithTitleEqualTo:@"2f"];
  [[button target] performSelector:@selector(openBookmarkFolderFromButton:)
                        withObject:button];
  BookmarkBarFolderController* folder = [bar_ folderController];
  EXPECT_TRUE(folder);

  // Test a series of points starting at the top of the folder.
  const CGFloat yTopOffset = 0.5 * bookmarks::kBookmarkTopVerticalPadding;
  const CGFloat yBottomOffset =
      0.5 * bookmarks::kBookmarkBottomVerticalPadding;
  BookmarkButton* targetButton = [folder buttonWithTitleEqualTo:@"2f1b"];
  ASSERT_TRUE(targetButton);
  NSPoint targetPoint = [targetButton top];
  CGFloat pos = [folder indicatorPosForDragToPoint:targetPoint];
  EXPECT_CGFLOAT_EQ(targetPoint.y + yTopOffset, pos);
  pos = [folder indicatorPosForDragToPoint:[targetButton bottom]];
  targetButton = [folder buttonWithTitleEqualTo:@"2f2f"];
  EXPECT_CGFLOAT_EQ([targetButton top].y + yTopOffset, pos);
  pos = [folder indicatorPosForDragToPoint:NSMakePoint(10,0)];
  targetButton = [folder buttonWithTitleEqualTo:@"2f3b"];
  EXPECT_CGFLOAT_EQ([targetButton bottom].y - yBottomOffset, pos);
}

TEST_F(BookmarkBarFolderControllerMenuTest, FolderTooltips) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* otherBookmarks = model->other_node();
  model->AddFolder(otherBookmarks, otherBookmarks->child_count(),
                   ASCIIToUTF16("short_name"));
  model->AddFolder(
      otherBookmarks, otherBookmarks->child_count(),
      ASCIIToUTF16("reallyReallyLongBookmarkNamereallyReallyLongBookmarkNamerea"
                   "llyReallyLongBookmarkNamereallyReallyLongBookmarkName"));
  BookmarkButton* otherButton = [bar_ otherBookmarksButton];
  ASSERT_TRUE(otherButton);

  [[otherButton target] openBookmarkFolderFromButton:otherButton];
  BookmarkBarFolderController* folder = [bar_ folderController];
  EXPECT_TRUE(folder);

  NSArray* buttons = [folder buttons];
  EXPECT_EQ(2U, [buttons count]);

  for (BookmarkButton* btn in buttons) {
    if ([[btn cell] cellSize].width >
        bookmarks::kBookmarkMenuButtonMaximumWidth) {
      EXPECT_TRUE([btn toolTip]);
    } else {
      EXPECT_FALSE([btn toolTip]);
    }
  }
}

@interface BookmarkBarControllerNoDelete : BookmarkBarController
- (IBAction)deleteBookmark:(id)sender;
@end

@implementation BookmarkBarControllerNoDelete
- (IBAction)deleteBookmark:(id)sender {
  // NOP
}
@end

class BookmarkBarFolderControllerClosingTest : public
    BookmarkBarFolderControllerMenuTest {
 public:
  void SetUp() override {
    BookmarkBarFolderControllerMenuTest::SetUp();
    ASSERT_TRUE(browser());

    bar_.reset([[BookmarkBarControllerNoDelete alloc]
        initWithBrowser:browser()
           initialWidth:NSWidth([parent_view_ frame])
               delegate:nil]);
    InstallAndToggleBar(bar_.get());
  }
};

TEST_F(BookmarkBarFolderControllerClosingTest, DeleteClosesFolder) {
  BookmarkModel* model = BookmarkModelFactory::GetForBrowserContext(profile());
  const BookmarkNode* root = model->bookmark_bar_node();
  const std::string model_string("1b 2f:[ 2f1b 2f2f:[ 2f2f1b 2f2f2b ] "
                                 "2f3b ] 3b ");
  bookmarks::test::AddNodesFromModelString(model, root, model_string);

  // Validate initial model.
  std::string actualModelString = bookmarks::test::ModelStringFromNode(root);
  EXPECT_EQ(model_string, actualModelString);

  // Open the folder menu and submenu.
  BookmarkButton* target = [bar_ buttonWithTitleEqualTo:@"2f"];
  ASSERT_TRUE(target);
  [[target target] performSelector:@selector(openBookmarkFolderFromButton:)
                              withObject:target];
  BookmarkBarFolderController* folder = [bar_ folderController];
  EXPECT_TRUE(folder);
  BookmarkButton* subTarget = [folder buttonWithTitleEqualTo:@"2f2f"];
  ASSERT_TRUE(subTarget);
  [[subTarget target] performSelector:@selector(openBookmarkFolderFromButton:)
                           withObject:subTarget];
  BookmarkBarFolderController* subFolder = [folder folderController];
  EXPECT_TRUE(subFolder);

  // Delete the folder node and verify the window closed down by looking
  // for its controller again.
  DeleteBookmark([folder parentButton], profile());
  EXPECT_FALSE([folder folderController]);
}

// TODO(jrg): draggingEntered: and draggingExited: trigger timers so
// they are hard to test.  Factor out "fire timers" into routines
// which can be overridden to fire immediately to make behavior
// confirmable.
// There is a similar problem with mouseEnteredButton: and
// mouseExitedButton:.
