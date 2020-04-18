// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_CONTROLLER_H_

#import <Cocoa/Cocoa.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <unordered_map>

#include "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_bridge.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_constants.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_state.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_bar_toolbar_view.h"
#import "chrome/browser/ui/cocoa/bookmarks/bookmark_button.h"
#import "chrome/browser/ui/cocoa/has_weak_browser_pointer.h"
#include "chrome/browser/ui/cocoa/tabs/tab_strip_model_observer_bridge.h"
#include "ui/base/window_open_disposition.h"

@class BookmarkBarController;
@class BookmarkBarFolderController;
@class BookmarkBarView;
@class BookmarkButtonCell;
@class BookmarkContextMenuCocoaController;
@class BookmarkFolderTarget;
class BookmarkModelObserverForCocoa;
class Browser;
class GURL;
namespace ui {
class ThemeProvider;
}

namespace bookmarks {

class BookmarkModel;
class BookmarkNode;
class ManagedBookmarkService;

// Magic numbers from Cole
// TODO(jrg): create an objc-friendly version of bookmark_bar_constants.h?

// Used as a maximum width for buttons on the bar.
const CGFloat kDefaultBookmarkWidth = 150.0;

// Right margin before the last button in the bookmark bar.
const CGFloat kBookmarkRightMargin = 8.0;

// Left margin before the first button in the bookmark bar.
const CGFloat kBookmarkLeftMargin = 8.0;

// Vertical frame inset for buttons in the bookmark bar.
const CGFloat kBookmarkVerticalPadding = 4.0;

// Horizontal frame inset for buttons in the bookmark bar.
const CGFloat kBookmarkHorizontalPadding = 4.0;

// Used as a min/max width for buttons on menus (not on the bar).
const CGFloat kBookmarkMenuButtonMinimumWidth = 100.0;
const CGFloat kBookmarkMenuButtonMaximumWidth = 485.0;

// The minimum separation between a folder menu and the edge of the screen.
// If the menu gets closer to the edge of the screen (either right or left)
// then it is pops up in the opposite direction.
// (See -[BookmarkBarFolderController childFolderWindowLeftForWidth:]).
const CGFloat kBookmarkHorizontalScreenPadding = 8.0;

// Our NSScrollView is supposed to be just barely big enough to fit its
// contentView.  It is actually a hair too small.
// This turns on horizontal scrolling which, although slight, is awkward.
// Make sure our window (and NSScrollView) are wider than its documentView
// by at least this much.
const CGFloat kScrollViewContentWidthMargin = 2;

// Make subfolder menus overlap their parent menu a bit to give a better
// perception of a menuing system.
const CGFloat kBookmarkMenuOverlap = 2.0;

// When constraining a scrolling bookmark bar folder window to the
// screen, shrink the "constrain" by this much vertically.  Currently
// this is 0.0 to avoid a problem with tracking areas leaving the
// window, but should probably be 8.0 or something.
const CGFloat kScrollWindowVerticalMargin = 6.0;

// How far to offset a folder menu from the top of the bookmark bar. This
// is set just above the bar so that it become distinctive when drawn.
const CGFloat kBookmarkBarMenuOffset = 2.0;

// How far to offset a folder menu's left edge horizontally in relation to
// the left edge of the button from which it springs. Because of drawing
// differences, simply aligning the |frame| of each does not render the
// pproper result, so we have to offset.
const CGFloat kBookmarkBarButtonOffset = 2.0;

// Delay before opening a subfolder (and closing the previous one)
// when hovering over a folder button.
const NSTimeInterval kHoverOpenDelay = 0.3;

// Delay on hover before a submenu opens when dragging.
// Experimentally a drag hover open delay needs to be bigger than a
// normal (non-drag) menu hover open such as used in the bookmark folder.
//  TODO(jrg): confirm feel of this constant with ui-team.
//  http://crbug.com/36276
const NSTimeInterval kDragHoverOpenDelay = 0.7;

// Notes on use of kDragHoverCloseDelay in
// -[BookmarkBarFolderController draggingEntered:].
//
// We have an implicit delay on stop-hover-open before a submenu
// closes.  This cannot be zero since it's nice to move the mouse in a
// direct line from "current position" to "position of item in
// submenu".  However, by doing so, it's possible to overlap a
// different button on the current menu.  Example:
//
//  Folder1
//  Folder2  ---> Sub1
//  Folder3       Sub2
//                Sub3
//
// If you hover over the F in Folder2 to open the sub, and then want to
// select Sub3, a direct line movement of the mouse may cross over
// Folder3.  Without this delay, that'll cause Sub to be closed before
// you get there, since a "hover over" of Folder3 gets activated.
// It's subtle but without the delay it feels broken.
//
// This is only really a problem with vertical menu --> vertical menu
// movement; the bookmark bar (horizontal menu, sort of) seems fine,
// perhaps because mouse move direction is purely vertical so there is
// no opportunity for overlap.
const NSTimeInterval kDragHoverCloseDelay = 0.4;

enum BookmarkBarVisibleElementsMask {
  kVisibleElementsMaskNone = 0,
  kVisibleElementsMaskAppsButton = 1 << 0,
  kVisibleElementsMaskManagedBookmarksButton = 1 << 1,
  kVisibleElementsMaskOffTheSideButton = 1 << 2,
  kVisibleElementsMaskOtherBookmarksButton = 1 << 3,
  kVisibleElementsMaskNoItemTextField = 1 << 4,
  kVisibleElementsMaskImportBookmarksButton = 1 << 5,
};

// Specifies the location and visibility of the various sub-elements
// of the bookmark bar. Allows calculating the layout and actually
// applying it to views to be decoupled. For example, applying
// the layout in an RTL context transforms all horizontal offsets
// transparently.
struct BookmarkBarLayout {
 public:
  BookmarkBarLayout();
  ~BookmarkBarLayout();
  BookmarkBarLayout(BookmarkBarLayout&& other);
  BookmarkBarLayout& operator=(BookmarkBarLayout&& other);

  bool IsAppsButtonVisible() const {
    return visible_elements & kVisibleElementsMaskAppsButton;
  }
  bool IsManagedBookmarksButtonVisible() const {
    return visible_elements & kVisibleElementsMaskManagedBookmarksButton;
  }
  bool IsOffTheSideButtonVisible() const {
    return visible_elements & kVisibleElementsMaskOffTheSideButton;
  }
  bool IsOtherBookmarksButtonVisible() const {
    return visible_elements & kVisibleElementsMaskOtherBookmarksButton;
  }
  bool IsNoItemTextFieldVisible() const {
    return visible_elements & kVisibleElementsMaskNoItemTextField;
  }
  bool IsImportBookmarksButtonVisible() const {
    return visible_elements & kVisibleElementsMaskImportBookmarksButton;
  }
  size_t VisibleButtonCount() const { return button_offsets.size(); }

  unsigned int visible_elements;
  CGFloat apps_button_offset;
  CGFloat managed_bookmarks_button_offset;
  CGFloat off_the_side_button_offset;
  CGFloat other_bookmarks_button_offset;
  CGFloat no_item_textfield_offset;
  CGFloat no_item_textfield_width;
  CGFloat import_bookmarks_button_offset;
  CGFloat import_bookmarks_button_width;
  CGFloat max_x;

  std::unordered_map<int64_t, CGFloat> button_offsets;
};

}  // namespace bookmarks

// The interface for the bookmark bar controller's delegate. Currently, the
// delegate is the BWC and is responsible for ensuring that the toolbar is
// displayed correctly (as specified by |-getDesiredToolbarHeightCompression|
// and |-toolbarDividerOpacity|) at the beginning and at the end of an animation
// (or after a state change).
@protocol BookmarkBarControllerDelegate

// Sent when the state has changed (after any animation), but before the final
// display update.
- (void)bookmarkBar:(BookmarkBarController*)controller
 didChangeFromState:(BookmarkBar::State)oldState
            toState:(BookmarkBar::State)newState;

// Sent before the animation begins.
- (void)bookmarkBar:(BookmarkBarController*)controller
willAnimateFromState:(BookmarkBar::State)oldState
            toState:(BookmarkBar::State)newState;

@end

// A controller for the bookmark bar in the browser window. Handles showing
// and hiding based on the preference in the given profile.
@interface BookmarkBarController
    : NSViewController<BookmarkBarState,
                       BookmarkBarToolbarViewController,
                       BookmarkButtonDelegate,
                       BookmarkButtonControllerProtocol,
                       NSDraggingDestination,
                       HasWeakBrowserPointer> {
 @private
  // The state of the bookmark bar. If an animation is running, this is set to
  // the "destination" and |lastState_| is set to the "original" state.
  BookmarkBar::State currentState_;

  // The "original" state of the bookmark bar if an animation is running.
  BookmarkBar::State lastState_;

  // YES if an animation is running.
  BOOL isAnimationRunning_;

  Browser* browser_;              // weak; owned by its window
  bookmarks::BookmarkModel* bookmarkModel_;  // weak; part of the profile owned
                                             // by the top-level Browser object.
  bookmarks::ManagedBookmarkService* managedBookmarkService_;

  // Our initial view width, which is applied in awakeFromNib.
  CGFloat initialWidth_;

  // Our bookmark buttons, ordered from L-->R.
  base::scoped_nsobject<NSMutableArray> buttons_;

  // The folder image so we can use one copy for all buttons
  base::scoped_nsobject<NSImage> folderImage_;

  // The Material Design Incognito folder image so we can use one copy for all
  // buttons
  base::scoped_nsobject<NSImage> folderImageWhite_;

  // The default image, so we can use one copy for all buttons.
  base::scoped_nsobject<NSImage> defaultImage_;

  // The Incognito version of the default image.
  base::scoped_nsobject<NSImage> defaultImageIncognito_;

  // If the bar is disabled, we hide it and ignore show/hide commands.
  // Set when using fullscreen mode.
  BOOL barIsEnabled_;

  // Bridge from Chrome-style C++ notifications (e.g. derived from
  // BookmarkModelObserver)
  std::unique_ptr<BookmarkBarBridge> bridge_;

  // Delegate that is informed about state changes in the bookmark bar.
  id<BookmarkBarControllerDelegate> delegate_;  // weak

  // Logic for dealing with a click on a bookmark folder button.
  base::scoped_nsobject<BookmarkFolderTarget> folderTarget_;

  // A controller for a pop-up bookmark folder window (custom menu).
  // This is not a scoped_nsobject because it owns itself (when its
  // window closes the controller gets autoreleased).
  BookmarkBarFolderController* folderController_;

  // The event tap that allows monitoring of all events, to properly close with
  // a click outside the bounds of the window.
  id exitEventTap_;

  base::scoped_nsobject<BookmarkBarView>
      buttonView_;  // Contains 'no items' text fields.
  base::scoped_nsobject<BookmarkButton> offTheSideButton_;  // aka the chevron.

  // "Apps" button on the left side.
  base::scoped_nsobject<BookmarkButton> appsPageShortcutButton_;

  // "Managed bookmarks" button on the left side, next to the apps button.
  base::scoped_nsobject<BookmarkButton> managedBookmarksButton_;

  // "Other bookmarks" button on the right side.
  base::scoped_nsobject<BookmarkButton> otherBookmarksButton_;

  // When doing a drag, this is folder button "hovered over" which we
  // may want to open after a short delay.  There are cases where a
  // mouse-enter can open a folder (e.g. if the menus are "active")
  // but that doesn't use this variable or need a delay so "hover" is
  // the wrong term.
  base::scoped_nsobject<BookmarkButton> hoverButton_;

  // We save the view width when we add bookmark buttons.  This lets
  // us avoid a rebuild until we've grown the window bigger than our
  // initial build.
  CGFloat savedFrameWidth_;

  // A state flag which tracks when the bar's folder menus should be shown.
  // An initial click in any of the folder buttons turns this on and
  // one of the following will turn it off: another click in the button,
  // the window losing focus, a click somewhere other than in the bar
  // or a folder menu.
  BOOL showFolderMenus_;

  // If YES then state changes (for example, from hidden to shown) are animated.
  // This is turned off for unit tests.
  BOOL stateAnimationsEnabled_;

  // If YES then changes inside the bookmark bar (for example, removing a
  // bookmark) are animated. This is turned off for unit tests.
  BOOL innerContentAnimationsEnabled_;

  // YES if there is a possible drop about to happen in the bar.
  BOOL hasInsertionPos_;

  // The x point on the bar where the left edge of the new item will end
  // up if it is dropped.
  CGFloat insertionPos_;

  // Controller responsible for all bookmark context menus.
  base::scoped_nsobject<BookmarkContextMenuCocoaController>
      contextMenuController_;

  // The pulsed button for the currently pulsing node. We need to store this as
  // it may not be possible to determine the pulsing button if the pulsing node
  // is deleted. Nil if there is no pulsing node.
  base::scoped_nsobject<BookmarkButton> pulsingButton_;

  // Specifically watch the currently pulsing node. This lets us stop pulsing
  // when anything happens to the node. Null if there is no pulsing node.
  std::unique_ptr<BookmarkModelObserverForCocoa> pulsingBookmarkObserver_;
}

@property(readonly, nonatomic) BookmarkBar::State currentState;
@property(readonly, nonatomic) BookmarkBar::State lastState;
@property(readonly, nonatomic) BOOL isAnimationRunning;
@property(assign, nonatomic) id<BookmarkBarControllerDelegate> delegate;
@property(assign, nonatomic) BOOL stateAnimationsEnabled;
@property(assign, nonatomic) BOOL innerContentAnimationsEnabled;

// Initializes the bookmark bar controller with the given browser and delegate.
// To properly manage vertical resizing of the bookmark bar, the caller must
// also call -setResizeDelegate on the -controlledView. This should be done once
// the initializer returns, since it will trigger nib loading.
- (id)initWithBrowser:(Browser*)browser
         initialWidth:(CGFloat)initialWidth
             delegate:(id<BookmarkBarControllerDelegate>)delegate;

// The Browser corresponding to this BookmarkBarController.
- (Browser*)browser;

// Strongly-typed version of [self view]. Note this may trigger nib loading.
- (BookmarkBarToolbarView*)controlledView;

// The controller for all bookmark bar context menus.
- (BookmarkContextMenuCocoaController*)menuController;

// Pulses the given bookmark node, or the closest parent node that is visible.
- (void)startPulsingBookmarkNode:(const bookmarks::BookmarkNode*)node;

// Stops pulsing any bookmark nodes.
- (void)stopPulsingBookmarkNode;

// Updates the bookmark bar (from its current, possibly in-transition) state to
// the new state.
- (void)updateState:(BookmarkBar::State)newState
         changeType:(BookmarkBar::AnimateChangeType)changeType;

// Update the visible state of the bookmark bar.
- (void)updateVisibility;

// Update the visible state of the extra buttons on the bookmark bar: the
// apps shortcut and the managed bookmarks folder.
- (void)updateExtraButtonsVisibility;

// Hides or shows the bookmark bar depending on the current state.
- (void)updateHiddenState;

// Turn on or off the bookmark bar and prevent or reallow its appearance. On
// disable, toggle off if shown. On enable, show only if needed. App and popup
// windows do not show a bookmark bar.
- (void)setBookmarkBarEnabled:(BOOL)enabled;

// Returns the amount by which the toolbar above should be compressed.
- (CGFloat)getDesiredToolbarHeightCompression;

// Gets the appropriate opacity for the toolbar's divider; 0 means that it
// shouldn't be shown.
- (CGFloat)toolbarDividerOpacity;

// Set the size of the view and perform layout.
- (void)layoutToFrame:(NSRect)frame;

// Called by our view when it is moved to a window.
- (void)viewDidMoveToWindow;

// Provide a favicon for a bookmark node, specifying whether or not it's for
// use with a dark window theme.  May return nil.
- (NSImage*)faviconForNode:(const bookmarks::BookmarkNode*)node
             forADarkTheme:(BOOL)forADarkTheme;

// Used for situations where the bookmark bar folder menus should no longer
// be actively popping up. Called when the window loses focus, a click has
// occured outside the menus or a bookmark has been activated. (Note that this
// differs from the behavior of the -[BookmarkButtonControllerProtocol
// closeAllBookmarkFolders] method in that the latter does not terminate menu
// tracking since it may be being called in response to actions (such as
// dragging) where a 'stale' menu presentation should first be collapsed before
// presenting a new menu.)
- (void)closeFolderAndStopTrackingMenus;

// Checks if operations such as edit or delete are allowed.
- (BOOL)canEditBookmark:(const bookmarks::BookmarkNode*)node;

// Checks if bookmark editing is enabled at all.
- (BOOL)canEditBookmarks;

// Actions for manipulating bookmarks.
// Open a normal bookmark or folder from a button, ...
- (void)openBookmark:(id)sender;
- (void)openBookmarkFolderFromButton:(id)sender;
// From the "off the side" button, ...
- (void)openOffTheSideFolderFromButton:(id)sender;
// Import bookmarks from another browser.
- (void)importBookmarks:(id)sender;

// Returns the app page shortcut button.
- (NSButton*)appsPageShortcutButton;

// Returns the "off the side" button (aka the chevron button).
- (NSButton*)offTheSideButton;

// Returns the "off the side" button image.
- (NSImage*)offTheSideButtonImage:(BOOL)forDarkMode;

@end

// Redirects from BookmarkBarBridge, the C++ object which glues us to
// the rest of Chromium.  Internal to BookmarkBarController.
@interface BookmarkBarController(BridgeRedirect)
- (void)loaded:(bookmarks::BookmarkModel*)model;
- (void)nodeAdded:(bookmarks::BookmarkModel*)model
           parent:(const bookmarks::BookmarkNode*)oldParent index:(int)index;
- (void)nodeChanged:(bookmarks::BookmarkModel*)model
               node:(const bookmarks::BookmarkNode*)node;
- (void)nodeMoved:(bookmarks::BookmarkModel*)model
        oldParent:(const bookmarks::BookmarkNode*)oldParent
         oldIndex:(int)oldIndex
        newParent:(const bookmarks::BookmarkNode*)newParent
         newIndex:(int)newIndex;
- (void)nodeRemoved:(bookmarks::BookmarkModel*)model
             parent:(const bookmarks::BookmarkNode*)oldParent
              index:(int)index;
- (void)nodeFaviconLoaded:(bookmarks::BookmarkModel*)model
                     node:(const bookmarks::BookmarkNode*)node;
- (void)nodeChildrenReordered:(bookmarks::BookmarkModel*)model
                         node:(const bookmarks::BookmarkNode*)node;
@end

// These APIs should only be used by unit tests (or used internally).
@interface BookmarkBarController(InternalOrTestingAPI)
- (bookmarks::BookmarkBarLayout)layoutFromCurrentState;
- (void)applyLayout:(const bookmarks::BookmarkBarLayout&)layout
           animated:(BOOL)animated;
- (void)rebuildLayoutWithAnimated:(BOOL)animated;
- (void)openBookmarkFolder:(id)sender;
- (void)openOrCloseBookmarkFolderForOffTheSideButton;
- (BookmarkBarView*)buttonView;
- (NSMutableArray*)buttons;
- (BookmarkButton*)otherBookmarksButton;
- (BookmarkButton*)managedBookmarksButton;
- (BookmarkButton*)otherBookmarksButton;
- (BookmarkBarFolderController*)folderController;
- (id)folderTarget;
- (void)openURL:(GURL)url disposition:(WindowOpenDisposition)disposition;
- (void)clearBookmarkBar;
- (BookmarkButtonCell*)cellForBookmarkNode:(const bookmarks::BookmarkNode*)node;
- (BookmarkButtonCell*)cellForCustomButtonWithText:(NSString*)text
                                             image:(NSImage*)image;
- (void)checkForBookmarkButtonGrowth:(NSButton*)button;
- (void)frameDidChange;
- (void)updateTheme:(const ui::ThemeProvider*)themeProvider;
- (BookmarkButton*)buttonForDroppingOnAtPoint:(NSPoint)point;
- (BOOL)isEventAnExitEvent:(NSEvent*)event;
- (void)unhighlightBookmark:(const bookmarks::BookmarkNode*)node;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BOOKMARKS_BOOKMARK_BAR_CONTROLLER_H_
