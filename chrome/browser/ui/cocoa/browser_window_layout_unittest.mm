// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#import "chrome/browser/ui/cocoa/browser_window_layout.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"

class BrowserWindowLayoutTest : public testing::Test {
 public:
  BrowserWindowLayoutTest() {}
  void SetUp() override {
    layout.reset([[BrowserWindowLayout alloc] init]);

    [layout setContentViewSize:NSMakeSize(600, 600)];
    [layout setWindowSize:NSMakeSize(600, 622)];
    [layout setInAnyFullscreen:NO];
    [layout setHasTabStrip:YES];
    [layout setFullscreenButtonFrame:NSMakeRect(575, 596, 16, 17)];
    [layout setShouldShowAvatar:YES];
    [layout setShouldUseNewAvatar:YES];
    [layout setIsGenericAvatar:NO];
    [layout setAvatarSize:NSMakeSize(63, 28)];
    [layout setAvatarLineWidth:1];
    [layout setHasToolbar:YES];
    [layout setToolbarHeight:32];
    [layout setPlaceBookmarkBarBelowInfoBar:NO];
    [layout setBookmarkBarHidden:NO];
    [layout setBookmarkBarHeight:26];
    [layout setInfoBarHeight:72];
    [layout setHasDownloadShelf:YES];
    [layout setDownloadShelfHeight:44];
    [layout setOSYosemiteOrLater:NO];
  }

  base::scoped_nsobject<BrowserWindowLayout> layout;

  // Updates the layout parameters with the state associated with a typical
  // fullscreened window.
  void ApplyStandardFullscreenLayoutParameters() {
    // Content view has same size as window in AppKit Fullscreen.
    [layout setContentViewSize:NSMakeSize(600, 622)];
    [layout setInAnyFullscreen:YES];
    [layout setFullscreenToolbarStyle:FullscreenToolbarStyle::TOOLBAR_PRESENT];
    [layout setFullscreenMenubarOffset:0];
    [layout setFullscreenToolbarFraction:1];
    [layout setFullscreenButtonFrame:NSZeroRect];
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserWindowLayoutTest);
};

TEST_F(BrowserWindowLayoutTest, TestAllViews) {
  chrome::LayoutOutput output = [layout computeLayout];

  EXPECT_NSEQ(NSMakeRect(0, 585, 600, 37), output.tabStripLayout.frame);
  EXPECT_NSEQ(NSMakeRect(502, 589, 63, 28), output.tabStripLayout.avatarFrame);
  EXPECT_EQ(70, output.tabStripLayout.leadingIndent);
  EXPECT_EQ(98, output.tabStripLayout.trailingIndent);
  EXPECT_NSEQ(NSMakeRect(0, 553, 600, 32), output.toolbarFrame);
  EXPECT_NSEQ(NSMakeRect(0, 527, 600, 26), output.bookmarkFrame);
  EXPECT_NSEQ(NSZeroRect, output.fullscreenBackingBarFrame);
  EXPECT_EQ(527, output.findBarMaxY);
  EXPECT_NSEQ(NSMakeRect(0, 455, 600, 72), output.infoBarFrame);
  EXPECT_NSEQ(NSMakeRect(0, 0, 600, 44), output.downloadShelfFrame);
  EXPECT_NSEQ(NSMakeRect(0, 44, 600, 411), output.contentAreaFrame);
}

TEST_F(BrowserWindowLayoutTest, TestAllViewsFullscreen) {
  ApplyStandardFullscreenLayoutParameters();

  chrome::LayoutOutput output = [layout computeLayout];

  EXPECT_NSEQ(NSMakeRect(0, 585, 600, 37), output.tabStripLayout.frame);
  EXPECT_NSEQ(NSMakeRect(533, 589, 63, 28), output.tabStripLayout.avatarFrame);
  EXPECT_EQ(0, output.tabStripLayout.leadingIndent);
  EXPECT_FALSE(output.tabStripLayout.addCustomWindowControls);
  EXPECT_EQ(67, output.tabStripLayout.trailingIndent);
  EXPECT_NSEQ(NSMakeRect(0, 553, 600, 32), output.toolbarFrame);
  EXPECT_NSEQ(NSMakeRect(0, 527, 600, 26), output.bookmarkFrame);
  EXPECT_NSEQ(NSMakeRect(0, 527, 600, 95), output.fullscreenBackingBarFrame);
  EXPECT_EQ(527, output.findBarMaxY);
  EXPECT_NSEQ(NSMakeRect(0, 455, 600, 72), output.infoBarFrame);
  EXPECT_NSEQ(NSMakeRect(0, 0, 600, 44), output.downloadShelfFrame);
  EXPECT_NSEQ(NSMakeRect(0, 44, 600, 411), output.contentAreaFrame);
}

// In fullscreen mode for Yosemite, the tab strip's left indent should be
// sufficiently large to accomodate the addition of traffic light buttons.
TEST_F(BrowserWindowLayoutTest, TestYosemiteFullscreenTrafficLights) {
  ApplyStandardFullscreenLayoutParameters();
  [layout setOSYosemiteOrLater:YES];

  chrome::LayoutOutput output = [layout computeLayout];

  EXPECT_EQ(70, output.tabStripLayout.leadingIndent);
  EXPECT_TRUE(output.tabStripLayout.addCustomWindowControls);
}

TEST_F(BrowserWindowLayoutTest, TestAllViewsFullscreenMenuBarShowing) {
  ApplyStandardFullscreenLayoutParameters();
  [layout setFullscreenMenubarOffset:-10];

  chrome::LayoutOutput output = [layout computeLayout];

  EXPECT_NSEQ(NSMakeRect(0, 575, 600, 37), output.tabStripLayout.frame);
  EXPECT_NSEQ(NSMakeRect(533, 579, 63, 28), output.tabStripLayout.avatarFrame);
  EXPECT_EQ(0, output.tabStripLayout.leadingIndent);
  EXPECT_FALSE(output.tabStripLayout.addCustomWindowControls);
  EXPECT_EQ(67, output.tabStripLayout.trailingIndent);
  EXPECT_NSEQ(NSMakeRect(0, 543, 600, 32), output.toolbarFrame);
  EXPECT_NSEQ(NSMakeRect(0, 517, 600, 26), output.bookmarkFrame);
  EXPECT_NSEQ(NSMakeRect(0, 517, 600, 95), output.fullscreenBackingBarFrame);
  EXPECT_EQ(517, output.findBarMaxY);
  EXPECT_NSEQ(NSMakeRect(0, 445, 600, 72), output.infoBarFrame);
  EXPECT_NSEQ(NSMakeRect(0, 0, 600, 44), output.downloadShelfFrame);
  EXPECT_NSEQ(NSMakeRect(0, 44, 600, 411), output.contentAreaFrame);
}

TEST_F(BrowserWindowLayoutTest, TestPopupWindow) {
  [layout setHasTabStrip:NO];
  [layout setHasToolbar:NO];
  [layout setHasLocationBar:YES];
  [layout setBookmarkBarHidden:YES];
  [layout setHasDownloadShelf:NO];

  chrome::LayoutOutput output = [layout computeLayout];

  EXPECT_NSEQ(NSZeroRect, output.tabStripLayout.frame);
  EXPECT_NSEQ(NSZeroRect, output.tabStripLayout.avatarFrame);
  EXPECT_EQ(0, output.tabStripLayout.leadingIndent);
  EXPECT_EQ(0, output.tabStripLayout.trailingIndent);
  EXPECT_NSEQ(NSMakeRect(1, 568, 598, 32), output.toolbarFrame);
  EXPECT_NSEQ(NSZeroRect, output.bookmarkFrame);
  EXPECT_NSEQ(NSZeroRect, output.fullscreenBackingBarFrame);
  EXPECT_EQ(567, output.findBarMaxY);
  EXPECT_NSEQ(NSMakeRect(0, 495, 600, 72), output.infoBarFrame);
  EXPECT_NSEQ(NSZeroRect, output.downloadShelfFrame);
  EXPECT_NSEQ(NSMakeRect(0, 0, 600, 495), output.contentAreaFrame);
}

// Old style avatar button is on the right of the fullscreen button.
// The tab strip's right indent goes up to the left side of the fullscreen
// button.
TEST_F(BrowserWindowLayoutTest, TestOldStyleAvatarButton) {
  NSRect fullscreenButtonFrame = NSMakeRect(510, 596, 16, 17);
  [layout setFullscreenButtonFrame:fullscreenButtonFrame];
  [layout setShouldUseNewAvatar:NO];

  chrome::TabStripLayout tabStripLayout = [layout computeLayout].tabStripLayout;

  EXPECT_LE(NSMaxX(fullscreenButtonFrame), NSMinX(tabStripLayout.avatarFrame));
  EXPECT_EQ(NSWidth(tabStripLayout.frame) - NSMinX(fullscreenButtonFrame),
            tabStripLayout.trailingIndent);
}

// New style avatar button is on the left of the fullscreen button.
// The tab strip's right indent goes up to the left side of the avatar button.
TEST_F(BrowserWindowLayoutTest, TestNewStyleAvatarButton) {
  NSRect fullscreenButtonFrame = NSMakeRect(575, 596, 16, 17);
  [layout setFullscreenButtonFrame:fullscreenButtonFrame];
  [layout setShouldUseNewAvatar:YES];

  chrome::TabStripLayout tabStripLayout = [layout computeLayout].tabStripLayout;

  EXPECT_LE(NSMaxX(tabStripLayout.avatarFrame), NSMinX(fullscreenButtonFrame));
  EXPECT_EQ(NSWidth(tabStripLayout.frame) - NSMinX(tabStripLayout.avatarFrame),
            tabStripLayout.trailingIndent);
}

// There is no fullscreen button when in fullscreen mode.
// The tab strip's right indent goes up to the left side of the avatar
// button.
TEST_F(BrowserWindowLayoutTest, TestAvatarButtonFullscreen) {
  [layout setInAnyFullscreen:YES];
  [layout setFullscreenButtonFrame:NSZeroRect];

  [layout setShouldUseNewAvatar:YES];
  chrome::TabStripLayout tabStripLayout = [layout computeLayout].tabStripLayout;
  EXPECT_EQ(NSWidth(tabStripLayout.frame) - NSMinX(tabStripLayout.avatarFrame),
            tabStripLayout.trailingIndent);

  [layout setShouldUseNewAvatar:NO];
  tabStripLayout = [layout computeLayout].tabStripLayout;
  EXPECT_EQ(NSWidth(tabStripLayout.frame) - NSMinX(tabStripLayout.avatarFrame),
            tabStripLayout.trailingIndent);
}

TEST_F(BrowserWindowLayoutTest, TestInfobarLayoutWithoutToolbarOrLocationBar) {
  [layout setHasTabStrip:NO];
  [layout setHasToolbar:NO];
  [layout setHasLocationBar:NO];
  [layout setBookmarkBarHidden:YES];
  [layout setHasDownloadShelf:NO];

  chrome::LayoutOutput output = [layout computeLayout];

  EXPECT_NSEQ(NSMakeRect(0, 528, 600, 72), output.infoBarFrame);
}

// Tests that the avatar button is not aligned on the half pixel.
TEST_F(BrowserWindowLayoutTest, TestAvatarButtonPixelAlignment) {
  [layout setAvatarSize:NSMakeSize(28, 28)];

  chrome::LayoutOutput output = [layout computeLayout];

  EXPECT_NSEQ(NSMakeRect(537, 589, 28, 28), output.tabStripLayout.avatarFrame);
}

// Tests that the avatar button is aligned properly on the right. The generic
// avatar button should be aligned differently.
TEST_F(BrowserWindowLayoutTest, TestAvatarButtonAlignment) {
  [layout setShouldUseNewAvatar:YES];
  [layout setFullscreenButtonFrame:NSZeroRect];
  [layout setIsGenericAvatar:NO];

  chrome::LayoutOutput output = [layout computeLayout];
  EXPECT_EQ(596, NSMaxX(output.tabStripLayout.avatarFrame));

  [layout setIsGenericAvatar:YES];
  output = [layout computeLayout];
  EXPECT_EQ(595, NSMaxX(output.tabStripLayout.avatarFrame));
}
