// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/clipping_mask_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - GradientView

namespace {

enum GradientViewDirection {
  GRADIENT_LEFT_TO_RIGHT,
  GRADIENT_RIGHT_TO_LEFT,
};

}  // namespace

// The gradient view is a view that displays a simple horizontal gradient, from
// black to transparent.
@interface GradientView : UIView
// Sets the gradient's direction. You have to call this at least once to
// configure the gradient.
@property(nonatomic, assign) GradientViewDirection direction;
@end

@implementation GradientView
@synthesize direction = _direction;

+ (Class)layerClass {
  return [CAGradientLayer class];
}

- (void)setDirection:(GradientViewDirection)direction {
  _direction = direction;

  CAGradientLayer* layer = static_cast<CAGradientLayer*>(self.layer);
  layer.colors = @[
    static_cast<id>([[UIColor clearColor] CGColor]),
    static_cast<id>([[UIColor blackColor] CGColor])
  ];

  if (direction == GRADIENT_LEFT_TO_RIGHT) {
    layer.transform = CATransform3DMakeRotation(M_PI_2, 0, 0, 1);
  } else {
    layer.transform = CATransform3DMakeRotation(-M_PI_2, 0, 0, 1);
  }

  [self setNeedsDisplay];
}

@end

#pragma mark - ClippingMaskView

@interface ClippingMaskView ()
// The left gradient view.
@property(nonatomic, strong) GradientView* leftGradientView;
// The right gradient view.
@property(nonatomic, strong) GradientView* rightGradientView;
// The middle view. Used to paint everything not covered by the gradients with
// opaque color.
@property(nonatomic, strong) UIView* middleView;
@end

@implementation ClippingMaskView
@synthesize fadeLeft = _fadeLeft;
@synthesize fadeRight = _fadeRight;
@synthesize middleView = _middleView;
@synthesize leftGradientView = _leftGradientView;
@synthesize rightGradientView = _rightGradientView;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _leftGradientView = [[GradientView alloc] init];
    [_leftGradientView setTranslatesAutoresizingMaskIntoConstraints:NO];
    _leftGradientView.direction = GRADIENT_RIGHT_TO_LEFT;

    _rightGradientView = [[GradientView alloc] init];
    [_rightGradientView setTranslatesAutoresizingMaskIntoConstraints:NO];
    _rightGradientView.direction = GRADIENT_LEFT_TO_RIGHT;

    _middleView = [[UIView alloc] init];
    [_middleView setTranslatesAutoresizingMaskIntoConstraints:NO];
    _middleView.backgroundColor = [UIColor blackColor];

    [self addSubview:_leftGradientView];
    [self addSubview:_middleView];
    [self addSubview:_rightGradientView];
  }
  return self;
}

- (void)setFadeLeft:(BOOL)fadeLeft {
  _fadeLeft = fadeLeft;
  self.leftGradientView.hidden = !fadeLeft;
  [self setNeedsLayout];
}

- (void)setFadeRight:(BOOL)fadeRight {
  _fadeRight = fadeRight;
  self.rightGradientView.hidden = !fadeRight;
  [self setNeedsLayout];
}

// Since this view is intended to be used as a maskView of another UIView, and
// maskViews are not in the view hierarchy, autolayout is not supported.
// Luckily, the desired layout is easy:
// |[leftGradientView][middleView][rightGradientView]|,
// where the grad views are sometimes hidden (and then the middle view is
// extended to that side). The gradients' widths are equal to the height of this
// view.
- (void)layoutSubviews {
  [super layoutSubviews];
  CGFloat height = self.bounds.size.height;
  CGFloat gradientWidth = height;
  CGFloat xOffset = 0;
  if (self.fadeLeft) {
    self.leftGradientView.frame = CGRectMake(0, 0, gradientWidth, height);
    xOffset += gradientWidth;
  }

  CGFloat midWidth = self.bounds.size.width - xOffset;
  if (self.fadeRight) {
    midWidth -= gradientWidth;
  }

  self.middleView.frame = CGRectMake(xOffset, 0, midWidth, height);
  xOffset += midWidth;

  if (self.fadeRight) {
    self.rightGradientView.frame =
        CGRectMake(xOffset, 0, gradientWidth, height);
  }
}

@end
