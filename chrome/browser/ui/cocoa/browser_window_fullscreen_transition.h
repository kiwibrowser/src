// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_FULLSCREEN_TRANSITION_H_
#define CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_FULLSCREEN_TRANSITION_H_

#import <Cocoa/Cocoa.h>

@class BrowserWindowController;

// This class is responsible for managing the custom transition of a
// BrowserWindow from its normal state into an AppKit Fullscreen state
// and vice versa.
//
// By default, when AppKit Fullscreens a window, it creates a new virtual
// desktop and slides it in from the right of the screen. At the same time, the
// old virtual desktop slides off to the left. This animation takes one second,
// and the time is not customizable without elevated privileges or a
// self-modifying binary
// (https://code.google.com/p/chromium/issues/detail?id=414527). During that
// second, no user interaction is possible.
//
// The default implementation of the AppKit transition smoothly animates a
// window from its original size to the expected size. At the
// beginning of the animation, it takes a snapshot of the window's current
// state. Then it resizes the window, calls drawRect: (theorized, not tested),
// and takes a snapshot of the window's final state. The animation is a simple
// crossfade between the two snapshots. This has a flaw. Frequently, the
// renderer has not yet drawn content for the resized window by the time
// drawRect: is called. As a result, the animation is effectively a no-op. When
// the animation is finished, the new web content flashes in.
//
// The window's delegate can override four methods to customize the transition.
//  -customWindowsToEnterFullScreenForWindow:
//    The return of this method is an array of NSWindows. Each window that is
//    returned will be added to the new virtual desktop after the animation is
//    finished, but will not be a part of the animation itself.
//  -window:startCustomAnimationToEnterFullScreenWithDuration:
//    In this method, the window's delegate adds animations to the windows
//    returned in the above method.
//  -customWindowsToExitFullScreenForWindow:
//    This method is similar to customWindowsToEnterFullScreenForWindow, but
//    will be used for exiting full screen
//  -window:startCustomAnimationToExitFullScreenWithDuration:
//    In this method, the window's delegate adds animations to the windows
//    returned in the above method.
//
// The goal of this class is to mimic the default animation, but instead of
// taking a snapshot of the final web content, it uses the live web content
// during the animation.
//
// See https://code.google.com/p/chromium/issues/detail?id=414527#c22 and its
// preceding comments for a more detailed description of the implementation,
// and the reasoning behind the decisions made.
//
// Recommended usage for entering full screen:
//  (Override method on NSWindow's delegate):
//  - (NSArray*)customWindowsToEnterFullScreenForWindow:(NSWindow*)window {
//    self.transition = [[[BrowserWindowEnterFullscreenTransition alloc]
//        initEnterWithWindow:window] autorelease];
//    return [self.transition customWindowsForFullScreen];
//  }
//
//  (Override method on NSWindow's delegate)
//  - (void)window:(NSWindow*)window
//  startCustomAnimationToEnterFullScreenWithDuration:(NSTimeInterval)duration {
//    [self.transition startCustomFullScreenAnimationWithDuration:duration];
//  }
//
//  (Override method on NSWindow's delegate)
//  - (void)windowDidEnterFullScreen:(NSNotification*)notification {
//    self.transition = nil;
//  }
//
//  (Override method on NSWindow)
//  - (NSRect)constrainFrameRect:(NSRect)frame toScreen:(NSScreen*)screen {
//    if (self.transition && ![self.transition shouldWindowBeConstrained])
//      return frame;
//    return [super constrainFrameRect:frame toScreen:screen];
//  }
//
//  For exiting fullscreen, you should do the same as above, but you must
//  override following methods instead.
//      -customWindowsToExitFullScreenForWindow:,
//      -startCustomAnimationToEnterFullScreenWithDuration:
//      -windowDidExitFullScreen:
//  In addition, you should use initExitWithWindow:frame: instead of
//  initEnterWithWindow:. For the frame parameter, you should pass the expected
//  frame of the window at the end of the transition. If you want the window to
//  resize and move to the frame it had before entering fullscreen, you will be
//  responsible for saving the value of the frame and passing it to the
//  parameter.

@interface BrowserWindowFullscreenTransition : NSObject

// Designated initializers. |controller| is the BrowserWindowController of the
// window that's going to be moved into a fullscreen Space.
- (instancetype)initEnterWithController:(BrowserWindowController*)controller;
- (instancetype)initExitWithController:(BrowserWindowController*)controller;

// Returns the windows to be used in the custom fullscreen transition.
- (NSArray*)customWindowsForFullScreenTransition;

// Returns true if the fullscreen transition is completed.
- (BOOL)isTransitionCompleted;

// This method begins animation for exit or enter fullscreen transition.
// In this method, the following happens:
//   - Animates the snapshot to the expected final size of the window while
//   fading it out.
//   - Animates the current window from its original to final location and size
//   while fading it in.
// If the transition is for exiting fullscreen, we would shrink the content view
// to the expected final size so that we can to avoid clipping from the
// window.
// Note: The two animations are added to different layers in different windows.
// There is no explicit logic to keep the two animations in sync. If this
// proves to be a problem, the relevant layers should attempt to sync up their
// time offsets with CACurrentMediaTime().
- (void)startCustomFullScreenAnimationWithDuration:(NSTimeInterval)duration;

// When this method returns true, the NSWindow method
// -constrainFrameRect:toScreen: must return the frame rect without
// constraining it. The owner of the instance of this class is responsible for
// hooking up this logic.
- (BOOL)shouldWindowBeUnconstrained;

// Returns the size of the window we expect the BrowserWindowLayout to have.
// During the exit fullscreen transition, the content size shrinks while the
// window frame stays the same. When that happens, we want to set the
// BrowserWindowLayout's window parameter to the content size instead of the
// actual window's size.
- (NSSize)desiredWindowLayoutSize;

// Called when the browser is destroyed.
- (void)browserWillBeDestroyed;

@end

#endif  // CHROME_BROWSER_UI_COCOA_BROWSER_WINDOW_FULLSCREEN_TRANSITION_H_
