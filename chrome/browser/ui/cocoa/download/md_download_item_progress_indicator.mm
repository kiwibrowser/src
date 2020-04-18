// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/download/md_download_item_progress_indicator.h"

#import <QuartzCore/QuartzCore.h>

#import "base/mac/scoped_cftyperef.h"
#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_service.h"
#import "chrome/browser/ui/cocoa/md_util.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/color_palette.h"

namespace {

typedef void (^AnimationBlock)();

constexpr NSString* kZRotationKeyPath = @"transform.rotation.z";

// An indeterminate progress indicator is a moving 50° arc…
constexpr CGFloat kIndeterminateArcSize = 50 / 360.0;
// …which completes a rotation around the circle every 4.5 seconds…
constexpr CGFloat kIndeterminateAnimationDuration = 4.5;
// …stored under this key.
NSString* const kIndeterminateAnimationKey = @"indeterminate";

constexpr CGFloat kIndeterminateAnimationParkDistance = M_PI / 8;
constexpr CGFloat kIndeterminateAnimationParkDuration = 0.8;
constexpr CGFloat kIndeterminateAnimationUnparkDuration = 0.4;

// Width of the progress stroke itself.
constexpr CGFloat kStrokeWidth = 2;

base::ScopedCFTypeRef<CGPathRef> CirclePathWithBounds(CGRect bounds) {
  CGMutablePathRef path = CGPathCreateMutable();
  CGPathAddArc(path, nullptr, NSMidX(bounds), NSMidY(bounds),
               NSWidth(bounds) / 2, 2 * M_PI + M_PI_2, M_PI_2, true);
  return base::ScopedCFTypeRef<CGPathRef>(path);
}

// The line under the arrow goes through three paths, in sequence:

// 1. A line under the arrow.
base::ScopedCFTypeRef<CGPathRef> UnderlineStartPath(CGRect bounds) {
  base::ScopedCFTypeRef<CGMutablePathRef> path(CGPathCreateMutable());
  CGPathMoveToPoint(path, NULL, NSMidX(bounds) - 7, NSMidY(bounds) - 8);
  CGPathAddLineToPoint(path, NULL, NSMidX(bounds) + 7, NSMidY(bounds) - 8);
  return base::ScopedCFTypeRef<CGPathRef>(CGPathCreateCopyByStrokingPath(
      path, NULL, kStrokeWidth, kCGLineCapButt, kCGLineJoinMiter, 10));
}

// 2. The same line, but above the arrow.
base::ScopedCFTypeRef<CGPathRef> UnderlineMidPath(CGRect bounds) {
  base::ScopedCFTypeRef<CGMutablePathRef> path(CGPathCreateMutable());
  CGPathMoveToPoint(path, NULL, NSMidX(bounds) - 7, NSMidY(bounds) + 7);
  CGPathAddLineToPoint(path, NULL, NSMidX(bounds) + 7, NSMidY(bounds) + 7);
  return base::ScopedCFTypeRef<CGPathRef>(CGPathCreateCopyByStrokingPath(
      path, NULL, kStrokeWidth, kCGLineCapButt, kCGLineJoinMiter, 10));
}

// 3. A tiny arc, even higher, that serves as the progress stroke's flat "tail".
base::ScopedCFTypeRef<CGPathRef> UnderlineEndPath(CGRect bounds) {
  base::ScopedCFTypeRef<CGMutablePathRef> path(CGPathCreateMutable());
  CGPathAddArc(path, nullptr, NSMidX(bounds), NSMidY(bounds),
               NSWidth(bounds) / 2 - kStrokeWidth / 2, M_PI_2, M_PI * 0.45,
               true);
  return base::ScopedCFTypeRef<CGPathRef>(CGPathCreateCopyByStrokingPath(
      path, NULL, kStrokeWidth, kCGLineCapButt, kCGLineJoinMiter, 10));
}

// The below shapes can all occupy a layer with these bounds.
constexpr CGRect kShapeBounds{{0, 0}, {14, 18}};

// The arrow part of the start state.
base::ScopedCFTypeRef<CGPathRef> ArrowPath() {
  constexpr CGPoint points[] = {
      {14, 7}, {10, 7}, {10, 13}, {4, 13}, {4, 7}, {0, 7}, {7, 0},
  };
  CGMutablePathRef path = CGPathCreateMutable();
  CGPathAddLines(path, nullptr, points, sizeof(points) / sizeof(*points));
  return base::ScopedCFTypeRef<CGPathRef>(path);
}

// ArrowPath() shifted slightly upward. Animating between two versions of the
// path was simpler than moving layers around.
base::ScopedCFTypeRef<CGPathRef> BeginArrowPath() {
  CGAffineTransform transform = CGAffineTransformMakeTranslation(0, 3);
  return base::ScopedCFTypeRef<CGPathRef>(
      CGPathCreateCopyByTransformingPath(ArrowPath(), &transform));
}

// A checkmark that represents a completed download. The number and order of
// points matches up with ArrowPath() so that one can morph into the other.
base::ScopedCFTypeRef<CGPathRef> CheckPath() {
  constexpr CGPoint points[] = {
      {11.1314, 7.5385}, {13.9697, 10.308}, {12.012, 12.366}, {4.5942, 4.9466},
      {2.0074, 7.528},   {0.0, 5.6687},     {4.5549, 1.0},
  };
  CGMutablePathRef path = CGPathCreateMutable();
  CGPathAddLines(path, nullptr, points, sizeof(points) / sizeof(*points));
  return base::ScopedCFTypeRef<CGPathRef>(path);
}

}  // namespace

@implementation MDDownloadItemProgressIndicator {
  CAShapeLayer* backgroundLayer_;
  CAShapeLayer* shapeLayer_;
  CAShapeLayer* strokeLayer_;
  CAShapeLayer* strokeTailLayer_;

  // The current and desired state and progress;
  MDDownloadItemProgressIndicatorState state_;
  MDDownloadItemProgressIndicatorState targetState_;
  CGFloat progress_;
  CGFloat targetProgress_;

  // An array of blocks representing queued animations.
  base::scoped_nsobject<NSMutableArray<AnimationBlock>> animations_;
  BOOL animationRunning_;
}

@synthesize paused = paused_;

- (void)setPaused:(BOOL)paused {
  if (paused_ == paused)
    return;
  paused_ = paused;
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    context.timingFunction =
        CAMediaTimingFunction.cr_materialEaseInOutTimingFunction;
    context.duration = paused ? 0.4 : 0.2;
    [self updateColors];
    context.duration = 0.3;
    backgroundLayer_.opacity = paused ? 0.08 : 0.15;
    strokeLayer_.opacity = paused ? 0.2 : 1;
    shapeLayer_.opacity = paused ? 0.2 : 1;
    [CATransaction setDisableActions:YES];
    if (paused_ && progress_ < 0) {
      // If showing indeterminate progress, set the rotation to where the
      // stroke will come to rest: the presentation layer's rotation plus the
      // park distance.
      strokeLayer_.transform = CATransform3DMakeRotation(
          static_cast<NSNumber*>([strokeLayer_.presentationLayer
                                     valueForKeyPath:kZRotationKeyPath])
                  .doubleValue -
              kIndeterminateAnimationParkDistance,
          0, 0, 1);
      [self updateStrokeSpinning];
    } else {
      [self updateStrokeSpinning];
      strokeLayer_.transform = CATransform3DIdentity;
    }
  }
                      completionHandler:nil];
}

- (void)updateColors {
  const BOOL darkTheme = [self.window hasDarkTheme];
  CGColorRef indicatorColor =
      skia::SkColorToCalibratedNSColor(
          paused_
              ? (darkTheme ? SK_ColorWHITE : SK_ColorBLACK)
              : state_ == MDDownloadItemProgressIndicatorState::kComplete
                    ? (darkTheme ? gfx::kGoogleGreen300 : gfx::kGoogleGreen700)
                    : (darkTheme ? gfx::kGoogleBlue300 : gfx::kGoogleBlue700))
          .CGColor;
  backgroundLayer_.fillColor = indicatorColor;
  shapeLayer_.fillColor =
      (state_ == MDDownloadItemProgressIndicatorState::kInProgress && !paused_)
          ? skia::SkColorToCalibratedNSColor(gfx::kGoogleBlue500).CGColor
          : indicatorColor;
  strokeLayer_.strokeColor = indicatorColor;
  strokeTailLayer_.fillColor = indicatorColor;
}

- (void)setInProgressState {
  if (state_ == MDDownloadItemProgressIndicatorState::kInProgress)
    return;
  state_ = MDDownloadItemProgressIndicatorState::kInProgress;
  CALayer* layer = self.layer;

  // Give users a chance to see the starting state before it changes.
  const CFTimeInterval beginTime = CACurrentMediaTime() + 0.3;
  const auto delayAnimation = [&](CAAnimation* anim) {
    anim.beginTime = beginTime;
    anim.fillMode = kCAFillModeBackwards;
  };

  // This layer sits on top of shapeLayer_ to temporarily hide its new color.
  base::scoped_nsobject<CAShapeLayer> scopedBeginShapeLayer(
      [[CAShapeLayer alloc] init]);
  CAShapeLayer* beginShapeLayer = scopedBeginShapeLayer;
  beginShapeLayer.frame = shapeLayer_.frame;
  beginShapeLayer.path = shapeLayer_.path;
  beginShapeLayer.fillColor = shapeLayer_.fillColor;
  [layer insertSublayer:beginShapeLayer above:shapeLayer_];

  // As strokeTailLayer_, currently in the shape of a line under shapeLayer_,
  // sweeps upward, this mask follows it to reveal the new color.
  base::scoped_nsobject<CALayer> scopedMaskLayer([[CALayer alloc] init]);
  CALayer* maskLayer = scopedMaskLayer;

  // The final y position was chosen empirically so that the bottom edge of the
  // mask matches up with the line as it sweeps upward.
  maskLayer.frame =
      NSOffsetRect(beginShapeLayer.bounds, 0, NSHeight(layer.bounds) / 2 + 9);
  maskLayer.backgroundColor = CGColorGetConstantColor(kCGColorBlack);
  beginShapeLayer.mask = maskLayer;

  // The circular background of the progress indicator begins to slowly fade in.
  {
    CABasicAnimation* anim = [CABasicAnimation animationWithKeyPath:@"opacity"];
    delayAnimation(anim);
    anim.fromValue = @(backgroundLayer_.opacity);
    anim.duration = 1;
    [backgroundLayer_ addAnimation:anim forKey:nil];
  }
  backgroundLayer_.opacity = 0.15;

  // Now, the line under the arrow sweeps upward to its final form: a small arc
  // segment that caps the progress stroke with a flat end.
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    context.duration = 2 / 3.0;
    // It accelerates with more "pep" than any standard MD curve.
    context.timingFunction = base::scoped_nsobject<CAMediaTimingFunction>(
        [[CAMediaTimingFunction alloc] initWithControlPoints:0.8:0:0.2:1]);

    {
      CABasicAnimation* anim =
          [CABasicAnimation animationWithKeyPath:@"position.y"];
      delayAnimation(anim);
      anim.fromValue = @(NSMidY(beginShapeLayer.bounds));
      [maskLayer addAnimation:anim forKey:nil];
    }

    strokeTailLayer_.path = UnderlineEndPath(layer.bounds);
    {
      CAKeyframeAnimation* anim =
          [CAKeyframeAnimation animationWithKeyPath:@"path"];
      delayAnimation(anim);
      anim.values = @[
        static_cast<id>(UnderlineStartPath(layer.bounds).get()),
        static_cast<id>(UnderlineMidPath(layer.bounds).get()),
        static_cast<id>(strokeTailLayer_.path),
      ];
      anim.keyTimes = @[ @0, @0.65, @1 ];
      [strokeTailLayer_ addAnimation:anim forKey:nil];
    }

    CABasicAnimation* arrowAnim =
        [CABasicAnimation animationWithKeyPath:@"path"];
    delayAnimation(arrowAnim);
    arrowAnim.fromValue = static_cast<id>(beginShapeLayer.path);

    shapeLayer_.path = beginShapeLayer.path = ArrowPath();
    [beginShapeLayer addAnimation:arrowAnim forKey:nil];
    [shapeLayer_ addAnimation:arrowAnim forKey:nil];
    [self updateColors];
  }
      completionHandler:^{
        [beginShapeLayer removeFromSuperlayer];
      }];
}

- (void)setCompleteState {
  if (state_ == MDDownloadItemProgressIndicatorState::kComplete)
    return;
  state_ = MDDownloadItemProgressIndicatorState::kComplete;
  CABasicAnimation* shapeAnim = [CABasicAnimation animationWithKeyPath:@"path"];
  shapeAnim.fromValue = static_cast<id>(shapeLayer_.path);
  [shapeLayer_ addAnimation:shapeAnim forKey:nil];
  shapeLayer_.path = CheckPath();
  strokeLayer_.opacity = 0;
  [self updateColors];
}

- (AnimationBlock)updateStateAnimation {
  return [[^{
    switch (targetState_) {
      case MDDownloadItemProgressIndicatorState::kNotStarted:
        NOTREACHED();
        break;
      case MDDownloadItemProgressIndicatorState::kInProgress:
        [self setInProgressState];
        break;
      case MDDownloadItemProgressIndicatorState::kComplete:
        [self setCompleteState];
        break;
    }
  } copy] autorelease];
}

- (void)updateStrokeSpinning {
  BOOL shouldBeSpinning = progress_ < 0 && !paused_;
  if (shouldBeSpinning ==
      ([strokeLayer_ animationForKey:kIndeterminateAnimationKey] != nil))
    return;

  CGFloat currentRotation =
      static_cast<NSNumber*>(
          [strokeLayer_.presentationLayer valueForKeyPath:kZRotationKeyPath])
          .doubleValue;

  [CATransaction begin];
  [CATransaction setAnimationTimingFunction:nil];
  CABasicAnimation* transition =
      [CABasicAnimation animationWithKeyPath:kZRotationKeyPath];
  transition.toValue = @(currentRotation - kIndeterminateAnimationParkDistance);
  transition.byValue = @(-kIndeterminateAnimationParkDistance);
  transition.duration = shouldBeSpinning ? kIndeterminateAnimationUnparkDuration
                                         : kIndeterminateAnimationParkDuration;
  transition.timingFunction =
      shouldBeSpinning ? CAMediaTimingFunction.cr_materialEaseInTimingFunction
                       : CAMediaTimingFunction.cr_materialEaseOutTimingFunction;
  [strokeLayer_ addAnimation:transition forKey:nil];
  [CATransaction commit];

  if (shouldBeSpinning) {
    CABasicAnimation* spinAnim =
        [CABasicAnimation animationWithKeyPath:kZRotationKeyPath];
    spinAnim.fromValue = transition.toValue;
    spinAnim.byValue = @(M_PI * -2);
    spinAnim.beginTime =
        CACurrentMediaTime() + kIndeterminateAnimationUnparkDuration;
    spinAnim.duration = kIndeterminateAnimationDuration;
    spinAnim.repeatCount = HUGE_VALF;

    // Attach the repeating animation in its own transaction so that it doesn't
    // stop completion handlers from running.
    dispatch_async(dispatch_get_main_queue(), ^{
      [strokeLayer_ addAnimation:spinAnim forKey:kIndeterminateAnimationKey];
      [CATransaction flush];
    });
  } else {
    [strokeLayer_ removeAnimationForKey:kIndeterminateAnimationKey];
  }
}

- (void)updateProgress {
  if (paused_)
    return;
  NSAccessibilityPostNotification(self,
                                  NSAccessibilityValueChangedNotification);
  if (progress_ < 0) {
    strokeLayer_.strokeEnd = kIndeterminateArcSize;
    [self updateStrokeSpinning];
  } else {
    strokeLayer_.strokeEnd = progress_;
    if ([strokeLayer_ animationForKey:kIndeterminateAnimationKey]) {
      [strokeLayer_ removeAnimationForKey:kIndeterminateAnimationKey];
      [strokeLayer_
          addAnimation:[CABasicAnimation animationWithKeyPath:kZRotationKeyPath]
                forKey:nil];
      strokeLayer_.transform = CATransform3DIdentity;
    }
  }
}

- (AnimationBlock)updateProgressAnimation {
  return [[^{
    if (progress_ == targetProgress_)
      return;
    progress_ = targetProgress_;
    [self updateProgress];
  } copy] autorelease];
}

- (void)runAnimations {
  if (animationRunning_ || ![animations_ count])
    return;
  animationRunning_ = YES;
  [NSAnimationContext runAnimationGroup:^(NSAnimationContext* context) {
    context.duration = 0.3;
    context.timingFunction =
        CAMediaTimingFunction.cr_materialEaseInOutTimingFunction;
    animations_[0]();
    [animations_ removeObjectAtIndex:0];
  }
      completionHandler:^{
        animationRunning_ = NO;
        [self runAnimations];
      }];
}

- (void)enqueue:(AnimationBlock)animation {
  if (!animations_)
    animations_.reset([[NSMutableArray alloc] init]);
  [animations_ addObject:[[animation copy] autorelease]];
  // If the view doesn't have a layer, it hasn't made it into a view hierarchy
  // yet and none of the sublayers have been created. In that case, leave it in
  // the queue — -setLayer: will call -runAnimations.
  if (self.layer)
    [self runAnimations];
}

// This utility method just chains an extra block onto the animation block if
// non-nil. It's used below to add the caller-supplied animation block.
- (void)enqueue:(AnimationBlock)animation withExtra:(AnimationBlock)extra {
  [self enqueue:(extra ? ^{ animation(); extra(); } : animation)];
}

- (void)setState:(MDDownloadItemProgressIndicatorState)state
        progress:(CGFloat)progress
      animations:(AnimationBlock)animations
      completion:(AnimationBlock)completion {
  if (state != targetState_) {
    // The animation code is only prepared to move the state forward.
    DCHECK(state > targetState_);
    targetState_ = state;
    targetProgress_ = progress;
    if (targetState_ == MDDownloadItemProgressIndicatorState::kInProgress) {
      // The transition to "in progress" must happen before progress updates.
      [self enqueue:[self updateStateAnimation] withExtra:animations];
      [self enqueue:[self updateProgressAnimation]];
    } else {
      // Other state changes, like completion, happen after progress updates.
      [self enqueue:[self updateProgressAnimation]];
      [self enqueue:[self updateStateAnimation] withExtra:animations];
    }
  } else if (progress != targetProgress_) {
    targetProgress_ = progress;
    [self enqueue:[self updateProgressAnimation] withExtra:animations];
  } else if (animations) {
    [self enqueue:animations];
  }
  if (completion)
    [self enqueue:completion];
}

// NSView overrides.

- (void)setLayer:(CALayer*)layer {
  super.layer = layer;

  base::scoped_nsobject<CAShapeLayer> backgroundLayer(
      [[CAShapeLayer alloc] init]);
  backgroundLayer_ = backgroundLayer;
  backgroundLayer_.frame = layer.bounds;
  backgroundLayer_.autoresizingMask =
      kCALayerWidthSizable | kCALayerHeightSizable;
  backgroundLayer_.opacity = 0;
  [layer addSublayer:backgroundLayer_];

  base::scoped_nsobject<CAShapeLayer> shapeLayer([[CAShapeLayer alloc] init]);
  shapeLayer_ = shapeLayer;
  shapeLayer_.bounds = kShapeBounds;
  shapeLayer_.path = BeginArrowPath();
  [layer addSublayer:shapeLayer_];

  base::scoped_nsobject<CAShapeLayer> strokeLayer([[CAShapeLayer alloc] init]);
  strokeLayer_ = strokeLayer;
  strokeLayer_.frame = layer.bounds;
  strokeLayer_.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
  strokeLayer_.fillColor = nil;
  strokeLayer_.lineWidth = kStrokeWidth;
  strokeLayer_.lineCap = kCALineCapRound;
  strokeLayer_.strokeStart = 0.02;
  strokeLayer_.strokeEnd = 0;
  [layer addSublayer:strokeLayer_];

  base::scoped_nsobject<CAShapeLayer> strokeTailLayer(
      [[CAShapeLayer alloc] init]);
  strokeTailLayer_ = strokeTailLayer;
  strokeTailLayer_.frame = layer.bounds;
  strokeTailLayer_.autoresizingMask =
      kCALayerWidthSizable | kCALayerHeightSizable;
  strokeTailLayer_.lineWidth = kStrokeWidth;
  [strokeLayer_ addSublayer:strokeTailLayer_];

  // If any state changes were queued up, kick 'em off.
  [self layout];
  [self runAnimations];
}

- (void)layout {
  backgroundLayer_.path = CirclePathWithBounds(self.layer.bounds);
  strokeLayer_.path = CirclePathWithBounds(
      CGRectInset(self.layer.bounds, kStrokeWidth / 2, kStrokeWidth / 2));
  shapeLayer_.position =
      CGPointMake(NSMidX(self.layer.bounds), NSMidY(self.layer.bounds) + 2);
  strokeTailLayer_.path =
      state_ == MDDownloadItemProgressIndicatorState::kNotStarted
          ? UnderlineStartPath(self.layer.bounds)
          : UnderlineEndPath(self.layer.bounds);
  strokeTailLayer_.position =
      CGPointMake(NSMidX(self.layer.bounds), NSMidY(self.layer.bounds));
  [self updateColors];
  [super layout];
}

// NSObject overrides.

- (BOOL)accessibilityIsIgnored {
  return NO;
}

- (id)accessibilityAttributeValue:(NSString*)attribute {
  if ([NSAccessibilityRoleAttribute isEqualToString:attribute])
    return NSAccessibilityProgressIndicatorRole;
  if (progress_ >= 0) {
    if ([NSAccessibilityValueAttribute isEqualToString:attribute])
      return @(progress_);
    if ([NSAccessibilityMinValueAttribute isEqualToString:attribute])
      return @0;
    if ([NSAccessibilityMaxValueAttribute isEqualToString:attribute])
      return @1;
  }
  return [super accessibilityAttributeValue:attribute];
}

@end
