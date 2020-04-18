// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/table_view/cells/table_view_text_button_item.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Text label gray color.
const CGFloat grayHexColor = 0x6d6d72;
// Action button blue background color.
const CGFloat blueHexColor = 0x1A73E8;
// Vertical spacing between stackView and cell contentView.
const CGFloat stackViewVerticalSpacing = 9.0;
// Horizontal spacing between stackView and cell contentView.
const CGFloat stackViewHorizontalSpacing = 16.0;
// SubView spacing within stackView.
const CGFloat stackViewSubViewSpacing = 13.0;
// Horizontal Inset between button contents and edge.
const CGFloat buttonTitleHorizontalContentInset = 40.0;
// Vertical Inset between button contents and edge.
const CGFloat buttonTitleVerticalContentInset = 8.0;
// Button corner radius.
const CGFloat buttonCornerRadius = 8;
}  // namespace

@implementation TableViewTextButtonItem
@synthesize delegate = _delegate;
@synthesize text = _text;
@synthesize buttonText = _buttonText;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [TableViewTextButtonCell class];
  }
  return self;
}

- (void)configureCell:(UITableViewCell*)tableCell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:tableCell withStyler:styler];
  TableViewTextButtonCell* cell =
      base::mac::ObjCCastStrict<TableViewTextButtonCell>(tableCell);
  cell.delegate = self.delegate;
  cell.textLabel.text = self.text;
  [cell.button setTitle:self.buttonText forState:UIControlStateNormal];
  [cell setSelectionStyle:UITableViewCellSelectionStyleNone];
}

@end

@implementation TableViewTextButtonCell
@synthesize delegate = _delegate;
@synthesize textLabel = _textLabel;
@synthesize button = _button;

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self) {
    // Create informative text label.
    self.textLabel = [[UILabel alloc] init];
    self.textLabel.numberOfLines = 0;
    self.textLabel.lineBreakMode = NSLineBreakByWordWrapping;
    self.textLabel.textAlignment = NSTextAlignmentCenter;
    self.textLabel.font =
        [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    self.textLabel.textColor = UIColorFromRGB(grayHexColor);

    // Create button.
    self.button = [[UIButton alloc] init];
    self.button.backgroundColor = UIColorFromRGB(blueHexColor);
    [self.button setTitleColor:[UIColor whiteColor]
                      forState:UIControlStateNormal];
    self.button.translatesAutoresizingMaskIntoConstraints = NO;
    [self.button.titleLabel
        setFont:[UIFont preferredFontForTextStyle:UIFontTextStyleHeadline]];
    self.button.layer.cornerRadius = buttonCornerRadius;
    self.button.clipsToBounds = YES;
    self.button.contentEdgeInsets = UIEdgeInsetsMake(
        buttonTitleVerticalContentInset, buttonTitleHorizontalContentInset,
        buttonTitleVerticalContentInset, buttonTitleHorizontalContentInset);
    [self.button addTarget:self
                    action:@selector(performButtonAction)
          forControlEvents:UIControlEventTouchUpInside];

    // Vertical stackView to hold label and button.
    UIStackView* verticalStackView = [[UIStackView alloc]
        initWithArrangedSubviews:@[ self.textLabel, self.button ]];
    verticalStackView.alignment = UIStackViewAlignmentCenter;
    verticalStackView.axis = UILayoutConstraintAxisVertical;
    verticalStackView.spacing = stackViewSubViewSpacing;
    verticalStackView.translatesAutoresizingMaskIntoConstraints = NO;

    [self.contentView addSubview:verticalStackView];

    // Add constraints for stackView
    [NSLayoutConstraint activateConstraints:@[
      [verticalStackView.leadingAnchor
          constraintEqualToAnchor:self.contentView.leadingAnchor
                         constant:stackViewHorizontalSpacing],
      [verticalStackView.trailingAnchor
          constraintEqualToAnchor:self.contentView.trailingAnchor
                         constant:-stackViewHorizontalSpacing],
      [verticalStackView.topAnchor
          constraintEqualToAnchor:self.contentView.topAnchor
                         constant:stackViewVerticalSpacing],
      [verticalStackView.bottomAnchor
          constraintEqualToAnchor:self.contentView.bottomAnchor
                         constant:-stackViewVerticalSpacing]
    ]];
  }
  return self;
}

#pragma mark - TextButtonItemDelegate

- (void)performButtonAction {
  [self.delegate performButtonAction];
}

@end
