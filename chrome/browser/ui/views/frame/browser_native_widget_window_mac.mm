// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/views/frame/browser_native_widget_window_mac.h"

#import <AppKit/AppKit.h>

namespace {
constexpr NSInteger kWindowButtonsOffsetFromBottom = 9;
constexpr NSInteger kWindowButtonsOffsetFromLeft = 11;
constexpr NSInteger kTitleBarHeight = 37;
}  // namespace

@interface NSWindow (PrivateAPI)
+ (Class)frameViewClassForStyleMask:(NSUInteger)windowStyle;
- (void)beginWindowDragWithEvent:(NSEvent*)event
    NS_DEPRECATED_MAC(10_10, 10_11, "Use performWindowDragWithEvent: instead.");
@end

// Weak lets Chrome launch even if a future macOS doesn't have NSThemeFrame.
WEAK_IMPORT_ATTRIBUTE
@interface NSThemeFrame : NSView
@end

@interface BrowserWindowFrame : NSThemeFrame
@end

@implementation BrowserWindowFrame

// NSThemeFrame overrides.

- (CGFloat)_minXTitlebarWidgetInset {
  return kWindowButtonsOffsetFromLeft;
}

- (CGFloat)_minYTitlebarButtonsOffset {
  return -kWindowButtonsOffsetFromBottom;
}

- (CGFloat)_titlebarHeight {
  return kTitleBarHeight;
}

// The base implementation justs tests [self class] == [NSThemeFrame class].
- (BOOL)_shouldFlipTrafficLightsForRTL API_AVAILABLE(macos(10.12)) {
  return [[self window] windowTitlebarLayoutDirection] ==
         NSUserInterfaceLayoutDirectionRightToLeft;
}

// Returning nil from _copyDragRegion prevents browser windows from being
// server-side draggable in the tab strip area. The area occupied by the title
// bar is normally draggable except where a view underlaps it which overrides
// -mouseDown: *and* returns YES from -acceptsFirstResponder. Currently, the
// tab strip is shown by a BridgedContentView which only sometimes returns YES
// from -acceptsFirstResponder. With this override, a window drag only starts
// after falling through -hitTest:.
//
// It would be ideal to avoid that round trip: right now, for example, browser
// windows aren't draggable while Chrome is hung or paused in a debugger.
// Another approach would be to expose an NSView for each views::View which
// exposes it to AppKit it as a non-draggable region. Cocoa app windows did
// this for draggable regions. (Tracked under https://crbug.com/830962.)
- (id)_copyDragRegion {
  return nil;
}

// Same as _copyDragRegion, but for 10.10.
- (NSRect)_draggableFrame NS_DEPRECATED_MAC(10_10, 10_11) {
  return NSZeroRect;
}

// Lets the window be dragged by its title bar on 10.11 and older.
- (void)mouseDown:(NSEvent*)event {
  if (@available(macOS 10.12, *))
    ;  // Not needed on 10.12 and up.
  else if (@available(macOS 10.11, *))
    [self.window performWindowDragWithEvent:event];
  else if (@available(macOS 10.10, *))
    [self.window beginWindowDragWithEvent:event];
  else
    NOTREACHED();
  [super mouseDown:event];
}

@end

@implementation BrowserNativeWidgetWindow

// NSWindow (PrivateAPI) overrides.

+ (Class)frameViewClassForStyleMask:(NSUInteger)windowStyle {
  // - NSThemeFrame and its subclasses will be nil if it's missing at runtime.
  if ([BrowserWindowFrame class]) {
    // TODO(crbug/825968): fullscreen should have a reduced titlebar height.
    return [BrowserWindowFrame class];
  }
  return [super frameViewClassForStyleMask:windowStyle];
}

// The base implementation returns YES if the window's frame view is a custom
// class, which causes undesirable changes in behavior. AppKit NSWindow
// subclasses are known to override it and return NO.
- (BOOL)_usesCustomDrawing {
  return NO;
}

@end
