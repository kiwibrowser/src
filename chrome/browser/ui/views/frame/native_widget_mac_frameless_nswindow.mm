// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/views/frame/native_widget_mac_frameless_nswindow.h"

@interface NSWindow (PrivateAPI)
+ (Class)frameViewClassForStyleMask:(NSUInteger)windowStyle;
- (void)beginWindowDragWithEvent:(NSEvent*)event
    NS_DEPRECATED_MAC(10_10, 10_11, "Use performWindowDragWithEvent: instead.");
@end

// Weak lets Chrome launch even if a future macOS doesn't have NSThemeFrame.
WEAK_IMPORT_ATTRIBUTE
@interface NSThemeFrame : NSView
@end

@interface NativeWidgetMacFramelessNSWindowFrame : NSThemeFrame
@end

@implementation NativeWidgetMacFramelessNSWindowFrame

// If a mouseDown: falls through to the frame view, turn it into a window drag.
- (void)mouseDown:(NSEvent*)event {
  if (@available(macOS 10.11, *))
    [self.window performWindowDragWithEvent:event];
  else if (@available(macOS 10.10, *))
    [self.window beginWindowDragWithEvent:event];
  else
    NOTREACHED();
  [super mouseDown:event];
}

- (BOOL)_hidingTitlebar {
  return YES;
}
@end

@implementation NativeWidgetMacFramelessNSWindow

+ (Class)frameViewClassForStyleMask:(NSUInteger)windowStyle {
  if ([NativeWidgetMacFramelessNSWindowFrame class]) {
    return [NativeWidgetMacFramelessNSWindowFrame class];
  }
  return [super frameViewClassForStyleMask:windowStyle];
}

- (BOOL)_usesCustomDrawing {
  return NO;
}

@end
