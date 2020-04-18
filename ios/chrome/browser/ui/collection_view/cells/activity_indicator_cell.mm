// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/collection_view/cells/activity_indicator_cell.h"

#import "ios/chrome/browser/ui/collection_view/cells/MDCCollectionViewCell+Chrome.h"
#import "ios/chrome/browser/ui/collection_view/cells/collection_view_item.h"
#import "ios/chrome/browser/ui/material_components/activity_indicator.h"
#import "ios/third_party/material_components_ios/src/components/ActivityIndicator/src/MaterialActivityIndicator.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

const CGFloat kIndicatorSize = 32;

@implementation ActivityIndicatorCell

@synthesize activityIndicator = _activityIndicator;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _activityIndicator = [[MDCActivityIndicator alloc]
        initWithFrame:CGRectMake(0, 0, kIndicatorSize, kIndicatorSize)];
    _activityIndicator.cycleColors = ActivityIndicatorBrandedCycleColors();
    [self.contentView addSubview:_activityIndicator];
    _activityIndicator.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
      [_activityIndicator.topAnchor
          constraintEqualToAnchor:self.contentView.topAnchor],
      [_activityIndicator.bottomAnchor
          constraintEqualToAnchor:self.contentView.bottomAnchor],
      [_activityIndicator.leadingAnchor
          constraintEqualToAnchor:self.contentView.leadingAnchor],
      [_activityIndicator.trailingAnchor
          constraintEqualToAnchor:self.contentView.trailingAnchor],
    ]];
    [_activityIndicator startAnimating];
  }
  return self;
}

@end
