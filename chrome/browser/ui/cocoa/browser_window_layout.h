// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_LAYOUT_H_
#define CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_LAYOUT_H_

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_controller.h"

namespace chrome {

// The height of the tab strip.
extern const CGFloat kTabStripHeight;

// Returns true if windows should use NSFullSizeContentViewWindowMask style
// mask.
bool ShouldUseFullSizeContentView();

// The parameters used to calculate the layout of the views managed by the
// BrowserWindowController.
struct LayoutParameters {
  // The size of the content view of the window.
  NSSize contentViewSize;
  // The size of the window.
  NSSize windowSize;

  // Whether the controller is in any fullscreen mode. This parameter should be
  // NO if the controller is in the process of entering fullscreen.
  BOOL inAnyFullscreen;
  // The fullscreen toolbar style. See fullscreen_toolbar_controller.h for more
  // details.
  FullscreenToolbarStyle toolbarStyle;
  // The minY of the AppKit Menu Bar, relative to the top of the screen. Ranges
  // from 0 to -22. Only relevant in fullscreen mode.
  CGFloat menubarOffset;
  // The fraction of the toolbar that is visible in fullscreen mode.
  // Ranges from 0 to 1. Only relevant in fullscreen mode.
  CGFloat toolbarFraction;

  BOOL hasTabStrip;
  // The frame of the fullscreen button. May be NSZeroRect if the fullscreen
  // button doesn't exist. Only needs to be set when hasTabStrip is YES.
  NSRect fullscreenButtonFrame;
  // Whether the avatar button should be shown. Only needs to be set when
  // hasTabStrip is YES.
  BOOL shouldShowAvatar;
  // Whether to use the new avatar button. Only needs to be set when
  // shouldShowAvatar is YES.
  BOOL shouldUseNewAvatar;
  // True if the avatar button is a generic avatar.
  BOOL isGenericAvatar;
  // The size of the avatar button. Only needs to be set when shouldShowAvatar
  // is YES.
  NSSize avatarSize;
  // The line width that will generate a 1 pixel wide line for the avatar's
  // superview. Only needs to be set when shouldShowAvatar is YES.
  CGFloat avatarLineWidth;

  BOOL hasToolbar;
  BOOL hasLocationBar;
  CGFloat toolbarHeight;

  BOOL bookmarkBarHidden;
  // If the bookmark bar is not hidden, then the bookmark bar should either be
  // directly below the omnibox, or directly below the info bar. This parameter
  // selects between those 2 cases.
  BOOL placeBookmarkBarBelowInfoBar;
  CGFloat bookmarkBarHeight;

  CGFloat infoBarHeight;

  BOOL hasDownloadShelf;
  CGFloat downloadShelfHeight;

  // This parameter exists so that unit tests can configure the OS version.
  BOOL isOSYosemiteOrLater;
};

// The parameters required to lay out the tab strip and its components.
struct TabStripLayout {
  // The frame of the tab strip in window coordinates.
  NSRect frame;
  // The leading indent for the controls of the TabStripController.
  CGFloat leadingIndent;
  // The trailing indent for the controls of the TabStripController.
  CGFloat trailingIndent;
  // Whether the TabStripController needs to add custom traffic light buttons.
  BOOL addCustomWindowControls;
  // The frame of the avatar in window coordinates.
  NSRect avatarFrame;
};

// The output frames of the views managed by the BrowserWindowController. All
// frames are in the coordinate system of the window. The lower-left corner of
// the contentView coincides with the lower-left corner of the window, so these
// frames are also in the coordinate system of the contentView.
struct LayoutOutput {
  TabStripLayout tabStripLayout;
  NSRect toolbarFrame;
  NSRect bookmarkFrame;
  NSRect fullscreenBackingBarFrame;
  CGFloat findBarMaxY;
  NSRect infoBarFrame;
  NSRect downloadShelfFrame;
  NSRect contentAreaFrame;
};

}  // namespace chrome

// This class is the sole entity responsible for calculating the layout of the
// views managed by the BrowserWindowController. The parameters used to
// calculate the layout are the fields of |parameters_|. These fields should be
// filled by calling the appropriate setters on this class. Once the parameters
// have been set, calling -computeLayout will return a LayoutOutput that
// includes sufficient information to lay out all views managed by
// BrowserWindowController.
//
// This is a lightweight class. It should be created on demand.
@interface BrowserWindowLayout : NSObject {
 @private
  // Stores the parameters used to compute layout.
  chrome::LayoutParameters parameters_;

  // Stores the layout output as it's being computed.
  chrome::LayoutOutput output_;

  // The offset of the maxY of the tab strip during fullscreen mode.
  CGFloat fullscreenYOffset_;

  // The views are laid out from highest Y to lowest Y. This variable holds the
  // current highest Y that the next view is expected to be laid under.
  CGFloat maxY_;
}

// Designated initializer.
- (instancetype)init;

// Performs the layout computation and returns the results. This method is fast
// and does not perform any caching.
- (chrome::LayoutOutput)computeLayout;

- (void)setContentViewSize:(NSSize)size;
- (void)setWindowSize:(NSSize)size;

// Whether the controller is in any fullscreen mode. |inAnyFullscreen| should
// be NO if the controller is in the process of entering fullscreen.
- (void)setInAnyFullscreen:(BOOL)inAnyFullscreen;
- (void)setFullscreenToolbarStyle:(FullscreenToolbarStyle)toolbarStyle;
- (void)setFullscreenMenubarOffset:(CGFloat)menubarOffset;
- (void)setFullscreenToolbarFraction:(CGFloat)toolbarFraction;

- (void)setHasTabStrip:(BOOL)hasTabStrip;
- (void)setFullscreenButtonFrame:(NSRect)frame;
- (void)setShouldShowAvatar:(BOOL)shouldShowAvatar;
- (void)setShouldUseNewAvatar:(BOOL)shouldUseNewAvatar;
- (void)setIsGenericAvatar:(BOOL)isGenericAvatar;
- (void)setAvatarSize:(NSSize)avatarSize;
- (void)setAvatarLineWidth:(CGFloat)avatarLineWidth;

- (void)setHasToolbar:(BOOL)hasToolbar;
- (void)setHasLocationBar:(BOOL)hasLocationBar;
- (void)setToolbarHeight:(CGFloat)toolbarHeight;

- (void)setBookmarkBarHidden:(BOOL)bookmarkBarHidden;
- (void)setPlaceBookmarkBarBelowInfoBar:(BOOL)placeBookmarkBarBelowInfoBar;
- (void)setBookmarkBarHeight:(CGFloat)bookmarkBarHeight;

- (void)setInfoBarHeight:(CGFloat)infoBarHeight;

- (void)setHasDownloadShelf:(BOOL)hasDownloadShelf;
- (void)setDownloadShelfHeight:(CGFloat)downloadShelfHeight;
@end

@interface BrowserWindowLayout (ExposedForTesting)
- (void)setOSYosemiteOrLater:(BOOL)osYosemiteOrLater;
@end

#endif  // CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_LAYOUT_H_
