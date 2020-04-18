// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_CONTROLLER_PRIVATE_H_
#define CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_CONTROLLER_PRIVATE_H_

#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_layout.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_controller.h"

@class BrowserWindowLayout;
class PermissionRequestManager;

namespace content {
class WebContents;
}  // content.

// There are 2 mechanisms for invoking fullscreen: AppKit and Immersive.
// PRESENTATION_MODE = 1 had been removed, but the enums aren't renumbered
// since they are associated with a histogram.
enum FullscreenStyle {
  IMMERSIVE_FULLSCREEN = 0,
  CANONICAL_FULLSCREEN = 2,
  FULLSCREEN_STYLE_COUNT = 3
};

// The source that triggered fullscreen. The enums should not be renumbered
// since they're ssociated with a histogram. Exposed for testing.
enum class FullscreenSource {
  BROWSER = 0,
  TAB,
  EXTENSION,
  FULLSCREEN_SOURCE_COUNT
};

// Private methods for the |BrowserWindowController|. This category should
// contain the private methods used by different parts of the BWC; private
// methods used only by single parts should be declared in their own file.
// TODO(viettrungluu): [crbug.com/35543] work on splitting out stuff from the
// BWC, and figuring out which methods belong here (need to unravel
// "dependencies").
@interface BrowserWindowController(Private)

// Create the appropriate tab strip controller based on whether or not side
// tabs are enabled. Replaces the current controller.
- (void)createTabStripController;

// Sets the window's collection behavior to the appropriate
// fullscreen behavior.
- (void)updateFullscreenCollectionBehavior;

// Saves the window's position in the local state preferences.
- (void)saveWindowPositionIfNeeded;

// We need to adjust where sheets come out of the window, as by default they
// erupt from the omnibox, which is rather weird.
- (NSRect)window:(NSWindow*)window
    willPositionSheet:(NSWindow*)sheet
            usingRect:(NSRect)defaultSheetRect;

// Repositions the window's subviews. From the top down: toolbar, normal
// bookmark bar (if shown), infobar, NTP detached bookmark bar (if shown),
// content area, download shelf (if any).
- (void)layoutSubviews;

// Shows the informational "how to exit fullscreen" bubble.
- (void)showFullscreenExitBubbleIfNecessary;

// Lays out the tab strip and avatar button.
- (void)applyTabStripLayout:(const chrome::TabStripLayout&)layout;

// Returns YES if the bookmark bar should be placed below the infobar, NO
// otherwise.
- (BOOL)placeBookmarkBarBelowInfoBar;

// Lays out the tab content area in the given frame. If the height changes,
// sends a message to the renderer to resize.
- (void)layoutTabContentArea:(NSRect)frame;

// Sets the toolbar's height to a value appropriate for the given compression.
// Also adjusts the bookmark bar's height by the opposite amount in order to
// keep the total height of the two views constant.
- (void)adjustToolbarAndBookmarkBarForCompression:(CGFloat)compression;

// Moves views between windows in preparation for fullscreen mode when not using
// Cocoa's System Fullscreen API.  (System Fullscreen reuses the original window
// for fullscreen mode, so there is no need to move views around.)  This method
// does not position views; callers must also call |-layoutSubviews:|.
- (void)moveViewsForImmersiveFullscreen:(BOOL)fullscreen
                          regularWindow:(NSWindow*)regularWindow
                       fullscreenWindow:(NSWindow*)fullscreenWindow;

// Updates the anchor position of the permission bubble.
- (void)updatePermissionBubbleAnchor;

// Enter or exit fullscreen without using Cocoa's System Fullscreen API.  These
// methods are internal implementations of |-setFullscreen:|.
- (void)enterImmersiveFullscreen;
- (void)exitImmersiveFullscreen;

// Register or deregister for content view resize notifications.  These
// notifications are used while transitioning into fullscreen mode using Cocoa's
// System Fullscreen API.
- (void)registerForContentViewResizeNotifications;
- (void)deregisterForContentViewResizeNotifications;

// The opacity for the toolbar divider; 0 means that it shouldn't be shown.
- (CGFloat)toolbarDividerOpacity;

// Enter fullscreen by toggling the AppKit Fullscreen API.
- (void)enterAppKitFullscreen;

// Exit fullscreen by toggling the AppKit Fullscreen API. If |async| is true,
// call -toggleFullscreen: asynchronously.
- (void)exitAppKitFullscreenAsync:(BOOL)async;

// Returns where the fullscreen button should be positioned in the window.
// Returns NSZeroRect if there is no fullscreen button (if currently in
// fullscreen, or if running 10.6 or 10.10+).
- (NSRect)fullscreenButtonFrame;

// Updates |layout| with the full set of parameters required to statelessly
// determine the layout of the views managed by this controller.
- (void)updateLayoutParameters:(BrowserWindowLayout*)layout;

// Applies a layout to the views managed by this controller.
- (void)applyLayout:(BrowserWindowLayout*)layout;

// Ensures that the window's content view's subviews have the correct
// z-ordering. Will add or remove subviews as necessary.
- (void)updateSubviewZOrder;

// Performs updateSubviewZOrder when this controller is not in fullscreen.
- (void)updateSubviewZOrderNormal;

// Performs updateSubviewZOrder when this controller is in fullscreen.
- (void)updateSubviewZOrderFullscreen;

// Sets the content view's subviews. Attempts to not touch the tabContentArea
// to prevent redraws.
- (void)setContentViewSubviews:(NSArray*)subviews;

// Whether the instance should use a custom transition when animating into and
// out of AppKit Fullscreen.
- (BOOL)shouldUseCustomAppKitFullscreenTransition:(BOOL)enterFullScreen;

// Resets the variables that were set from using custom AppKit fullscreen
// animation.
- (void)resetCustomAppKitFullscreenVariables;

- (content::WebContents*)webContents;
- (PermissionRequestManager*)permissionRequestManager;

// Hides or unhides any displayed modal sheet for fullscreen transition.
// Modal sheets should be hidden at the beginning and then shown at the end.
- (void)setSheetHiddenForFullscreenTransition:(BOOL)shoudHide;

// Adjusts the UI and destroys the exit bubble when we are exiting fullscreen.
- (void)adjustUIForExitingFullscreen;

// Determines the appropriate sliding fullscreen style and adjusts the UI to
// it when we are entering fullscreen.
- (void)adjustUIForEnteringFullscreen;

// Records fullscreen metrics when the browser enters fullscreen.
- (void)recordEnterFullscreenMetrics:(FullscreenStyle)style;

// Accessor for the controller managing the fullscreen toolbar visibility
// locks.
- (FullscreenToolbarVisibilityLockController*)
    fullscreenToolbarVisibilityLockController;

@end  // @interface BrowserWindowController(Private)

#endif  // CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_CONTROLLER_PRIVATE_H_
