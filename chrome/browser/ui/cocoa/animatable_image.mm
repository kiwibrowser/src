// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/animatable_image.h"

#include "base/logging.h"
#import "base/mac/sdk_forward_declarations.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSAnimation+Duration.h"

@interface AnimatableImage (Private) <CAAnimationDelegate>
@end

@implementation AnimatableImage

@synthesize startFrame = startFrame_;
@synthesize endFrame = endFrame_;
@synthesize startOpacity = startOpacity_;
@synthesize endOpacity = endOpacity_;
@synthesize duration = duration_;
@synthesize timingFunction = timingFunction_;

- (id)initWithImage:(NSImage*)image
     animationFrame:(NSRect)animationFrame {
  if ((self = [super initWithContentRect:animationFrame
                               styleMask:NSBorderlessWindowMask
                                 backing:NSBackingStoreBuffered
                                   defer:NO])) {
    DCHECK(image);
    image_.reset([image retain]);
    duration_ = 1.0;
    timingFunction_ =
        [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
    startOpacity_ = 1.0;
    endOpacity_ = 1.0;

    [self setOpaque:NO];
    [self setBackgroundColor:[NSColor clearColor]];
    [self setIgnoresMouseEvents:YES];

    // Must be set or else self will be leaked.
    [self setReleasedWhenClosed:YES];
  }
  return self;
}

- (void)startAnimation {
  // Set up the root layer. By calling -setLayer: followed by -setWantsLayer:
  // the view becomes a layer hosting view as opposed to a layer backed view.
  NSView* view = [self contentView];
  CALayer* rootLayer = [CALayer layer];
  [view setLayer:rootLayer];
  [view setWantsLayer:YES];

  // Create the layer that will be animated.
  CALayer* layer = [CALayer layer];
  [layer setContents:image_.get()];
  [layer setAnchorPoint:CGPointMake(0, 1)];
  [layer setFrame:[self startFrame]];
  [layer setNeedsDisplayOnBoundsChange:YES];
  [rootLayer addSublayer:layer];

  // Animate the bounds only if the image is resized.
  CABasicAnimation* boundsAnimation = nil;
  if (CGRectGetWidth([self startFrame]) != CGRectGetWidth([self endFrame]) ||
      CGRectGetHeight([self startFrame]) != CGRectGetHeight([self endFrame])) {
    boundsAnimation = [CABasicAnimation animationWithKeyPath:@"bounds"];
    NSRect startRect = NSMakeRect(0, 0,
                                  CGRectGetWidth([self startFrame]),
                                  CGRectGetHeight([self startFrame]));
    [boundsAnimation setFromValue:[NSValue valueWithRect:startRect]];
    NSRect endRect = NSMakeRect(0, 0,
                                CGRectGetWidth([self endFrame]),
                                CGRectGetHeight([self endFrame]));
    [boundsAnimation setToValue:[NSValue valueWithRect:endRect]];
    [boundsAnimation gtm_setDuration:[self duration]
                           eventMask:NSLeftMouseUpMask];
    [boundsAnimation setTimingFunction:timingFunction_];
  }

  // Positional animation.
  CABasicAnimation* positionAnimation =
      [CABasicAnimation animationWithKeyPath:@"position"];
  [positionAnimation setFromValue:
      [NSValue valueWithPoint:NSPointFromCGPoint([self startFrame].origin)]];
  [positionAnimation setToValue:
      [NSValue valueWithPoint:NSPointFromCGPoint([self endFrame].origin)]];
  [positionAnimation gtm_setDuration:[self duration]
                           eventMask:NSLeftMouseUpMask];
  [positionAnimation setTimingFunction:timingFunction_];

  // Opacity animation.
  CABasicAnimation* opacityAnimation =
      [CABasicAnimation animationWithKeyPath:@"opacity"];
  [opacityAnimation setFromValue:
      [NSNumber numberWithFloat:[self startOpacity]]];
  [opacityAnimation setToValue:[NSNumber numberWithFloat:[self endOpacity]]];
  [opacityAnimation gtm_setDuration:[self duration]
                          eventMask:NSLeftMouseUpMask];
  [opacityAnimation setTimingFunction:timingFunction_];
  // Set the delegate just for one of the animations so that this window can
  // be closed upon completion.
  [opacityAnimation setDelegate:self];

  // The CAAnimations only affect the presentational value of a layer, not the
  // model value. This means that after the animation is done, it can flicker
  // back to the original values. To avoid this, create an implicit animation of
  // the values, which are then overridden with the CABasicAnimations.
  //
  // Ideally, a call to |-setBounds:| should be here, but, for reasons that
  // are not understood, doing so causes the animation to break.
  [layer setPosition:[self endFrame].origin];
  [layer setOpacity:[self endOpacity]];

  // Start the animations.
  [CATransaction begin];
  [CATransaction setValue:[NSNumber numberWithFloat:[self duration]]
                   forKey:kCATransactionAnimationDuration];
  if (boundsAnimation) {
    [layer addAnimation:boundsAnimation forKey:@"bounds"];
  }
  [layer addAnimation:positionAnimation forKey:@"position"];
  [layer addAnimation:opacityAnimation forKey:@"opacity"];
  [CATransaction commit];
}

- (void)animationDidStart:(CAAnimation*)animation {
}

// CAAnimation delegate method called when the animation is complete.
- (void)animationDidStop:(CAAnimation*)animation finished:(BOOL)flag {
  // Close the window, releasing self.
  [self close];
}

@end
