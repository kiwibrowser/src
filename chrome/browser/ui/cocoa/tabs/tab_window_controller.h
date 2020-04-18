// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_TABS_TAB_WINDOW_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_TABS_TAB_WINDOW_CONTROLLER_H_

// A class acting as the Objective-C window controller for a window that has
// tabs which can be dragged around. Tabs can be re-arranged within the same
// window or dragged into other TabWindowController windows. This class doesn't
// know anything about the actual tab implementation or model, as that is fairly
// application-specific. It only provides an API to be overridden by subclasses
// to fill in the details.
// Note that under the hood the TabWindowController is neither an
// NSWindowController nor its window's delegate, though it receives all
// NSWindowDelegate methods as if it were. It also resides in the responder
// chain.

#import <Cocoa/Cocoa.h>

#include "base/mac/availability.h"
#include "base/mac/scoped_nsobject.h"

@class FastResizeView;
@class FocusTracker;
@class NSVisualEffectView;
@class TabStripView;
@class TabView;

@interface TabWindowController : NSResponder<NSWindowDelegate> {
 @private
  // Wrapper view around web content, and the developer tools view.
  base::scoped_nsobject<FastResizeView> tabContentArea_;
  base::scoped_nsobject<NSView> tabStripBackgroundView_;

  // Used to blur the titlebar. nil if window does not have titlebar.
  API_AVAILABLE(macos(10.10))
  base::scoped_nsobject<NSVisualEffectView> visualEffectView_;

  // The tab strip overlaps the titlebar of the window.
  base::scoped_nsobject<TabStripView> tabStripView_;

  // No views should be added directly to the root view. Views that overlap
  // the title bar should be added to the window's contentView. All other views
  // should be added to chromeContentView_. This allows tab dragging and
  // fullscreen logic to easily move the views that don't need special
  // treatment.
  base::scoped_nsobject<NSView> chromeContentView_;

  // The child window used during dragging to achieve the opacity tricks.
  NSWindow* overlayWindow_;

  // The contentView of the original window that is moved (for the duration
  // of the drag) to the |overlayWindow_|.
  NSView* originalContentView_;  // weak

  base::scoped_nsobject<FocusTracker> focusBeforeOverlay_;
  BOOL closeDeferred_;  // If YES, call performClose: in removeOverlay:.
}

// Returns the NSWindowController that manages the TabWindowController's
// NSWindow. In the past the TabWindowController was also the window's
// NSWindowController but they are now separate objects. Use
// +tabWindowControllerForWindow: to retrieve a TabWindowController from a
// given NSWindow.
@property(readonly, nonatomic)
    NSWindowController<NSWindowDelegate>* nsWindowController;
@property(retain, nonatomic) NSWindow* window;
@property(readonly, nonatomic) API_AVAILABLE(macos(10.10))
    NSVisualEffectView* visualEffectView;
@property(readonly, nonatomic) NSView* tabStripBackgroundView;
@property(readonly, nonatomic) TabStripView* tabStripView;
@property(readonly, nonatomic) FastResizeView* tabContentArea;
@property(readonly, nonatomic) NSView* chromeContentView;

// A convenience class method which returns the |TabWindowController| for
// |window|, or nil if neither |window| nor its parent or any other ancestor
// has one.
+ (TabWindowController*)tabWindowControllerForWindow:(NSWindow*)window;

// This is the designated initializer for this class.
- (id)initTabWindowControllerWithTabStrip:(BOOL)hasTabStrip
                                 titleBar:(BOOL)hasTitleBar;

// Used during tab dragging to turn on/off the overlay window when a tab
// is torn off. If -deferPerformClose (below) is used, -removeOverlay will
// cause the controller to be autoreleased before returning.
- (void)showOverlay;
- (void)removeOverlay;
- (NSWindow*)overlayWindow;

// Returns YES if it is ok to constrain the window's frame to fit the screen.
- (BOOL)shouldConstrainFrameRect;

// A collection of methods, stubbed out in this base class, that provide
// the implementation of tab dragging based on whatever model is most
// appropriate.

// Layout the tabs based on the current ordering of the model.
- (void)layoutTabs;

// Creates a new window by pulling the given tabs out and placing it in
// the new window. Returns the controller for the new window. The size of the
// new window will be the same size as this window.
- (TabWindowController*)detachTabsToNewWindow:(NSArray*)tabViews
                                   draggedTab:(NSView*)draggedTab;

// Make room in the tab strip for |tab| at the given x coordinate. Will hide the
// new tab button while there's a placeholder. Subclasses need to call the
// superclass implementation.
- (void)insertPlaceholderForTab:(TabView*)tab frame:(NSRect)frame;

// Removes the placeholder installed by |-insertPlaceholderForTab:atLocation:|
// and restores the new tab button. Subclasses need to call the superclass
// implementation.
- (void)removePlaceholder;

// Returns whether one of the window's tabs is being dragged.
- (BOOL)isDragSessionActive;

// The follow return YES if tab dragging/tab tearing (off the tab strip)/window
// movement is currently allowed. Any number of things can choose to disable it,
// such as pending animations. The default implementations always return YES.
// Subclasses should override as appropriate.
- (BOOL)tabDraggingAllowed;
- (BOOL)tabTearingAllowed;
- (BOOL)windowMovementAllowed;

// Show or hide the new tab button. The button is hidden immediately, but
// waits until the next call to |-layoutTabs| to show it again.
- (void)showNewTabButton:(BOOL)show;

// Returns whether or not |tab| can still be fully seen in the tab strip or if
// its current position would cause it be obscured by things such as the edge
// of the window or the window decorations. Returns YES only if the entire tab
// is visible. The default implementation always returns YES.
- (BOOL)isTabFullyVisible:(TabView*)tab;

// Called to check if the receiver can receive dragged tabs from
// source.  Return YES if so.  The default implementation returns NO.
- (BOOL)canReceiveFrom:(TabWindowController*)source;

// Move given tab views to the location of the current placeholder. If there is
// no placeholder, it will go at the end. |controller| is the window controller
// of a tab being dropped from a different window. It will be nil if the drag is
// within the window, otherwise the tab is removed from that window before being
// placed into this one. The implementation will call |-removePlaceholder| since
// the drag is now complete.  This also calls |-layoutTabs| internally so
// clients do not need to call it again.
- (void)moveTabViews:(NSArray*)views
      fromController:(TabWindowController*)controller;

// Called if the tab is in a detached window and has finished dragging.
// If the source window is in fullscreen, the detached window will also
// enter fullscreen.
- (void)detachedWindowEnterFullscreenIfNeeded:(TabWindowController*)source;

// Number of tabs in the tab strip. Useful, for example, to know if we're
// dragging the only tab in the window. This includes pinned tabs (both live
// and not).
- (NSInteger)numberOfTabs;

// YES if there are tabs in the tab strip which have content, allowing for
// the notion of tabs in the tab strip that are placeholders but currently have
// no content.
- (BOOL)hasLiveTabs;

// Returns all tab views.
- (NSArray*)tabViews;

// Return the view of the active tab.
- (NSView*)activeTabView;

// The title of the active tab.
- (NSString*)activeTabTitle;

// Called to check whether or not this controller's window has a tab strip (YES
// if it does, NO otherwise). The default implementation returns YES.
- (BOOL)hasTabStrip;

// Gets whether a particular tab is draggable between windows.
- (BOOL)isTabDraggable:(NSView*)tabView;

// In any fullscreen mode, the y offset to use for the content at the top of
// the screen (tab strip, omnibox, bookmark bar, etc). Ranges from 0 to -22.
- (CGFloat)menubarOffset;

// The height of the menubar.
- (CGFloat)menubarHeight;

// Returns YES if the browser window is in or entering any fullscreen mode.
- (BOOL)isInAnyFullscreenMode;

// Returns the view of the avatar button.
- (NSView*)avatarView;

// Tell the window that it needs to call performClose: as soon as the current
// drag is complete. This prevents a window (and its overlay) from going away
// during a drag.
- (void)deferPerformClose;

@end

@interface TabWindowController(ProtectedMethods)
// Tells the tab strip to forget about this tab in preparation for it being
// put into a different tab strip, such as during a drop on another window.
- (void)detachTabView:(NSView*)view;

// Called when the size of the window content area has changed. Override to
// position specific views. Base class implementation does nothing.
- (void)layoutSubviews;
@end

#endif  // CHROME_BROWSER_UI_COCOA_TABS_TAB_WINDOW_CONTROLLER_H_
