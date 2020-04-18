// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_FULLSCREEN_FULLSCREEN_TOOLBAR_CONTROLLER_H_
#define CHROME_BROWSER_UI_COCOA_FULLSCREEN_FULLSCREEN_TOOLBAR_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsobject.h"

@class BrowserWindowController;
@class FullscreenMenubarTracker;
class FullscreenToolbarAnimationController;
@class FullscreenToolbarMouseTracker;
@class FullscreenToolbarVisibilityLockController;
@class ImmersiveFullscreenController;

namespace content {
class WebContents;
}

// This enum class represents the appearance of the fullscreen toolbar, which
// includes the tab strip and omnibox. These values are logged in a histogram
// and shouldn't be renumbered or removed.
enum class FullscreenToolbarStyle {
  // The toolbar is present. Moving the cursor to the top
  // causes the menubar to appear and the toolbar to slide down.
  TOOLBAR_PRESENT = 0,
  // The toolbar is hidden. Moving cursor to top shows the
  // toolbar and menubar.
  TOOLBAR_HIDDEN,
  // Toolbar is hidden. Moving cursor to top causes the menubar
  // to appear, but not the toolbar.
  TOOLBAR_NONE,
  // The last enum value. Used for logging in a histogram.
  TOOLBAR_LAST = TOOLBAR_NONE
};

static constexpr int kFullscreenToolbarStyleCount =
    (int)FullscreenToolbarStyle::TOOLBAR_LAST + 1;

// This struct contains the calculated values of the fullscreen toolbar layout.
struct FullscreenToolbarLayout {
  // The toolbar style.
  FullscreenToolbarStyle toolbarStyle;

  // The fraction of the toolbar that is shown in the screen.
  CGFloat toolbarFraction;

  // The amount the menuber should be offset from the top of the screen.
  CGFloat menubarOffset;
};

// Provides a controller to the fullscreen toolbar for a single browser
// window. This class sets up the animation manager, visibility locks, menubar
// tracking, and mouse tracking associated with the toolbar. It receives input
// from these objects to update and recompute the fullscreen toolbar laytout.

@interface FullscreenToolbarController : NSObject {
 @private
  // Our parent controller.
  BrowserWindowController* browserController_;  // weak

  // Whether or not we are in fullscreen mode.
  BOOL inFullscreenMode_;

  // Updates the fullscreen toolbar layout for changes in the menubar. This
  // object is only set when the browser is in fullscreen mode.
  base::scoped_nsobject<FullscreenMenubarTracker> menubarTracker_;

  // Maintains the toolbar's visibility locks for the TOOLBAR_HIDDEN style.
  base::scoped_nsobject<FullscreenToolbarVisibilityLockController>
      visibilityLockController_;

  // Manages the toolbar animations for the TOOLBAR_HIDDEN style.
  std::unique_ptr<FullscreenToolbarAnimationController> animationController_;

  // When the menu bar and toolbar are visible, creates a tracking area which
  // is used to keep them visible until the mouse moves far enough away from
  // them. Only set when the browser is in fullscreen mode.
  base::scoped_nsobject<FullscreenToolbarMouseTracker> mouseTracker_;

  // Controller for immersive fullscreen.
  base::scoped_nsobject<ImmersiveFullscreenController>
      immersiveFullscreenController_;

  // The style of the fullscreen toolbar.
  FullscreenToolbarStyle toolbarStyle_;
}

// Designated initializer.
- (id)initWithBrowserController:(BrowserWindowController*)controller;

// Informs the controller that the browser has entered or exited fullscreen
// mode. |-enterFullscreenMode| should be called when the window is about to
// enter fullscreen. |-exitFullscreenMode| should be called before any views
// are moved back to the non-fullscreen window.
- (void)enterFullscreenMode;
- (void)exitFullscreenMode;

// Cancels any running animation and timers.
- (void)cancelAnimationAndTimer;

// Animates the toolbar dropping down to show changes to the tab strip.
- (void)revealToolbarForWebContents:(content::WebContents*)contents
                       inForeground:(BOOL)inForeground;

// Returns the fraction of the toolbar exposed at the top.
// It returns 1.0 if the toolbar is fully shown and 0.0 if the toolbar is
// hidden. Otherwise, if the toolbar is in progress of animating, it will
// return a float that ranges from (0, 1).
- (CGFloat)toolbarFraction;

// Returns |toolbarStyle_|.
- (FullscreenToolbarStyle)toolbarStyle;

// Computes and return the layout for the fullscreen toolbar.
- (FullscreenToolbarLayout)computeLayout;

// Returns YES if the fullscreen toolbar must be shown.
- (BOOL)mustShowFullscreenToolbar;

// Called by the BrowserWindowController to update toolbar frame.
- (void)updateToolbarFrame:(NSRect)frame;

// Updates the toolbar style. If the style has changed, then the toolbar will
// relayout.
- (void)layoutToolbarStyleIsExitingTabFullscreen:(BOOL)isExitingTabFullscreen;

// Updates the toolbar by updating the layout.
- (void)layoutToolbar;

// Returns YES if the browser in in fullscreen.
- (BOOL)isInFullscreen;

// Returns |browserController_|.
- (BrowserWindowController*)browserWindowController;

// Returns the object in |visibilityLockController_|;
- (FullscreenToolbarVisibilityLockController*)visibilityLockController;

@end

// Private methods exposed for testing.
@interface FullscreenToolbarController (ExposedForTesting)

// Returns |animationController_|.
- (FullscreenToolbarAnimationController*)animationController;

// Allows tests to set a mock FullscreenMenubarTracker object.
- (void)setMenubarTracker:(FullscreenMenubarTracker*)tracker;

// Allows tests to set a mock FullscreenToolbarMouseTracker object.
- (void)setMouseTracker:(FullscreenToolbarMouseTracker*)tracker;

// Sets the value of |toolbarStyle_|.
- (void)setToolbarStyle:(FullscreenToolbarStyle)style;

// Sets the value of |inFullscreenMode_|.
- (void)setTestFullscreenMode:(BOOL)isInFullscreen;

@end

#endif  // CHROME_BROWSER_UI_COCOA_FULLSCREEN_FULLSCREEN_TOOLBAR_CONTROLLER_H_
