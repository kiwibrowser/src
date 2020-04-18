// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/grid/top_aligned_image_view.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TopAlignedImageView ()
// The backing image view.
@property(nonatomic, weak) UIImageView* innerImageView;
@end

@implementation TopAlignedImageView
@synthesize innerImageView = _innerImageView;

- (instancetype)init {
  if (self = [super initWithFrame:CGRectZero]) {
    UIImageView* innerImageView = [[UIImageView alloc] init];
    [self addSubview:innerImageView];
    _innerImageView = innerImageView;
    _innerImageView.contentMode = UIViewContentModeScaleAspectFill;
    _innerImageView.backgroundColor = [UIColor clearColor];
    self.clipsToBounds = YES;
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  if (!self.image)
    return;
  CGFloat widthScaleFactor = CGRectGetWidth(self.frame) / self.image.size.width;
  CGFloat heightScaleFactor =
      CGRectGetHeight(self.frame) / self.image.size.height;
  CGFloat imageViewWidth;
  CGFloat imageViewHeight;
  if (widthScaleFactor > heightScaleFactor) {
    imageViewWidth = self.image.size.width * widthScaleFactor;
    imageViewHeight = self.image.size.height * widthScaleFactor;
  } else {
    imageViewWidth = self.image.size.width * heightScaleFactor;
    imageViewHeight = self.image.size.height * heightScaleFactor;
  }
  self.innerImageView.frame =
      CGRectMake((self.frame.size.width - imageViewWidth) / 2.0f, 0,
                 imageViewWidth, imageViewHeight);
}

#pragma mark - Public properties

- (void)setImage:(UIImage*)image {
  self.innerImageView.image = image;
  [self setNeedsLayout];
}

- (UIImage*)image {
  return self.innerImageView.image;
}

@end
