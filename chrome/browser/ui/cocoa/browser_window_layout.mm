// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser_window_layout.h"

#include <math.h>
#include <string.h>

#include "base/logging.h"
#include "base/mac/mac_util.h"
#include "chrome/browser/ui/cocoa/l10n_util.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_controller.h"
#include "chrome/common/chrome_features.h"

namespace chrome {

// The height of the tab strip.
const CGFloat kTabStripHeight = 37;

bool ShouldUseFullSizeContentView() {
  // Chrome historically added a subview to the window's frame view
  // (window.contentView.superview), which AppKit warns about at runtime. This
  // stopped the window buttons from being covered by the content view. This
  // may break in a future macOS release. NSFullSizeContentViewWindowMask is a
  // new (10.10+), supported way to make the content view the full size of the
  // window without covering the controls. It's only enabled in 10.12+ due to a
  // performance regression seen in 10.11 (https://crbug.com/742472).
  return base::FeatureList::IsEnabled(features::kMacFullSizeContentView) &&
         base::mac::IsAtLeastOS10_12();
}

}  // namespace chrome

namespace {

// Insets for the location bar, used when the full toolbar is hidden.
// TODO(viettrungluu): We can argue about the "correct" insetting; I like the
// following best, though arguably 0 inset is better/more correct.
const CGFloat kLocBarLeftRightInset = 1;
const CGFloat kLocBarTopInset = 0;
const CGFloat kLocBarBottomInset = 1;

// Space between the incognito badge and the right edge of the window.
const CGFloat kAvatarRightOffset = 4;

// Space between the generic avatar and the right edge of the window.
const CGFloat kGenericAvatarRightOffset = 5;

// Space between the location bar and the right edge of the window, when there
// are no extension buttons present.
// When there is a fullscreen button to the right of the new style profile
// button, we align the profile button with the location bar (although it won't
// be aligned when there are extension buttons).
const CGFloat kLocationBarRightOffset = 35;

}  // namespace

@interface BrowserWindowLayout ()

// Computes the y offset to use when laying out the tab strip in fullscreen
// mode.
- (void)computeFullscreenYOffset;

// Computes the layout of the tab strip.
- (void)computeTabStripLayout;

// Computes the layout of the subviews of the content view.
- (void)computeContentViewLayout;

// Computes the height of the backing bar for the views in the omnibox area in
// fullscreen mode.
- (CGFloat)fullscreenBackingBarHeight;

@end

@implementation BrowserWindowLayout

- (instancetype)init {
  if ((self = [super init])) {
    parameters_.isOSYosemiteOrLater = base::mac::IsAtLeastOS10_10();
  }
  return self;
}

- (chrome::LayoutOutput)computeLayout {
  memset(&output_, 0, sizeof(chrome::LayoutOutput));

  [self computeFullscreenYOffset];
  [self computeTabStripLayout];
  [self computeContentViewLayout];

  return output_;
}

- (void)setContentViewSize:(NSSize)size {
  parameters_.contentViewSize = size;
}

- (void)setWindowSize:(NSSize)size {
  parameters_.windowSize = size;
}

- (void)setInAnyFullscreen:(BOOL)inAnyFullscreen {
  parameters_.inAnyFullscreen = inAnyFullscreen;
}

- (void)setFullscreenToolbarStyle:(FullscreenToolbarStyle)style {
  parameters_.toolbarStyle = style;
}

- (void)setFullscreenMenubarOffset:(CGFloat)menubarOffset {
  parameters_.menubarOffset = menubarOffset;
}

- (void)setFullscreenToolbarFraction:(CGFloat)toolbarFraction {
  parameters_.toolbarFraction = toolbarFraction;
}

- (void)setHasTabStrip:(BOOL)hasTabStrip {
  parameters_.hasTabStrip = hasTabStrip;
}

- (void)setFullscreenButtonFrame:(NSRect)frame {
  parameters_.fullscreenButtonFrame = frame;
}

- (void)setShouldShowAvatar:(BOOL)shouldShowAvatar {
  parameters_.shouldShowAvatar = shouldShowAvatar;
}

- (void)setShouldUseNewAvatar:(BOOL)shouldUseNewAvatar {
  parameters_.shouldUseNewAvatar = shouldUseNewAvatar;
}

- (void)setIsGenericAvatar:(BOOL)isGenericAvatar {
  parameters_.isGenericAvatar = isGenericAvatar;
}

- (void)setAvatarSize:(NSSize)avatarSize {
  parameters_.avatarSize = avatarSize;
}

- (void)setAvatarLineWidth:(CGFloat)avatarLineWidth {
  parameters_.avatarLineWidth = avatarLineWidth;
}

- (void)setHasToolbar:(BOOL)hasToolbar {
  parameters_.hasToolbar = hasToolbar;
}

- (void)setHasLocationBar:(BOOL)hasLocationBar {
  parameters_.hasLocationBar = hasLocationBar;
}

- (void)setToolbarHeight:(CGFloat)toolbarHeight {
  parameters_.toolbarHeight = toolbarHeight;
}

- (void)setBookmarkBarHidden:(BOOL)bookmarkBarHidden {
  parameters_.bookmarkBarHidden = bookmarkBarHidden;
}

- (void)setPlaceBookmarkBarBelowInfoBar:(BOOL)placeBookmarkBarBelowInfoBar {
  parameters_.placeBookmarkBarBelowInfoBar = placeBookmarkBarBelowInfoBar;
}

- (void)setBookmarkBarHeight:(CGFloat)bookmarkBarHeight {
  parameters_.bookmarkBarHeight = bookmarkBarHeight;
}

- (void)setInfoBarHeight:(CGFloat)infoBarHeight {
  parameters_.infoBarHeight = infoBarHeight;
}

- (void)setHasDownloadShelf:(BOOL)hasDownloadShelf {
  parameters_.hasDownloadShelf = hasDownloadShelf;
}

- (void)setDownloadShelfHeight:(CGFloat)downloadShelfHeight {
  parameters_.downloadShelfHeight = downloadShelfHeight;
}

- (void)computeFullscreenYOffset {
  CGFloat yOffset = 0;
  if (parameters_.inAnyFullscreen) {
    yOffset += parameters_.menubarOffset;
    yOffset += std::floor((1 - parameters_.toolbarFraction) *
                          [self fullscreenBackingBarHeight]);
  }
  fullscreenYOffset_ = yOffset;
}

- (void)computeTabStripLayout {
  if (!parameters_.hasTabStrip) {
    maxY_ = parameters_.contentViewSize.height + fullscreenYOffset_;
    return;
  }

  // Temporary variable to hold the output.
  chrome::TabStripLayout layout = {};

  // Lay out the tab strip.
  maxY_ = parameters_.windowSize.height + fullscreenYOffset_;
  CGFloat width = parameters_.contentViewSize.width;
  layout.frame = NSMakeRect(
      0, maxY_ - chrome::kTabStripHeight, width, chrome::kTabStripHeight);
  maxY_ = NSMinY(layout.frame);

  // In Yosemite, there is no longer an exit fullscreen button in the top-right
  // corner of the OSX Menu Bar. Instead, a Cocoa application's toolbar is
  // expected to have traffic lights. Chrome doesn't use an NSToolbar, so it
  // needs to manually add the traffic lights to the tab strip.
  layout.addCustomWindowControls =
      parameters_.inAnyFullscreen && parameters_.isOSYosemiteOrLater;

  // Set left indentation based on fullscreen mode status.
  if (!parameters_.inAnyFullscreen || layout.addCustomWindowControls)
    layout.leadingIndent = [TabStripController defaultLeadingIndentForControls];

  // Lay out the icognito/avatar badge because calculating the indentation on
  // the right depends on it.
  if (parameters_.shouldShowAvatar) {
    CGFloat badgeXOffset = parameters_.isGenericAvatar
                               ? -kGenericAvatarRightOffset
                               : -kAvatarRightOffset;
    CGFloat badgeYOffset = 0;
    CGFloat buttonHeight = parameters_.avatarSize.height;

    if (parameters_.shouldUseNewAvatar) {
      // The fullscreen icon (if present) is displayed to the right of the
      // new style profile button.
      if (!NSIsEmptyRect(parameters_.fullscreenButtonFrame))
        badgeXOffset = -kLocationBarRightOffset;

      // Center the button, but make sure that it's pixel aligned on non-retina
      // displays. Use trunc() instead of round() to mimic the behavior of
      // autoresizesSubviews.
      badgeYOffset = trunc((chrome::kTabStripHeight - buttonHeight) / 2);
    } else {
      // Actually place the badge *above* |maxY|, by +2 to miss the divider.
      badgeYOffset = 2 * parameters_.avatarLineWidth;
    }

    NSSize size = NSMakeSize(parameters_.avatarSize.width,
                             std::min(buttonHeight, chrome::kTabStripHeight));
    NSPoint origin =
        NSMakePoint(width - parameters_.avatarSize.width + badgeXOffset,
                    maxY_ + badgeYOffset);
    layout.avatarFrame =
        NSMakeRect(origin.x, origin.y, size.width, size.height);
  }

  // Calculate the right indentation.
  // On 10.7 Lion to 10.9 Mavericks, there will be a fullscreen button when not
  // in fullscreen mode.
  // There may also be a profile button, which can be on the right of the
  // fullscreen button (old style), or to its left (new style).
  // The right indentation is calculated to prevent the tab strip from
  // overlapping these buttons.
  CGFloat maxX = width;
  if (!NSIsEmptyRect(parameters_.fullscreenButtonFrame)) {
    maxX = NSMinX(parameters_.fullscreenButtonFrame);
  }
  if (parameters_.shouldShowAvatar) {
    maxX = std::min(maxX, NSMinX(layout.avatarFrame));
  }
  layout.trailingIndent = width - maxX;

  if (cocoa_l10n_util::ShouldDoExperimentalRTLLayout()) {
    if (cocoa_l10n_util::ShouldFlipWindowControlsInRTL())
      layout.avatarFrame.origin.x =
          width - parameters_.avatarSize.width - layout.avatarFrame.origin.x;
    else
      std::swap(layout.leadingIndent, layout.trailingIndent);
  }

  output_.tabStripLayout = layout;
}

- (void)computeContentViewLayout {
  chrome::LayoutParameters parameters = parameters_;
  CGFloat maxY = maxY_;

  // Sanity-check |maxY|.
  DCHECK_GE(maxY, 0);
  DCHECK_LE(maxY, parameters_.contentViewSize.height + fullscreenYOffset_);

  CGFloat width = parameters_.contentViewSize.width;

  // Lay out the toolbar.
  if (parameters.hasToolbar) {
    CGFloat toolbarY = maxY;
    if (parameters_.inAnyFullscreen &&
        parameters_.toolbarStyle == FullscreenToolbarStyle::TOOLBAR_NONE) {
      toolbarY = parameters_.windowSize.height + fullscreenYOffset_;
    }

    output_.toolbarFrame = NSMakeRect(0, toolbarY - parameters_.toolbarHeight,
                                      width, parameters_.toolbarHeight);
    maxY = NSMinY(output_.toolbarFrame);
  } else if (parameters_.hasLocationBar) {
    CGFloat toolbarX = kLocBarLeftRightInset;
    CGFloat toolbarY = maxY - parameters_.toolbarHeight - kLocBarTopInset;
    CGFloat toolbarWidth = width - 2 * kLocBarLeftRightInset;
    output_.toolbarFrame =
        NSMakeRect(toolbarX, toolbarY, toolbarWidth, parameters_.toolbarHeight);
    maxY = NSMinY(output_.toolbarFrame) - kLocBarBottomInset;
  }

  // Lay out the bookmark bar, if it's above the info bar.
  if (!parameters.bookmarkBarHidden &&
      !parameters.placeBookmarkBarBelowInfoBar) {
    output_.bookmarkFrame = NSMakeRect(0,
                                       maxY - parameters.bookmarkBarHeight,
                                       width,
                                       parameters.bookmarkBarHeight);
    maxY = NSMinY(output_.bookmarkFrame);
  }

  // Lay out the backing bar in fullscreen mode.
  if (parameters_.inAnyFullscreen) {
    output_.fullscreenBackingBarFrame =
        NSMakeRect(0, maxY, width, [self fullscreenBackingBarHeight]);
  }

  // Place the find bar immediately below the toolbar/attached bookmark bar.
  output_.findBarMaxY = maxY;

  if (parameters_.inAnyFullscreen &&
      (parameters_.toolbarStyle == FullscreenToolbarStyle::TOOLBAR_HIDDEN ||
       parameters_.toolbarStyle == FullscreenToolbarStyle::TOOLBAR_NONE)) {
    // If the toolbar is hidden or missing, reset |maxY| to top of screen, so
    // that the toolbar slides over the things which appear to be in the
    // content area.
    maxY = parameters_.windowSize.height;
  }

  // Lay out the info bar. It is never hidden.
  if (parameters_.infoBarHeight != 0) {
    CGFloat infoBarMaxY = maxY;
    CGFloat infoBarMinY = maxY - parameters_.infoBarHeight;

    output_.infoBarFrame =
        NSMakeRect(0, infoBarMinY, width, infoBarMaxY - infoBarMinY);
    maxY = NSMinY(output_.infoBarFrame);
  } else {
    // The info bar has 0 height, but tests still expect it in the right
    // location.
    output_.infoBarFrame = NSMakeRect(0, maxY, width, 0);
  }

  // Lay out the bookmark bar when it is below the info bar.
  if (!parameters.bookmarkBarHidden &&
      parameters.placeBookmarkBarBelowInfoBar) {
    output_.bookmarkFrame = NSMakeRect(0,
                                       maxY - parameters.bookmarkBarHeight,
                                       width,
                                       parameters.bookmarkBarHeight);
    maxY = NSMinY(output_.bookmarkFrame);
  }

  // Layout the download shelf at the bottom of the content view.
  CGFloat minY = 0;
  if (parameters.hasDownloadShelf) {
    output_.downloadShelfFrame =
        NSMakeRect(0, 0, width, parameters.downloadShelfHeight);
    minY = NSMaxY(output_.downloadShelfFrame);
  }

  if (parameters_.inAnyFullscreen &&
      parameters_.toolbarStyle == FullscreenToolbarStyle::TOOLBAR_PRESENT) {
    // If in Canonical Fullscreen, content should be shifted down by an amount
    // equal to all the widgets and views at the top of the window. It should
    // not be further shifted by the appearance/disappearance of the AppKit
    // menu bar.
    maxY = parameters_.windowSize.height;
    maxY -= NSHeight(output_.toolbarFrame) +
            NSHeight(output_.tabStripLayout.frame) +
            NSHeight(output_.bookmarkFrame) + parameters.infoBarHeight;
  }

  // All the remaining space becomes the frame of the content area.
  output_.contentAreaFrame = NSMakeRect(0, minY, width, maxY - minY);
}

- (CGFloat)fullscreenBackingBarHeight {
  if (!parameters_.inAnyFullscreen)
    return 0;

  CGFloat totalHeight = 0;
  if (parameters_.hasTabStrip)
    totalHeight += chrome::kTabStripHeight;

  if (parameters_.hasToolbar) {
    totalHeight += parameters_.toolbarHeight;
  } else if (parameters_.hasLocationBar) {
    totalHeight +=
        parameters_.toolbarHeight + kLocBarTopInset + kLocBarBottomInset;
  }

  if (!parameters_.bookmarkBarHidden &&
      !parameters_.placeBookmarkBarBelowInfoBar)
    totalHeight += parameters_.bookmarkBarHeight;

  return totalHeight;
}

@end

@implementation BrowserWindowLayout (ExposedForTesting)

- (void)setOSYosemiteOrLater:(BOOL)osYosemiteOrLater {
  parameters_.isOSYosemiteOrLater = osYosemiteOrLater;
}

@end
