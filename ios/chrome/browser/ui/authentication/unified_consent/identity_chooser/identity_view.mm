// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_view.h"

#include "base/logging.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Sizes.
const CGFloat kAvatarSize = 40.;
const CGFloat kTitleFontSize = 14.;
const CGFloat kSubtitleFontSize = 12.;
// Distances/margins.
const CGFloat kTitleOffset = 2;
const CGFloat kHorizontalAvatarMargin = 16.;
const CGFloat kVerticalMargin = 12.;
// Colors
const CGFloat kTitleTextColorAlpha = .87;
const CGFloat kSubtitleTextColorAlpha = .54;

}  // namespace

@interface IdentityView ()

// Avatar.
@property(nonatomic, strong) UIImageView* avatarView;
// Contains the name if it exists, otherwise it contains the email.
@property(nonatomic, strong) UILabel* title;
// Contains the email if the name exists, otherwise it is hidden.
@property(nonatomic, strong) UILabel* subtitle;
// Constraints if the name exists.
@property(nonatomic, strong) NSLayoutConstraint* titleConstraintForNameAndEmail;
// Constraints if the name doesn't exist.
@property(nonatomic, strong) NSLayoutConstraint* titleConstraintForEmailOnly;
// Constraints to update when |self.minimumVerticalMargin| is changed.
@property(nonatomic, strong) NSArray<NSLayoutConstraint*>* verticalConstraints;

@end

@implementation IdentityView

@synthesize avatarView = _avatarView;
@synthesize title = _title;
@synthesize subtitle = _subtitle;
@synthesize titleConstraintForNameAndEmail = _titleConstraintForNameAndEmail;
@synthesize titleConstraintForEmailOnly = _titleConstraintForEmailOnly;
@synthesize verticalConstraints = _verticalConstraints;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    self.userInteractionEnabled = NO;
    // Avatar view.
    _avatarView = [[UIImageView alloc] init];
    _avatarView.translatesAutoresizingMaskIntoConstraints = NO;
    [self addSubview:_avatarView];

    // Title.
    _title = [[UILabel alloc] init];
    _title.translatesAutoresizingMaskIntoConstraints = NO;
    _title.textColor = [UIColor colorWithWhite:0 alpha:kTitleTextColorAlpha];
    _title.font = [UIFont systemFontOfSize:kTitleFontSize];
    [self addSubview:_title];

    // Subtitle.
    _subtitle = [[UILabel alloc] init];
    _subtitle.translatesAutoresizingMaskIntoConstraints = NO;
    _subtitle.textColor =
        [UIColor colorWithWhite:0 alpha:kSubtitleTextColorAlpha];
    _subtitle.font = [UIFont systemFontOfSize:kSubtitleFontSize];
    [self addSubview:_subtitle];

    // Layout constraints.
    NSDictionary* views = @{
      @"avatar" : _avatarView,
      @"title" : _title,
      @"subtitle" : _subtitle,
    };
    NSDictionary* metrics = @{
      @"AvatarSize" : @(kAvatarSize),
      @"HAvatarMargin" : @(kHorizontalAvatarMargin),
      @"VMargin" : @(kVerticalMargin),
    };
    NSArray* constraints = @[
      // Horizontal constraints.
      @"H:|-(HAvatarMargin)-[avatar]-(HAvatarMargin)-[title]|",
      // Vertical constraints.
      // Size constraints.
      @"H:[avatar(AvatarSize)]",
      @"V:[avatar(AvatarSize)]",
    ];
    AddSameCenterYConstraint(self, _avatarView);
    ApplyVisualConstraintsWithMetrics(constraints, views, metrics);
    AddSameConstraintsToSides(_title, _subtitle,
                              LayoutSides::kLeading | LayoutSides::kTrailing);
    _titleConstraintForNameAndEmail =
        [self.centerYAnchor constraintEqualToAnchor:_title.bottomAnchor
                                           constant:kTitleOffset];
    _titleConstraintForEmailOnly =
        [self.centerYAnchor constraintEqualToAnchor:_title.centerYAnchor];
    [self.centerYAnchor constraintEqualToAnchor:_subtitle.topAnchor
                                       constant:-kTitleOffset]
        .active = YES;
    _verticalConstraints = @[
      [_avatarView.topAnchor constraintEqualToAnchor:self.topAnchor
                                            constant:kVerticalMargin],
      [self.bottomAnchor constraintEqualToAnchor:_avatarView.bottomAnchor
                                        constant:kVerticalMargin],
      [_title.topAnchor constraintGreaterThanOrEqualToAnchor:self.topAnchor
                                                    constant:kVerticalMargin],
      [self.bottomAnchor
          constraintGreaterThanOrEqualToAnchor:_subtitle.bottomAnchor
                                      constant:kVerticalMargin],
    ];
    [NSLayoutConstraint activateConstraints:_verticalConstraints];
  }
  return self;
}

#pragma mark - Setter

- (void)setAvatar:(UIImage*)avatarImage {
  if (avatarImage) {
    self.avatarView.image = CircularImageFromImage(avatarImage, kAvatarSize);
  } else {
    self.avatarView.image = nil;
  }
}

- (void)setTitle:(NSString*)title subtitle:(NSString*)subtitle {
  DCHECK(title);
  self.title.text = title;
  if (!subtitle.length) {
    self.titleConstraintForNameAndEmail.active = NO;
    self.titleConstraintForEmailOnly.active = YES;
    self.subtitle.hidden = YES;
  } else {
    self.titleConstraintForEmailOnly.active = NO;
    self.titleConstraintForNameAndEmail.active = YES;
    self.subtitle.hidden = NO;
    self.subtitle.text = subtitle;
  }
}

- (void)setMinimumVerticalMargin:(CGFloat)minimumVerticalMargin {
  for (NSLayoutConstraint* constraint in self.verticalConstraints) {
    constraint.constant = minimumVerticalMargin;
  }
}

- (CGFloat)minimumVerticalMargin {
  DCHECK(self.verticalConstraints.count);
  return self.verticalConstraints[0].constant;
}

@end
