// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_cell.h"

#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_view.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Identity view.
CGFloat kLeadingMargin = 8.;
CGFloat kIdentityViewVerticalMargin = 7.;
// Checkmark image.
CGFloat kAccessoryImageHeigth = 15.;
CGFloat kAccessoryImageTrailingMargin = 10.;
CGFloat kAccessoryImageWidth = 19.;
}  // namespace

@interface IdentityChooserCell ()
@property(nonatomic, strong) IdentityView* identityView;
@end

@implementation IdentityChooserCell

@synthesize identityView = _identityView;

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self) {
    _identityView = [[IdentityView alloc] initWithFrame:CGRectZero];
    _identityView.translatesAutoresizingMaskIntoConstraints = NO;
    _identityView.minimumVerticalMargin = kIdentityViewVerticalMargin;
    [self.contentView addSubview:_identityView];
    LayoutSides sideFlags = LayoutSides::kLeading | LayoutSides::kTrailing |
                            LayoutSides::kBottom | LayoutSides::kTop;
    ChromeDirectionalEdgeInsets insets =
        ChromeDirectionalEdgeInsetsMake(0, kLeadingMargin, 0, 0);
    AddSameConstraintsToSidesWithInsets(_identityView, self.contentView,
                                        sideFlags, insets);
  }
  return self;
}

- (void)configureCellWithTitle:(NSString*)title
                      subtitle:(NSString*)subtitle
                         image:(UIImage*)image
                       checked:(BOOL)checked {
  [self.identityView setTitle:title subtitle:subtitle];
  [self.identityView setAvatar:image];
  if (!checked) {
    self.accessoryView = nil;
  } else {
    // TODO(crbug.com/827072): Needs the real image.
    UIImage* checkedImage = ImageWithColor(UIColor.redColor);
    checkedImage = ResizeImage(
        checkedImage, CGSizeMake(kAccessoryImageWidth, kAccessoryImageHeigth),
        ProjectionMode::kFill);
    UIImageView* checkedImageView =
        [[UIImageView alloc] initWithImage:checkedImage];
    checkedImageView.translatesAutoresizingMaskIntoConstraints = NO;
    UIView* accessoryView = [[UIView alloc]
        initWithFrame:CGRectMake(
                          0, 0,
                          kAccessoryImageWidth + kAccessoryImageTrailingMargin,
                          kAccessoryImageHeigth)];
    [accessoryView addSubview:checkedImageView];
    LayoutSides sideFlags = LayoutSides::kLeading | LayoutSides::kTrailing |
                            LayoutSides::kBottom | LayoutSides::kTop;
    ChromeDirectionalEdgeInsets insets =
        ChromeDirectionalEdgeInsetsMake(0, 0, 0, kAccessoryImageTrailingMargin);
    AddSameConstraintsToSidesWithInsets(checkedImageView, accessoryView,
                                        sideFlags, insets);
    self.accessoryView = accessoryView;
  }
}

@end
