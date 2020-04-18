// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_controller.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"
#import "ui/base/cocoa/tracking_area.h"

class Profile;

@class BookmarkBarController;
@class BookmarkBarFolderView;
@class BookmarkBarFolderHoverState;
@class BookmarkBarFolderWindow;
@class BookmarkBarFolderWindowContentView;
@class BookmarkFolderTarget;

namespace bookmarks {

// The padding between the top of the folder menu and the topmost button.
const CGFloat kBookmarkTopVerticalPadding = bookmarks::kBookmarkVerticalPadding;

// The padding between the bottom of the folder menu and the bottommost button.
const CGFloat kBookmarkBottomVerticalPadding = 0.0;

}  // bookmarks

// A controller for the pop-up windows from bookmark folder buttons
// which look sort of like menus.
@interface BookmarkBarFolderController :
    NSWindowController<BookmarkButtonDelegate,
                       BookmarkButtonControllerProtocol> {
 @private
  // The button whose click opened us.
  base::scoped_nsobject<BookmarkButton> parentButton_;

  // Bookmark bar folder controller chains are torn down in two ways:
  // 1. Clicking "outside" the folder (see use of the NSEvent local event
  //    monitor in the bookmark bar controller).
  // 2. Engaging a different folder (via hover over or explicit click).
  //
  // In either case, the BookmarkButtonControllerProtocol method
  // closeAllBookmarkFolders gets called.  For bookmark bar folder
  // controllers, this is passed up the chain so we begin with a top
  // level "close".
  // When any bookmark folder window closes, it necessarily tells
  // subcontroller windows to close (down the chain), and autoreleases
  // the controller.  (Must autorelease since the controller can still
  // get delegate events such as windowDidClose).
  //
  // Bookmark bar folder controllers own their buttons.  When doing
  // drag and drop of a button from one sub-sub-folder to a different
  // sub-sub-folder, we need to make sure the button's pointers stay
  // valid until we've dropped (or cancelled).  Note that such a drag
  // causes the source sub-sub-folder (previous parent window) to go
  // away (windows close, controllers autoreleased) since you're
  // hovering over a different folder chain for dropping.  To keep
  // things valid (like the button's target, its delegate, the parent
  // cotroller that we have a pointer to below [below], etc), we heep
  // strong pointers to our owning controller, so the entire chain
  // stays owned.

  // Our parent controller, if we are a nested folder, otherwise nil.
  // Strong to insure the object lives as long as we need it.
  base::scoped_nsobject<BookmarkBarFolderController> parentController_;

  // The main bar controller from whence we or a parent sprang.
  BookmarkBarController* barController_;  // WEAK: It owns us.

  // Our buttons.  We do not have buttons for nested folders.
  base::scoped_nsobject<NSMutableArray> buttons_;

  // The scroll view that contains our main button view (below).
  IBOutlet NSScrollView* scrollView_;

  // The view defining the visible area in which we draw our content.
  IBOutlet BookmarkBarFolderWindowContentView* visibleView_;

  // The main view of this window (where the buttons go) within the scroller.
  IBOutlet BookmarkBarFolderView* folderView_;

  // A window used to show the shadow behind the main window when it is
  // scrollable. (A 'shadow' window is needed because the main window, when
  // scrollable in either or both directions, will reach completely to the
  // top and/or bottom edge of the screen in order to support mouse tracking
  // during scrolling operations. In that case, though, the 'visible'
  // window must be inset a bit from the edge of the screen for aesthetics;
  // it will also be inset much more from the bottom of the screen when the
  // Dock is showing. When scrollable, the main window would show a shadow
  // incorrectly positioned, hence the 'shadow' window.)
  IBOutlet BookmarkBarFolderWindow* shadowWindow_;

  // The up and down scroll arrow views. These arrows are hidden and shown
  // as necessary (when scrolling is possible) and are contained in the nib
  // as siblings to the scroll view.
  IBOutlet NSView* scrollDownArrowView_;  // Positioned at the top.
  IBOutlet NSView* scrollUpArrowView_;  // Positioned at the bottom.

  // YES if subfolders should grow to the right (the default).
  // Direction switches if we'd grow off the screen.
  BOOL subFolderGrowthToRight_;

  // Weak; we keep track to work around a
  // setShowsBorderOnlyWhileMouseInside bug.
  BookmarkButton* buttonThatMouseIsIn_;

  // We model hover state as a state machine with specific allowable
  // transitions.  |hoverState_| is the state of this machine at any
  // given time.
  base::scoped_nsobject<BookmarkBarFolderHoverState> hoverState_;

  // Logic for dealing with a click on a bookmark folder button.
  base::scoped_nsobject<BookmarkFolderTarget> folderTarget_;

  // A controller for a pop-up bookmark folder window (custom menu).
  // We (self) are the parentController_ for our folderController_.
  // This is not a scoped_nsobject because it owns itself (when its
  // window closes the controller gets autoreleased).
  BookmarkBarFolderController* folderController_;

  // Implement basic menu scrolling through this tracking area.
  ui::ScopedCrTrackingArea scrollTrackingArea_;

  // Timer to continue scrolling as needed.  We own the timer but
  // don't release it when done (we invalidate it).
  NSTimer* scrollTimer_;

  // Precalculated sum of left and right edge padding of buttons in a
  // folder menu window. This is calculated from the widths of the main
  // folder menu window and the scroll view within.
  CGFloat padding_;

  // Amount to scroll by on each timer fire.  Can be + or -.
  CGFloat verticalScrollDelta_;

  // We need to know the size of the vertical scrolling arrows so we
  // can obscure/unobscure them.
  CGFloat verticalScrollArrowHeight_;

  // Set to YES to prevent any node animations. Useful for unit testing so that
  // incomplete animations do not cause valgrind complaints.
  BOOL ignoreAnimations_;

  int selectedIndex_;
  NSString* typedPrefix_;

  Profile* profile_;
  BOOL isScrolling_;
}

// Designated initializer.
- (id)initWithParentButton:(BookmarkButton*)button
          parentController:(BookmarkBarFolderController*)parentController
             barController:(BookmarkBarController*)barController
                   profile:(Profile*)profile;

// Return the parent button that owns the bookmark folder we represent.
- (BookmarkButton*)parentButton;

// Text typed by user, for type-select and arrow key support.
// Returns YES if the menu should be closed now.
- (BOOL)handleInputText:(NSString*)newText;

// If you wanted to clear the type-select buffer. Currently only used
// internally.
- (void)clearInputText;

// Gets notified when a fav icon asynchronously loads, so we can now use the
// real icon instead of a generic placeholder.
- (void)faviconLoadedForNode:(const bookmarks::BookmarkNode*)node;

- (void)setSelectedButtonByIndex:(int)index;

// Offset our folder menu window. This is usually needed in response to a
// parent folder menu window or the bookmark bar changing position due to
// the dragging of a bookmark node from the parent into this folder menu.
- (void)offsetFolderMenuWindow:(NSSize)offset;

// Re-layout the window menu in case some buttons were added or removed,
// specifically as a result of the bookmark bar changing configuration
// and altering the contents of the off-the-side folder.
- (void)reconfigureMenu;

// Passed up by a child view to tell us of a desire to scroll.
- (void)scrollWheel:(NSEvent *)theEvent;

- (void)mouseDragged:(NSEvent*)theEvent;

@property(assign, nonatomic) BOOL subFolderGrowthToRight;

@end

@interface BookmarkBarFolderController(TestingAPI)
- (NSPoint)windowTopLeftForWidth:(int)windowWidth
                          height:(int)windowHeight;
- (NSArray*)buttons;
- (BookmarkBarFolderController*)folderController;
- (id)folderTarget;
- (void)configureWindowLevel;
- (void)performOneScroll:(CGFloat)delta
    updateMouseSelection:(BOOL)updateMouseSelection;
- (BookmarkButton*)buttonThatMouseIsIn;
// Set to YES in order to prevent animations.
- (void)setIgnoreAnimations:(BOOL)ignore;

// Return YES if the scroll-up or scroll-down arrows are showing.
- (BOOL)canScrollUp;
- (BOOL)canScrollDown;
- (BOOL)isScrolling;
- (CGFloat)verticalScrollArrowHeight;
- (NSView*)visibleView;
- (NSScrollView*)scrollView;
- (NSView*)folderView;

- (IBAction)openBookmarkFolderFromButton:(id)sender;

- (BookmarkButton*)buttonForDroppingOnAtPoint:(NSPoint)point;
@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_FOLDER_CONTROLLER_H_
