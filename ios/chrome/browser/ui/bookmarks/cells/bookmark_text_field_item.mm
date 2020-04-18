// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/cells/bookmark_text_field_item.h"

#include "base/logging.h"
#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/experimental_flags.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_ui_constants.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_utils_ios.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/public/provider/chrome/browser/chrome_browser_provider.h"
#import "ios/public/provider/chrome/browser/ui/text_field_styling.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - BookmarkTextFieldItem

@implementation BookmarkTextFieldItem

@synthesize text = _text;
@synthesize placeholder = _placeholder;
@synthesize delegate = _delegate;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (experimental_flags::IsBookmarksUIRebootEnabled()) {
    self.cellClass = [BookmarkTextFieldCell class];
  } else {
    self.cellClass = [LegacyBookmarkTextFieldCell class];
  }
  return self;
}

#pragma mark TableViewItem

- (void)configureCell:(UITableViewCell*)tableCell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:tableCell withStyler:styler];

  if (experimental_flags::IsBookmarksUIRebootEnabled()) {
    BookmarkTextFieldCell* cell =
        base::mac::ObjCCastStrict<BookmarkTextFieldCell>(tableCell);
    cell.textField.text = self.text;
    cell.titleLabel.text = self.placeholder;
    cell.textField.placeholder = self.placeholder;
    cell.textField.tag = self.type;
    [cell.textField addTarget:self
                       action:@selector(textFieldDidChange:)
             forControlEvents:UIControlEventEditingChanged];
    cell.textField.delegate = self.delegate;
    cell.textField.accessibilityLabel = self.text;
    cell.textField.accessibilityIdentifier = [NSString
        stringWithFormat:@"%@_textField", self.accessibilityIdentifier];
    cell.validState = YES;
    cell.selectionStyle = UITableViewCellSelectionStyleNone;
  } else {
    LegacyBookmarkTextFieldCell* cell =
        base::mac::ObjCCastStrict<LegacyBookmarkTextFieldCell>(tableCell);
    cell.textField.text = self.text;
    cell.textField.placeholder = self.placeholder;
    cell.textField.tag = self.type;
    [cell.textField addTarget:self
                       action:@selector(textFieldDidChange:)
             forControlEvents:UIControlEventEditingChanged];
    cell.textField.delegate = self.delegate;
    cell.textField.accessibilityLabel = self.text;
    cell.textField.accessibilityIdentifier = [NSString
        stringWithFormat:@"%@_textField", self.accessibilityIdentifier];
  }
}

#pragma mark UIControlEventEditingChanged

- (void)textFieldDidChange:(UITextField*)textField {
  DCHECK_EQ(textField.tag, self.type);
  self.text = textField.text;
  [self.delegate textDidChangeForItem:self];
}

@end

#pragma mark - BookmarkTextFieldCell

@interface BookmarkTextFieldCell ()
@property(nonatomic, strong) UILabel* invalidURLLabel;
@end

@implementation BookmarkTextFieldCell
@synthesize textField = _textField;
@synthesize titleLabel = _titleLabel;
@synthesize invalidURLLabel = _invalidURLLabel;
@synthesize validState = _validState;

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (!self)
    return nil;

  // Label.
  self.titleLabel = [[UILabel alloc] init];
  self.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
  self.titleLabel.adjustsFontForContentSizeCategory = YES;
  [self.titleLabel setContentHuggingPriority:UILayoutPriorityRequired
                                     forAxis:UILayoutConstraintAxisHorizontal];
  [self.titleLabel
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisHorizontal];
  [self.titleLabel
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisVertical];

  // Textfield.
  self.textField = [[UITextField alloc] init];
  self.textField.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
  self.textField.adjustsFontForContentSizeCategory = YES;
  self.textField.textColor = [UIColor lightGrayColor];
  self.textField.clearButtonMode = UITextFieldViewModeWhileEditing;
  self.textField.textAlignment = NSTextAlignmentRight;
  [self.textField setContentHuggingPriority:UILayoutPriorityDefaultLow
                                    forAxis:UILayoutConstraintAxisHorizontal];
  [self.textField
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisVertical];

  // Horizontal StackView.
  UIStackView* horizontalStack = [[UIStackView alloc]
      initWithArrangedSubviews:@[ self.titleLabel, self.textField ]];
  horizontalStack.axis = UILayoutConstraintAxisHorizontal;
  horizontalStack.spacing = kBookmarkCellViewSpacing;
  horizontalStack.distribution = UIStackViewDistributionFill;
  horizontalStack.alignment = UIStackViewAlignmentCenter;
  [horizontalStack
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisVertical];

  // Invalid URL label
  self.invalidURLLabel = [[UILabel alloc] init];
  self.invalidURLLabel.text =
      l10n_util::GetNSString(IDS_IOS_BOOKMARK_URL_FIELD_VALIDATION_FAILED);
  self.invalidURLLabel.textColor = [UIColor redColor];
  self.invalidURLLabel.font =
      [UIFont preferredFontForTextStyle:UIFontTextStyleCaption1];
  self.invalidURLLabel.adjustsFontForContentSizeCategory = YES;
  [self.invalidURLLabel
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisVertical];
  [self.invalidURLLabel
      setContentHuggingPriority:UILayoutPriorityDefaultLow
                        forAxis:UILayoutConstraintAxisHorizontal];

  // Vertical StackView.
  UIStackView* verticalStack = [[UIStackView alloc]
      initWithArrangedSubviews:@[ horizontalStack, self.invalidURLLabel ]];
  verticalStack.axis = UILayoutConstraintAxisVertical;
  verticalStack.translatesAutoresizingMaskIntoConstraints = NO;
  [verticalStack
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisVertical];
  [self.contentView addSubview:verticalStack];

  // Set up constraints.
  [NSLayoutConstraint activateConstraints:@[
    [verticalStack.topAnchor
        constraintEqualToAnchor:self.contentView.topAnchor
                       constant:kBookmarkCellVerticalInset],
    [verticalStack.bottomAnchor
        constraintEqualToAnchor:self.contentView.bottomAnchor
                       constant:-kBookmarkCellVerticalInset],
    [verticalStack.leadingAnchor
        constraintEqualToAnchor:self.contentView.leadingAnchor
                       constant:kBookmarkCellHorizontalLeadingInset],
    [verticalStack.trailingAnchor
        constraintEqualToAnchor:self.contentView.trailingAnchor
                       constant:-kBookmarkCellHorizontalTrailingInset],
  ]];

  return self;
}

- (void)setValidState:(BOOL)validState {
  _validState = validState;
  if (validState) {
    self.invalidURLLabel.hidden = YES;
    self.textField.textColor = [UIColor lightGrayColor];
  } else {
    self.invalidURLLabel.hidden = NO;
    self.textField.textColor = [UIColor redColor];
  }
}

- (void)prepareForReuse {
  [super prepareForReuse];
  [self.textField resignFirstResponder];
  [self.textField removeTarget:nil
                        action:NULL
              forControlEvents:UIControlEventAllEvents];
  self.validState = YES;
  self.textField.delegate = nil;
  self.textField.text = nil;
}

@end

#pragma mark - LegacyBookmarkTextFieldCell

@interface LegacyBookmarkTextFieldCell ()
@property(nonatomic, readwrite, strong)
    UITextField<TextFieldStyling>* textField;
@end

@implementation LegacyBookmarkTextFieldCell

@synthesize textField = _textField;

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self) {
    _textField =
        ios::GetChromeBrowserProvider()->CreateStyledTextField(CGRectZero);
    _textField.translatesAutoresizingMaskIntoConstraints = NO;
    _textField.textColor = bookmark_utils_ios::darkTextColor();
    _textField.clearButtonMode = UITextFieldViewModeWhileEditing;
    _textField.placeholderStyle =
        TextFieldStylingPlaceholderFloatingPlaceholder;
    [self.contentView addSubview:_textField];
    const CGFloat kHorizontalPadding = 15;
    const CGFloat kTopPadding = 8;
    [NSLayoutConstraint activateConstraints:@[
      [_textField.leadingAnchor
          constraintEqualToAnchor:self.contentView.leadingAnchor
                         constant:kHorizontalPadding],
      [_textField.topAnchor constraintEqualToAnchor:self.contentView.topAnchor
                                           constant:kTopPadding],
      [_textField.trailingAnchor
          constraintEqualToAnchor:self.contentView.trailingAnchor
                         constant:-kHorizontalPadding],
      [_textField.bottomAnchor
          constraintEqualToAnchor:self.contentView.bottomAnchor
                         constant:-kTopPadding],
    ]];
  }
  return self;
}

- (void)prepareForReuse {
  [super prepareForReuse];
  [self.textField resignFirstResponder];
  [self.textField removeTarget:nil
                        action:NULL
              forControlEvents:UIControlEventAllEvents];
  self.textField.delegate = nil;
  self.textField.text = nil;
  self.textField.textValidator = nil;
}

@end
