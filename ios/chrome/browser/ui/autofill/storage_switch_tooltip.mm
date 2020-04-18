// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/autofill/storage_switch_tooltip.h"

#include "base/logging.h"
#include "components/strings/grit/components_strings.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const CGFloat kCornerRadius = 2.0f;
const CGFloat kFontSize = 12.0f;
const CGFloat kInset = 8.0f;

}  // namespace

@implementation StorageSwitchTooltip

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    NSString* tooltipText =
        l10n_util::GetNSString(IDS_AUTOFILL_CARD_UNMASK_PROMPT_STORAGE_TOOLTIP);
    [self setText:tooltipText];
    [self setTextColor:[UIColor whiteColor]];
    [self setBackgroundColor:[UIColor colorWithWhite:0.0 alpha:0.9]];
    [[self layer] setCornerRadius:kCornerRadius];
    [[self layer] setMasksToBounds:YES];
    [self setFont:[[MDCTypography fontLoader] regularFontOfSize:kFontSize]];
    [self setNumberOfLines:0];  // Allows multi-line layout.
  }
  return self;
}

- (instancetype)init {
  return [self initWithFrame:CGRectZero];
}

- (instancetype)initWithCoder:(NSCoder*)aDecoder {
  NOTREACHED();
  return nil;
}

// The logic in textRectForBounds:limitedToNumberOfLines: and drawTextInRect:
// adds an inset. Based on
// http://stackoverflow.com/questions/21167226/resizing-a-uilabel-to-accomodate-insets/21267507#21267507
- (CGRect)textRectForBounds:(CGRect)bounds
     limitedToNumberOfLines:(NSInteger)numberOfLines {
  UIEdgeInsets insets = {kInset, kInset, kInset, kInset};
  CGRect rect = [super textRectForBounds:UIEdgeInsetsInsetRect(bounds, insets)
                  limitedToNumberOfLines:numberOfLines];

  rect.origin.x -= insets.left;
  rect.origin.y -= insets.top;
  rect.size.width += (insets.left + insets.right);
  rect.size.height += (insets.top + insets.bottom);

  return rect;
}

- (void)drawTextInRect:(CGRect)rect {
  UIEdgeInsets insets = {kInset, kInset, kInset, kInset};
  [super drawTextInRect:UIEdgeInsetsInsetRect(rect, insets)];
}

@end
