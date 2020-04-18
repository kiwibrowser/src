// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_controller.h"

#include <stddef.h>

#include "base/mac/bundle_locations.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/strings/sys_string_conversions.h"
#import "chrome/browser/bookmarks/bookmark_model_factory.h"
#import "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#import "chrome/browser/profiles/profile.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_constants.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_button_cell.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_hover_state.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_view.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_folder_window.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_folder_target.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_menu_cocoa_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/l10n_util.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node_data.h"
#import "components/bookmarks/managed/managed_bookmark_service.h"
#include "ui/base/clipboard/clipboard_util_mac.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/resources/grit/ui_resources.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using bookmarks::BookmarkNodeData;
using bookmarks::kBookmarkBarMenuCornerRadius;

namespace {

// Frequency of the scrolling timer in seconds.
const NSTimeInterval kBookmarkBarFolderScrollInterval = 0.1;

// Amount to scroll by per timer fire.  We scroll rather slowly; to
// accomodate we do several at a time.
const CGFloat kBookmarkBarFolderScrollAmount =
    3 * bookmarks::kBookmarkFolderButtonHeight;

// Amount to scroll for each scroll wheel roll.
const CGFloat kBookmarkBarFolderScrollWheelAmount =
    1 * bookmarks::kBookmarkFolderButtonHeight;

// Determining adjustments to the layout of the folder menu window in response
// to resizing and scrolling relies on many visual factors. The following
// struct is used to pass around these factors to the several support
// functions involved in the adjustment calculations and application.
struct LayoutMetrics {
  // Metrics applied during the final layout adjustments to the window,
  // the main visible content view, and the menu content view (i.e. the
  // scroll view).
  CGFloat windowLeft;
  NSSize windowSize;
  // The proposed and then final scrolling adjustment made to the scrollable
  // area of the folder menu. This may be modified during the window layout
  // primarily as a result of hiding or showing the scroll arrows.
  CGFloat scrollDelta;
  NSRect windowFrame;
  NSRect visibleFrame;
  NSRect scrollerFrame;
  NSPoint scrollPoint;
  // The difference between 'could' and 'can' in these next four data members
  // is this: 'could' represents the previous condition for scrollability
  // while 'can' represents what the new condition will be for scrollability.
  BOOL couldScrollUp;
  BOOL canScrollUp;
  BOOL couldScrollDown;
  BOOL canScrollDown;
  // Determines the optimal time during folder menu layout when the contents
  // of the button scroll area should be scrolled in order to prevent
  // flickering.
  BOOL preScroll;

  // Intermediate metrics used in determining window vertical layout changes.
  CGFloat deltaWindowHeight;
  CGFloat deltaWindowY;
  CGFloat deltaVisibleHeight;
  CGFloat deltaVisibleY;
  CGFloat deltaScrollerHeight;
  CGFloat deltaScrollerY;

  // Convenience metrics used in multiple functions (carried along here in
  // order to eliminate the need to calculate in multiple places and
  // reduce the possibility of bugs).

  // Bottom of the screen's available area (excluding dock height and padding).
  CGFloat minimumY;
  // Bottom of the screen.
  CGFloat screenBottomY;
  CGFloat oldWindowY;
  CGFloat folderY;
  CGFloat folderTop;

  LayoutMetrics(CGFloat windowLeft, NSSize windowSize, CGFloat scrollDelta) :
    windowLeft(windowLeft),
    windowSize(windowSize),
    scrollDelta(scrollDelta),
    couldScrollUp(NO),
    canScrollUp(NO),
    couldScrollDown(NO),
    canScrollDown(NO),
    preScroll(NO),
    deltaWindowHeight(0.0),
    deltaWindowY(0.0),
    deltaVisibleHeight(0.0),
    deltaVisibleY(0.0),
    deltaScrollerHeight(0.0),
    deltaScrollerY(0.0),
    minimumY(0.0),
    screenBottomY(0.0),
    oldWindowY(0.0),
    folderY(0.0),
    folderTop(0.0) {}
};

NSRect GetFirstButtonFrameForHeight(CGFloat height) {
  CGFloat y = height - bookmarks::kBookmarkFolderButtonHeight -
      bookmarks::kBookmarkTopVerticalPadding;
  return NSMakeRect(0, y, bookmarks::kDefaultBookmarkWidth,
                    bookmarks::kBookmarkFolderButtonHeight);
}

}  // namespace


// Required to set the right tracking bounds for our fake menus.
@interface NSView(Private)
- (void)_updateTrackingAreas;
@end

@interface BookmarkBarFolderController ()
- (void)configureWindow;
- (void)addOrUpdateScrollTracking;
- (void)removeScrollTracking;
- (void)endScroll;
- (void)addScrollTimerWithDelta:(CGFloat)delta;

// Return the screen to which the menu should be restricted. The screen list is
// very volatile and can change with very short notice so it isn't worth
// caching. http://crbug.com/463458
- (NSScreen*)menuScreen;

// Helper function to configureWindow which performs a basic layout of
// the window subviews, in particular the menu buttons and the window width.
- (void)layOutWindowWithHeight:(CGFloat)height;

// Determine the best button width (which will be the widest button or the
// maximum allowable button width, whichever is less) and resize all buttons.
// Return the new width so that the window can be adjusted.
- (CGFloat)adjustButtonWidths;

// Returns the total menu height needed to display |buttonCount| buttons.
// Does not do any fancy tricks like trimming the height to fit on the screen.
- (int)menuHeightForButtonCount:(int)buttonCount;

// Adjust layout of the folder menu window components, showing/hiding the
// scroll up/down arrows, and resizing as necessary for a proper disaplay.
// In order to reduce window flicker, all layout changes are deferred until
// the final step of the adjustment. To accommodate this deferral, window
// height and width changes needed by callers to this function pass their
// desired window changes in |size|. When scrolling is to be performed
// any scrolling change is given by |scrollDelta|. The ultimate amount of
// scrolling may be different from |scrollDelta| in order to accommodate
// changes in the scroller view layout. These proposed window adjustments
// are passed to helper functions using a LayoutMetrics structure.
//
// This function should be called when: 1) initially setting up a folder menu
// window, 2) responding to scrolling of the contents (which may affect the
// height of the window), 3) addition or removal of bookmark items (such as
// during cut/paste/delete/drag/drop operations).
- (void)adjustWindowLeft:(CGFloat)windowLeft
                    size:(NSSize)windowSize
             scrollingBy:(CGFloat)scrollDelta;

// Support function for adjustWindowLeft:size:scrollingBy: which initializes
// the layout adjustments by gathering current folder menu window and subviews
// positions and sizes. This information is set in the |layoutMetrics|
// structure.
- (void)gatherMetrics:(LayoutMetrics*)layoutMetrics;

// Support function for adjustWindowLeft:size:scrollingBy: which calculates
// the changes which must be applied to the folder menu window and subviews
// positions and sizes. |layoutMetrics| contains the proposed window size
// and scrolling along with the other current window and subview layout
// information. The values in |layoutMetrics| are then adjusted to
// accommodate scroll arrow presentation and window growth.
- (void)adjustMetrics:(LayoutMetrics*)layoutMetrics;

// Support function for adjustMetrics: which calculates the layout changes
// required to accommodate changes in the position and scrollability
// of the top of the folder menu window.
- (void)adjustMetricsForMenuTopChanges:(LayoutMetrics*)layoutMetrics;

// Support function for adjustMetrics: which calculates the layout changes
// required to accommodate changes in the position and scrollability
// of the bottom of the folder menu window.
- (void)adjustMetricsForMenuBottomChanges:(LayoutMetrics*)layoutMetrics;

// Support function for adjustWindowLeft:size:scrollingBy: which applies
// the layout adjustments to the folder menu window and subviews.
- (void)applyMetrics:(LayoutMetrics*)layoutMetrics;

// This function is called when buttons are added or removed from the folder
// menu, and which may require a change in the layout of the folder menu
// window. Such layout changes may include horizontal placement, width,
// height, and scroller visibility changes. (This function calls through
// to -[adjustWindowLeft:size:scrollingBy:].)
// |buttonCount| should contain the updated count of menu buttons.
- (void)adjustWindowForButtonCount:(NSUInteger)buttonCount;

// A helper function which takes the desired amount to scroll, given by
// |scrollDelta|, and calculates the actual scrolling change to be applied
// taking into account the layout of the folder menu window and any
// changes in it's scrollability. (For example, when scrolling down and the
// top-most menu item is coming into view we will only scroll enough for
// that item to be completely presented, which may be less than the
// scroll amount requested.)
- (CGFloat)determineFinalScrollDelta:(CGFloat)scrollDelta;

// |point| is in the base coordinate system of the destination window;
// it comes from an id<NSDraggingInfo>. |copy| is YES if a copy is to be
// made and inserted into the new location while leaving the bookmark in
// the old location, otherwise move the bookmark by removing from its old
// location and inserting into the new location.
- (BOOL)dragBookmark:(const BookmarkNode*)sourceNode
                  to:(NSPoint)point
                copy:(BOOL)copy;

@end

@interface BookmarkButton (BookmarkBarFolderMenuHighlighting)

// Make the button's border frame always appear when |forceOn| is YES,
// otherwise only border the button when the mouse is inside the button.
- (void)forceButtonBorderToStayOnAlways:(BOOL)forceOn;

@end

@implementation BookmarkButton (BookmarkBarFolderMenuHighlighting)

- (void)forceButtonBorderToStayOnAlways:(BOOL)forceOn {
  [self setShowsBorderOnlyWhileMouseInside:!forceOn];
  [self setNeedsDisplay];
}

@end

@implementation BookmarkBarFolderController

@synthesize subFolderGrowthToRight = subFolderGrowthToRight_;

- (id)initWithParentButton:(BookmarkButton*)button
          parentController:(BookmarkBarFolderController*)parentController
             barController:(BookmarkBarController*)barController
                   profile:(Profile*)profile {
  NSString* nibPath =
      [base::mac::FrameworkBundle() pathForResource:@"BookmarkBarFolderWindow"
                                             ofType:@"nib"];
  if ((self = [super initWithWindowNibPath:nibPath owner:self])) {
    parentButton_.reset([button retain]);
    selectedIndex_ = -1;

    profile_ = profile;

    // We want the button to remain bordered as part of the menu path.
    [button forceButtonBorderToStayOnAlways:YES];

    parentController_.reset([parentController retain]);
    if (!parentController_)
      [self setSubFolderGrowthToRight:!cocoa_l10n_util::
                                          ShouldDoExperimentalRTLLayout()];
    else
      [self setSubFolderGrowthToRight:[parentController
                                        subFolderGrowthToRight]];
    barController_ = barController;  // WEAK
    buttons_.reset([[NSMutableArray alloc] init]);
    folderTarget_.reset(
        [[BookmarkFolderTarget alloc] initWithController:self profile:profile]);
    [self configureWindow];
    hoverState_.reset([[BookmarkBarFolderHoverState alloc] init]);
  }
  return self;
}

- (void)dealloc {
  [self clearInputText];

  // The button is no longer part of the menu path.
  [parentButton_ forceButtonBorderToStayOnAlways:NO];
  [parentButton_ setNeedsDisplay];

  [self removeScrollTracking];
  [self endScroll];
  [hoverState_ draggingExited];

  // Delegate pattern does not retain; make sure pointers to us are removed.
  for (BookmarkButton* button in buttons_.get()) {
    [button setDelegate:nil];
    [button setTarget:nil];
    [button setAction:nil];
  }

  // Note: we don't need to
  //   [NSObject cancelPreviousPerformRequestsWithTarget:self];
  // Because all of our performSelector: calls use withDelay: which
  // retains us.
  [super dealloc];
}

- (void)awakeFromNib {
  NSRect windowFrame = [[self window] frame];
  NSRect scrollViewFrame = [scrollView_ frame];
  // Each menu row spans the entire width of the menu.
  scrollViewFrame.size.width += NSWidth(windowFrame) - NSWidth(scrollViewFrame);
  scrollViewFrame.origin.x = 0;
  // If we leave the scrollview's vertical position and height as is, it will
  // cover the menu's rounded corners and we'll get a rectangular menu.
  scrollViewFrame.origin.y += bookmarks::kBookmarkVerticalPadding;
  scrollViewFrame.size.height -= 2 * bookmarks::kBookmarkVerticalPadding;
  [scrollView_ setFrame:scrollViewFrame];
  padding_ = 0;
  verticalScrollArrowHeight_ = NSHeight([scrollUpArrowView_ frame]);

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  NSImage* image = rb.GetNativeImageNamed(IDR_MENU_OVERFLOW_DOWN).ToNSImage();
  [[scrollUpArrowView_.subviews objectAtIndex:0] setImage:image];

  image = rb.GetNativeImageNamed(IDR_MENU_OVERFLOW_UP).ToNSImage();
  [[scrollDownArrowView_.subviews objectAtIndex:0] setImage:image];
}

// Overriden from NSWindowController to call childFolderWillShow: before showing
// the window.
- (void)showWindow:(id)sender {
  [barController_ childFolderWillShow:self];
  [super showWindow:sender];
}

- (int)buttonCount {
  return [[self buttons] count];
}

- (BookmarkButton*)parentButton {
  return parentButton_.get();
}

- (void)offsetFolderMenuWindow:(NSSize)offset {
  NSWindow* window = [self window];
  NSRect windowFrame = [window frame];
  windowFrame.origin.x -= offset.width;
  windowFrame.origin.y += offset.height;  // Yes, in the opposite direction!
  [window setFrame:windowFrame display:YES];
  [folderController_ offsetFolderMenuWindow:offset];
}

- (void)reconfigureMenu {
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  for (BookmarkButton* button in buttons_.get()) {
    [button setDelegate:nil];
    [button removeFromSuperview];
  }
  [buttons_ removeAllObjects];
  [self configureWindow];
}

#pragma mark Private Methods

- (BookmarkButtonCell*)cellForBookmarkNode:(const BookmarkNode*)child {
  NSImage* image = child ? [barController_ faviconForNode:child
                                            forADarkTheme:NO]
                         : nil;
  BookmarkContextMenuCocoaController* menuController =
      [barController_ menuController];
  BookmarkBarFolderButtonCell* cell =
      [BookmarkBarFolderButtonCell buttonCellForNode:child
                                                text:nil
                                               image:image
                                      menuController:menuController];
  [cell setTag:kMaterialMenuButtonTypeWithLimitedClickFeedback];
  return cell;
}

// Redirect to our logic shared with BookmarkBarController.
- (IBAction)openBookmarkFolderFromButton:(id)sender {
  [folderTarget_ openBookmarkFolderFromButton:sender];
}

// Create a bookmark button for the given node using frame.
//
// If |node| is NULL this is an "(empty)" button.
// Does NOT add this button to our button list.
// Returns an autoreleased button.
// Adjusts the input frame width as appropriate.
//
// TODO(jrg): combine with addNodesToButtonList: code from
// bookmark_bar_controller.mm, and generalize that to use both x and y
// offsets.
// http://crbug.com/35966
- (BookmarkButton*)makeButtonForNode:(const BookmarkNode*)node
                               frame:(NSRect)frame {
  BookmarkButtonCell* cell = [self cellForBookmarkNode:node];
  DCHECK(cell);

  // We must decide if we draw the folder arrow before we ask the cell
  // how big it needs to be.
  if (node && node->is_folder()) {
    // Warning when combining code with bookmark_bar_controller.mm:
    // this call should NOT be made for the bar buttons; only for the
    // subfolder buttons.
    [cell setDrawFolderArrow:YES];
  }

  // The "+2" is needed because, sometimes, Cocoa is off by a tad when
  // returning the value it thinks it needs.
  CGFloat desired = [cell cellSize].width + 2;
  // The width is determined from the maximum of the proposed width
  // (provided in |frame|) or the natural width of the title, then
  // limited by the abolute minimum and maximum allowable widths.
  frame.size.width =
      std::min(std::max(bookmarks::kBookmarkMenuButtonMinimumWidth,
                        std::max(frame.size.width, desired)),
               bookmarks::kBookmarkMenuButtonMaximumWidth);

  BookmarkButton* button = [[[BookmarkButton alloc] initWithFrame:frame]
                               autorelease];
  DCHECK(button);

  // Folder menu buttons have a flat background color.
  [button setBackgroundColor:
      [BookmarkBarFolderWindowContentView backgroundColor]];
  [button setCell:cell];
  [button setDelegate:self];
  if (node) {
    if (node->is_folder()) {
      [button setTarget:self];
      [button setAction:@selector(openBookmarkFolderFromButton:)];
    } else {
      // Make the button do something.
      [button setTarget:barController_];
      [button setAction:@selector(openBookmark:)];
      [button setAcceptsTrackIn:YES];
    }
    // Add a tooltip.
    if (node->is_folder()) {
      NSString* title = base::SysUTF16ToNSString(node->GetTitle());
      if ([title length] &&
          [[button cell] cellSize].width >
              bookmarks::kBookmarkMenuButtonMaximumWidth) {
        [button setToolTip:[BookmarkMenuCocoaController tooltipForNode:node]];
      }
    } else {
      [button setToolTip:[BookmarkMenuCocoaController tooltipForNode:node]];
    }
  } else {
    [button setEnabled:NO];
    [button setBordered:NO];
  }
  return button;
}

- (id)folderTarget {
  return folderTarget_.get();
}


// Our parent controller is another BookmarkBarFolderController, so
// our window is to the right or left of it.  We use a little overlap
// since it looks much more menu-like than with none.  If we would
// grow off the screen, switch growth to the other direction.  Growth
// direction sticks for folder windows which are descendents of us.
// If we have tried both directions and neither fits, degrade to a
// default.
- (CGFloat)childFolderWindowLeftForWidth:(int)windowWidth {
  // We may legitimately need to try two times (growth to right and
  // left but not in that order).  Limit us to three tries in case
  // the folder window can't fit on either side of the screen; we
  // don't want to loop forever.
  NSRect screenVisibleFrame = [[self menuScreen] visibleFrame];
  CGFloat x;
  int tries = 0;
  while (tries < 2) {
    // Try to grow right.
    if ([self subFolderGrowthToRight]) {
      tries++;
      x = NSMaxX([[parentButton_ window] frame]) -
          bookmarks::kBookmarkMenuOverlap;
      // If off the screen, switch direction.
      if ((x + windowWidth + bookmarks::kBookmarkHorizontalScreenPadding) >
          NSMaxX(screenVisibleFrame)) {
        [self setSubFolderGrowthToRight:NO];
      } else {
        return x;
      }
    }
    // Try to grow left.
    if (![self subFolderGrowthToRight]) {
      tries++;
      x = NSMinX([[parentButton_ window] frame]) +
          bookmarks::kBookmarkMenuOverlap -
          windowWidth;
      // If off the screen, switch direction.
      if (x < NSMinX(screenVisibleFrame)) {
        [self setSubFolderGrowthToRight:YES];
      } else {
        return x;
      }
    }
  }
  // Unhappy; do the best we can.
  return NSMaxX(screenVisibleFrame) - windowWidth;
}


// Compute and return the top left point of our window (screen
// coordinates).  The top left is positioned in a manner similar to
// cascading menus.  Windows may grow to either the right or left of
// their parent (if a sub-folder) so we need to know |windowWidth|.
- (NSPoint)windowTopLeftForWidth:(int)windowWidth height:(int)windowHeight {
  CGFloat kMinSqueezedMenuHeight = bookmarks::kBookmarkFolderButtonHeight * 2.0;
  NSPoint newWindowTopLeft;
  if (![parentController_ isKindOfClass:[self class]]) {
    // If we're not popping up from one of ourselves, we must be
    // popping up from the bookmark bar itself.  In this case, start
    // BELOW the parent button. Our leading edge is the button leading
    // edge; our top is bottom of button's parent view.
    BOOL isRTL = cocoa_l10n_util::ShouldDoExperimentalRTLLayout();
    NSPoint buttonAnchorPoint =
        isRTL ? NSMakePoint(NSWidth([parentButton_ frame]), 0) : NSZeroPoint;
    NSPoint buttonAnchorPointInScreen = ui::ConvertPointFromWindowToScreen(
        [parentButton_ window],
        [parentButton_ convertPoint:buttonAnchorPoint toView:nil]);
    NSPoint bookmarkBarAnchorPoint =
        isRTL ? NSMakePoint(NSWidth([[parentButton_ superview] frame]), 0)
              : NSZeroPoint;
    NSPoint bookmarkBarAnchorPointInScreen = ui::ConvertPointFromWindowToScreen(
        [parentButton_ window],
        [[parentButton_ superview] convertPoint:bookmarkBarAnchorPoint
                                         toView:nil]);
    newWindowTopLeft = NSMakePoint(
        isRTL ? buttonAnchorPointInScreen.x - windowWidth
              : buttonAnchorPointInScreen.x,
        bookmarkBarAnchorPointInScreen.y + bookmarks::kBookmarkBarMenuOffset);
    // Make sure the window is on-screen; if not, push left or right. It is
    // intentional that top level folders "push left" or "push right" slightly
    // different than subfolders.
    NSRect screenVisibleFrame = [[self menuScreen] visibleFrame];
    // Test if window goes off-screen on the right side.
    CGFloat spillOff =
        newWindowTopLeft.x + windowWidth - NSMaxX(screenVisibleFrame);
    if (spillOff > 0.0) {
      newWindowTopLeft.x = std::max(newWindowTopLeft.x - spillOff,
                                    NSMinX(screenVisibleFrame));
    } else if (newWindowTopLeft.x < NSMinX(screenVisibleFrame)) {
      // For left side.
      newWindowTopLeft.x = NSMinX(screenVisibleFrame);
    }
    // The menu looks bad when it is squeezed up against the bottom of the
    // screen and ends up being only a few pixels tall. If it meets the
    // threshold for this case, instead show the menu above the button.
    CGFloat availableVerticalSpace = newWindowTopLeft.y -
        (NSMinY(screenVisibleFrame) + bookmarks::kScrollWindowVerticalMargin);
    if ((availableVerticalSpace < kMinSqueezedMenuHeight) &&
        (windowHeight > availableVerticalSpace)) {
      newWindowTopLeft.y = std::min(
          newWindowTopLeft.y + windowHeight + NSHeight([parentButton_ frame]),
          NSMaxY(screenVisibleFrame));
    }
  } else {
    // Parent is a folder: expose as much as we can vertically; grow right/left.
    newWindowTopLeft.x = [self childFolderWindowLeftForWidth:windowWidth];
    NSPoint topOfWindow =
        NSMakePoint(0, NSMaxY([parentButton_ frame]) +
            bookmarks::kBookmarkTopVerticalPadding);
    topOfWindow = ui::ConvertPointFromWindowToScreen(
        [parentButton_ window],
        [[parentButton_ superview] convertPoint:topOfWindow toView:nil]);
    newWindowTopLeft.y = topOfWindow.y;
  }
  return newWindowTopLeft;
}

// Set our window level to the right spot so we're above the menubar, dock, etc.
// Factored out so we can override/noop in a unit test. This uses
// NSStatusWindowLevel specifically so that this menu will be at the same level
// as the "poof" animation window created by NSShowAnimationEffect(). This is
// lower than the level of other popup menus (which are at
// NSPopUpMenuWindowLevel) so this menu *could* get overdrawn by other menus,
// but that shouldn't happen in practice.
- (void)configureWindowLevel {
  [[self window] setLevel:NSStatusWindowLevel];
}

- (int)menuHeightForButtonCount:(int)buttonCount {
  // This does not take into account any padding which may be required at the
  // top and/or bottom of the window.
  return (buttonCount * bookmarks::kBookmarkFolderButtonHeight) +
      bookmarks::kBookmarkTopVerticalPadding +
      bookmarks::kBookmarkBottomVerticalPadding;
}

- (void)adjustWindowLeft:(CGFloat)windowLeft
                    size:(NSSize)windowSize
             scrollingBy:(CGFloat)scrollDelta {
  // Callers of this function should make adjustments to the vertical
  // attributes of the folder view only (height, scroll position).
  // This function will then make appropriate layout adjustments in order
  // to accommodate screen/dock margins, scroll-up and scroll-down arrow
  // presentation, etc.
  // The 4 views whose vertical height and origins may be adjusted
  // by this function are:
  //  1) window, 2) visible content view, 3) scroller view, 4) folder view.

  LayoutMetrics layoutMetrics(windowLeft, windowSize, scrollDelta);
  [self gatherMetrics:&layoutMetrics];
  [self adjustMetrics:&layoutMetrics];
  [self applyMetrics:&layoutMetrics];
}

- (void)gatherMetrics:(LayoutMetrics*)layoutMetrics {
  LayoutMetrics& metrics(*layoutMetrics);
  NSWindow* window = [self window];
  metrics.windowFrame = [window frame];
  metrics.visibleFrame = [visibleView_ frame];
  metrics.scrollerFrame = [scrollView_ frame];
  metrics.scrollPoint = [scrollView_ documentVisibleRect].origin;
  metrics.scrollPoint.y -= metrics.scrollDelta;
  metrics.couldScrollUp = ![scrollUpArrowView_ isHidden];
  metrics.couldScrollDown = ![scrollDownArrowView_ isHidden];

  metrics.deltaWindowHeight = 0.0;
  metrics.deltaWindowY = 0.0;
  metrics.deltaVisibleHeight = 0.0;
  metrics.deltaVisibleY = 0.0;
  metrics.deltaScrollerHeight = 0.0;
  metrics.deltaScrollerY = 0.0;

  metrics.minimumY = NSMinY([[self menuScreen] visibleFrame]) +
                     bookmarks::kScrollWindowVerticalMargin;
  metrics.screenBottomY = NSMinY([[self menuScreen] frame]);
  metrics.oldWindowY = NSMinY(metrics.windowFrame);
  metrics.folderY =
      metrics.scrollerFrame.origin.y + metrics.visibleFrame.origin.y +
      metrics.oldWindowY - metrics.scrollPoint.y;
  metrics.folderTop = metrics.folderY + NSHeight([folderView_ frame]);
}

- (void)adjustMetrics:(LayoutMetrics*)layoutMetrics {
  LayoutMetrics& metrics(*layoutMetrics);
  CGFloat effectiveFolderY = metrics.folderY;
  if (!metrics.couldScrollUp && !metrics.couldScrollDown)
    effectiveFolderY -= metrics.windowSize.height;
  metrics.canScrollUp = effectiveFolderY < metrics.minimumY;
  CGFloat maximumY =
      NSMaxY([[self menuScreen] visibleFrame]) -
          bookmarks::kScrollWindowVerticalMargin;
  metrics.canScrollDown = metrics.folderTop > maximumY;

  // Accommodate changes in the bottom of the menu.
  [self adjustMetricsForMenuBottomChanges:layoutMetrics];

  // Accommodate changes in the top of the menu.
  [self adjustMetricsForMenuTopChanges:layoutMetrics];

  metrics.scrollerFrame.origin.y += metrics.deltaScrollerY;
  metrics.scrollerFrame.size.height += metrics.deltaScrollerHeight;
  metrics.visibleFrame.origin.y += metrics.deltaVisibleY;
  metrics.visibleFrame.size.height += metrics.deltaVisibleHeight;
  metrics.preScroll = metrics.canScrollUp && !metrics.couldScrollUp &&
      metrics.scrollDelta == 0.0 && metrics.deltaWindowHeight >= 0.0;
  metrics.windowFrame.origin.y += metrics.deltaWindowY;
  metrics.windowFrame.origin.x = metrics.windowLeft;
  metrics.windowFrame.size.height += metrics.deltaWindowHeight;
  metrics.windowFrame.size.width = metrics.windowSize.width;
}

- (void)adjustMetricsForMenuBottomChanges:(LayoutMetrics*)layoutMetrics {
  LayoutMetrics& metrics(*layoutMetrics);
  if (metrics.canScrollUp) {
    if (!metrics.couldScrollUp) {
      // Couldn't -> Can
      metrics.deltaWindowY = metrics.screenBottomY - metrics.oldWindowY;
      metrics.deltaWindowHeight = -metrics.deltaWindowY;
      metrics.deltaVisibleY = metrics.minimumY - metrics.screenBottomY;
      metrics.deltaVisibleHeight = -metrics.deltaVisibleY;
      metrics.deltaScrollerY = verticalScrollArrowHeight_;
      metrics.deltaScrollerHeight = -metrics.deltaScrollerY;
      // Adjust the scroll delta if we've grown the window and it is
      // now scroll-up-able, but don't adjust it if we've
      // scrolled down and it wasn't scroll-up-able but now is.
      if (metrics.canScrollDown == metrics.couldScrollDown) {
        CGFloat deltaScroll = metrics.deltaWindowY - metrics.screenBottomY +
                              metrics.deltaScrollerY + metrics.deltaVisibleY;
        metrics.scrollPoint.y += deltaScroll + metrics.windowSize.height;
      }
    } else if (!metrics.canScrollDown && metrics.windowSize.height > 0.0) {
      metrics.scrollPoint.y += metrics.windowSize.height;
    }
  } else {
    if (metrics.couldScrollUp) {
      // Could -> Can't
      metrics.deltaWindowY = metrics.folderY - metrics.oldWindowY;
      metrics.deltaWindowHeight = -metrics.deltaWindowY;
      metrics.deltaVisibleY = -metrics.visibleFrame.origin.y;
      metrics.deltaVisibleHeight = -metrics.deltaVisibleY;
      metrics.deltaScrollerY = -verticalScrollArrowHeight_;
      metrics.deltaScrollerHeight = -metrics.deltaScrollerY;
      // We are no longer scroll-up-able so the scroll point drops to zero.
      metrics.scrollPoint.y = 0.0;
    } else {
      // Couldn't -> Can't
      // Check for menu height change by looking at the relative tops of the
      // menu folder and the window folder, which previously would have been
      // the same.
      metrics.deltaWindowY = NSMaxY(metrics.windowFrame) - metrics.folderTop;
      metrics.deltaWindowHeight = -metrics.deltaWindowY;
    }
  }
}

- (void)adjustMetricsForMenuTopChanges:(LayoutMetrics*)layoutMetrics {
  LayoutMetrics& metrics(*layoutMetrics);
  if (metrics.canScrollDown == metrics.couldScrollDown) {
    if (!metrics.canScrollDown) {
      // Not scroll-down-able but the menu top has changed.
      metrics.deltaWindowHeight += metrics.scrollDelta;
    }
  } else {
    if (metrics.canScrollDown) {
      // Couldn't -> Can
      const CGFloat maximumY = NSMaxY([[self menuScreen] visibleFrame]);
      metrics.deltaWindowHeight += (maximumY - NSMaxY(metrics.windowFrame));
      metrics.deltaVisibleHeight -= bookmarks::kScrollWindowVerticalMargin;
      metrics.deltaScrollerHeight -= verticalScrollArrowHeight_;
    } else {
      // Could -> Can't
      metrics.deltaWindowHeight -= bookmarks::kScrollWindowVerticalMargin;
      metrics.deltaVisibleHeight += bookmarks::kScrollWindowVerticalMargin;
      metrics.deltaScrollerHeight += verticalScrollArrowHeight_;
    }
  }
}

- (void)applyMetrics:(LayoutMetrics*)layoutMetrics {
  LayoutMetrics& metrics(*layoutMetrics);
  // Hide or show the scroll arrows.
  if (metrics.canScrollUp != metrics.couldScrollUp)
    [scrollUpArrowView_ setHidden:metrics.couldScrollUp];
  if (metrics.canScrollDown != metrics.couldScrollDown)
    [scrollDownArrowView_ setHidden:metrics.couldScrollDown];

  // Adjust the geometry. The order is important because of sizer dependencies.
  [scrollView_ setFrame:metrics.scrollerFrame];
  [visibleView_ setFrame:metrics.visibleFrame];
  // This little bit of trickery handles the one special case where
  // the window is now scroll-up-able _and_ going to be resized -- scroll
  // first in order to prevent flashing.
  if (metrics.preScroll)
    [[scrollView_ documentView] scrollPoint:metrics.scrollPoint];

  [[self window] setFrame:metrics.windowFrame display:YES];

  // In all other cases we defer scrolling until the window has been resized
  // in order to prevent flashing.
  if (!metrics.preScroll)
    [[scrollView_ documentView] scrollPoint:metrics.scrollPoint];

  // TODO(maf) find a non-SPI way to do this.
  // Hack. This is the only way I've found to get the tracking area cache
  // to update properly during a mouse tracking loop.
  // Without this, the item tracking-areas are wrong when using a scrollable
  // menu with the mouse held down.
  NSView *contentView = [[self window] contentView] ;
  if ([contentView respondsToSelector:@selector(_updateTrackingAreas)])
    [contentView _updateTrackingAreas];


  if (metrics.canScrollUp != metrics.couldScrollUp ||
      metrics.canScrollDown != metrics.couldScrollDown ||
      metrics.scrollDelta != 0.0) {
    if (metrics.canScrollUp || metrics.canScrollDown)
      [self addOrUpdateScrollTracking];
    else
      [self removeScrollTracking];
  }
}

- (void)adjustWindowForButtonCount:(NSUInteger)buttonCount {
  NSRect folderFrame = [folderView_ frame];
  CGFloat newMenuHeight =
      (CGFloat)[self menuHeightForButtonCount:buttonCount];
  CGFloat deltaMenuHeight = newMenuHeight - NSHeight(folderFrame);
  // If the height has changed then also change the origin, and adjust the
  // scroll (if scrolling).
  if ([self canScrollUp]) {
    NSPoint scrollPoint = [scrollView_ documentVisibleRect].origin;
    scrollPoint.y += deltaMenuHeight;
    [[scrollView_ documentView] scrollPoint:scrollPoint];
  }
  folderFrame.size.height += deltaMenuHeight;
  [folderView_ setFrameSize:folderFrame.size];
  CGFloat windowWidth = [self adjustButtonWidths] + padding_;
  NSPoint newWindowTopLeft = [self windowTopLeftForWidth:windowWidth
                                                  height:deltaMenuHeight];
  CGFloat left = newWindowTopLeft.x;
  NSSize newSize = NSMakeSize(windowWidth, deltaMenuHeight);
  [self adjustWindowLeft:left size:newSize scrollingBy:0.0];
}

// Determine window size and position.
// Create buttons for all our nodes.
// TODO(jrg): break up into more and smaller routines for easier unit testing.
- (void)configureWindow {
  const BookmarkNode* node = [parentButton_ bookmarkNode];
  DCHECK(node);
  int startingIndex = [[parentButton_ cell] startingChildIndex];
  DCHECK_LE(startingIndex, node->child_count());
  // Must have at least 1 button (for "empty")
  int buttons = std::max(node->child_count() - startingIndex, 1);

  // Prelim height of the window.  We'll trim later as needed.
  int height = [self menuHeightForButtonCount:buttons];
  // We'll need this soon...
  [self window];

  // TODO(jrg): combine with frame code in bookmark_bar_controller.mm
  // http://crbug.com/35966
  NSRect buttonsOuterFrame = GetFirstButtonFrameForHeight(height);

  // TODO(jrg): combine with addNodesToButtonList: code from
  // bookmark_bar_controller.mm (but use y offset)
  // http://crbug.com/35966
  if (node->empty()) {
    // If no children we are the empty button.
    BookmarkButton* button = [self makeButtonForNode:nil
                                               frame:buttonsOuterFrame];
    [buttons_ addObject:button];
    [folderView_ addSubview:button];
  } else {
    for (int i = startingIndex; i < node->child_count(); ++i) {
      const BookmarkNode* child = node->GetChild(i);
      BookmarkButton* button = [self makeButtonForNode:child
                                                 frame:buttonsOuterFrame];
      [buttons_ addObject:button];
      [folderView_ addSubview:button];
      buttonsOuterFrame.origin.y -= bookmarks::kBookmarkFolderButtonHeight;
    }
  }
  [self layOutWindowWithHeight:height];
}

- (void)layOutWindowWithHeight:(CGFloat)height {
  // Lay out the window by adjusting all button widths to be consistent, then
  // base the window width on this ideal button width.
  CGFloat buttonWidth = [self adjustButtonWidths];
  CGFloat windowWidth = buttonWidth + padding_;
  NSPoint newWindowTopLeft = [self windowTopLeftForWidth:windowWidth
                                                  height:height];

  // Make sure as much of a submenu is exposed (which otherwise would be a
  // problem if the parent button is close to the bottom of the screen).
  if ([parentController_ isKindOfClass:[self class]]) {
    CGFloat minimumY = NSMinY([[self menuScreen] visibleFrame]) +
                       bookmarks::kScrollWindowVerticalMargin +
                       height;
    newWindowTopLeft.y = MAX(newWindowTopLeft.y, minimumY);
  }

  NSWindow* window = [self window];
  NSRect windowFrame = NSMakeRect(newWindowTopLeft.x,
                                  newWindowTopLeft.y - height,
                                  windowWidth, height);
  [window setFrame:windowFrame display:NO];

  NSRect folderFrame = NSMakeRect(0, 0, windowWidth, height);
  // Each menu button is 24pt tall but its highlight is only 20pt - effectively
  // each menu button includes 4pts of vertical padding. This wasn't a problem
  // pre-Material, but now that the |scrollView_| is shorter we have an extra
  // 4pt of padding pushing the topmost item beyond the top of the
  // |scrollView_|. Scoot the |folderView_| down by this padding to avoid
  // clipping the topmost item.
  folderFrame.origin.y -= bookmarks::kBookmarkVerticalPadding;
  [folderView_ setFrame:folderFrame];

  // For some reason, when opening a "large" bookmark folder (containing 12 or
  // more items) using the keyboard, the scroll view seems to want to be
  // offset by default: [ http://crbug.com/101099 ].  Explicitly reseting the
  // scroll position here is a bit hacky, but it does seem to work.
  [[scrollView_ contentView] scrollToPoint:NSZeroPoint];

  NSSize newSize = NSMakeSize(windowWidth, 0.0);
  [self adjustWindowLeft:newWindowTopLeft.x size:newSize scrollingBy:0.0];
  [self configureWindowLevel];

  [window display];
}

// TODO(mrossetti): See if the following can be moved into view's viewWillDraw:.
- (CGFloat)adjustButtonWidths {
  CGFloat width = bookmarks::kBookmarkMenuButtonMinimumWidth;
  // Use the cell's size as the base for determining the desired width of the
  // button rather than the button's current width. -[cell cellSize] always
  // returns the 'optimum' size of the cell based on the cell's contents even
  // if it's less than the current button size. Relying on the button size
  // would result in buttons that could only get wider but we want to handle
  // the case where the widest button gets removed from a folder menu.
  for (BookmarkButton* button in buttons_.get())
    width = std::max(width, [[button cell] cellSize].width);
  width = std::min(width, bookmarks::kBookmarkMenuButtonMaximumWidth);
  // Things look and feel more menu-like if all the buttons are the
  // full width of the window, especially if there are submenus.
  for (BookmarkButton* button in buttons_.get()) {
    NSRect buttonFrame = [button frame];
    buttonFrame.size.width = width;
    [button setFrame:buttonFrame];
  }
  return width;
}

// Start a "scroll up" timer.
- (void)beginScrollWindowUp {
  [self addScrollTimerWithDelta:kBookmarkBarFolderScrollAmount];
}

// Start a "scroll down" timer.
- (void)beginScrollWindowDown {
  [self addScrollTimerWithDelta:-kBookmarkBarFolderScrollAmount];
}

// End a scrolling timer.  Can be called excessively with no harm.
- (void)endScroll {
  if (scrollTimer_) {
    [scrollTimer_ invalidate];
    scrollTimer_ = nil;
    verticalScrollDelta_ = 0;
  }
}

- (int)indexOfButton:(BookmarkButton*)button {
  if (button == nil)
    return -1;
  NSInteger index = [buttons_ indexOfObject:button];
  return (index == NSNotFound) ? -1 : index;
}

- (BookmarkButton*)buttonAtIndex:(int)which {
  if (which < 0 || which >= [self buttonCount])
    return nil;
  return [buttons_ objectAtIndex:which];
}

// Private, called by performOneScroll only.
// If the button at index contains the mouse it will select it and return YES.
// Otherwise returns NO.
- (BOOL)selectButtonIfHoveredAtIndex:(int)index {
  BookmarkButton* button = [self buttonAtIndex:index];
  if ([[button cell] isMouseReallyInside]) {
    buttonThatMouseIsIn_ = button;
    [self setSelectedButtonByIndex:index];
    return YES;
  }
  return NO;
}

// Perform a single scroll of the specified amount. If |updateMouseSection| is
// YES, and the mouse cursor is over the currently selected item, then change
// the selection to the item under the mouse cursor after the scroll.
- (void)performOneScroll:(CGFloat)delta
    updateMouseSelection:(BOOL)updateMouseSelection {
  if (delta == 0.0)
    return;
  CGFloat finalDelta = [self determineFinalScrollDelta:delta];
  if (finalDelta == 0.0)
    return;
  int index = [self indexOfButton:buttonThatMouseIsIn_];
  // Check for a current mouse-initiated selection.
  BOOL maintainHoverSelection =
      (updateMouseSelection &&
      buttonThatMouseIsIn_ &&
      [[buttonThatMouseIsIn_ cell] isMouseReallyInside] &&
      selectedIndex_ != -1 &&
      index == selectedIndex_);
  NSRect windowFrame = [[self window] frame];
  NSSize newSize = NSMakeSize(NSWidth(windowFrame), 0.0);
  isScrolling_ = YES;
  [self adjustWindowLeft:windowFrame.origin.x
                    size:newSize
             scrollingBy:finalDelta];
  // We have now scrolled.
  if (!maintainHoverSelection)
    return;
  // Is mouse still in the same hovered button?
  if ([[buttonThatMouseIsIn_ cell] isMouseReallyInside])
    return;
  // The finalDelta scroll direction will tell us us whether to search up or
  // down the buttons array for the newly hovered button.
  if (finalDelta < 0.0) { // Scrolled up, so search backwards for new hover.
    index--;
    while (index >= 0) {
      if ([self selectButtonIfHoveredAtIndex:index])
        return;
      index--;
    }
  } else { // Scrolled down, so search forward for new hovered button.
    index++;
    int btnMax = [self buttonCount];
    while (index < btnMax) {
      if ([self selectButtonIfHoveredAtIndex:index])
        return;
      index++;
    }
  }
}

- (CGFloat)determineFinalScrollDelta:(CGFloat)delta {
  if ((delta > 0.0 && ![scrollUpArrowView_ isHidden]) ||
      (delta < 0.0 && ![scrollDownArrowView_ isHidden])) {
    NSWindow* window = [self window];
    NSRect windowFrame = [window frame];
    NSRect screenVisibleFrame = [[self menuScreen] visibleFrame];
    NSPoint scrollPosition = [scrollView_ documentVisibleRect].origin;
    CGFloat scrollY = scrollPosition.y;
    NSRect scrollerFrame = [scrollView_ frame];
    CGFloat scrollerY = NSMinY(scrollerFrame);
    NSRect visibleFrame = [visibleView_ frame];
    CGFloat visibleY = NSMinY(visibleFrame);
    CGFloat windowY = NSMinY(windowFrame);
    CGFloat offset = scrollerY + visibleY + windowY;

    if (delta > 0.0) {
      // Scrolling up.
      CGFloat minimumY = NSMinY(screenVisibleFrame) +
                         bookmarks::kScrollWindowVerticalMargin;
      CGFloat maxUpDelta = scrollY - offset + minimumY;
      delta = MIN(delta, maxUpDelta);
    } else {
      // Scrolling down.
      CGFloat topOfScreen = NSMaxY(screenVisibleFrame);
      NSRect folderFrame = [folderView_ frame];
      CGFloat folderHeight = NSHeight(folderFrame);
      CGFloat folderTop = folderHeight - scrollY + offset;
      CGFloat maxDownDelta =
          topOfScreen - folderTop - bookmarks::kScrollWindowVerticalMargin;
      delta = MAX(delta, maxDownDelta);
    }
  } else {
    delta = 0.0;
  }
  return delta;
}

// Perform a scroll of the window on the screen.
// Called by a timer when scrolling.
- (void)performScroll:(NSTimer*)timer {
  DCHECK(verticalScrollDelta_);
  // Since this scroll was initiated by hovering over an arrow, there should be
  // no mouse selection to update.
  [self performOneScroll:verticalScrollDelta_ updateMouseSelection:NO];
}


// Add a timer to fire at a regular interval which scrolls the
// window vertically |delta|.
- (void)addScrollTimerWithDelta:(CGFloat)delta {
  if (scrollTimer_ && verticalScrollDelta_ == delta)
    return;
  [self endScroll];
  verticalScrollDelta_ = delta;
  scrollTimer_ = [NSTimer timerWithTimeInterval:kBookmarkBarFolderScrollInterval
                                         target:self
                                       selector:@selector(performScroll:)
                                       userInfo:nil
                                        repeats:YES];

  [[NSRunLoop mainRunLoop] addTimer:scrollTimer_ forMode:NSRunLoopCommonModes];
}

- (NSScreen*)menuScreen {
  // Return the parent button's screen for use as the screen upon which all
  // display happens. This loop over all screens is not equivalent to
  // |[[button window] screen]|. BookmarkButtons are commonly positioned near
  // the edge of their windows (both in the bookmark bar and in other bookmark
  // menus), and |[[button window] screen]| would return the screen that the
  // majority of their window was on even if the parent button were clearly
  // contained within a different screen.
  NSButton* button = parentButton_.get();
  NSRect parentButtonGlobalFrame =
      [button convertRect:[button bounds] toView:nil];
  parentButtonGlobalFrame =
      [[button window] convertRectToScreen:parentButtonGlobalFrame];
  for (NSScreen* screen in [NSScreen screens]) {
    if (NSIntersectsRect([screen frame], parentButtonGlobalFrame))
      return screen;
  }

  // The parent button is offscreen. The ideal thing to do would be to calculate
  // the "closest" screen, the screen which has an edge parallel to, and the
  // least distance from, one of the edges of the button. However, popping a
  // subfolder from an offscreen button is an unrealistic edge case and so this
  // ideal remains unrealized. Cheat instead; this code is wrong but a lot
  // simpler.
  return [[button window] screen];
}

// Called as a result of our tracking area.  Warning: on the main
// screen (of a single-screened machine), the minimum mouse y value is
// 1, not 0.  Also, we do not get events when the mouse is above the
// menubar (to be fixed by setting the proper window level; see
// initializer).
// Note [theEvent window] may not be our window, as we also get these messages
// forwarded from BookmarkButton's mouse tracking loop.
- (void)mouseMovedOrDragged:(NSEvent*)theEvent {
  NSPoint eventScreenLocation = ui::ConvertPointFromWindowToScreen(
      [theEvent window], [theEvent locationInWindow]);

  // Base hot spot calculations on the positions of the scroll arrow views.
  NSRect testRect = [scrollDownArrowView_ frame];
  NSPoint testPoint = [visibleView_ convertPoint:testRect.origin
                                                  toView:nil];
  testPoint = ui::ConvertPointFromWindowToScreen([self window], testPoint);
  CGFloat closeToTopOfScreen = testPoint.y;

  testRect = [scrollUpArrowView_ frame];
  testPoint = [visibleView_ convertPoint:testRect.origin toView:nil];
  testPoint = ui::ConvertPointFromWindowToScreen([self window], testPoint);
  CGFloat closeToBottomOfScreen = testPoint.y + testRect.size.height;
  if (eventScreenLocation.y <= closeToBottomOfScreen &&
      ![scrollUpArrowView_ isHidden]) {
    [self beginScrollWindowUp];
  } else if (eventScreenLocation.y > closeToTopOfScreen &&
      ![scrollDownArrowView_ isHidden]) {
    [self beginScrollWindowDown];
  } else {
    [self endScroll];
  }
}

- (void)mouseMoved:(NSEvent*)theEvent {
  [self mouseMovedOrDragged:theEvent];
}

- (void)mouseDragged:(NSEvent*)theEvent {
  [self mouseMovedOrDragged:theEvent];
}

- (void)mouseExited:(NSEvent*)theEvent {
  [self endScroll];
}

// Add a tracking area so we know when the mouse is pinned to the top
// or bottom of the screen.  If that happens, and if the mouse
// position overlaps the window, scroll it.
- (void)addOrUpdateScrollTracking {
  [self removeScrollTracking];
  NSView* view = [[self window] contentView];
  scrollTrackingArea_.reset([[CrTrackingArea alloc]
                              initWithRect:[view bounds]
                                   options:(NSTrackingMouseMoved |
                                            NSTrackingMouseEnteredAndExited |
                                            NSTrackingActiveAlways |
                                            NSTrackingEnabledDuringMouseDrag
                                            )
                                     owner:self
                                  userInfo:nil]);
  [view addTrackingArea:scrollTrackingArea_.get()];
}

// Remove the tracking area associated with scrolling.
- (void)removeScrollTracking {
  if (scrollTrackingArea_.get()) {
    [[[self window] contentView] removeTrackingArea:scrollTrackingArea_.get()];
    [scrollTrackingArea_.get() clearOwner];
  }
  scrollTrackingArea_.reset();
}

// Close the old hover-open bookmark folder, and open a new one.  We
// do both in one step to allow for a delay in closing the old one.
// See comments above kDragHoverCloseDelay (bookmark_bar_controller.h)
// for more details.
- (void)openBookmarkFolderFromButtonAndCloseOldOne:(id)sender {
  // Ignore if sender button is in a window that's just been hidden - that
  // would leave us with an orphaned menu. BUG 69002
  if ([[sender window] isVisible] != YES)
    return;
  // If an old submenu exists, close it immediately.
  [self closeBookmarkFolder:sender];

  // Open a new one if meaningful.
  if ([sender isFolder])
    [folderTarget_ openBookmarkFolderFromButton:sender];
}

- (NSArray*)buttons {
  return buttons_.get();
}

- (void)close {
  [folderController_ close];
  [super close];
}

- (void)scrollWheel:(NSEvent *)theEvent {
  if (![scrollUpArrowView_ isHidden] || ![scrollDownArrowView_ isHidden]) {
    // We go negative since an NSScrollView has a flipped coordinate frame.
    CGFloat amt = kBookmarkBarFolderScrollWheelAmount * -[theEvent deltaY];
    // Make sure that the selection stays under the mouse for scroll wheel
    // scrolls.
    [self performOneScroll:amt updateMouseSelection:YES];
  }
}

#pragma mark Drag & Drop

// Find something like std::is_between<T>?  I can't believe one doesn't exist.
// http://crbug.com/35966
static BOOL ValueInRangeInclusive(CGFloat low, CGFloat value, CGFloat high) {
  return ((value >= low) && (value <= high));
}

// Return the proposed drop target for a hover open button, or nil if none.
//
// TODO(jrg): this is just like the version in
// bookmark_bar_controller.mm, but vertical instead of horizontal.
// Generalize to be axis independent then share code.
// http://crbug.com/35966
- (BookmarkButton*)buttonForDroppingOnAtPoint:(NSPoint)point {
  NSPoint localPoint = [folderView_ convertPoint:point fromView:nil];
  for (BookmarkButton* button in buttons_.get()) {
    // No early break -- makes no assumption about button ordering.

    // Intentionally NOT using NSPointInRect() so that scrolling into
    // a submenu doesn't cause it to be closed.
    if (ValueInRangeInclusive(NSMinY([button frame]),
                              localPoint.y,
                              NSMaxY([button frame]))) {

      // Over a button but let's be a little more specific
      // (e.g. over the middle half).
      NSRect frame = [button frame];
      NSRect middleHalfOfButton = NSInsetRect(frame, 0, frame.size.height / 4);
      if (ValueInRangeInclusive(NSMinY(middleHalfOfButton),
                                localPoint.y,
                                NSMaxY(middleHalfOfButton))) {
        // It makes no sense to drop on a non-folder; there is no hover.
        if (![button isFolder])
          return nil;
        // Got it!
        return button;
      } else {
        // Over a button but not over the middle half.
        return nil;
      }
    }
  }
  // Not hovering over a button.
  return nil;
}

// TODO(jrg): again we have code dup, sort of, with
// bookmark_bar_controller.mm, but the axis is changed.  One minor
// difference is accomodation for the "empty" button (which may not
// exist in the future).
// http://crbug.com/35966
- (int)indexForDragToPoint:(NSPoint)point {
  // Identify which buttons we are between.  For now, assume a button
  // location is at the center point of its view, and that an exact
  // match means "place before".
  // TODO(jrg): revisit position info based on UI team feedback.
  // dropLocation is in bar local coordinates.
  // http://crbug.com/36276
  NSPoint dropLocation =
      [folderView_ convertPoint:point
                       fromView:[[self window] contentView]];
  BookmarkButton* buttonToTheTopOfDraggedButton = nil;
  // Buttons are laid out in this array from top to bottom (screen
  // wise), which means "biggest y" --> "smallest y".
  for (BookmarkButton* button in buttons_.get()) {
    CGFloat midpoint = NSMidY([button frame]);
    if (dropLocation.y > midpoint) {
      break;
    }
    buttonToTheTopOfDraggedButton = button;
  }

  // TODO(jrg): On Windows, dropping onto (empty) highlights the
  // entire drop location and does not use an insertion point.
  // http://crbug.com/35967
  if (!buttonToTheTopOfDraggedButton) {
    // We are at the very top (we broke out of the loop on the first try).
    return 0;
  }
  if ([buttonToTheTopOfDraggedButton isEmpty]) {
    // There is a button but it's an empty placeholder.
    // Default to inserting on top of it.
    return 0;
  }
  const BookmarkNode* beforeNode = [buttonToTheTopOfDraggedButton
                                       bookmarkNode];
  DCHECK(beforeNode);
  // Be careful if the number of buttons != number of nodes.
  return ((beforeNode->parent()->GetIndexOf(beforeNode) + 1) -
          [[parentButton_ cell] startingChildIndex]);
}

// TODO(jrg): Yet more code dup.
// http://crbug.com/35966
- (BOOL)dragBookmark:(const BookmarkNode*)sourceNode
                  to:(NSPoint)point
                copy:(BOOL)copy {
  DCHECK(sourceNode);

  // Drop destination.
  const BookmarkNode* destParent = NULL;
  int destIndex = 0;

  // First check if we're dropping on a button.  If we have one, and
  // it's a folder, drop in it.
  BookmarkButton* button = [self buttonForDroppingOnAtPoint:point];
  if ([button isFolder]) {
    destParent = [button bookmarkNode];
    // Drop it at the end.
    destIndex = [button bookmarkNode]->child_count();
  } else {
    // Else we're dropping somewhere in the folder, so find the right spot.
    destParent = [parentButton_ bookmarkNode];
    destIndex = [self indexForDragToPoint:point];
    // Be careful if the number of buttons != number of nodes.
    destIndex += [[parentButton_ cell] startingChildIndex];
  }

  bookmarks::ManagedBookmarkService* managed =
      ManagedBookmarkServiceFactory::GetForProfile(profile_);
  if (!managed->CanBeEditedByUser(destParent))
    return NO;
  if (!managed->CanBeEditedByUser(sourceNode))
    copy = YES;

  // Prevent cycles.
  BOOL wasCopiedOrMoved = NO;
  if (!destParent->HasAncestor(sourceNode)) {
    if (copy)
      [self bookmarkModel]->Copy(sourceNode, destParent, destIndex);
    else
      [self bookmarkModel]->Move(sourceNode, destParent, destIndex);
    wasCopiedOrMoved = YES;
    // Movement of a node triggers observers (like us) to rebuild the
    // bar so we don't have to do so explicitly.
  }

  return wasCopiedOrMoved;
}

// TODO(maf): Implement live drag & drop animation using this hook.
- (void)setDropInsertionPos:(CGFloat)where {
}

// TODO(maf): Implement live drag & drop animation using this hook.
- (void)clearDropInsertionPos {
}

#pragma mark NSWindowDelegate Functions

- (void)windowWillClose:(NSNotification*)notification {
  // Also done by the dealloc method, but also doing it here is quicker and
  // more reliable.
  [parentButton_ forceButtonBorderToStayOnAlways:NO];

  // If a "hover open" is pending when the bookmark bar folder is
  // closed, be sure it gets cancelled.
  [NSObject cancelPreviousPerformRequestsWithTarget:self];

  [self endScroll];  // Just in case we were scrolling.
  [barController_ childFolderWillClose:self];
  [self closeBookmarkFolder:self];
  [self autorelease];
}

#pragma mark BookmarkButtonDelegate Protocol

- (NSPasteboardItem*)pasteboardItemForDragOfButton:(BookmarkButton*)button {
  return [[self folderTarget] pasteboardItemForDragOfButton:button];
}

- (void)willBeginPasteboardDrag {
  // Close our folder menu and submenus since we know we're going to be dragged.
  [self closeBookmarkFolder:self];
}

// Called from BookmarkButton.
// Unlike bookmark_bar_controller's version, we DO default to being enabled.
- (void)mouseEnteredButton:(BookmarkButton*)button event:(NSEvent*)event {
  // Prevent unnecessary button selection change while scrolling due to the
  // size changing that happens in -performOneScroll:.
  if (isScrolling_)
    return;

  [[NSCursor arrowCursor] set];

  // Make sure the mouse is still within the button's bounds (it might not be if
  // the mouse is moving quickly). Skip this check if |event| is nil (as
  // documented in the header).
  if (event && ![[button cell] isMouseReallyInside]) {
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    return;
  }

  buttonThatMouseIsIn_ = button;
  [self setSelectedButtonByIndex:[self indexOfButton:button]];

  // Cancel a previous hover if needed.
  [NSObject cancelPreviousPerformRequestsWithTarget:self];

  // If already opened, then we exited but re-entered the button
  // (without entering another button open), do nothing.
  if ([folderController_ parentButton] == button)
    return;

  // If right click was done immediately on entering a button, then open the
  // folder without delay so that context menu appears over the folder menu.
  if ([event type] == NSRightMouseDown)
    [self openBookmarkFolderFromButtonAndCloseOldOne:button];
  else
    [self performSelector:@selector(openBookmarkFolderFromButtonAndCloseOldOne:)
               withObject:button
               afterDelay:bookmarks::kHoverOpenDelay
                  inModes:[NSArray arrayWithObject:NSRunLoopCommonModes]];
}

// Called from the BookmarkButton
- (void)mouseExitedButton:(BookmarkButton*)button event:(NSEvent*)event {
  if (buttonThatMouseIsIn_ == button) {
    buttonThatMouseIsIn_ = nil;
    [self setSelectedButtonByIndex:-1];
  }

  // During scrolling -mouseExitedButton: stops scrolling, so update the
  // corresponding status field to reflect is has stopped.
  isScrolling_ = NO;

  // Stop any timer about opening a new hover-open folder.

  // Since a performSelector:withDelay: on self retains self, it is
  // possible that a cancelPreviousPerformRequestsWithTarget: reduces
  // the refcount to 0, releasing us.  That's a bad thing to do while
  // this object (or others it may own) is in the event chain.  Thus
  // we have a retain/autorelease.
  [self retain];
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  [self autorelease];
}

- (NSWindow*)browserWindow {
  return [barController_ browserWindow];
}

- (BOOL)canDragBookmarkButtonToTrash:(BookmarkButton*)button {
  return [barController_ canEditBookmarks] &&
         [barController_ canEditBookmark:[button bookmarkNode]];
}

- (void)didDragBookmarkToTrash:(BookmarkButton*)button {
  [barController_ didDragBookmarkToTrash:button];
}

- (void)bookmarkDragDidEnd:(BookmarkButton*)button
                 operation:(NSDragOperation)operation {
  [barController_ bookmarkDragDidEnd:button
                           operation:operation];
}


#pragma mark BookmarkButtonControllerProtocol

// Recursively close all bookmark folders.
- (void)closeAllBookmarkFolders {
  // Closing the top level implicitly closes all children.
  [barController_ closeAllBookmarkFolders];
}

// Close our bookmark folder (a sub-controller) if we have one.
- (void)closeBookmarkFolder:(id)sender {
  if (folderController_) {
    // Make this menu key, so key status doesn't go back to the browser
    // window when the submenu closes.
    [[self window] makeKeyWindow];
    [self setSubFolderGrowthToRight:!cocoa_l10n_util::
                                        ShouldDoExperimentalRTLLayout()];
    [[folderController_ window] close];
    folderController_ = nil;
  }
}

- (BookmarkModel*)bookmarkModel {
  return [barController_ bookmarkModel];
}

- (BOOL)draggingAllowed:(id<NSDraggingInfo>)info {
  return [barController_ draggingAllowed:info];
}

// TODO(jrg): Refactor BookmarkBarFolder common code. http://crbug.com/35966
// Most of the work (e.g. drop indicator) is taken care of in the
// folder_view.  Here we handle hover open issues for subfolders.
// Caution: there are subtle differences between this one and
// bookmark_bar_controller.mm's version.
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)info {
  NSPoint currentLocation = [info draggingLocation];
  BookmarkButton* button = [self buttonForDroppingOnAtPoint:currentLocation];

  // Don't allow drops that would result in cycles.
  if (button) {
    NSData* data = [[info draggingPasteboard]
        dataForType:ui::ClipboardUtil::UTIForPasteboardType(
                        kBookmarkButtonDragType)];
    if (data && [info draggingSource]) {
      BookmarkButton* sourceButton = nil;
      [data getBytes:&sourceButton length:sizeof(sourceButton)];
      const BookmarkNode* sourceNode = [sourceButton bookmarkNode];
      const BookmarkNode* destNode = [button bookmarkNode];
      if (destNode->HasAncestor(sourceNode))
        button = nil;
    }
  }
  // Delegate handling of dragging over a button to the |hoverState_| member.
  return [hoverState_ draggingEnteredButton:button];
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)info {
  return NSDragOperationMove;
}

// Unlike bookmark_bar_controller, we need to keep track of dragging state.
// We also need to make sure we cancel the delayed hover close.
- (void)draggingExited:(id<NSDraggingInfo>)info {
  // NOT the same as a cancel --> we may have moved the mouse into the submenu.
  // Delegate handling of the hover button to the |hoverState_| member.
  [hoverState_ draggingExited];
}

- (BOOL)dragShouldLockBarVisibility {
  return [parentController_ dragShouldLockBarVisibility];
}

// TODO(jrg): ARGH more code dup.
// http://crbug.com/35966
- (BOOL)dragButton:(BookmarkButton*)sourceButton
                to:(NSPoint)point
              copy:(BOOL)copy {
  DCHECK([sourceButton isKindOfClass:[BookmarkButton class]]);
  const BookmarkNode* sourceNode = [sourceButton bookmarkNode];
  return [self dragBookmark:sourceNode to:point copy:copy];
}

// TODO(mrossetti,jrg): Identical to the same function in BookmarkBarController.
// http://crbug.com/35966
- (BOOL)dragBookmarkData:(id<NSDraggingInfo>)info {
  BOOL dragged = NO;
  std::vector<const BookmarkNode*> nodes([self retrieveBookmarkNodeData]);
  if (nodes.size()) {
    BOOL copy = !([info draggingSourceOperationMask] & NSDragOperationMove);
    NSPoint dropPoint = [info draggingLocation];
    for (std::vector<const BookmarkNode*>::const_iterator it = nodes.begin();
         it != nodes.end(); ++it) {
      const BookmarkNode* sourceNode = *it;
      dragged = [self dragBookmark:sourceNode to:dropPoint copy:copy];
    }
  }
  return dragged;
}

// TODO(mrossetti,jrg): Identical to the same function in BookmarkBarController.
// http://crbug.com/35966
- (std::vector<const BookmarkNode*>)retrieveBookmarkNodeData {
  std::vector<const BookmarkNode*> dragDataNodes;
  BookmarkNodeData dragData;
  if (dragData.ReadFromClipboard(ui::CLIPBOARD_TYPE_DRAG)) {
    BookmarkModel* bookmarkModel = [self bookmarkModel];
    std::vector<const BookmarkNode*> nodes(
        dragData.GetNodes(bookmarkModel, profile_->GetPath()));
    dragDataNodes.assign(nodes.begin(), nodes.end());
  }
  return dragDataNodes;
}

// Return YES if we should show the drop indicator, else NO.
// TODO(jrg): ARGH code dup!
// http://crbug.com/35966
- (BOOL)shouldShowIndicatorShownForPoint:(NSPoint)point {
  return ![self buttonForDroppingOnAtPoint:point];
}

// Button selection change code to support type to select and arrow key events.
#pragma mark Keyboard Support

// Scroll the menu to show the selected button, if it's not already visible.
- (void)showSelectedButton {
  int bMaxIndex = [self buttonCount] - 1; // Max array index in button array.

  // Is there a valid selected button?
  if (bMaxIndex < 0 || selectedIndex_ < 0 || selectedIndex_ > bMaxIndex)
    return;

  // Is the menu scrollable anyway?
  if (![self canScrollUp] && ![self canScrollDown])
    return;

  // Now check to see if we need to scroll, which way, and how far.
  CGFloat delta = 0.0;
  NSPoint scrollPoint = [scrollView_ documentVisibleRect].origin;
  CGFloat itemBottom = (bMaxIndex - selectedIndex_) *
      bookmarks::kBookmarkFolderButtonHeight;
  CGFloat itemTop = itemBottom + bookmarks::kBookmarkFolderButtonHeight;
  CGFloat viewHeight = NSHeight([scrollView_  frame]);

  if (scrollPoint.y > itemBottom) { // Need to scroll down.
    delta = scrollPoint.y - itemBottom;
  } else if ((scrollPoint.y + viewHeight) < itemTop) { // Need to scroll up.
    delta = -(itemTop - (scrollPoint.y + viewHeight));
  } else { // No need to scroll.
    return;
  }

  // We have just updated the selection and are about to scroll it to be
  // visible; don't change the selection based on the mouse cursor location.
  [self performOneScroll:delta updateMouseSelection:NO];
}

// All changes to selectedness of buttons (aka fake menu items) ends up
// calling this method to actually flip the state of items.
// Needs to handle -1 as the invalid index (when nothing is selected) and
// greater than range values too.
- (void)setStateOfButtonByIndex:(int)index
                          state:(bool)state {
  if (index >= 0 && index < [self buttonCount])
    [[buttons_ objectAtIndex:index] highlight:state];
}

// Selects the required button and deselects the previously selected one.
// An index of -1 means no selection.
- (void)setSelectedButtonByIndex:(int)index {
  if (index == selectedIndex_)
    return;

  if (index >= 0 && [[buttons_ objectAtIndex:index] isEmpty])
    index = -1;

  [self setStateOfButtonByIndex:selectedIndex_ state:NO];
  [self setStateOfButtonByIndex:index state:YES];
  selectedIndex_ = index;

  [self showSelectedButton];
}

- (void)clearInputText {
  [typedPrefix_ release];
  typedPrefix_ = nil;
}

// Find the earliest item in the folder which has the target prefix.
// Returns nil if there is no prefix or there are no matches.
// These are in no particular order, and not particularly numerous, so linear
// search should be OK.
// -1 means no match.
- (int)earliestBookmarkIndexWithPrefix:(NSString*)prefix {
  if ([prefix length] == 0) // Also handles nil.
    return -1;
  int maxButtons = [buttons_ count];
  NSString* lowercasePrefix = [prefix lowercaseString];
  for (int i = 0 ; i < maxButtons ; ++i) {
    BookmarkButton* button = [buttons_ objectAtIndex:i];
    if ([[[button title] lowercaseString] hasPrefix:lowercasePrefix])
      return i;
  }
  return -1;
}

- (void)setSelectedButtonByPrefix:(NSString*)prefix {
  [self setSelectedButtonByIndex:[self earliestBookmarkIndexWithPrefix:prefix]];
}

- (void)selectPrevious {
  int newIndex;
  if (selectedIndex_ == 0)
    return;
  if (selectedIndex_ < 0)
    newIndex = [self buttonCount] -1;
  else
    newIndex = std::max(selectedIndex_ - 1, 0);
  [self setSelectedButtonByIndex:newIndex];
}

- (void)selectNext {
  if (selectedIndex_ + 1 < [self buttonCount])
    [self setSelectedButtonByIndex:selectedIndex_ + 1];
}

- (BOOL)handleInputText:(NSString*)newText {
  const unichar kUnicodeEscape = 0x001B;
  const unichar kUnicodeSpace = 0x0020;

  // Event goes to the deepest nested open submenu.
  if (folderController_)
    return [folderController_ handleInputText:newText];

  // Look for arrow keys or other function keys.
  if ([newText length] == 1) {
    // Get the 16-bit unicode char.
    unichar theChar = [newText characterAtIndex:0];
    switch (theChar) {

      // Keys that cancel and close the menu.
      case kUnicodeEscape:
      case NSDeleteCharacter:
      case NSBackspaceCharacter:
        [self clearInputText];
        return YES;
      // Keys that change selection directionally.
      case NSUpArrowFunctionKey:
        [self clearInputText];
        [self selectPrevious];
        return NO;
      case NSDownArrowFunctionKey:
        [self clearInputText];
        [self selectNext];
        return NO;
      // Keys that trigger opening of the selection.
      case kUnicodeSpace:  // Space.
      case NSNewlineCharacter:
      case NSCarriageReturnCharacter:
      case NSEnterCharacter: {
        BookmarkButton* btn = [self buttonAtIndex:selectedIndex_];
        if (!btn)
          return YES;  // Triggering with no selection closes the menu.

        if (![btn isFolder]) {
          [barController_ openBookmark:[buttons_ objectAtIndex:selectedIndex_]];
          return NO;  // NO because the selection-handling code will close
                      // later.
        }
        FALLTHROUGH;
      }
      // Keys that open and close submenus.
      case NSRightArrowFunctionKey: {
        BookmarkButton* btn = [self buttonAtIndex:selectedIndex_];
        if (btn && [btn isFolder]) {
          [self openBookmarkFolderFromButtonAndCloseOldOne:btn];
          [folderController_ selectNext];
        }
        [self clearInputText];
        return NO;
      }
      case NSLeftArrowFunctionKey:
        [self clearInputText];
        [parentController_ closeBookmarkFolder:self];
        return NO;

      // Check for other keys that should close the menu.
      default: {
        if (theChar > NSUpArrowFunctionKey &&
            theChar <= NSModeSwitchFunctionKey) {
          [self clearInputText];
          return YES;
        }
        break;
      }
    }
  }

  // It is a char or string worth adding to the type-select buffer.
  NSString* newString = (!typedPrefix_) ?
      newText : [typedPrefix_ stringByAppendingString:newText];
  [typedPrefix_ release];
  typedPrefix_ = [newString retain];
  [self setSelectedButtonByPrefix:typedPrefix_];
  return NO;
}

// Return the y position for a drop indicator.
//
// TODO(jrg): again we have code dup, sort of, with
// bookmark_bar_controller.mm, but the axis is changed.
// http://crbug.com/35966
- (CGFloat)indicatorPosForDragToPoint:(NSPoint)point {
  CGFloat y = 0;
  int destIndex = [self indexForDragToPoint:point];
  int numButtons = static_cast<int>([buttons_ count]);

  // If it's a drop strictly between existing buttons or at the very beginning
  if (destIndex >= 0 && destIndex < numButtons) {
    // ... put the indicator right between the buttons.
    BookmarkButton* button =
        [buttons_ objectAtIndex:static_cast<NSUInteger>(destIndex)];
    DCHECK(button);
    NSRect buttonFrame = [button frame];
    y = NSMaxY(buttonFrame) + 0.5 * bookmarks::kBookmarkTopVerticalPadding;

    // If it's a drop at the end (past the last button, if there are any) ...
  } else if (destIndex == numButtons) {
    // and if it's past the last button ...
    if (numButtons > 0) {
      // ... find the last button, and put the indicator below it.
      BookmarkButton* button =
          [buttons_ objectAtIndex:static_cast<NSUInteger>(destIndex - 1)];
      DCHECK(button);
      NSRect buttonFrame = [button frame];
      y = buttonFrame.origin.y -
          0.5 * bookmarks::kBookmarkBottomVerticalPadding;

    }
  } else {
    NOTREACHED();
  }

  return y;
}

- (Profile*)profile {
  return profile_;
}

- (void)childFolderWillShow:(id<BookmarkButtonControllerProtocol>)child {
  // Do nothing.
}

- (void)childFolderWillClose:(id<BookmarkButtonControllerProtocol>)child {
  // Do nothing.
}

- (BookmarkBarFolderController*)folderController {
  return folderController_;
}

- (void)faviconLoadedForNode:(const BookmarkNode*)node {
  for (BookmarkButton* button in buttons_.get()) {
    if ([button bookmarkNode] == node) {
      BOOL darkTheme = [[button window] hasDarkTheme];
      [button setImage:[barController_ faviconForNode:node
                                        forADarkTheme:darkTheme]];
      [button setNeedsDisplay:YES];
      return;
    }
  }

  // Node was not in this menu, try submenu.
  if (folderController_)
    [folderController_ faviconLoadedForNode:node];
}

// Add a new folder controller as triggered by the given folder button.
- (void)addNewFolderControllerWithParentButton:(BookmarkButton*)parentButton {
  if (folderController_)
    [self closeBookmarkFolder:self];

  // Folder controller, like many window controllers, owns itself.
  folderController_ =
      [[BookmarkBarFolderController alloc] initWithParentButton:parentButton
                                               parentController:self
                                                  barController:barController_
                                                        profile:profile_];
  [folderController_ showWindow:self];
}

- (void)openAll:(const BookmarkNode*)node
    disposition:(WindowOpenDisposition)disposition {
  [barController_ openAll:node disposition:disposition];
}

- (void)addButtonForNode:(const BookmarkNode*)node
                 atIndex:(NSInteger)buttonIndex {
  // Propose the frame for the new button. By default, this will be set to the
  // topmost button's frame (and there will always be one) offset upward in
  // anticipation of insertion.
  NSRect newButtonFrame = [[buttons_ objectAtIndex:0] frame];
  newButtonFrame.origin.y += bookmarks::kBookmarkFolderButtonHeight;
  // When adding a button to an empty folder we must remove the 'empty'
  // placeholder button. This can be detected by checking for a parent
  // child count of 1.
  const BookmarkNode* parentNode = node->parent();
  if (parentNode->child_count() == 1) {
    BookmarkButton* emptyButton = [buttons_ lastObject];
    newButtonFrame = [emptyButton frame];
    [emptyButton setDelegate:nil];
    [emptyButton removeFromSuperview];
    [buttons_ removeLastObject];
  }

  if (buttonIndex == -1 || buttonIndex > (NSInteger)[buttons_ count])
    buttonIndex = [buttons_ count];

  // Offset upward by one button height all buttons above insertion location.
  BookmarkButton* button = nil;  // Remember so it can be de-highlighted.
  for (NSInteger i = 0; i < buttonIndex; ++i) {
    button = [buttons_ objectAtIndex:i];
    // Remember this location in case it's the last button being moved
    // which is where the new button will be located.
    newButtonFrame = [button frame];
    NSRect buttonFrame = [button frame];
    buttonFrame.origin.y += bookmarks::kBookmarkFolderButtonHeight;
    [button setFrame:buttonFrame];
  }
  [[button cell] mouseExited:nil];  // De-highlight.
  BookmarkButton* newButton = [self makeButtonForNode:node
                                                frame:newButtonFrame];
  [buttons_ insertObject:newButton atIndex:buttonIndex];
  [folderView_ addSubview:newButton];

  // Close any child folder(s) which may still be open.
  [self closeBookmarkFolder:self];

  [self adjustWindowForButtonCount:[buttons_ count]];
}

// More code which essentially duplicates that of BookmarkBarController.
// TODO(mrossetti,jrg): http://crbug.com/35966
- (BOOL)addURLs:(NSArray*)urls withTitles:(NSArray*)titles at:(NSPoint)point {
  DCHECK([urls count] == [titles count]);
  BOOL nodesWereAdded = NO;
  // Figure out where these new bookmarks nodes are to be added.
  BookmarkButton* button = [self buttonForDroppingOnAtPoint:point];
  BookmarkModel* bookmarkModel = [self bookmarkModel];
  const BookmarkNode* destParent = NULL;
  int destIndex = 0;
  if ([button isFolder]) {
    destParent = [button bookmarkNode];
    // Drop it at the end.
    destIndex = [button bookmarkNode]->child_count();
  } else {
    // Else we're dropping somewhere in the folder, so find the right spot.
    destParent = [parentButton_ bookmarkNode];
    destIndex = [self indexForDragToPoint:point];
    // Be careful if the number of buttons != number of nodes.
    destIndex += [[parentButton_ cell] startingChildIndex];
  }

  bookmarks::ManagedBookmarkService* managed =
      ManagedBookmarkServiceFactory::GetForProfile(profile_);
  if (!managed->CanBeEditedByUser(destParent))
    return NO;

  // Create and add the new bookmark nodes.
  size_t urlCount = [urls count];
  for (size_t i = 0; i < urlCount; ++i) {
    GURL gurl;
    const char* string = [[urls objectAtIndex:i] UTF8String];
    if (string)
      gurl = GURL(string);
    // We only expect to receive valid URLs.
    DCHECK(gurl.is_valid());
    if (gurl.is_valid()) {
      bookmarkModel->AddURL(destParent,
                            destIndex++,
                            base::SysNSStringToUTF16([titles objectAtIndex:i]),
                            gurl);
      nodesWereAdded = YES;
    }
  }
  return nodesWereAdded;
}

- (void)moveButtonFromIndex:(NSInteger)fromIndex toIndex:(NSInteger)toIndex {
  if (fromIndex != toIndex) {
    if (toIndex == -1)
      toIndex = [buttons_ count];
    BookmarkButton* movedButton = [buttons_ objectAtIndex:fromIndex];
    if (movedButton == buttonThatMouseIsIn_)
      buttonThatMouseIsIn_ = nil;
    [buttons_ removeObjectAtIndex:fromIndex];
    NSRect movedFrame = [movedButton frame];
    NSPoint toOrigin = movedFrame.origin;
    [movedButton setHidden:YES];
    if (fromIndex < toIndex) {
      BookmarkButton* targetButton = [buttons_ objectAtIndex:toIndex - 1];
      toOrigin = [targetButton frame].origin;
      for (NSInteger i = fromIndex; i < toIndex; ++i) {
        BookmarkButton* button = [buttons_ objectAtIndex:i];
        NSRect frame = [button frame];
        frame.origin.y += bookmarks::kBookmarkFolderButtonHeight;
        [button setFrameOrigin:frame.origin];
      }
    } else {
      BookmarkButton* targetButton = [buttons_ objectAtIndex:toIndex];
      toOrigin = [targetButton frame].origin;
      for (NSInteger i = fromIndex - 1; i >= toIndex; --i) {
        BookmarkButton* button = [buttons_ objectAtIndex:i];
        NSRect buttonFrame = [button frame];
        buttonFrame.origin.y -= bookmarks::kBookmarkFolderButtonHeight;
        [button setFrameOrigin:buttonFrame.origin];
      }
    }
    [buttons_ insertObject:movedButton atIndex:toIndex];
    [movedButton setFrameOrigin:toOrigin];
    [movedButton setHidden:NO];
  }
}

// TODO(jrg): Refactor BookmarkBarFolder common code. http://crbug.com/35966
- (void)removeButton:(NSInteger)buttonIndex animate:(BOOL)animate {
  // TODO(mrossetti): Get disappearing animation to work. http://crbug.com/42360
  BookmarkButton* oldButton = [buttons_ objectAtIndex:buttonIndex];
  NSPoint poofPoint = [oldButton screenLocationForRemoveAnimation];

  // If this button has an open sub-folder, close it.
  if ([folderController_ parentButton] == oldButton)
    [self closeBookmarkFolder:self];

  // If a hover-open is pending, cancel it.
  if (oldButton == buttonThatMouseIsIn_) {
    [NSObject cancelPreviousPerformRequestsWithTarget:self];
    buttonThatMouseIsIn_ = nil;
  }

  // Deleting a button causes rearrangement that enables us to lose a
  // mouse-exited event.  This problem doesn't appear to exist with
  // other keep-menu-open options (e.g. add folder).  Since the
  // showsBorderOnlyWhileMouseInside uses a tracking area, simple
  // tricks (e.g. sending an extra mouseExited: to the button) don't
  // fix the problem.
  // http://crbug.com/54324
  for (NSButton* button in buttons_.get()) {
    if ([button showsBorderOnlyWhileMouseInside]) {
      [button setShowsBorderOnlyWhileMouseInside:NO];
      [button setShowsBorderOnlyWhileMouseInside:YES];
    }
  }

  [oldButton setDelegate:nil];
  [oldButton removeFromSuperview];
  [buttons_ removeObjectAtIndex:buttonIndex];
  for (NSInteger i = 0; i < buttonIndex; ++i) {
    BookmarkButton* button = [buttons_ objectAtIndex:i];
    NSRect buttonFrame = [button frame];
    buttonFrame.origin.y -= bookmarks::kBookmarkFolderButtonHeight;
    [button setFrame:buttonFrame];
  }
  // Search for and adjust submenus, if necessary.
  NSInteger buttonCount = [buttons_ count];
  if (buttonCount) {
    BookmarkButton* subButton = [folderController_ parentButton];
    NSInteger targetIndex = 0;
    for (NSButton* aButton in buttons_.get()) {
      targetIndex++;
      // If this button is showing its menu and is below the removed button,
      // i.e its index is greater, then we need to move the menu too.
      if (aButton == subButton && targetIndex > buttonIndex) {
          [folderController_ offsetFolderMenuWindow:NSMakeSize(0.0,
                                 bookmarks::kBookmarkFolderButtonHeight)];
          break;
      }
    }
  } else if (parentButton_ != [barController_ otherBookmarksButton]) {
    // If all nodes have been removed from this folder then add in the
    // 'empty' placeholder button except for "Other bookmarks" folder
    // as we are going to hide it.
    NSRect buttonFrame =
        GetFirstButtonFrameForHeight([self menuHeightForButtonCount:1]);
    BookmarkButton* button = [self makeButtonForNode:nil
                                               frame:buttonFrame];
    [buttons_ addObject:button];
    [folderView_ addSubview:button];
    buttonCount = 1;
  }

  // buttonCount will be 0 if "Other bookmarks" folder is empty, so close
  // the folder before hiding it.
  if (buttonCount == 0)
    [barController_ closeBookmarkFolder:nil];
  else if (buttonCount > 0)
    [self adjustWindowForButtonCount:buttonCount];

  if (animate && !ignoreAnimations_) {
    NSShowAnimationEffect(NSAnimationEffectDisappearingItemDefault, poofPoint,
                          NSZeroSize, nil, nil, nil);
  }
}

- (id<BookmarkButtonControllerProtocol>)controllerForNode:
    (const BookmarkNode*)node {
  // See if we are holding this node, otherwise see if it is in our
  // hierarchy of visible folder menus.
  if ([parentButton_ bookmarkNode] == node)
    return self;
  return [folderController_ controllerForNode:node];
}

#pragma mark TestingAPI Only

- (BOOL)canScrollUp {
  return ![scrollUpArrowView_ isHidden];
}

- (BOOL)canScrollDown {
  return ![scrollDownArrowView_ isHidden];
}

- (BOOL)isScrolling {
  return isScrolling_;
}

- (CGFloat)verticalScrollArrowHeight {
  return verticalScrollArrowHeight_;
}

- (NSView*)visibleView {
  return visibleView_;
}

- (NSScrollView*)scrollView {
  return scrollView_;
}

- (NSView*)folderView {
  return folderView_;
}

- (void)setIgnoreAnimations:(BOOL)ignore {
  ignoreAnimations_ = ignore;
}

- (BookmarkButton*)buttonThatMouseIsIn {
  return buttonThatMouseIsIn_;
}

@end  // BookmarkBarFolderController
