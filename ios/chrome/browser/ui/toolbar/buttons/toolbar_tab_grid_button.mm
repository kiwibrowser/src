// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_tab_grid_button.h"

#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_constants.h"
#import "ios/chrome/browser/ui/toolbar/public/toolbar_controller_base_feature.h"
#include "ios/chrome/browser/ui/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kLabelMargin = 7;
}  // namespace

@implementation ToolbarTabGridButton

- (void)setTabCount:(int)tabCount {
  // Update the text shown in the title of this button. Note that
  // the button's title may be empty or contain an easter egg, but the
  // accessibility value will always be equal to |tabCount|.
  NSString* tabStripButtonValue = [NSString stringWithFormat:@"%d", tabCount];
  NSString* tabStripButtonTitle = tabStripButtonValue;
  self.titleLabel.font =
      [UIFont systemFontOfSize:kTabGridButtonFontSize weight:UIFontWeightBold];
  if (tabCount <= 0) {
    tabStripButtonTitle = @"";
  } else if (tabCount > kShowTabStripButtonMaxTabCount) {
    // As an easter egg, show a smiley face instead of the count if the user has
    // more than 99 tabs open.
    tabStripButtonTitle = @":)";
  }

  // TODO(crbug.com/799601): Delete this once its not needed.
  if (base::FeatureList::IsEnabled(kMemexTabSwitcher)) {
    tabStripButtonTitle = @"M";
  }

  self.titleLabel.adjustsFontSizeToFitWidth = YES;
  self.titleLabel.minimumScaleFactor = 0.1;
  self.titleLabel.baselineAdjustment = UIBaselineAdjustmentAlignCenters;

  [self setTitle:tabStripButtonTitle forState:UIControlStateNormal];
  [self setAccessibilityValue:tabStripButtonValue];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  CGSize size = self.bounds.size;
  CGPoint center = CGPointMake(size.width / 2, size.height / 2);
  self.imageView.center = center;
  CGRect imageFrame = self.imageView.frame;
  self.imageView.frame = AlignRectToPixel(imageFrame);
  imageFrame = UIEdgeInsetsInsetRect(
      imageFrame,
      UIEdgeInsetsMake(kLabelMargin, kLabelMargin, kLabelMargin, kLabelMargin));
  self.titleLabel.frame = imageFrame;
}

@end
