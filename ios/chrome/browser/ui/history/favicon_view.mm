// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/favicon_view.h"

#import "ios/chrome/browser/ui/util/constraints_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Default corner radius for the favicon image view.
const CGFloat kDefaultCornerRadius = 3;
}

@interface FaviconView () {
  // Property releaser for FaviconView.
}
// Size constraints for the favicon views.
@property(nonatomic, copy) NSArray* faviconSizeConstraints;
@end

@implementation FaviconView

@synthesize size = _size;
@synthesize faviconImage = _faviconImage;
@synthesize faviconFallbackLabel = _faviconFallbackLabel;
@synthesize faviconSizeConstraints = _faviconSizeConstraints;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _faviconImage = [[UIImageView alloc] init];
    _faviconImage.clipsToBounds = YES;
    _faviconImage.layer.cornerRadius = kDefaultCornerRadius;
    _faviconImage.image = nil;

    _faviconFallbackLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _faviconFallbackLabel.backgroundColor = [UIColor clearColor];
    _faviconFallbackLabel.textAlignment = NSTextAlignmentCenter;
    _faviconFallbackLabel.isAccessibilityElement = NO;
    _faviconFallbackLabel.text = nil;

    [self addSubview:_faviconImage];
    [self addSubview:_faviconFallbackLabel];

    [_faviconImage setTranslatesAutoresizingMaskIntoConstraints:NO];
    [_faviconFallbackLabel setTranslatesAutoresizingMaskIntoConstraints:NO];
    AddSameConstraints(_faviconImage, self);
    AddSameConstraints(_faviconFallbackLabel, self);
    _faviconSizeConstraints = @[
      [self.widthAnchor constraintEqualToConstant:0],
      [self.heightAnchor constraintEqualToConstant:0],
    ];
    [NSLayoutConstraint activateConstraints:_faviconSizeConstraints];
  }
  return self;
}

- (void)setSize:(CGFloat)size {
  _size = size;
  for (NSLayoutConstraint* constraint in self.faviconSizeConstraints) {
    constraint.constant = size;
  }
}

@end
