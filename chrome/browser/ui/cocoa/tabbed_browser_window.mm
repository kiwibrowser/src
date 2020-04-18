// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/tabbed_browser_window.h"

#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/browser_window_layout.h"

// Implementer's note: Moving the window controls is tricky. When altering the
// code, ensure that:
// - accessibility hit testing works
// - the accessibility hierarchy is correct
// - close/min in the background don't bring the window forward
// - rollover effects work correctly

namespace {
// Size of the gradient. Empirically determined so that the gradient looks
// like what the heuristic does when there are just a few tabs.
constexpr CGFloat kWindowGradientHeight = 24.0;

// Offsets from the bottom/left of the titlebar to the bottom/left of the
// window buttons (zoom, close, miniaturize).
constexpr NSInteger kWindowButtonsOffsetFromBottom = 9;
constexpr NSInteger kWindowButtonsOffsetFromLeft = 11;
}  // namespace

@interface TabbedBrowserWindow ()
- (CGFloat)fullScreenButtonOriginAdjustment;
@end

// Weak so that Chrome will launch if a future macOS doesn't have NSThemeFrame.
WEAK_IMPORT_ATTRIBUTE
@interface NSThemeFrame : NSView
- (NSView*)fullScreenButton
    __attribute__((availability(macos, obsoleted = 10.10)));
@end

@interface NSWindow (PrivateAPI)
+ (Class)frameViewClassForStyleMask:(NSUInteger)windowStyle;
@end

@interface NSWindow (TenTwelveSDK)
@property(readonly)
    NSUserInterfaceLayoutDirection windowTitlebarLayoutDirection;
@end

@interface TabbedBrowserWindowFrame : NSThemeFrame
@end

@implementation TabbedBrowserWindowFrame

// NSThemeFrame overrides.

- (CGFloat)_minXTitlebarWidgetInset {
  return kWindowButtonsOffsetFromLeft;
}

- (CGFloat)_minYTitlebarButtonsOffset {
  return -kWindowButtonsOffsetFromBottom;
}

- (CGFloat)_titlebarHeight {
  return chrome::kTabStripHeight;
}

// AppKit's implementation only returns YES if [self class] == [NSThemeFrame
// class]. TabbedBrowserWindowFrame could override -class to return that, but
// then -[NSWindow setStyleMask:] would recreate the frame view each time the
// style mask is touched because it wouldn't match the return value of
// +[TabbedBrowserWindow frameViewClassForStyleMask:].
- (BOOL)_shouldFlipTrafficLightsForRTL API_AVAILABLE(macos(10.12)) {
  return [[self window] windowTitlebarLayoutDirection] ==
         NSUserInterfaceLayoutDirectionRightToLeft;
}

@end

// By default, contentView does not occupy the full size of a titled window,
// and Chrome wants to draw in the title bar. Historically, Chrome did this by
// adding subviews directly to the root view. This causes several problems. The
// most egregious is related to layer ordering when the root view does not have
// a layer. By giving the contentView the same size as the window, there is no
// need to add subviews to the root view.
//
// TODO(sdy): This can be deleted once ShouldUseFullSizeContentView is
// perma-on. See https://crbug.com/605219.
@interface FullSizeTabbedBrowserWindowFrame : TabbedBrowserWindowFrame
@end

@implementation FullSizeTabbedBrowserWindowFrame

+ (CGRect)contentRectForFrameRect:(CGRect)frameRect
                        styleMask:(NSUInteger)style {
  return frameRect;
}

+ (CGRect)frameRectForContentRect:(CGRect)contentRect
                        styleMask:(NSUInteger)style {
  return contentRect;
}

- (CGRect)contentRectForFrameRect:(CGRect)frameRect
                        styleMask:(NSUInteger)style {
  return frameRect;
}

- (CGRect)frameRectForContentRect:(CGRect)contentRect
                        styleMask:(NSUInteger)style {
  return contentRect;
}

@end

@implementation TabbedBrowserWindow

// FramedBrowserWindow overrides.

+ (NSUInteger)defaultStyleMask {
  NSUInteger styleMask = [super defaultStyleMask];
  if (chrome::ShouldUseFullSizeContentView()) {
    if (@available(macOS 10.10, *))
      styleMask |= NSFullSizeContentViewWindowMask;
  }
  return styleMask;
}

// NSWindow (PrivateAPI) overrides.

+ (Class)frameViewClassForStyleMask:(NSUInteger)windowStyle {
  // Because NSThemeFrame is imported weakly, if it's not present at runtime
  // then it and its subclasses will be nil.
  if ([TabbedBrowserWindowFrame class]) {
    return chrome::ShouldUseFullSizeContentView()
               ? [TabbedBrowserWindowFrame class]
               : [FullSizeTabbedBrowserWindowFrame class];
  }
  return [super frameViewClassForStyleMask:windowStyle];
}

// NSWindow's implementation of _usesCustomDrawing returns YES when the window
// has a custom frame view class, which causes several undesirable changes in
// AppKit's behavior. NSWindow subclasses in AppKit override it and return NO.
- (BOOL)_usesCustomDrawing {
  return NO;
}

// FramedBrowserWindow overrides.

- (id)initWithContentRect:(NSRect)contentRect {
  if ((self = [super initWithContentRect:contentRect])) {
    // The following two calls fix http://crbug.com/25684 by preventing the
    // window from recalculating the border thickness as the window is
    // resized.
    // This was causing the window tint to change for the default system theme
    // when the window was being resized.
    [self setAutorecalculatesContentBorderThickness:NO forEdge:NSMaxYEdge];
    [self setContentBorderThickness:kWindowGradientHeight forEdge:NSMaxYEdge];
  }
  return self;
}

// TabbedBrowserWindow () implementation.

- (CGFloat)fullScreenButtonOriginAdjustment {
  // If there is a profile avatar icon present, shift the button over by its
  // width and some padding. The new avatar button is displayed to the right
  // of the fullscreen icon, so it doesn't need to be shifted.
  BrowserWindowController* bwc =
      [BrowserWindowController browserWindowControllerForWindow:self];
  if ([bwc shouldShowAvatar] && ![bwc shouldUseNewAvatarButton]) {
    NSView* avatarButton = [[bwc avatarButtonController] view];
    return NSWidth([avatarButton frame]) - 3;
  }
  return 0;
}

@end
