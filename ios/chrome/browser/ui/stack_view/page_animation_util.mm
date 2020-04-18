// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/stack_view/page_animation_util.h"

#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/animation_util.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/stack_view/card_view.h"
#import "ios/chrome/common/material_timing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ios::material::TimingFunction;

namespace {

const NSTimeInterval kAnimationDuration = 0.25;
const NSTimeInterval kAnimationHesitation = 0.2;

// Constants used for rotating/translating in in transition-in animations and
// rotating/translating out in transition-out animations.
const CGFloat kDefaultRotation = 0.2094;  // 12 degrees.
// The amount by which the card should be translated along the axis on which
// its short side is oriented (horizontal in portrait, vertical in landscape).
const CGFloat kDefaultShortSideAxisTranslation = 240;
// The amount by which the card should be translated along the axis on which
// its long side is oriented (vertical in portrait, horizontal in landscape).
const CGFloat kDefaultLongSideAxisTranslation = 10;

// Transitioning in on landscape has a special-case animation.
const CGFloat kLandscapeAnimateInRotation = 0.9423;  // 54 degrees.
const CGFloat kLandscapeAnimateInShortSideAxisTranslation = -180;
const CGFloat kLandscapeAnimateInLongSideAxisTranslation = 140;

NSString* const kViewAnimateInKey = @"ViewAnimateIn";
NSString* const kPaperAnimateInKey = @"PaperAnimateIn";

// When animating out, a card shrinks slightly.
const CGFloat kAnimateOutScale = 0.7;
const CGFloat kAnimateOutAnchorX = 0.9;
const CGFloat kAnimateOutAnchorY = 0;
}

@interface PaperView : UIView

@end

@implementation PaperView

- (id)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    const UIEdgeInsets kShadowStretchInsets = {28.0, 28.0, 28.0, 28.0};
    const UIEdgeInsets kShadowLayoutOutset = {-10.0, -11.0, -12.0, -11.0};
    CGRect shadowFrame = UIEdgeInsetsInsetRect(frame, kShadowLayoutOutset);
    UIImageView* frameShadowImageView =
        [[UIImageView alloc] initWithFrame:shadowFrame];
    [frameShadowImageView
        setAutoresizingMask:(UIViewAutoresizingFlexibleWidth |
                             UIViewAutoresizingFlexibleHeight)];
    [self addSubview:frameShadowImageView];

    UIImage* image = [UIImage imageNamed:@"popup_background"];
    image = [image resizableImageWithCapInsets:kShadowStretchInsets];
    [frameShadowImageView setImage:image];
  }
  return self;
}

@end

namespace page_animation_util {

const CGFloat kCardMargin = 14.0;

void SetNewTabAnimationStartPositionForView(UIView* view, BOOL isPortrait) {
  CGAffineTransform transform = CGAffineTransformMakeTranslation(
      (isPortrait ? kDefaultShortSideAxisTranslation
                  : kLandscapeAnimateInLongSideAxisTranslation),
      (isPortrait ? kDefaultLongSideAxisTranslation
                  : kLandscapeAnimateInShortSideAxisTranslation));
  transform = CGAffineTransformRotate(
      transform, (isPortrait ? kDefaultRotation : kLandscapeAnimateInRotation));
  view.transform = transform;
}

void AnimateInPaperWithAnimationAndCompletion(UIView* view,
                                              CGFloat paperOffset,
                                              CGFloat contentOffset,
                                              CGPoint origin,
                                              BOOL isOffTheRecord,
                                              void (^extraAnimation)(void),
                                              void (^completion)(void)) {
  CGRect endFrame = view.frame;
  UIView* parent = [view superview];
  NSInteger index = [[parent subviews] indexOfObject:view];

  // Create paper background.
  CGRect paperFrame = CGRectOffset(endFrame, 0, paperOffset);
  paperFrame.size.height -= paperOffset;
  PaperView* paper = [[PaperView alloc] initWithFrame:paperFrame];
  [parent insertSubview:paper belowSubview:view];
  [paper addSubview:view];
  [paper setBackgroundColor:isOffTheRecord
                                ? [UIColor colorWithWhite:34 / 255.0 alpha:1]
                                : [UIColor whiteColor]];

  [CATransaction begin];
  [CATransaction setCompletionBlock:^{

    // Put view back where it belongs, with its original frame.
    [parent insertSubview:view atIndex:index];
    [paper removeFromSuperview];
    [[view layer] removeAnimationForKey:kViewAnimateInKey];
    view.frame = endFrame;
    if (completion)
      completion();
  }];
  [CATransaction setAnimationDuration:ios::material::kDuration5];
  CAMediaTimingFunction* transformCurve2 = ios::material::TransformCurve2();

  //  // Animate paper to full size.
  CABasicAnimation* scaleAnimation =
      [CABasicAnimation animationWithKeyPath:@"transform"];
  scaleAnimation.fromValue =
      [NSValue valueWithCATransform3D:CATransform3DMakeScale(0.03, 0.03, 1)];
  scaleAnimation.timingFunction = transformCurve2;
  scaleAnimation.duration = ios::material::kDuration1;

  CABasicAnimation* positionAnimation =
      [CABasicAnimation animationWithKeyPath:@"position"];
  positionAnimation.fromValue = [NSValue valueWithCGPoint:origin];
  positionAnimation.timingFunction = transformCurve2;
  positionAnimation.duration = ios::material::kDuration1;

  CAAnimation* fadeAnimation = OpacityAnimationMake(0, 1);
  fadeAnimation.timingFunction = transformCurve2;
  fadeAnimation.duration = ios::material::kDuration1;
  [[paper layer]
      addAnimation:AnimationGroupMake(
                       @[ scaleAnimation, positionAnimation, fadeAnimation ])
            forKey:kPaperAnimateInKey];

  // Animate content from -10px to full size, as a child of the paper parent.
  // At the half-way point, the child will be offset / 2 vertically higher than
  // the paper parent, but be sure to account for paperOriginYOffset, which
  // allows for pages to draw above |parent| (as the new tab page does).
  CGFloat offset = -10;
  CGFloat width = endFrame.size.width;
  CGFloat height = endFrame.size.height - contentOffset;
  CGRect startFrame = CGRectMake(0, offset, width, height);
  CGRect middleFrame =
      CGRectMake(0, offset / 2 - paperOffset + contentOffset, width, height);
  CGRect childEndFrame =
      CGRectMake(0, -paperOffset + contentOffset, width, height);

  CAAnimation* frameAnimation =
      FrameAnimationMake([view layer], startFrame, middleFrame);
  frameAnimation.timingFunction = transformCurve2;
  frameAnimation.duration = ios::material::kDuration1;
  frameAnimation.fillMode = kCAFillModeBackwards;

  CAMediaTimingFunction* fadeInCurve =
      TimingFunction(ios::material::CurveEaseOut);
  CAAnimation* frameAnimation2 =
      FrameAnimationMake([view layer], middleFrame, childEndFrame);
  frameAnimation2.timingFunction = fadeInCurve;
  frameAnimation2.duration = ios::material::kDuration1;
  frameAnimation2.beginTime = ios::material::kDuration1;
  frameAnimation2.fillMode = kCAFillModeForwards;

  fadeAnimation = OpacityAnimationMake(0, 1);
  fadeAnimation.timingFunction = fadeInCurve;
  fadeAnimation.duration = ios::material::kDuration5;
  [[view layer]
      addAnimation:AnimationGroupMake(
                       @[ frameAnimation, frameAnimation2, fadeAnimation ])
            forKey:kViewAnimateInKey];

  [CATransaction commit];
}

void AnimateInCardWithAnimationAndCompletion(UIView* view,
                                             void (^extraAnimation)(void),
                                             void (^completion)(void)) {
  SetNewTabAnimationStartPositionForView(view, true);
  [UIView animateWithDuration:kAnimationDuration
      delay:0
      options:UIViewAnimationCurveEaseOut
      animations:^{
        view.transform = CGAffineTransformIdentity;
        if (extraAnimation)
          extraAnimation();
      }
      completion:^(BOOL finished) {
        if (completion)
          completion();
      }];
}

void AnimateNewBackgroundPageWithCompletion(CardView* currentPageCard,
                                            CGRect displayFrame,
                                            CGRect imageFrame,
                                            BOOL isPortrait,
                                            void (^completion)(void)) {
  // Create paper background.
  PaperView* paper = [[PaperView alloc] initWithFrame:CGRectZero];
  UIView* parent = [currentPageCard superview];
  [parent insertSubview:paper aboveSubview:currentPageCard];
  CGRect pageBounds = currentPageCard.bounds;
  [paper setCenter:CGPointMake(CGRectGetMidX(pageBounds),
                               CGRectGetMidY(pageBounds))];
  [paper setBackgroundColor:[UIColor whiteColor]];
  [paper setAlpha:0.0];

  CGSize pageSize = currentPageCard.bounds.size;
  CGRect paperFrame =
      CGRectMake((displayFrame.size.width - pageSize.width) / 2,
                 CGRectGetMidY(pageBounds), pageSize.width, pageSize.height);

  // The animation of the current page during the new background card animation
  // has three parts:
  // 1. It shrinks the current tab image into an inset card view.
  // 2. It hesitates for a fraction of a second.
  // 3. It expands back out, transforming again into the current tab.
  // |currentPageCard| gives the card at the correct size for step 2, as it
  // appears in the slight hesitation. Here, the code creates the transform
  // that will start by displaying the card at full size; the animation then
  // moves the card into its original state, and back out to full screen size.
  CGSize displaySize = displayFrame.size;
  CGFloat fullScreenScale =
      (displaySize.width + kCardImageInsets.left + kCardImageInsets.right +
       kCardFrameImageSnapshotOverlap) /
      currentPageCard.frame.size.width;
  // Align the bottom of |currentPageCard|'s snapshot with the bottom of the
  // screen, so that snapshots of any height are correctly aligned with the
  // tab's content.
  currentPageCard.center =
      CGPointMake(displaySize.width / 2.0, displaySize.height -
                                               (imageFrame.size.height / 2.0) -
                                               kCardImageInsets.top / 2);
  CGAffineTransform fullScreenTransform =
      CGAffineTransformMakeScale(fullScreenScale, fullScreenScale);
  currentPageCard.transform = fullScreenTransform;
  [currentPageCard setTabOpacity:0.0];

  [CATransaction begin];
  [CATransaction
      setAnimationTimingFunction:TimingFunction(ios::material::CurveEaseIn)];
  [UIView animateWithDuration:kAnimationDuration
      delay:0
      options:UIViewAnimationCurveLinear
      animations:^{
        [currentPageCard setTabOpacity:1.0];
        currentPageCard.transform = CGAffineTransformIdentity;
        [paper setFrame:paperFrame];
        [paper setAlpha:1.0];
      }
      completion:^(BOOL finished) {
        // Zoom out the top tab, slide away the paper view.
        [UIView animateWithDuration:kAnimationDuration
            delay:kAnimationHesitation
            options:UIViewAnimationCurveLinear
            animations:^{
              [currentPageCard setTabOpacity:0.0];
              currentPageCard.transform = fullScreenTransform;
              [paper setFrame:CGRectOffset(paperFrame, 0,
                                           CGRectGetMaxY(paperFrame))];
              [paper setAlpha:0.0];

            }
            completion:^(BOOL finished) {
              [paper removeFromSuperview];
              if (completion)
                completion();
            }];
      }];
  [CATransaction commit];
}

void AnimateNewBackgroundTabWithCompletion(CardView* currentPageCard,
                                           CardView* newCard,
                                           CGRect displayFrame,
                                           BOOL isPortrait,
                                           void (^completion)(void)) {
  // The animation of the current page during the new background card animation
  // has three parts:
  // 1. It shrinks the current tab image into an inset card view.
  // 2. It hesitates for a fraction of a second.
  // 3. It expands back out, transforming again into the current tab.
  // |currentPageCard| gives the card at the correct size for step 2, as it
  // appears in the slight hesitation. Here, the code creates the transform
  // that will start by displaying the card at full size; the animation then
  // moves the card into its original state, and back out to full screen size.
  CGSize displaySize = displayFrame.size;
  CGFloat fullScreenScale =
      (displaySize.width + kCardImageInsets.left + kCardImageInsets.right) /
      currentPageCard.frame.size.width;
  // Align the bottom of |currentPageCard|'s snapshot with the bottom of the
  // screen, so that snapshots of any height are correctly aligned with the
  // tab's content.
  currentPageCard.center = CGPointMake(
      displaySize.width / 2.0,
      displaySize.height - (currentPageCard.image.size.height / 2.0));
  CGAffineTransform fullScreenTransform =
      CGAffineTransformMakeScale(fullScreenScale, fullScreenScale);
  currentPageCard.transform = fullScreenTransform;
  [currentPageCard setTabOpacity:0.0];

  // The new background card animation has three parts:
  // 1. It moves from offscreen onto the screen (in a "rotating" motion).
  // 2. It hesitates for a fraction of a second, halfway on the screen.
  // 3. It moves from the screen offscreen (in a "sliding" motion).
  // In the setup code below, we position the card on the screen as it will
  // appear in step 2; in the animation code, we then move it to and from this
  // original onscreen position.
  //
  // In portrait mode, the card in step 2 appears to be halfway off the bottom
  // edge of the screen; in landscape mode, it appears to be halfway off the
  // right edge. The x and y offsets below set up this screen position.
  CGFloat yOffset = -displayFrame.origin.y + kCardMargin +
                    (isPortrait ? displaySize.height / 2.0 : 0);
  CGFloat xOffset = isPortrait ? kCardMargin : displaySize.width / 2.0;
  CGRect newCardFrame = newCard.frame;
  newCardFrame.origin.x += xOffset;
  newCardFrame.origin.y += yOffset;
  newCard.frame = newCardFrame;

  // For step 1, we apply a transform to the card that moves it offscreen and
  // rotates it away in preparation for the "rotate in" animation that starts
  // any new tab appearance.
  SetNewTabAnimationStartPositionForView(newCard, isPortrait);

  // For step 3, we create a transform which will slide the card offscreen along
  // its longer axis to end the animation.
  CGAffineTransform slideAwayTransform =
      isPortrait
          ? CGAffineTransformMakeTranslation(0, newCard.frame.size.height)
          : CGAffineTransformMakeTranslation(newCard.frame.size.width, 0);

  [UIView animateWithDuration:kAnimationDuration
      delay:0
      options:UIViewAnimationCurveEaseOut
      animations:^{
        [currentPageCard setTabOpacity:1.0];
        currentPageCard.transform = CGAffineTransformIdentity;
        newCard.transform = CGAffineTransformIdentity;
      }
      completion:^(BOOL finished) {
        // Zoom out the top tab, slide away the new card.
        [UIView animateWithDuration:kAnimationDuration
            delay:kAnimationHesitation
            options:UIViewAnimationCurveEaseOut
            animations:^{
              [currentPageCard setTabOpacity:0.0];
              currentPageCard.transform = fullScreenTransform;
              newCard.transform = slideAwayTransform;
            }
            completion:^(BOOL finished) {
              if (completion)
                completion();
            }];
      }];
}

void UpdateLayorAnchorWithTransform(CALayer* layer,
                                    CGPoint newAnchor,
                                    CGAffineTransform transform) {
  CGSize size = layer.bounds.size;
  CGPoint oldAnchor = layer.anchorPoint;
  CGPoint newCenter =
      CGPointMake(size.width * newAnchor.x, size.height * newAnchor.y);
  CGPoint oldCenter =
      CGPointMake(size.width * oldAnchor.x, size.height * oldAnchor.y);

  newCenter = CGPointApplyAffineTransform(newCenter, transform);
  oldCenter = CGPointApplyAffineTransform(oldCenter, transform);

  CGPoint position = layer.position;
  position.x = position.x - oldCenter.x + newCenter.x;
  position.y = position.y - oldCenter.y + newCenter.y;
  layer.position = position;

  layer.anchorPoint = newAnchor;
}

void AnimateOutWithCompletion(UIView* view,
                              NSTimeInterval delay,
                              BOOL clockwise,
                              BOOL isPortrait,
                              void (^completion)(void)) {
  // The close animation spec calls for the anchor point to be the upper right.
  CGPoint newAnchorPoint = CGPointMake(kAnimateOutAnchorX, kAnimateOutAnchorY);
  CALayer* layer = [view layer];
  UpdateLayorAnchorWithTransform(layer, newAnchorPoint, view.transform);

  [CATransaction begin];
  if (completion)
    [CATransaction setCompletionBlock:completion];

  [CATransaction setAnimationDuration:ios::material::kDuration6];
  CAMediaTimingFunction* timing = TimingFunction(ios::material::CurveEaseIn);
  [CATransaction setAnimationTimingFunction:timing];

  CABasicAnimation* scaleAnimation =
      [CABasicAnimation animationWithKeyPath:@"transform"];
  CATransform3D transform = CATransform3DScale(
      layer.transform, kAnimateOutScale, kAnimateOutScale, 1);
  [scaleAnimation setToValue:[NSValue valueWithCATransform3D:transform]];

  CABasicAnimation* fadeAnimation =
      [CABasicAnimation animationWithKeyPath:@"opacity"];
  [fadeAnimation setFromValue:[NSNumber numberWithFloat:[layer opacity]]];
  [fadeAnimation setToValue:@0];

  [layer addAnimation:AnimationGroupMake(@[ scaleAnimation, fadeAnimation ])
               forKey:@"animateOut"];
  [CATransaction commit];
}

CGAffineTransform AnimateOutTransform(CGFloat fraction,
                                      BOOL clockwise,
                                      BOOL isPortrait) {
  CGFloat horizontalTranslation = isPortrait ? kDefaultShortSideAxisTranslation
                                             : kDefaultLongSideAxisTranslation;
  CGFloat verticalTranslation = isPortrait ? kDefaultLongSideAxisTranslation
                                           : kDefaultShortSideAxisTranslation;
  CGFloat rotationAmount = kDefaultRotation;

  if (!isPortrait && UseRTLLayout()) {
    rotationAmount *= -1;
    horizontalTranslation *= -1;
  }

  horizontalTranslation *= fraction;
  verticalTranslation *= fraction;
  rotationAmount *= fraction;
  if (!clockwise)
    rotationAmount *= -1;

  // In portrait, rotating counterclockwise pushes the animation to the left.
  if (isPortrait && !clockwise) {
    horizontalTranslation *= -1;
  }

  // In landscape, rotating clockwise pushes the animation up.
  if (!isPortrait && clockwise) {
    verticalTranslation *= -1;
  }

  // Scale the card between full-scale and the final desired scale based on
  // |fraction|.
  CGFloat differenceInScale = 1.0 - kAnimateOutScale;
  CGFloat scaleAmount = 1.0 - (differenceInScale * fraction);
  CGAffineTransform transform = CGAffineTransformMakeTranslation(
      horizontalTranslation, verticalTranslation);
  transform = CGAffineTransformRotate(transform, rotationAmount);
  transform = CGAffineTransformScale(transform, scaleAmount, scaleAmount);
  return transform;
}

CGFloat AnimateOutTransformBreadth() {
  return kDefaultShortSideAxisTranslation;
}

}  // namespace page_animation_util
