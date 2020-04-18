// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/bookmarks/bookmark_empty_background.h"

#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
NSString* const kBookmarkGrayStar = @"bookmark_gray_star_large";
const CGFloat kEmptyBookmarkTextSize = 16.0;
// Offset of the image view on top of the text.
const CGFloat kImageViewOffsetFromText = 5.0;
// Height of the bottom toolbar outside of this background.
const CGFloat kToolbarHeight = 48.0;
}  // namespace

@interface BookmarkEmptyBackground ()

// Star image view shown on top of the label.
@property(nonatomic, retain) UIImageView* emptyBookmarksImageView;
// Label centered on the view showing the empty bookmarks text.
@property(nonatomic, retain) UILabel* emptyBookmarksLabel;

@end

@implementation BookmarkEmptyBackground

@synthesize emptyBookmarksImageView = _emptyBookmarksImageView;
@synthesize emptyBookmarksLabel = _emptyBookmarksLabel;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _emptyBookmarksImageView = [self newBookmarkImageView];
    [self addSubview:_emptyBookmarksImageView];
    _emptyBookmarksLabel = [self newEmptyBookmarkLabel];
    _emptyBookmarksLabel.accessibilityIdentifier = @"empty_background_label";
    [self addSubview:_emptyBookmarksLabel];
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  _emptyBookmarksLabel.frame = [self emptyBookmarkLabelFrame];
  _emptyBookmarksImageView.frame = [self bookmarkImageViewFrame];
}

- (NSString*)text {
  return self.emptyBookmarksLabel.text;
}

- (void)setText:(NSString*)text {
  self.emptyBookmarksLabel.text = text;
  [self setNeedsLayout];
}

#pragma mark - Private

- (UILabel*)newEmptyBookmarkLabel {
  UILabel* label = [[UILabel alloc] initWithFrame:CGRectZero];
  label.backgroundColor = [UIColor clearColor];
  label.font =
      [[MDCTypography fontLoader] mediumFontOfSize:kEmptyBookmarkTextSize];
  label.textColor = [UIColor colorWithWhite:0 alpha:110.0 / 255];
  label.textAlignment = NSTextAlignmentCenter;
  return label;
}

- (UIImageView*)newBookmarkImageView {
  UIImageView* imageView = [[UIImageView alloc] initWithFrame:CGRectZero];
  imageView.image = [UIImage imageNamed:kBookmarkGrayStar];
  return imageView;
}

// Returns vertically centered label frame.
- (CGRect)emptyBookmarkLabelFrame {
  const CGSize labelSizeThatFit =
      [self.emptyBookmarksLabel sizeThatFits:CGSizeZero];
  return CGRectMake(0,
                    (CGRectGetHeight(self.bounds) + kToolbarHeight +
                     labelSizeThatFit.height) /
                        2.0,
                    CGRectGetWidth(self.bounds), labelSizeThatFit.height);
}

// Returns imageView frame above the text with kImageViewOffsetFromText from
// text.
- (CGRect)bookmarkImageViewFrame {
  const CGRect labelRect = [self emptyBookmarkLabelFrame];
  const CGSize imageViewSize = self.emptyBookmarksImageView.image.size;
  return CGRectMake((CGRectGetWidth(self.bounds) - imageViewSize.width) / 2.0,
                    CGRectGetMinY(labelRect) - kImageViewOffsetFromText -
                        imageViewSize.height,
                    imageViewSize.width, imageViewSize.height);
}

@end
