// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/spinner_view.h"

#import <QuartzCore/QuartzCore.h>

#include "base/mac/scoped_cftyperef.h"
#include "base/mac/sdk_forward_declarations.h"
#import "chrome/browser/ui/cocoa/md_util.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/geometry/angle_conversions.h"
#include "ui/native_theme/native_theme.h"

namespace {
constexpr CGFloat kDegrees90 = gfx::DegToRad(90.0f);
constexpr CGFloat kDegrees135 = gfx::DegToRad(135.0f);
constexpr CGFloat kDegrees180 = gfx::DegToRad(180.0f);
constexpr CGFloat kDegrees270 = gfx::DegToRad(270.0f);
constexpr CGFloat kDegrees360 = gfx::DegToRad(360.0f);
constexpr CGFloat kSpinnerViewUnitWidth = 28.0;
constexpr CGFloat kSpinnerUnitInset = 2.0;
constexpr CGFloat kArcDiameter =
    (kSpinnerViewUnitWidth - kSpinnerUnitInset * 2.0);
constexpr CGFloat kArcRadius = kArcDiameter / 2.0;
constexpr CGFloat kArcLength =
    kDegrees135 * kArcDiameter;  // 135 degrees of circumference.
constexpr CGFloat kArcStrokeWidth = 3.0;
constexpr CGFloat kArcAnimationTime = 1.333;
constexpr CGFloat kRotationTime = 1.56863;
NSString* const kSpinnerAnimationName  = @"SpinnerAnimationName";
NSString* const kRotationAnimationName = @"RotationAnimationName";
}

@implementation SpinnerView {
  CAShapeLayer* shapeLayer_;  // Weak.
  CALayer* rotationLayer_;    // Weak.
}

@synthesize spinnerAnimation = spinnerAnimation_;
@synthesize rotationAnimation = rotationAnimation_;

+ (CGFloat)arcRotationTime {
  return kRotationTime;
}

+ (CGFloat)arcUnitRadius {
  return kArcRadius;
}

- (instancetype)initWithFrame:(NSRect)frame {
  if (self = [super initWithFrame:frame]) {
    [self setWantsLayer:YES];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  self.spinnerAnimation = nil;
  self.rotationAnimation = nil;
  [super dealloc];
}

- (CGFloat)scaleFactor {
  return [self bounds].size.width / kSpinnerViewUnitWidth;
}

// Register/unregister for window miniaturization event notifications so that
// the spinner can stop animating if the window is minaturized
// (i.e. not visible).
- (void)viewWillMoveToWindow:(NSWindow*)newWindow {
  if ([self window]) {
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
                  name:NSWindowWillMiniaturizeNotification
                object:[self window]];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
                  name:NSWindowDidDeminiaturizeNotification
                object:[self window]];
  }

  if (newWindow) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(updateAnimation:)
               name:NSWindowWillMiniaturizeNotification
             object:newWindow];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(updateAnimation:)
               name:NSWindowDidDeminiaturizeNotification
             object:newWindow];
  }
}

// Start or stop the animation whenever the view is added to or removed from a
// window.
- (void)viewDidMoveToWindow {
  [self updateAnimation:nil];
}

- (BOOL)isAnimating {
  return [shapeLayer_ animationForKey:kSpinnerAnimationName] != nil ||
         [rotationLayer_ animationForKey:kRotationAnimationName] != nil;
}

- (NSColor*)spinnerColor {
  SkColor skSpinnerColor =
      ui::NativeTheme::GetInstanceForNativeUi()->GetSystemColor(
          ui::NativeTheme::kColorId_ThrobberSpinningColor);
  return skia::SkColorToSRGBNSColor(skSpinnerColor);
}

- (CGFloat)arcStartAngle {
  return kDegrees180;
}

- (CGFloat)arcEndAngleDelta {
  return -kDegrees270;
}

- (CGFloat)arcLength {
  return kArcLength;
}

- (void)updateSpinnerColor {
  [shapeLayer_ setStrokeColor:[[self spinnerColor] CGColor]];
}

- (void)updateSpinnerPath {
  CGRect bounds = [self bounds];
  CGFloat scaleFactor = [self scaleFactor];

  // Create the arc that, when stroked, creates the spinner.
  base::ScopedCFTypeRef<CGMutablePathRef> shapePath(CGPathCreateMutable());
  CGFloat startAngle = [self arcStartAngle];
  CGFloat endAngleDelta = [self arcEndAngleDelta];
  bool counterClockwise = endAngleDelta < 0;
  endAngleDelta = ABS(endAngleDelta);

  CGPathAddArc(shapePath, NULL, bounds.size.width / 2.0,
               bounds.size.height / 2.0, kArcRadius * scaleFactor, startAngle,
               startAngle + endAngleDelta, !counterClockwise);
  [shapeLayer_ setPath:shapePath];

  // Set the line dash pattern. Animating the pattern causes the arc to
  // grow from start angle to end angle.
  [shapeLayer_ setLineDashPattern:@[ @([self arcLength] * scaleFactor) ]];

  [self updateSpinnerColor];
}

- (CALayer*)makeBackingLayer {
  shapeLayer_ = [CAShapeLayer layer];

  CGRect bounds = [self bounds];
  [shapeLayer_ setBounds:bounds];

  // Per the design, the line width does not scale linearly.
  CGFloat scaleFactor = [self scaleFactor];
  CGFloat scaledDiameter = kArcDiameter * scaleFactor;
  CGFloat lineWidth;
  if (scaledDiameter < kArcDiameter) {
    lineWidth = kArcStrokeWidth - (kArcDiameter - scaledDiameter) / 16.0;
  } else {
    lineWidth = kArcStrokeWidth + (scaledDiameter - kArcDiameter) / 11.0;
  }
  [shapeLayer_ setLineWidth:lineWidth];
  [shapeLayer_ setLineCap:kCALineCapRound];
  [shapeLayer_ setFillColor:NULL];

  [self updateSpinnerPath];

  // Place |shapeLayer_| in a layer so that it's easy to rotate the entire
  // spinner animation.
  rotationLayer_ = [CALayer layer];
  [rotationLayer_ setBounds:bounds];
  [rotationLayer_ addSublayer:shapeLayer_];
  [shapeLayer_ setPosition:CGPointMake(NSMidX(bounds), NSMidY(bounds))];

  // Place |rotationLayer_| in a parent layer so that it's easy to rotate
  // |rotationLayer_| around the center of the view.
  CALayer* parentLayer = [CALayer layer];
  [parentLayer setBounds:bounds];
  [parentLayer addSublayer:rotationLayer_];
  [rotationLayer_ setPosition:CGPointMake(bounds.size.width / 2.0,
                                          bounds.size.height / 2.0)];
  return parentLayer;
}

// Starts or stops the animation whenever the view's visibility changes.
- (void)setHidden:(BOOL)flag {
  BOOL wasHidden = [self isHidden];

  [super setHidden:flag];

  if (wasHidden != flag) {
    [self updateAnimation:nil];
  }
}

// The spinner animation consists of four cycles that it continuously repeats.
// Each cycle consists of one complete rotation of the spinner's arc plus a
// rotation adjustment at the end of each cycle (see rotation animation comment
// below for the reason for the adjustment). The arc's length also grows and
// shrinks over the course of each cycle, which the spinner achieves by drawing
// the arc using a (solid) dashed line pattern and animating the "lineDashPhase"
// property.
- (void)initializeAnimation {
  // Make sure |shapeLayer_|'s content scale factor matches the window's
  // backing depth (e.g. it's 2.0 on Retina Macs). Don't worry about adjusting
  // any other layers because |shapeLayer_| is the only one displaying content.
  CGFloat backingScaleFactor = [[self window] backingScaleFactor];
  [shapeLayer_ setContentsScale:backingScaleFactor];

  // Create the first half of the arc animation, where it grows from a short
  // block to its full length.
  base::scoped_nsobject<CAMediaTimingFunction> timingFunction(
      [CAMediaTimingFunction.cr_materialEaseInOutTimingFunction retain]);
  base::scoped_nsobject<CAKeyframeAnimation> firstHalfAnimation(
      [[CAKeyframeAnimation alloc] init]);
  [firstHalfAnimation setTimingFunction:timingFunction];
  [firstHalfAnimation setKeyPath:@"lineDashPhase"];
  // Begin the lineDashPhase animation just short of the full arc length,
  // otherwise the arc will be zero length at start.
  CGFloat scaleFactor = [self scaleFactor];
  NSArray* animationValues = @[ @(-(kArcLength - 0.4) * scaleFactor), @(0.0) ];
  [firstHalfAnimation setValues:animationValues];
  NSArray* keyTimes = @[ @(0.0), @(1.0) ];
  [firstHalfAnimation setKeyTimes:keyTimes];
  [firstHalfAnimation setDuration:kArcAnimationTime / 2.0];
  [firstHalfAnimation setRemovedOnCompletion:NO];
  [firstHalfAnimation setFillMode:kCAFillModeForwards];

  // Create the second half of the arc animation, where it shrinks from full
  // length back to a short block.
  base::scoped_nsobject<CAKeyframeAnimation> secondHalfAnimation(
      [[CAKeyframeAnimation alloc] init]);
  [secondHalfAnimation setTimingFunction:timingFunction];
  [secondHalfAnimation setKeyPath:@"lineDashPhase"];
  // Stop the lineDashPhase animation just before it reaches the full arc
  // length, otherwise the arc will be zero length at the end.
  animationValues = @[ @(0.0), @((kArcLength - 0.3) * scaleFactor) ];
  [secondHalfAnimation setValues:animationValues];
  [secondHalfAnimation setKeyTimes:keyTimes];
  [secondHalfAnimation setDuration:kArcAnimationTime / 2.0];
  [secondHalfAnimation setRemovedOnCompletion:NO];
  [secondHalfAnimation setFillMode:kCAFillModeForwards];

  // Make four copies of the arc animations, to cover the four complete cycles
  // of the full animation.
  NSMutableArray* animations = [NSMutableArray array];
  CGFloat beginTime = 0;
  for (NSUInteger i = 0; i < 4; i++, beginTime += kArcAnimationTime) {
    [firstHalfAnimation setBeginTime:beginTime];
    [secondHalfAnimation setBeginTime:beginTime + kArcAnimationTime / 2.0];
    [animations addObject:firstHalfAnimation];
    [animations addObject:secondHalfAnimation];
    firstHalfAnimation.reset([firstHalfAnimation copy]);
    secondHalfAnimation.reset([secondHalfAnimation copy]);
  }

  // Create a step rotation animation, which rotates the arc 90 degrees on each
  // cycle. Each arc starts as a short block at degree 0 and ends as a short
  // block at degree -270. Without a 90 degree rotation at the end of each
  // cycle, the short block would appear to suddenly jump from -270 degrees to
  // -360 degrees. The full animation has to contain four of these 90 degree
  // adjustments in order for the arc to return to its starting point, at which
  // point the full animation can smoothly repeat.
  CAKeyframeAnimation* stepRotationAnimation = [CAKeyframeAnimation animation];
  [stepRotationAnimation setTimingFunction:
      [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear]];
  [stepRotationAnimation setKeyPath:@"transform.rotation"];
  animationValues = @[ @(0.0), @(0.0),
                       @(kDegrees90),
                       @(kDegrees90),
                       @(kDegrees180),
                       @(kDegrees180),
                       @(kDegrees270),
                       @(kDegrees270)];
  [stepRotationAnimation setValues:animationValues];
  keyTimes = @[ @(0.0), @(0.25), @(0.25), @(0.5), @(0.5), @(0.75), @(0.75),
                @(1.0) ];
  [stepRotationAnimation setKeyTimes:keyTimes];
  [stepRotationAnimation setDuration:kArcAnimationTime * 4.0];
  [stepRotationAnimation setRemovedOnCompletion:NO];
  [stepRotationAnimation setFillMode:kCAFillModeForwards];
  [stepRotationAnimation setRepeatCount:HUGE_VALF];
  [animations addObject:stepRotationAnimation];

  // Use an animation group so that the animations are easier to manage, and to
  // give them the best chance of firing synchronously.
  CAAnimationGroup* group = [CAAnimationGroup animation];
  [group setDuration:kArcAnimationTime * 4];
  [group setRepeatCount:HUGE_VALF];
  [group setFillMode:kCAFillModeForwards];
  [group setRemovedOnCompletion:NO];
  [group setAnimations:animations];

  self.spinnerAnimation = group;

  // Finally, create an animation that rotates the entire spinner layer.
  CABasicAnimation* rotationAnimation = [CABasicAnimation animation];
  rotationAnimation.keyPath = @"transform.rotation";
  [rotationAnimation setFromValue:@0];
  [rotationAnimation setToValue:@(-kDegrees360)];
  [rotationAnimation setDuration:kRotationTime];
  [rotationAnimation setRemovedOnCompletion:NO];
  [rotationAnimation setFillMode:kCAFillModeForwards];
  [rotationAnimation setRepeatCount:HUGE_VALF];

  self.rotationAnimation = rotationAnimation;
}

- (void)updateAnimation:(NSNotification*)notification {
  // Only animate the spinner if it's within a window, and that window is not
  // currently minimized or being minimized.
  if ([self window] && ![[self window] isMiniaturized] && ![self isHidden] &&
      ![[notification name] isEqualToString:
           NSWindowWillMiniaturizeNotification]) {
    [self updateSpinnerPath];
    if (!spinnerAnimation_) {
      [self initializeAnimation];
    }
    if (![self isAnimating]) {
      [shapeLayer_ addAnimation:spinnerAnimation_ forKey:kSpinnerAnimationName];
      [rotationLayer_ addAnimation:rotationAnimation_
                            forKey:kRotationAnimationName];
    }
  } else {
    [shapeLayer_ removeAllAnimations];
    [rotationLayer_ removeAllAnimations];
  }
}

- (void)restartAnimation {
  self.spinnerAnimation = nil;
  self.rotationAnimation = nil;
  [shapeLayer_ removeAllAnimations];
  [rotationLayer_ removeAllAnimations];

  [self updateAnimation:nil];
}

@end
