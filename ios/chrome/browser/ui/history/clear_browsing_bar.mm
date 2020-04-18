// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/clear_browsing_bar.h"

#include "base/logging.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#import "ios/chrome/browser/ui/history/history_ui_constants.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"
#import "ui/gfx/ios/NSString+CrStringDrawing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Shadow opacity for the clear browsing bar.
const CGFloat kShadowOpacity = 0.12f;
// Shadow radius for the clear browsing bar.
const CGFloat kShadowRadius = 12.0f;
// Horizontal margin and spacing for the contents of clear browsing bar.
const CGFloat kHorizontalMargin = 16.0f;
// Vertical margin for the contents of clear browsing bar.
const CGFloat kVerticalMargin = 8.0f;
// Horizontal spacing between the buttons inside clear browsing bar.
const CGFloat kHorizontalSpacing = 8.0f;
// Height of the toolbar in normal state.
const CGFloat kToolbarNormalHeight = 48.0f;
// Height of the expanded toolbar (buttons on multiple lines).
const CGFloat kToolbarExpandedHeight = 58.0f;
// Maximum proportion of the width of a button in the toolbar.
const CGFloat kMaxButtonWidthRatio = 0.52f;
// Enum to specify button position in the clear browsing bar.
typedef NS_ENUM(BOOL, ButtonPlacement) { Leading, Trailing };
}  // namespace

@interface ClearBrowsingBar ()

// Button that displays "Clear Browsing Data...".
@property(nonatomic, strong) UIButton* clearBrowsingDataButton;
@property(nonatomic, strong) UIView* clearButtonContainer;
// Button that displays "Edit".
@property(nonatomic, strong) UIButton* editButton;
@property(nonatomic, strong) UIView* editButtonContainer;
// Button that displays "Delete".
@property(nonatomic, strong) UIButton* deleteButton;
@property(nonatomic, strong) UIView* deleteButtonContainer;
// Button that displays "Cancel".
@property(nonatomic, strong) UIButton* cancelButton;
@property(nonatomic, strong) UIView* cancelButtonContainer;
// Stack view for arranging the buttons.
@property(nonatomic, strong) UIStackView* stackView;
// Height constraint for the stack view containing the buttons.
@property(nonatomic, strong) NSLayoutConstraint* heightConstraint;

// Styles button for Leading or Trailing placement. Leading buttons have red
// text that is aligned to the leading edge. Trailing buttons have blue text
// that is aligned to the trailing edge.
- (void)styleButton:(UIButton*)button forPlacement:(ButtonPlacement)placement;

@end

@implementation ClearBrowsingBar

@synthesize editing = _editing;
@synthesize clearBrowsingDataButton = _clearBrowsingDataButton;
@synthesize clearButtonContainer = _clearButtonContainer;
@synthesize editButton = _editButton;
@synthesize editButtonContainer = _editButtonContainer;
@synthesize deleteButton = _deleteButton;
@synthesize deleteButtonContainer = _deleteButtonContainer;
@synthesize cancelButton = _cancelButton;
@synthesize cancelButtonContainer = _cancelButtonContainer;
@synthesize stackView = _stackView;
@synthesize heightConstraint = _heightConstraint;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    NSDictionary* views = nil;
    NSArray* constraints = nil;

    _clearBrowsingDataButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [_clearBrowsingDataButton
        setTitle:l10n_util::GetNSStringWithFixup(
                     IDS_HISTORY_OPEN_CLEAR_BROWSING_DATA_DIALOG)
        forState:UIControlStateNormal];
    _clearBrowsingDataButton.accessibilityIdentifier =
        kHistoryToolbarClearBrowsingButtonIdentifier;

    [self styleButton:_clearBrowsingDataButton forPlacement:Leading];
    _clearButtonContainer = [[UIView alloc] init];
    [_clearButtonContainer addSubview:_clearBrowsingDataButton];
    views = @{@"button" : _clearBrowsingDataButton};
    constraints = @[ @"V:|[button]|", @"H:|[button]" ];
    ApplyVisualConstraints(constraints, views);
    [_clearBrowsingDataButton.trailingAnchor
        constraintLessThanOrEqualToAnchor:_clearButtonContainer.trailingAnchor]
        .active = YES;

    _editButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [_editButton
        setTitle:l10n_util::GetNSString(IDS_HISTORY_START_EDITING_BUTTON)
        forState:UIControlStateNormal];
    _editButton.accessibilityIdentifier = kHistoryToolbarEditButtonIdentifier;

    [self styleButton:_editButton forPlacement:Trailing];
    _editButtonContainer = [[UIView alloc] init];
    [_editButtonContainer addSubview:_editButton];
    views = @{@"button" : _editButton};
    constraints = @[ @"V:|[button]|", @"H:[button]|" ];
    ApplyVisualConstraints(constraints, views);
    [_editButton.leadingAnchor
        constraintGreaterThanOrEqualToAnchor:_editButtonContainer.leadingAnchor]
        .active = YES;

    _deleteButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [_deleteButton setTitle:l10n_util::GetNSString(
                                IDS_HISTORY_DELETE_SELECTED_ENTRIES_BUTTON)
                   forState:UIControlStateNormal];
    _deleteButton.accessibilityIdentifier =
        kHistoryToolbarDeleteButtonIdentifier;
    [self styleButton:_deleteButton forPlacement:Leading];
    _deleteButtonContainer = [[UIView alloc] init];
    [_deleteButtonContainer addSubview:_deleteButton];
    views = @{@"button" : _deleteButton};
    constraints = @[ @"V:|[button]|", @"H:|[button]" ];
    ApplyVisualConstraints(constraints, views);
    [_deleteButton.trailingAnchor
        constraintLessThanOrEqualToAnchor:_deleteButtonContainer.trailingAnchor]
        .active = YES;

    _cancelButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [_cancelButton
        setTitle:l10n_util::GetNSString(IDS_HISTORY_CANCEL_EDITING_BUTTON)
        forState:UIControlStateNormal];
    _cancelButton.accessibilityIdentifier =
        kHistoryToolbarCancelButtonIdentifier;
    [self styleButton:_cancelButton forPlacement:Trailing];
    _cancelButtonContainer = [[UIView alloc] init];
    [_cancelButtonContainer addSubview:_cancelButton];
    views = @{@"button" : _cancelButton};
    constraints = @[ @"V:|[button]|", @"H:[button]|" ];
    ApplyVisualConstraints(constraints, views);
    [_cancelButton.leadingAnchor
        constraintGreaterThanOrEqualToAnchor:_cancelButtonContainer
                                                 .leadingAnchor]
        .active = YES;

    _stackView = [[UIStackView alloc] initWithArrangedSubviews:@[
      _clearButtonContainer, _editButtonContainer, _deleteButtonContainer,
      _cancelButtonContainer
    ]];
    _stackView.alignment = UIStackViewAlignmentCenter;
    _stackView.distribution = UIStackViewDistributionEqualSpacing;
    _stackView.axis = UILayoutConstraintAxisHorizontal;

    [self addSubview:_stackView];
    _stackView.translatesAutoresizingMaskIntoConstraints = NO;

    PinToSafeArea(_stackView, self);
    _heightConstraint = [_stackView.heightAnchor
        constraintEqualToConstant:kToolbarNormalHeight];
    _heightConstraint.active = YES;
    _stackView.layoutMarginsRelativeArrangement = YES;
    _stackView.layoutMargins = UIEdgeInsetsMake(
        kVerticalMargin, kHorizontalMargin, kVerticalMargin, kHorizontalMargin);
    _stackView.spacing = kHorizontalSpacing;

    for (UIButton* button in @[
           _clearBrowsingDataButton, _editButton, _deleteButton, _cancelButton
         ]) {
      [button.widthAnchor
          constraintLessThanOrEqualToAnchor:_stackView.widthAnchor
                                 multiplier:kMaxButtonWidthRatio]
          .active = YES;
    }

    [self setBackgroundColor:[UIColor whiteColor]];
    [[self layer] setShadowOpacity:kShadowOpacity];
    [[self layer] setShadowRadius:kShadowRadius];
    [self setEditing:NO];
  }
  return self;
}

#pragma mark Public Methods

- (void)setEditing:(BOOL)editing {
  _editing = editing;
  self.clearButtonContainer.hidden = editing;
  self.editButtonContainer.hidden = editing;
  self.deleteButtonContainer.hidden = !editing;
  self.cancelButtonContainer.hidden = !editing;

  [self updateHeight];
}

- (BOOL)isEditButtonEnabled {
  return self.editButton.enabled;
}

- (void)setEditButtonEnabled:(BOOL)editButtonEnabled {
  self.editButton.enabled = editButtonEnabled;
}

- (BOOL)isDeleteButtonEnabled {
  return self.deleteButton.enabled;
}

- (void)setDeleteButtonEnabled:(BOOL)deleteButtonEnabled {
  self.deleteButton.enabled = deleteButtonEnabled;
}

- (void)setClearBrowsingDataTarget:(id)target action:(SEL)action {
  [self.clearBrowsingDataButton addTarget:target
                                   action:action
                         forControlEvents:UIControlEventTouchUpInside];
}

- (void)setEditTarget:(id)target action:(SEL)action {
  [self.editButton addTarget:target
                      action:action
            forControlEvents:UIControlEventTouchUpInside];
}

- (void)setDeleteTarget:(id)target action:(SEL)action {
  [self.deleteButton addTarget:target
                        action:action
              forControlEvents:UIControlEventTouchUpInside];
}

- (void)setCancelTarget:(id)target action:(SEL)action {
  [self.cancelButton addTarget:target
                        action:action
              forControlEvents:UIControlEventTouchUpInside];
}

- (void)updateHeight {
  NSArray* buttons =
      @[ _clearBrowsingDataButton, _editButton, _deleteButton, _cancelButton ];

  CGFloat buttonMaxWidth = self.frame.size.width * kMaxButtonWidthRatio;
  CGFloat availableWidth = self.frame.size.width - kHorizontalMargin * 2;
  NSUInteger visibleCount = 0;

  // Count the number of visible buttons and deduct the button spacings from
  // availableWidth.
  for (UIButton* button in buttons) {
    if (!button.superview.hidden) {
      visibleCount++;
      if (visibleCount > 1) {
        availableWidth -= kHorizontalSpacing;
      }
    }
  }

  // Expand toolbar height in case word wrapping happens.
  for (UIButton* button in buttons) {
    if (!button.superview.hidden) {
      CGFloat rect = [button.titleLabel.text
                         cr_pixelAlignedSizeWithFont:button.titleLabel.font]
                         .width;
      if (rect > availableWidth || rect > buttonMaxWidth) {
        self.heightConstraint.constant = kToolbarExpandedHeight;
        return;
      }
      availableWidth -= rect;
    }
  }

  // Use the normal height when there is no word wrapping.
  self.heightConstraint.constant = kToolbarNormalHeight;
}

#pragma mark Private Methods

- (void)styleButton:(UIButton*)button forPlacement:(ButtonPlacement)placement {
  BOOL leading = placement == Leading;
  BOOL alignmentLeft = leading ^ UseRTLLayout();
  [button setBackgroundColor:[UIColor whiteColor]];
  UIColor* textColor = leading ? [[MDCPalette cr_redPalette] tint500]
                               : [[MDCPalette cr_bluePalette] tint500];
  [button setTitleColor:textColor forState:UIControlStateNormal];
  [button setTitleColor:[[MDCPalette greyPalette] tint500]
               forState:UIControlStateDisabled];
  [[button titleLabel] setFont:[MDCTypography subheadFont]];
  button.titleLabel.textAlignment =
      alignmentLeft ? NSTextAlignmentLeft : NSTextAlignmentRight;
  button.contentHorizontalAlignment =
      alignmentLeft ? UIControlContentHorizontalAlignmentLeft
                    : UIControlContentHorizontalAlignmentRight;
  [button setTranslatesAutoresizingMaskIntoConstraints:NO];
  button.titleLabel.numberOfLines = 2;
  button.titleLabel.adjustsFontSizeToFitWidth = YES;
}

@end
