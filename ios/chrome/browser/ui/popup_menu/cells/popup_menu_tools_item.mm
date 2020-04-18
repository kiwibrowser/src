// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/popup_menu/cells/popup_menu_tools_item.h"

#include <stdlib.h>

#include "base/logging.h"
#import "ios/chrome/browser/ui/popup_menu/popup_menu_constants.h"
#import "ios/chrome/browser/ui/reading_list/number_badge_view.h"
#import "ios/chrome/browser/ui/reading_list/text_badge_view.h"
#import "ios/chrome/browser/ui/table_view/chrome_table_view_styler.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/chrome/common/material_timing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const int kEnabledDefaultColor = 0x1A73E8;
const int kEnabledDestructiveColor = 0xEA4334;
const CGFloat kImageLength = 28;
const CGFloat kCellHeight = 44;
const CGFloat kInnerMargin = 11;
const CGFloat kMargin = 15;
const CGFloat kTopMargin = 8;
const CGFloat kTopMarginBadge = 14;
const CGFloat kMaxHeight = 100;
NSString* const kToolsMenuTextBadgeAccessibilityIdentifier =
    @"kToolsMenuTextBadgeAccessibilityIdentifier";
}  // namespace

@implementation PopupMenuToolsItem

@synthesize actionIdentifier = _actionIdentifier;
@synthesize badgeNumber = _badgeNumber;
@synthesize badgeText = _badgeText;
@synthesize image = _image;
@synthesize title = _title;
@synthesize enabled = _enabled;
@synthesize destructiveAction = _destructiveAction;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [PopupMenuToolsCell class];
    _enabled = YES;
  }
  return self;
}

- (void)configureCell:(PopupMenuToolsCell*)cell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:cell withStyler:styler];
  cell.titleLabel.text = self.title;
  cell.imageView.image = self.image;
  cell.accessibilityTraits = UIAccessibilityTraitButton;
  cell.userInteractionEnabled = self.enabled;
  cell.destructiveAction = self.destructiveAction;
  [cell setBadgeNumber:self.badgeNumber];
  [cell setBadgeText:self.badgeText];
}

#pragma mark - PopupMenuItem

- (CGSize)cellSizeForWidth:(CGFloat)width {
  // TODO(crbug.com/828357): This should be done at the table view level.
  static PopupMenuToolsCell* cell;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    cell = [[PopupMenuToolsCell alloc] init];
  });

  [self configureCell:cell withStyler:[[ChromeTableViewStyler alloc] init]];
  cell.frame = CGRectMake(0, 0, width, kMaxHeight);
  [cell setNeedsLayout];
  [cell layoutIfNeeded];
  return [cell systemLayoutSizeFittingSize:CGSizeMake(width, kMaxHeight)];
}

@end

#pragma mark - PopupMenuToolsCell

@interface PopupMenuToolsCell ()

// Title label for the cell, redefined as readwrite.
@property(nonatomic, strong, readwrite) UILabel* titleLabel;
// Image view for the cell, redefined as readwrite.
@property(nonatomic, strong, readwrite) UIImageView* imageView;
// Badge displaying a number.
@property(nonatomic, strong) NumberBadgeView* numberBadgeView;
// Badge displaying text.
@property(nonatomic, strong) TextBadgeView* textBadgeView;
// Constraints between the trailing of the label and the badges.
@property(nonatomic, strong) NSLayoutConstraint* titleToBadgeConstraint;
// Color for the title and the image.
@property(nonatomic, strong, readonly) UIColor* contentColor;

@end

@implementation PopupMenuToolsCell

@synthesize imageView = _imageView;
@synthesize numberBadgeView = _numberBadgeView;
@synthesize textBadgeView = _textBadgeView;
@synthesize titleLabel = _titleLabel;
@synthesize titleToBadgeConstraint = _titleToBadgeConstraint;
@synthesize destructiveAction = _destructiveAction;

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self) {
    UIView* selectedBackgroundView = [[UIView alloc] init];
    selectedBackgroundView.backgroundColor =
        [UIColor colorWithWhite:0 alpha:kSelectedItemBackgroundAlpha];
    self.selectedBackgroundView = selectedBackgroundView;

    _titleLabel = [[UILabel alloc] init];
    _titleLabel.numberOfLines = 0;
    _titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    [_titleLabel
        setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                        forAxis:
                                            UILayoutConstraintAxisHorizontal];
    [_titleLabel setContentHuggingPriority:UILayoutPriorityDefaultLow - 1
                                   forAxis:UILayoutConstraintAxisHorizontal];
    _titleLabel.translatesAutoresizingMaskIntoConstraints = NO;

    _imageView = [[UIImageView alloc] init];
    _imageView.translatesAutoresizingMaskIntoConstraints = NO;

    _numberBadgeView = [[NumberBadgeView alloc] init];
    _numberBadgeView.translatesAutoresizingMaskIntoConstraints = NO;

    _textBadgeView = [[TextBadgeView alloc] initWithText:nil];
    _textBadgeView.translatesAutoresizingMaskIntoConstraints = NO;
    _textBadgeView.accessibilityIdentifier =
        kToolsMenuTextBadgeAccessibilityIdentifier;
    _textBadgeView.hidden = YES;

    [self.contentView addSubview:_titleLabel];
    [self.contentView addSubview:_imageView];
    [self.contentView addSubview:_numberBadgeView];
    [self.contentView addSubview:_textBadgeView];

    ApplyVisualConstraintsWithMetrics(
        @[
          @"H:|-(margin)-[image(imageLength)]-(innerMargin)-[label]",
          @"H:[numberBadge]-(margin)-|", @"H:[textBadge]-(margin)-|",
          @"V:|-(topMargin)-[image(imageLength)]",
          @"V:|-(topMarginBadge)-[numberBadge]",
          @"V:|-(topMarginBadge)-[textBadge]",
          @"V:|-(topMargin)-[label]-(topMargin)-|"
        ],
        @{
          @"image" : _imageView,
          @"label" : _titleLabel,
          @"numberBadge" : _numberBadgeView,
          @"textBadge" : _textBadgeView
        },
        @{
          @"margin" : @(kMargin),
          @"innerMargin" : @(kInnerMargin),
          @"topMargin" : @(kTopMargin),
          @"topMarginBadge" : @(kTopMarginBadge),
          @"imageLength" : @(kImageLength),
        });

    [self.contentView.heightAnchor
        constraintGreaterThanOrEqualToConstant:kCellHeight]
        .active = YES;
    NSLayoutConstraint* trailingEdge = [_titleLabel.trailingAnchor
        constraintEqualToAnchor:self.contentView.trailingAnchor
                       constant:-kMargin];
    trailingEdge.priority = UILayoutPriorityDefaultHigh - 2;
    trailingEdge.active = YES;

    self.isAccessibilityElement = YES;
  }
  return self;
}

- (void)setBadgeNumber:(NSInteger)badgeNumber {
  BOOL wasHidden = self.numberBadgeView.hidden;
  [self.numberBadgeView setNumber:badgeNumber animated:NO];
  // If the number badge is shown, then the text badge must be hidden.
  if (!self.numberBadgeView.hidden && !self.textBadgeView.hidden) {
    [self setBadgeText:nil];
  }
  if (!self.numberBadgeView.hidden && wasHidden) {
    self.titleToBadgeConstraint.active = NO;
    self.titleToBadgeConstraint = [self.numberBadgeView.leadingAnchor
        constraintGreaterThanOrEqualToAnchor:self.titleLabel.trailingAnchor
                                    constant:kInnerMargin];
    self.titleToBadgeConstraint.active = YES;
  } else if (self.numberBadgeView.hidden && !wasHidden) {
    self.titleToBadgeConstraint.active = NO;
  }
}

- (void)setBadgeText:(NSString*)badgeText {
  // Only 1 badge can be visible at a time, and the number badge takes priority.
  if (badgeText && !self.numberBadgeView.isHidden) {
    return;
  }

  if (badgeText) {
    [self.textBadgeView setText:badgeText];
    if (self.textBadgeView.hidden) {
      self.textBadgeView.hidden = NO;
      self.titleToBadgeConstraint.active = NO;
      self.titleToBadgeConstraint = [self.textBadgeView.leadingAnchor
          constraintGreaterThanOrEqualToAnchor:self.titleLabel.trailingAnchor
                                      constant:kInnerMargin];
      self.titleToBadgeConstraint.active = YES;
      self.textBadgeView.alpha = 1;
    }
  } else if (!self.textBadgeView.hidden) {
    self.textBadgeView.hidden = YES;
    self.titleToBadgeConstraint.active = NO;
  }
}

- (void)setDestructiveAction:(BOOL)destructiveAction {
  _destructiveAction = destructiveAction;
  if (self.userInteractionEnabled) {
    self.titleLabel.textColor = self.contentColor;
    self.imageView.tintColor = self.contentColor;
  }
}

- (UIColor*)contentColor {
  if (self.destructiveAction)
    return UIColorFromRGB(kEnabledDestructiveColor);
  return UIColorFromRGB(kEnabledDefaultColor);
}

- (void)layoutSubviews {
  [super layoutSubviews];

  // Adjust the text label preferredMaxLayoutWidth when the parent's width
  // changes, for instance on screen rotation.
  CGFloat parentWidth = CGRectGetWidth(self.contentView.bounds);

  CGFloat trailingMargin = kMargin;
  if (!self.numberBadgeView.hidden) {
    trailingMargin += self.numberBadgeView.bounds.size.width + kInnerMargin;
  } else if (!self.textBadgeView.hidden) {
    trailingMargin += self.textBadgeView.bounds.size.width + kInnerMargin;
  }
  CGFloat leadingMargin = kMargin + kImageLength + kInnerMargin;

  self.titleLabel.preferredMaxLayoutWidth =
      parentWidth - leadingMargin - trailingMargin;

  // Re-layout with the new preferred width to allow the label to adjust its
  // height.
  [super layoutSubviews];
}

#pragma mark - UITableViewCell

- (void)prepareForReuse {
  [super prepareForReuse];
  self.userInteractionEnabled = YES;
  self.accessibilityTraits &= ~UIAccessibilityTraitNotEnabled;
}

- (void)setUserInteractionEnabled:(BOOL)userInteractionEnabled {
  [super setUserInteractionEnabled:userInteractionEnabled];
  if (userInteractionEnabled) {
    self.titleLabel.textColor = self.contentColor;
    self.imageView.tintColor = self.contentColor;
  } else {
    self.titleLabel.textColor = [[self class] disabledColor];
    self.imageView.tintColor = [[self class] disabledColor];
    self.accessibilityTraits |= UIAccessibilityTraitNotEnabled;
  }
}

#pragma mark - Private

// Returns the color of the disabled button's title.
+ (UIColor*)disabledColor {
  static UIColor* systemTintColorForDisabled = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    UIButton* button = [UIButton buttonWithType:UIButtonTypeSystem];
    systemTintColorForDisabled =
        [button titleColorForState:UIControlStateDisabled];
  });
  return systemTintColorForDisabled;
}

@end
