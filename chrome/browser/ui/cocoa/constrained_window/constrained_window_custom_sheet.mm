// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_custom_sheet.h"

#import <IOKit/IOKitLib.h>

#include "base/mac/mach_logging.h"
#include "base/mac/scoped_ioobject.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_sheet_controller.h"
#import "ui/base/cocoa/constrained_window/constrained_window_animation.h"

namespace {

// Length of the animation in seconds.
const NSTimeInterval kAnimationDuration = 0.18;

// If the machine has a DisplayLink device attached, the WindowServer has a
// tendency to crash https://crbug.com/755516. To avoid this, probe the
// IOKit registry for a DisplayLink service, and turn on useSimpleAnimations_
// if one is found. The result of this is not cached, in case the hardware
// is attached after the first ConstrainedWindow is shown.
bool HasDisplayLinkDevice() {
  // The |matching| reference is consumed by IOServiceGetMatchingServices().
  CFDictionaryRef matching = IOServiceMatching("DisplayLinkFramebuffer");
  base::mac::ScopedIOObject<io_iterator_t> it;
  kern_return_t kr = IOServiceGetMatchingServices(
      kIOMasterPortDefault, matching, it.InitializeInto());
  if (kr != KERN_SUCCESS) {
    MACH_LOG(ERROR, kr) << "IOServiceGetMatchingServices";
    return false;
  }

  base::mac::ScopedIOObject<io_object_t> object(IOIteratorNext(it));
  return object != IO_OBJECT_NULL;
}

}  // namespace

@implementation CustomConstrainedWindowSheet

- (id)initWithCustomWindow:(NSWindow*)customWindow {
  if ((self = [super init])) {
    customWindow_.reset([customWindow retain]);
    useSimpleAnimations_ = HasDisplayLinkDevice();
  }
  return self;
}

- (void)setUseSimpleAnimations:(BOOL)simpleAnimations {
  useSimpleAnimations_ = simpleAnimations;
}

- (void)showSheetForWindow:(NSWindow*)window {
  base::scoped_nsobject<NSAnimation> animation;
  if (useSimpleAnimations_) {
    [customWindow_ setAlphaValue:0.0];
  } else {
    animation.reset(
        [[ConstrainedWindowAnimationShow alloc] initWithWindow:customWindow_]);
  }
  [window addChildWindow:customWindow_
                 ordered:NSWindowAbove];
  [self unhideSheet];
  [self updateSheetPosition];
  [customWindow_ makeKeyAndOrderFront:nil];

  if (useSimpleAnimations_) {
    [NSAnimationContext beginGrouping];
    [[NSAnimationContext currentContext] setDuration:kAnimationDuration];
    [[customWindow_ animator] setAlphaValue:1.0];
    [NSAnimationContext endGrouping];
  } else {
    [animation startAnimation];
  }
}

- (void)closeSheetWithAnimation:(BOOL)withAnimation {
  if (withAnimation) {
    base::scoped_nsobject<NSAnimation> animation;
    if (useSimpleAnimations_) {
      // Use a blocking animation, rather than -[NSWindow animator].
      NSArray* animationArray = @[ @{
        NSViewAnimationTargetKey : customWindow_,
        NSViewAnimationEffectKey : NSViewAnimationFadeOutEffect,
      } ];
      animation.reset(
          [[NSViewAnimation alloc] initWithViewAnimations:animationArray]);
      [animation setDuration:kAnimationDuration];
      [animation setAnimationBlockingMode:NSAnimationBlocking];
    } else {
      animation.reset([[ConstrainedWindowAnimationHide alloc]
          initWithWindow:customWindow_]);
    }
    [animation startAnimation];
  }

  [[customWindow_ parentWindow] removeChildWindow:customWindow_];
  [customWindow_ close];
}

- (void)hideSheet {
  // Hide the sheet window, and any of its direct child windows, by setting the
  // alpha to 0. This technique is used instead of -orderOut: because that may
  // cause a Spaces change or window ordering change.
  [customWindow_ setAlphaValue:0.0];
  for (NSWindow* window : [customWindow_ childWindows]) {
    [window setAlphaValue:0.0];
  }
}

- (void)unhideSheet {
  [customWindow_ setAlphaValue:1.0];
  for (NSWindow* window : [customWindow_ childWindows]) {
    [window setAlphaValue:1.0];
  }
}

- (void)pulseSheet {
  base::scoped_nsobject<NSAnimation> animation(
      [[ConstrainedWindowAnimationPulse alloc] initWithWindow:customWindow_]);
  [animation startAnimation];
}

- (void)makeSheetKeyAndOrderFront {
  [customWindow_ makeKeyAndOrderFront:nil];
}

- (void)updateSheetPosition {
  ConstrainedWindowSheetController* controller =
      [ConstrainedWindowSheetController controllerForSheet:self];
  NSPoint origin = [controller originForSheet:self
                               withWindowSize:[customWindow_ frame].size];
  [customWindow_ setFrameOrigin:origin];
}

- (void)resizeWithNewSize:(NSSize)size {
  // NOOP
}

- (NSWindow*)sheetWindow {
  return customWindow_;
}

@end
