// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/reading_list_toolbar.h"

#import "ios/chrome/browser/ui/alert_coordinator/action_sheet_coordinator.h"
#import "ios/chrome/browser/ui/reading_list/reading_list_toolbar_button.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kToolbarNormalHeight = 48.0f;
// Height of the toolbar when button labels has word-wrap up to 2 lines.
const CGFloat kToolbarTwoLinesHeight = 58.0f;
// Height of the toolbar when button labels has word-wrap up to 3 lines.
const CGFloat kToolbarThreeLinesHeight = 68.0f;
// Shadow opacity.
const CGFloat kShadowOpacity = 0.12f;
// Shadow radius.
const CGFloat kShadowRadius = 12.0f;
// Horizontal margin for the stack view content.
const CGFloat kHorizontalMargin = 16.0f;
// Vertical margin for the stack view content.
const CGFloat kVerticalMargin = 8.0f;
// Horizontal spacing between the buttons of the stack view.  This is used for
// calculating the maximum width of buttons but NOT used directly to set
// stackView.spacing.
const CGFloat kHorizontalSpacing = 8.0f;

}  // namespace

@interface ReadingListToolbar ()

// Button that displays "Edit".
@property(nonatomic, strong) ReadingListToolbarButton* editButton;
// Button that displays "Delete".
@property(nonatomic, strong) ReadingListToolbarButton* deleteButton;
// Button that displays "Delete All Read".
@property(nonatomic, strong) ReadingListToolbarButton* deleteAllButton;
// Button that displays "Cancel".
@property(nonatomic, strong) ReadingListToolbarButton* cancelButton;
// Button that displays the mark options.
@property(nonatomic, strong) ReadingListToolbarButton* markButton;
// Stack view for arranging the buttons.
@property(nonatomic, strong) UIStackView* stackView;
// Height constraint for the stack view containing the buttons.
@property(nonatomic, strong) NSLayoutConstraint* heightConstraint;

// Set the mark button label to |text|.
- (void)setMarkButtonText:(NSString*)text;
// Updates the button labels to match an empty selection.
- (void)updateButtonsForEmptySelection;
// Updates the button labels to match a selection containing only read items.
- (void)updateButtonsForOnlyReadSelection;
// Updates the button labels to match a selection containing only unread items.
- (void)updateButtonsForOnlyUnreadSelection;
// Updates the button labels to match a selection containing unread and read
// items.
- (void)updateButtonsForOnlyMixedSelection;

@end

@implementation ReadingListToolbar

@synthesize editButton = _editButton;
@synthesize deleteButton = _deleteButton;
@synthesize deleteAllButton = _deleteAllButton;
@synthesize cancelButton = _cancelButton;
@synthesize markButton = _markButton;
@synthesize stackView = _stackView;
@synthesize state = _state;
@synthesize heightConstraint = _heightConstraint;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _deleteButton = [[ReadingListToolbarButton alloc]
        initWithText:l10n_util::GetNSString(IDS_IOS_READING_LIST_DELETE_BUTTON)
         destructive:YES
            position:Leading];

    _deleteAllButton = [[ReadingListToolbarButton alloc]
        initWithText:l10n_util::GetNSString(
                         IDS_IOS_READING_LIST_DELETE_ALL_READ_BUTTON)
         destructive:YES
            position:Leading];

    _markButton = [[ReadingListToolbarButton alloc]
        initWithText:l10n_util::GetNSString(
                         IDS_IOS_READING_LIST_MARK_ALL_BUTTON)
         destructive:NO
            position:Centered];

    _cancelButton = [[ReadingListToolbarButton alloc]
        initWithText:l10n_util::GetNSString(IDS_IOS_READING_LIST_CANCEL_BUTTON)
         destructive:NO
            position:Trailing];

    _editButton = [[ReadingListToolbarButton alloc]
        initWithText:l10n_util::GetNSString(IDS_IOS_READING_LIST_EDIT_BUTTON)
         destructive:NO
            position:Trailing];

    [_editButton addTarget:nil
                    action:@selector(enterEditingModePressed)
          forControlEvents:UIControlEventTouchUpInside];

    [_deleteButton addTarget:nil
                      action:@selector(deletePressed)
            forControlEvents:UIControlEventTouchUpInside];

    [_deleteAllButton addTarget:nil
                         action:@selector(deletePressed)
               forControlEvents:UIControlEventTouchUpInside];

    [_markButton addTarget:nil
                    action:@selector(markPressed)
          forControlEvents:UIControlEventTouchUpInside];

    [_cancelButton addTarget:nil
                      action:@selector(exitEditingModePressed)
            forControlEvents:UIControlEventTouchUpInside];

    _stackView = [[UIStackView alloc] initWithArrangedSubviews:@[
      _editButton, _deleteButton, _deleteAllButton, _markButton, _cancelButton
    ]];
    _stackView.axis = UILayoutConstraintAxisHorizontal;
    _stackView.alignment = UIStackViewAlignmentFill;
    _stackView.distribution = UIStackViewDistributionFillEqually;

    [self addSubview:_stackView];
    _stackView.translatesAutoresizingMaskIntoConstraints = NO;

    PinToSafeArea(_stackView, self);
    _heightConstraint = [_stackView.heightAnchor
        constraintEqualToConstant:kToolbarNormalHeight];
    _heightConstraint.active = YES;

    _stackView.layoutMargins = UIEdgeInsetsMake(
        kVerticalMargin, kHorizontalMargin, kVerticalMargin, kHorizontalMargin);
    _stackView.layoutMarginsRelativeArrangement = YES;

    self.backgroundColor = [UIColor whiteColor];
    [[self layer] setShadowOpacity:kShadowOpacity];
    [[self layer] setShadowRadius:kShadowRadius];
    [self setEditing:NO];
  }
  return self;
}

#pragma mark Public Methods

- (void)setEditing:(BOOL)editing {
  self.editButton.hidden = editing;
  self.deleteButton.hidden = YES;
  self.deleteAllButton.hidden = !editing;
  self.cancelButton.hidden = !editing;
  self.markButton.hidden = !editing;

  [self updateHeight];
}

- (void)setState:(ReadingListToolbarState)state {
  switch (state) {
    case NoneSelected:
      [self updateButtonsForEmptySelection];
      break;
    case OnlyReadSelected:
      [self updateButtonsForOnlyReadSelection];
      break;
    case OnlyUnreadSelected:
      [self updateButtonsForOnlyUnreadSelection];
      break;
    case MixedItemsSelected:
      [self updateButtonsForOnlyMixedSelection];
      break;
  }
  _state = state;

  [self updateHeight];
}

- (void)setHasReadItem:(BOOL)hasRead {
  [self.deleteAllButton setEnabled:hasRead];
}

- (ActionSheetCoordinator*)actionSheetForMarkWithBaseViewController:
    (UIViewController*)viewController {
  return [[ActionSheetCoordinator alloc]
      initWithBaseViewController:viewController
                           title:nil
                         message:nil
                            rect:self.markButton.bounds
                            view:self.markButton];
}

#pragma mark Private Methods

- (void)updateButtonsForEmptySelection {
  self.deleteAllButton.hidden = NO;
  self.deleteButton.hidden = YES;
  [self setMarkButtonText:l10n_util::GetNSStringWithFixup(
                              IDS_IOS_READING_LIST_MARK_ALL_BUTTON)];
}

- (void)updateButtonsForOnlyReadSelection {
  self.deleteAllButton.hidden = YES;
  self.deleteButton.hidden = NO;
  [self setMarkButtonText:l10n_util::GetNSStringWithFixup(
                              IDS_IOS_READING_LIST_MARK_UNREAD_BUTTON)];
}

- (void)updateButtonsForOnlyUnreadSelection {
  self.deleteAllButton.hidden = YES;
  self.deleteButton.hidden = NO;
  [self setMarkButtonText:l10n_util::GetNSStringWithFixup(
                              IDS_IOS_READING_LIST_MARK_READ_BUTTON)];
}

- (void)updateButtonsForOnlyMixedSelection {
  self.deleteAllButton.hidden = YES;
  self.deleteButton.hidden = NO;
  [self setMarkButtonText:l10n_util::GetNSStringWithFixup(
                              IDS_IOS_READING_LIST_MARK_BUTTON)];
}


- (void)setMarkButtonText:(NSString*)text {
  [self.markButton setTitle:text];
}

- (void)updateHeight {
  NSArray* buttons = @[
    _editButton, _deleteButton, _deleteAllButton, _markButton, _cancelButton
  ];

  CGFloat availableWidth = self.frame.size.width - kHorizontalMargin * 2;
  NSUInteger visibleCount = 0;

  // Count the number of visible buttons and deduct the button spacings from
  // availableWidth.
  for (ReadingListToolbarButton* button in buttons) {
    if (!button.hidden) {
      visibleCount++;
      if (visibleCount > 1) {
        availableWidth -= kHorizontalSpacing;
      }
    }
  }

  // Set the button width manually here instead of relying on UIStackView's auto
  // width distribution which is unpredictable when rounding happens.
  CGFloat maxButtonWidth = ceil(availableWidth / visibleCount);
  for (ReadingListToolbarButton* button in buttons) {
    if (!button.hidden) {
      [button setMaxWidth:maxButtonWidth];
    }
  }

  CGFloat toolbarHeight = kToolbarNormalHeight;
  CGFloat lineHeight = ceil([ReadingListToolbarButton textFont].lineHeight);
  CGSize labelBounds = CGSizeMake(maxButtonWidth, CGFLOAT_MAX);
  // Expand toolbar height in case word wrapping happens.
  for (ReadingListToolbarButton* button in buttons) {
    if (!button.hidden) {
      CGFloat labelHeight =
          [[button titleLabel] sizeThatFits:labelBounds].height;
      if (labelHeight > lineHeight * 2) {
        toolbarHeight = kToolbarThreeLinesHeight;
        break;
      }
      if (labelHeight > lineHeight) {
        toolbarHeight = kToolbarTwoLinesHeight;
      }
    }
  }
  self.heightConstraint.constant = toolbarHeight;
}

@end
