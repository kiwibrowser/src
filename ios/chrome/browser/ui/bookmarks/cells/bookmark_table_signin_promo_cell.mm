// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/cells/bookmark_table_signin_promo_cell.h"

#import "ios/chrome/browser/ui/authentication/signin_promo_view.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_chromium_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const NSInteger kSigninPromoMargin = 8;
}

@implementation BookmarkTableSigninPromoCell {
  SigninPromoView* _signinPromoView;
  UIButton* _closeButton;
}

@synthesize signinPromoView = _signinPromoView;

+ (NSString*)reuseIdentifier {
  return @"BookmarkTableSigninPromoCell";
}

+ (BOOL)requiresConstraintBasedLayout {
  return YES;
}

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self) {
    UIView* contentView = self.contentView;
    _signinPromoView = [[SigninPromoView alloc] initWithFrame:self.bounds];
    _signinPromoView.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_signinPromoView];
    _signinPromoView.layer.borderColor =
        [UIColor colorWithWhite:0.0 alpha:0.08].CGColor;
    _signinPromoView.layer.borderWidth = 1.0f;
    NSArray* visualConstraints = @[
      @"V:|-(margin)-[signin_promo_view]-(margin)-|",
      @"H:|-(margin)-[signin_promo_view]-(margin)-|",
    ];
    NSDictionary* views = @{@"signin_promo_view" : _signinPromoView};
    NSDictionary* metrics = @{ @"margin" : @(kSigninPromoMargin) };
    ApplyVisualConstraintsWithMetrics(visualConstraints, views, metrics);
    _signinPromoView.backgroundColor = [UIColor whiteColor];
    _signinPromoView.textLabel.text =
        l10n_util::GetNSString(IDS_IOS_SIGNIN_PROMO_BOOKMARKS);
  }
  return self;
}

- (void)layoutSubviews {
  // Adjust the text label preferredMaxLayoutWidth according self.frame.width,
  // so the text will adjust its height and not its width.
  CGFloat parentWidth = CGRectGetWidth(self.bounds);
  _signinPromoView.textLabel.preferredMaxLayoutWidth =
      parentWidth - 2 * _signinPromoView.horizontalPadding;

  // Re-layout with the new preferred width to allow the label to adjust its
  // height.
  [super layoutSubviews];
}

- (void)prepareForReuse {
  [super prepareForReuse];
  [_signinPromoView prepareForReuse];
}

@end
