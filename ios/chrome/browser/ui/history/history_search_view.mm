// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/history_search_view.h"

#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Shadow opacity for the search view.
CGFloat kShadowOpacity = 0.2f;
// Margin for the search view.
CGFloat kHorizontalMargin = 16.0f;
}  // namespace

@interface HistorySearchView ()

// Stack view for laying out the text field and cancel button.
@property(nonatomic, strong) UIStackView* stackView;
// Text field for the search view.
@property(nonatomic, strong) UITextField* textField;
// Cancel button for dismissing the search view.
@property(nonatomic, strong) UIButton* cancelButton;
// Constraint for the top anchor.
@property(nonatomic, strong) NSLayoutConstraint* topAnchorConstraint;

@end

@implementation HistorySearchView

@synthesize enabled = _enabled;
@synthesize stackView = _stackView;
@synthesize textField = _textField;
@synthesize cancelButton = _cancelButton;
@synthesize topAnchorConstraint = _topAnchorConstraint;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    [self setBackgroundColor:[UIColor whiteColor]];
    [[self layer] setShadowOpacity:kShadowOpacity];

    _textField = [[UITextField alloc] init];
    _textField.contentVerticalAlignment =
        UIControlContentVerticalAlignmentCenter;
    _textField.backgroundColor = [UIColor whiteColor];
    _textField.textColor =
        [UIColor colorWithWhite:0 alpha:[MDCTypography body1FontOpacity]];
    _textField.font = [MDCTypography subheadFont];
    _textField.borderStyle = UITextBorderStyleNone;
    [_textField setLeftViewMode:UITextFieldViewModeNever];
    _textField.clearButtonMode = UITextFieldViewModeAlways;
    _textField.placeholder = l10n_util::GetNSString(IDS_HISTORY_SEARCH_BUTTON);

    _cancelButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [_cancelButton setImage:[UIImage imageNamed:@"collapse"]
                   forState:UIControlStateNormal];
    [_cancelButton setImage:[UIImage imageNamed:@"collapse_pressed"]
                   forState:UIControlStateHighlighted];
    [_cancelButton
        setContentCompressionResistancePriority:UILayoutPriorityRequired
                                        forAxis:
                                            UILayoutConstraintAxisHorizontal];
    [_cancelButton setContentHuggingPriority:UILayoutPriorityRequired
                                     forAxis:UILayoutConstraintAxisHorizontal];
    [_cancelButton setAccessibilityLabel:l10n_util::GetNSString(IDS_CANCEL)];

    _stackView = [[UIStackView alloc]
        initWithArrangedSubviews:@[ _textField, _cancelButton ]];
    _stackView.alignment = UIStackViewAlignmentFill;
    _stackView.axis = UILayoutConstraintAxisHorizontal;
    _stackView.distribution = UIStackViewDistributionFill;
    [self addSubview:_stackView];
    _stackView.translatesAutoresizingMaskIntoConstraints = NO;
    _stackView.layoutMarginsRelativeArrangement = YES;

    CGFloat topAnchorConstant = IsCompactWidth() ? StatusBarHeight() : 0;
    _topAnchorConstraint =
        [_stackView.topAnchor constraintEqualToAnchor:self.topAnchor
                                             constant:topAnchorConstant];
    [NSLayoutConstraint activateConstraints:@[
      _topAnchorConstraint,
      [_stackView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
      [_stackView.layoutMarginsGuide.leadingAnchor
          constraintEqualToAnchor:self.leadingAnchor
                         constant:kHorizontalMargin],
      [_stackView.layoutMarginsGuide.trailingAnchor
          constraintEqualToAnchor:self.trailingAnchor
                         constant:-kHorizontalMargin],
    ]];
  }
  return self;
}

- (BOOL)becomeFirstResponder {
  return [self.textField becomeFirstResponder];
}

- (void)setEnabled:(BOOL)enabled {
  _enabled = enabled;
  self.cancelButton.enabled = enabled;
  self.textField.enabled = enabled;
  self.textField.clearButtonMode =
      enabled ? UITextFieldViewModeAlways : UITextFieldViewModeNever;
}

- (void)setCancelTarget:(id)target action:(SEL)action {
  [_cancelButton addTarget:target
                    action:action
          forControlEvents:UIControlEventTouchUpInside];
}

- (void)setSearchBarDelegate:(id<UITextFieldDelegate>)delegate {
  [self.textField setDelegate:delegate];
}

- (void)clearText {
  self.textField.text = nil;
}

#pragma mark - UITraitEnvironment

- (void)traitCollectionDidChange:(UITraitCollection*)previousTraitCollection {
  self.topAnchorConstraint.constant = IsCompactWidth() ? StatusBarHeight() : 0;
}

@end
