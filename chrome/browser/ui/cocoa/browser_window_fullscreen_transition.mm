// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser_window_fullscreen_transition.h"

#include <QuartzCore/QuartzCore.h>

#include <memory>

#include "base/mac/foundation_util.h"
#include "base/mac/mac_util.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/macros.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/framed_browser_window.h"
#import "chrome/browser/ui/cocoa/tabs/tab_strip_background_view.h"

namespace {

NSString* const kPrimaryWindowAnimationID = @"PrimaryWindowAnimationID";
NSString* const kSnapshotWindowAnimationID = @"SnapshotWindowAnimationID";
NSString* const kAnimationIDKey = @"AnimationIDKey";

// The fraction of the duration from AppKit's startCustomAnimation methods
// that we want our animation to run in. Yosemite's fraction is smaller
// since its fullscreen transition is significantly slower.
CGFloat const kAnimationDurationFraction = 0.5;
CGFloat const kAnimationDurationFractionYosemite = 0.3;

// This class has two simultaneous animations to resize and reposition layers.
// These animations must use the same timing function, otherwise there will be
// visual discordance.
NSString* TransformAnimationTimingFunction() {
  return kCAMediaTimingFunctionEaseInEaseOut;
}

// This class locks and unlocks the FrameBrowserWindow. Its destructor ensures
// that the lock gets released.
class FrameAndStyleLock {
 public:
  explicit FrameAndStyleLock(FramedBrowserWindow* window) : window_(window) {}

  ~FrameAndStyleLock() { set_lock(NO); }

  void set_lock(bool lock) { [window_ setStyleMaskLock:lock]; }

 private:
  FramedBrowserWindow* window_;  // weak

  DISALLOW_COPY_AND_ASSIGN(FrameAndStyleLock);
};

}  // namespace

// This view draws a dummy toolbar over the resized content view during
// the exit fullscreen animation. It is removed at the end of the animation.
@interface FullscreenTabStripBackgroundView : NSView {
  base::scoped_nsobject<NSColor> windowBackgroundColor_;
}

- (instancetype)initWithFrame:(NSRect)frame background:(NSColor*)color;

@end

@implementation FullscreenTabStripBackgroundView

- (instancetype)initWithFrame:(NSRect)frame background:(NSColor*)color {
  if ((self = [super initWithFrame:frame])) {
    windowBackgroundColor_.reset([color copy]);
  }
  return self;
}

// Override this method so that we can paint the toolbar in this view.
// This method first fill itself with the toolbar's background. After that,
// it will paint the window's theme if applicable.
- (void)drawRect:(NSRect)frame {
  [windowBackgroundColor_ set];
  NSRectFillUsingOperation(frame, NSCompositeDestinationOver);

  [FramedBrowserWindow drawWindowThemeInDirtyRect:frame
                                          forView:self
                                           bounds:[self bounds]
                             forceBlackBackground:NO];
}

@end

@interface BrowserWindowFullscreenTransition ()
    <CAAnimationDelegate, CALayerDelegate> {
  // Flag to keep track of whether we are entering or exiting fullscreen.
  BOOL isEnteringFullscreen_;

  // The window which is undergoing the fullscreen transition.
  base::scoped_nsobject<FramedBrowserWindow> primaryWindow_;

  // The window which is undergoing the fullscreen transition.
  BrowserWindowController* controller_; // weak

  // A layer that holds a snapshot of the original state of |primaryWindow_|.
  base::scoped_nsobject<CALayer> snapshotLayer_;

  // A temporary window that holds |snapshotLayer_|.
  base::scoped_nsobject<NSWindow> snapshotWindow_;

  // The tabstrip background view in the window. During the exit fullscreen
  // animation, this view be hidden while a dummy tabstrip background will be
  // drawn over the content view.
  base::scoped_nsobject<NSView> tabStripBackgroundView_;

  // The background color of |primaryWindow_| before the transition began.
  base::scoped_nsobject<NSColor> primaryWindowInitialBackgroundColor_;

  // Whether |primaryWindow_| was opaque before the transition began.
  BOOL primaryWindowInitialOpaque_;

  // The initial anchor point of the root layer.
  CGPoint initialRootAnchorPoint_;

  // The initial origin of the content view.
  NSPoint initialContentViewOrigin_;

  // Whether the instance is in the process of changing the size of
  // |primaryWindow_|.
  BOOL changingPrimaryWindowSize_;

  // The frame of the |primaryWindow_| before it starts the transition.
  NSRect initialFrame_;

  // The frame that |primaryWindow_| is expected to have after the transition
  // is finished.
  NSRect finalFrame_;

  // This view draws the tabstrip background during the exit animation.
  base::scoped_nsobject<FullscreenTabStripBackgroundView>
      fullscreenTabStripBackgroundView_;

  // Locks and unlocks the FullSizeContentWindow.
  std::unique_ptr<FrameAndStyleLock> lock_;

  // Flag that indicates if the animation was completed. Sets to true at the
  // end of the animation.
  BOOL completedTransition_;
}

// Takes a snapshot of |primaryWindow_| and puts it in |snapshotLayer_|.
- (void)takeSnapshot;

// Creates |snapshotWindow_| and adds |snapshotLayer_| to it.
- (void)makeAndPrepareSnapshotWindow;

// This method has several effects on |primaryWindow_|:
//  - Saves current state.
//  - Makes window transparent, with clear background.
//  - If we are entering fullscreen, it will also:
//    - Add NSFullScreenWindowMask style mask.
//    - Set the size to the screen's size.
- (void)preparePrimaryWindowForAnimation;

// Applies the fullscreen animation to |snapshotLayer_|.
- (void)animateSnapshotWindowWithDuration:(CGFloat)duration;

// Sets |primaryWindow_|'s frame to the expected frame.
- (void)changePrimaryWindowToFinalFrame;

// Overrides of CAAnimation delegate methods.
- (void)animationDidStart:(CAAnimation*)theAnimation;
- (void)animationDidStop:(CAAnimation*)theAnimation finished:(BOOL)finished;

// Returns the layer of the root view of |window|.
- (CALayer*)rootLayerOfWindow:(NSWindow*)window;

// Convert the point to be relative to the screen the primary window is on.
// This is important because if we're using multiple screens, the coordinate
// system extends to the second screen.
//
// For example, if the screen width is 1440, the second screen's frame origin
// is located at (1440, 0) and any x coordinate on the second screen will be
// >= 1440. If we move a window on the first screen to the same location on
// second screen, the window's frame origin will change from (x, y) to
// (x + 1440, y).
//
// When we animate the window, we want to use (x, y), the coordinates that are
// relative to the second screen. As a result, we use this method to convert a
// NSPoint so that it's relative to the screen it's on.
- (NSPoint)pointRelativeToCurrentScreen:(NSPoint)point;

@end

@implementation BrowserWindowFullscreenTransition

// -------------------------Public Methods----------------------------

- (instancetype)initEnterWithController:(BrowserWindowController*)controller {
  DCHECK(controller);
  DCHECK([self rootLayerOfWindow:[controller window]]);
  if ((self = [super init])) {
    controller_ = controller;
    FramedBrowserWindow* framedBrowserWindow =
        base::mac::ObjCCast<FramedBrowserWindow>([controller window]);
    primaryWindow_.reset([framedBrowserWindow retain]);

    isEnteringFullscreen_ = YES;
    initialFrame_ = [primaryWindow_ frame];
    finalFrame_ = [[primaryWindow_ screen] frame];
  }
  return self;
}

- (instancetype)initExitWithController:(BrowserWindowController*)controller {
  DCHECK(controller);
  DCHECK([self rootLayerOfWindow:[controller window]]);
  if ((self = [super init])) {
    controller_ = controller;
    FramedBrowserWindow* framedBrowserWindow =
        base::mac::ObjCCast<FramedBrowserWindow>([controller window]);
    primaryWindow_.reset([framedBrowserWindow retain]);

    isEnteringFullscreen_ = NO;
    initialFrame_ = [[primaryWindow_ screen] frame];
    finalFrame_ = [controller savedRegularWindowFrame];
    tabStripBackgroundView_.reset([[controller tabStripBackgroundView] retain]);

    lock_.reset(new FrameAndStyleLock(framedBrowserWindow));
  }
  return self;
}

- (NSArray*)customWindowsForFullScreenTransition {
  [self takeSnapshot];
  [self makeAndPrepareSnapshotWindow];
  return @[ primaryWindow_.get(), snapshotWindow_.get() ];
}

- (BOOL)isTransitionCompleted {
  return completedTransition_;
}

- (void)startCustomFullScreenAnimationWithDuration:(NSTimeInterval)duration {
  CGFloat durationFraction = base::mac::IsOS10_10()
                                 ? kAnimationDurationFractionYosemite
                                 : kAnimationDurationFraction;
  CGFloat animationDuration = duration * durationFraction;
  [self preparePrimaryWindowForAnimation];
  [self animatePrimaryWindowWithDuration:animationDuration];
  [self animateSnapshotWindowWithDuration:animationDuration];
}

- (BOOL)shouldWindowBeUnconstrained {
  return changingPrimaryWindowSize_;
}

- (NSSize)desiredWindowLayoutSize {
  return isEnteringFullscreen_ ? [primaryWindow_ frame].size
                               : [[primaryWindow_ contentView] bounds].size;
}

- (void)browserWillBeDestroyed {
  CALayer* root = [self rootLayerOfWindow:primaryWindow_];
  [root removeAllAnimations];
  [snapshotLayer_ removeAllAnimations];

  controller_ = nil;
}

// -------------------------Private Methods----------------------------

- (void)takeSnapshot {
  base::ScopedCFTypeRef<CGImageRef> windowSnapshot(CGWindowListCreateImage(
      CGRectNull, kCGWindowListOptionIncludingWindow,
      [primaryWindow_ windowNumber], kCGWindowImageBoundsIgnoreFraming));
  snapshotLayer_.reset([[CALayer alloc] init]);
  [snapshotLayer_ setFrame:NSRectToCGRect([primaryWindow_ frame])];
  [snapshotLayer_ setContents:static_cast<id>(windowSnapshot.get())];
  [snapshotLayer_ setAnchorPoint:CGPointMake(0, 0)];
  CGColorRef colorRef = CGColorCreateGenericRGB(0, 0, 0, 0);
  [snapshotLayer_ setBackgroundColor:colorRef];
  CGColorRelease(colorRef);
}

- (void)makeAndPrepareSnapshotWindow {
  DCHECK(snapshotLayer_);

  snapshotWindow_.reset([[NSWindow alloc]
      initWithContentRect:[[primaryWindow_ screen] frame]
                styleMask:0
                  backing:NSBackingStoreBuffered
                    defer:NO]);
  [[snapshotWindow_ contentView] setWantsLayer:YES];
  [snapshotWindow_ setOpaque:NO];
  [snapshotWindow_ setBackgroundColor:[NSColor clearColor]];
  [snapshotWindow_ setAnimationBehavior:NSWindowAnimationBehaviorNone];

  [[[snapshotWindow_ contentView] layer] addSublayer:snapshotLayer_];

  // Compute the frame of the snapshot layer such that the snapshot is
  // positioned exactly on top of the original position of |primaryWindow_|.
  NSRect snapshotLayerFrame =
      [snapshotWindow_ convertRectFromScreen:[primaryWindow_ frame]];
  [snapshotLayer_ setFrame:snapshotLayerFrame];

  // If the primary window is in fullscreen mode, we can't move the snapshot
  // window in front of it. As a result, at the beginning of the transition to
  // exit fullscreen, we should order the snapshot window to the front ASAP.
  if (isEnteringFullscreen_)
    [snapshotWindow_ orderFront:nil];
}

- (void)preparePrimaryWindowForAnimation {
  // Save the initial state of the primary window.
  primaryWindowInitialBackgroundColor_.reset(
      [[primaryWindow_ backgroundColor] copy]);
  primaryWindowInitialOpaque_ = [primaryWindow_ isOpaque];

  // Make |primaryWindow_| invisible. This must happen before the window is
  // resized, since resizing the window will call drawRect: and cause content
  // to flash over the entire screen.
  [primaryWindow_ setOpaque:NO];

  if (isEnteringFullscreen_) {
    // As soon as the style mask includes the flag NSFullScreenWindowMask, the
    // window is expected to receive fullscreen layout. This must be set before
    // the window is resized, as that causes a relayout.
    CALayer* root = [self rootLayerOfWindow:primaryWindow_];
    root.opacity = 0;

    [primaryWindow_
        setStyleMask:[primaryWindow_ styleMask] | NSFullScreenWindowMask];
    [self changePrimaryWindowToFinalFrame];
  } else {
    [snapshotWindow_ orderFront:nil];

    NSView* contentView = [primaryWindow_ contentView];
    NSView* rootView = [contentView superview];

    // Since only the content view is resized, the window's background
    // must be transparent. This is a hack that forces the layer to remove
    // the textured background and replace it with clearColor.
    [rootView setWantsLayer:NO];
    [primaryWindow_ setBackgroundColor:[NSColor clearColor]];
    [primaryWindow_ setStyleMask:[primaryWindow_ styleMask] &
                                 ~NSTexturedBackgroundWindowMask];
    [rootView setWantsLayer:YES];

    CALayer* root = [self rootLayerOfWindow:primaryWindow_];
    root.opacity = 0;

    // Right before the animation begins, change the contentView size to the
    // expected size at the end of the animation. Afterwards, lock the
    // |primaryWindow_| so that AppKit will not be able to make unwanted
    // changes to it during the animation.
    initialContentViewOrigin_ = [[primaryWindow_ contentView] frame].origin;
    initialRootAnchorPoint_ = root.anchorPoint;

    NSPoint contentViewOrigin =
        [self pointRelativeToCurrentScreen:finalFrame_.origin];
    NSRect relativeContentFinalFrame =
        NSMakeRect(contentViewOrigin.x, contentViewOrigin.y,
                   finalFrame_.size.width, finalFrame_.size.height);
    [[primaryWindow_ contentView] setFrame:relativeContentFinalFrame];

    fullscreenTabStripBackgroundView_.reset(
        [[FullscreenTabStripBackgroundView alloc]
            initWithFrame:finalFrame_
               background:primaryWindowInitialBackgroundColor_]);
    [fullscreenTabStripBackgroundView_ setFrameOrigin:NSZeroPoint];
    [contentView addSubview:fullscreenTabStripBackgroundView_.get()
                 positioned:NSWindowBelow
                 relativeTo:nil];

    [tabStripBackgroundView_ setHidden:YES];

    // Set anchor point to be the center of the content view
    CGFloat anchorPointX =
        NSMidX(relativeContentFinalFrame) / NSWidth(initialFrame_);
    CGFloat anchorPointY =
        NSMidY(relativeContentFinalFrame) / NSHeight(initialFrame_);
    root.anchorPoint = CGPointMake(anchorPointX, anchorPointY);

    lock_->set_lock(YES);
  }
}

- (void)animateSnapshotWindowWithDuration:(CGFloat)duration {
  // Calculate the frame so that it's relative to the screen.
  NSRect finalFrameRelativeToScreen =
      [snapshotWindow_ convertRectFromScreen:finalFrame_];

  // Move the snapshot layer until it's bottom-left corner is at the the
  // bottom-left corner of the expected frame.
  CABasicAnimation* positionAnimation =
      [CABasicAnimation animationWithKeyPath:@"position"];
  positionAnimation.toValue =
      [NSValue valueWithPoint:finalFrameRelativeToScreen.origin];
  positionAnimation.timingFunction = [CAMediaTimingFunction
      functionWithName:TransformAnimationTimingFunction()];

  // Resize the bounds until it reaches the expected size at the end of the
  // animation.
  NSRect finalBounds =
      NSMakeRect(0, 0, NSWidth(finalFrame_), NSHeight(finalFrame_));
  CABasicAnimation* boundsAnimation =
      [CABasicAnimation animationWithKeyPath:@"bounds"];
  boundsAnimation.toValue = [NSValue valueWithRect:finalBounds];
  boundsAnimation.timingFunction = [CAMediaTimingFunction
      functionWithName:TransformAnimationTimingFunction()];

  // Fade out the snapshot layer.
  CABasicAnimation* opacityAnimation =
      [CABasicAnimation animationWithKeyPath:@"opacity"];
  opacityAnimation.toValue = @(0.0);
  opacityAnimation.timingFunction =
      [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];

  // Fill forwards, and don't remove the animation. When the animation
  // completes, the entire window will be removed.
  CAAnimationGroup* group = [CAAnimationGroup animation];
  group.removedOnCompletion = NO;
  group.fillMode = kCAFillModeForwards;
  group.animations = @[ positionAnimation, boundsAnimation, opacityAnimation ];
  group.duration = duration;
  [group setValue:kSnapshotWindowAnimationID forKey:kAnimationIDKey];
  group.delegate = self;

  [snapshotLayer_ addAnimation:group forKey:nil];
}

- (void)animatePrimaryWindowWithDuration:(CGFloat)duration {
  // As soon as the window's root layer is scaled down, the opacity should be
  // set back to 1. There are a couple of ways to do this. The easiest is to
  // just have a dummy animation as part of the same animation group.
  CABasicAnimation* opacityAnimation =
      [CABasicAnimation animationWithKeyPath:@"opacity"];
  opacityAnimation.fromValue = @(1.0);
  opacityAnimation.toValue = @(1.0);

  // The root layer's size should start scaled down to the initial size of
  // |primaryWindow_|. The animation increases the size until the root layer
  // fills the screen.
  NSRect initialFrame = initialFrame_;
  NSRect endFrame = finalFrame_;
  CGFloat xScale = NSWidth(initialFrame) / NSWidth(endFrame);
  CGFloat yScale = NSHeight(initialFrame) / NSHeight(endFrame);
  CATransform3D initial = CATransform3DMakeScale(xScale, yScale, 1);
  CABasicAnimation* transformAnimation =
      [CABasicAnimation animationWithKeyPath:@"transform"];
  transformAnimation.fromValue = [NSValue valueWithCATransform3D:initial];

  // Animate the primary window from its initial position, the center of the
  // initial window.
  CABasicAnimation* positionAnimation =
      [CABasicAnimation animationWithKeyPath:@"position"];
  NSPoint centerOfInitialFrame =
      NSMakePoint(NSMidX(initialFrame), NSMidY(initialFrame));
  NSPoint startingLayerPoint =
      [self pointRelativeToCurrentScreen:centerOfInitialFrame];
  positionAnimation.fromValue = [NSValue valueWithPoint:startingLayerPoint];

  NSPoint endingLayerPoint =
      [self pointRelativeToCurrentScreen:NSMakePoint(NSMidX(endFrame),
                                                     NSMidY(endFrame))];
  positionAnimation.toValue = [NSValue valueWithPoint:endingLayerPoint];

  CAAnimationGroup* group = [CAAnimationGroup animation];
  group.removedOnCompletion = NO;
  group.fillMode = kCAFillModeForwards;
  group.animations =
      @[ opacityAnimation, positionAnimation, transformAnimation ];
  group.timingFunction = [CAMediaTimingFunction
      functionWithName:TransformAnimationTimingFunction()];
  group.duration = duration;
  [group setValue:kPrimaryWindowAnimationID forKey:kAnimationIDKey];
  group.delegate = self;

  CALayer* root = [self rootLayerOfWindow:primaryWindow_];
  [root addAnimation:group forKey:kPrimaryWindowAnimationID];
}

- (void)changePrimaryWindowToFinalFrame {
  changingPrimaryWindowSize_ = YES;
  [primaryWindow_ setFrame:finalFrame_ display:NO];
  changingPrimaryWindowSize_ = NO;
}

- (void)animationDidStart:(CAAnimation*)theAnimation {
  // CAAnimationDelegate method added on OSX 10.12.
}

- (void)animationDidStop:(CAAnimation*)theAnimation finished:(BOOL)finished {
  NSString* animationID = [theAnimation valueForKey:kAnimationIDKey];

  // Remove the snapshot window.
  if ([animationID isEqual:kSnapshotWindowAnimationID]) {
    [snapshotWindow_ orderOut:nil];
    snapshotWindow_.reset();
    snapshotLayer_.reset();
    return;
  }

  if ([animationID isEqual:kPrimaryWindowAnimationID]) {
    // If we're exiting full screen, we want to set the |primaryWindow_|'s
    // frame to the expected frame at the end of the animation. The window's
    // lock must also be released.
    if (!isEnteringFullscreen_) {
      lock_->set_lock(NO);

      CALayer* root = [self rootLayerOfWindow:primaryWindow_];
      root.anchorPoint = initialRootAnchorPoint_;

      NSUInteger styleMask =
          ([primaryWindow_ styleMask] & ~NSFullScreenWindowMask) |
          NSTexturedBackgroundWindowMask;
      [primaryWindow_ setStyleMask:styleMask];

      NSView* content = [primaryWindow_ contentView];
      [content setFrameOrigin:initialContentViewOrigin_];
      [self changePrimaryWindowToFinalFrame];

      [tabStripBackgroundView_ setHidden:NO];
      [fullscreenTabStripBackgroundView_ removeFromSuperview];
    }

    // Check if the contentView size is correct.
    // TODO (spqchan): Currently, a popup window will fail this check since
    // AppKit will shift the contentView up right after the window is resized.
    // This will create a small janky movement at the end of the animation.
    NSSize expectedSize = finalFrame_.size;
    NSView* content = [primaryWindow_ contentView];
    DCHECK_EQ(NSHeight(content.frame), expectedSize.height);
    DCHECK_EQ(NSWidth(content.frame), expectedSize.width);

    // Restore the state of the primary window and make it visible again.
    [primaryWindow_ setOpaque:primaryWindowInitialOpaque_];
    [primaryWindow_ setBackgroundColor:primaryWindowInitialBackgroundColor_];

    CALayer* root = [self rootLayerOfWindow:primaryWindow_];
    [root removeAnimationForKey:kPrimaryWindowAnimationID];
    root.opacity = 1;

    completedTransition_ = YES;

    if (!isEnteringFullscreen_)
      [controller_ exitFullscreenAnimationFinished];
  }
}

- (CALayer*)rootLayerOfWindow:(NSWindow*)window {
  return [[[window contentView] superview] layer];
}

- (NSPoint)pointRelativeToCurrentScreen:(NSPoint)point {
  NSRect screenFrame = [[primaryWindow_ screen] frame];
  return NSMakePoint(point.x - screenFrame.origin.x,
                     point.y - screenFrame.origin.y);
}

@end
