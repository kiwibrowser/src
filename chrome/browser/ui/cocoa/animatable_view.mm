// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

#import "chrome/browser/ui/cocoa/animatable_view.h"
#import "third_party/google_toolbox_for_mac/src/AppKit/GTMNSAnimation+Duration.h"

// NSAnimation subclass that animates the height of an AnimatableView.  Allows
// the caller to start and cancel the animation as desired.
@interface HeightAnimation : NSAnimation {
 @private
  AnimatableView* view_;  // weak, owns us.
  CGFloat startHeight_;
  CGFloat endHeight_;
}

// Initialize a new height animation for the given view.  The animation will not
// start until startAnimation: is called.
- (id)initWithView:(AnimatableView*)view
       finalHeight:(CGFloat)height
          duration:(NSTimeInterval)duration;
@end

@implementation HeightAnimation
- (id)initWithView:(AnimatableView*)view
       finalHeight:(CGFloat)height
          duration:(NSTimeInterval)duration {
  if ((self = [super gtm_initWithDuration:duration
                                eventMask:NSLeftMouseUpMask
                           animationCurve:NSAnimationEaseIn])) {
    view_ = view;
    startHeight_ = [view_ height];
    endHeight_ = height;
    [self setAnimationBlockingMode:NSAnimationNonblocking];
    [self setDelegate:view_];
  }
  return self;
}

// Overridden to call setHeight for each progress tick.
- (void)setCurrentProgress:(NSAnimationProgress)progress {
  [super setCurrentProgress:progress];
  [view_ setHeight:((progress * (endHeight_ - startHeight_)) + startHeight_)];
}
@end


@implementation AnimatableView
@synthesize delegate = delegate_;
@synthesize resizeDelegate = resizeDelegate_;

- (void)dealloc {
  // Stop the animation if it is running, since it holds a pointer to this view.
  [self stopAnimation];
  [super dealloc];
}

- (CGFloat)height {
  return [self frame].size.height;
}

- (void)setHeight:(CGFloat)newHeight {
  // Force the height to be an integer because some animations look terrible
  // with non-integer intermediate heights.  We only ever set integer heights
  // for our views, so this shouldn't be a limitation in practice.
  int height = floor(newHeight);
  [resizeDelegate_ resizeView:self newHeight:height];
}

- (void)animateToNewHeight:(CGFloat)newHeight
                  duration:(NSTimeInterval)duration {
  [currentAnimation_ stopAnimation];

  currentAnimation_.reset([[HeightAnimation alloc] initWithView:self
                                                    finalHeight:newHeight
                                                       duration:duration]);
  if ([resizeDelegate_ respondsToSelector:@selector(setAnimationInProgress:)])
    [resizeDelegate_ setAnimationInProgress:YES];
  [currentAnimation_ startAnimation];
}

- (void)stopAnimation {
  [currentAnimation_ stopAnimation];
}

- (NSAnimationProgress)currentAnimationProgress {
  return [currentAnimation_ currentProgress];
}

- (void)animationDidStop:(NSAnimation*)animation {
  if ([resizeDelegate_ respondsToSelector:@selector(setAnimationInProgress:)])
    [resizeDelegate_ setAnimationInProgress:NO];
  if ([delegate_ respondsToSelector:@selector(animationDidStop:)])
    [delegate_ animationDidStop:animation];
  currentAnimation_.reset(nil);
}

- (void)animationDidEnd:(NSAnimation*)animation {
  if ([resizeDelegate_ respondsToSelector:@selector(setAnimationInProgress:)])
    [resizeDelegate_ setAnimationInProgress:NO];
  if ([delegate_ respondsToSelector:@selector(animationDidEnd:)])
    [delegate_ animationDidEnd:animation];
  currentAnimation_.reset(nil);
}

@end
