// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/cells/bookmark_table_cell.h"

#include "components/bookmarks/browser/bookmark_model.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_utils_ios.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Image size, in points.
const CGFloat kBookmarkTableCellImageSize = 16.0;

// Padding in table cell.
const CGFloat kBookmarkTableCellImagePadding = 16.0;
}  // namespace

@interface BookmarkTableCell ()<UITextFieldDelegate>

// Icon view.
@property(nonatomic, weak) UIImageView* iconView;

// The label, that displays placeholder text when favicon is missing.
@property(nonatomic, strong) UILabel* placeholderLabel;

// The title text.
@property(nonatomic, strong) UITextField* titleText;

// Separator view. Displayed at 1 pixel height at the bottom.
@property(nonatomic, strong) UIView* separatorView;

// Lists the accessibility elements that are to be seen by UIAccessibility.
@property(nonatomic, readonly) NSMutableArray* accessibilityElements;

// True when title text has ended editing and committed.
@property(nonatomic, assign) BOOL isTextCommitted;

@end

@implementation BookmarkTableCell
@synthesize iconView = _iconView;
@synthesize placeholderLabel = _placeholderLabel;
@synthesize titleText = _titleText;
@synthesize textDelegate = _textDelegate;
@synthesize separatorView = _separatorView;
@synthesize accessibilityElements = _accessibilityElements;
@synthesize isTextCommitted = _isTextCommitted;

#pragma mark - Initializer

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)bookmarkCellIdentifier {
  self = [super initWithStyle:style reuseIdentifier:bookmarkCellIdentifier];
  if (self) {
    _titleText = [[UITextField alloc] initWithFrame:CGRectZero];
    _titleText.textColor = [[MDCPalette greyPalette] tint900];
    _titleText.font = [MDCTypography subheadFont];
    _titleText.userInteractionEnabled = NO;

    // Create icon view.
    UIImageView* iconView = [[UIImageView alloc] init];
    _iconView = iconView;
    [_iconView setHidden:NO];
    [_iconView.widthAnchor
        constraintEqualToConstant:kBookmarkTableCellImageSize]
        .active = YES;
    [_iconView.heightAnchor
        constraintEqualToConstant:kBookmarkTableCellImageSize]
        .active = YES;

    // Create placeholder label.
    _placeholderLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _placeholderLabel.textAlignment = NSTextAlignmentCenter;
    _placeholderLabel.font = [MDCTypography captionFont];
    [_placeholderLabel setHidden:YES];
    [_placeholderLabel.widthAnchor
        constraintEqualToConstant:kBookmarkTableCellImageSize]
        .active = YES;
    [_placeholderLabel.heightAnchor
        constraintEqualToConstant:kBookmarkTableCellImageSize]
        .active = YES;

    // Create and configure StackView.
    UIStackView* contentStack = [[UIStackView alloc]
        initWithArrangedSubviews:@[ _iconView, _placeholderLabel, _titleText ]];
    [self.contentView addSubview:contentStack];
    contentStack.spacing = kBookmarkTableCellImagePadding;
    contentStack.alignment = UIStackViewAlignmentCenter;
    contentStack.translatesAutoresizingMaskIntoConstraints = NO;
    [NSLayoutConstraint activateConstraints:@[
      [contentStack.topAnchor
          constraintEqualToAnchor:self.contentView.topAnchor],
      [contentStack.bottomAnchor
          constraintEqualToAnchor:self.contentView.bottomAnchor],
      [contentStack.leadingAnchor
          constraintEqualToAnchor:self.contentView.leadingAnchor
                         constant:kBookmarkTableCellImagePadding],
      [contentStack.trailingAnchor
          constraintEqualToAnchor:self.contentView.trailingAnchor
                         constant:-kBookmarkTableCellImagePadding]
    ]];

    // Add separator view.
    _separatorView = [[UIView alloc] initWithFrame:CGRectZero];
    [self.contentView addSubview:_separatorView];

    CGFloat pixelSize = 1 / [[UIScreen mainScreen] scale];
    [NSLayoutConstraint activateConstraints:@[
      [_separatorView.leadingAnchor
          constraintEqualToAnchor:_titleText.leadingAnchor],
      [_separatorView.trailingAnchor
          constraintEqualToAnchor:self.trailingAnchor],
      [_separatorView.heightAnchor constraintEqualToConstant:pixelSize],
      [_separatorView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor],
    ]];
    _separatorView.translatesAutoresizingMaskIntoConstraints = NO;
    _separatorView.backgroundColor = [UIColor colorWithWhite:0.0 alpha:.12];

    // Setup accessibility elements.
    _accessibilityElements = [[NSMutableArray alloc] init];
    self.contentView.isAccessibilityElement = YES;
    self.contentView.accessibilityTraits |= UIAccessibilityTraitButton;
    [_accessibilityElements addObject:self.contentView];
  }
  return self;
}

#pragma mark - Public

- (void)setNode:(const bookmarks::BookmarkNode*)node {
  self.titleText.text = bookmark_utils_ios::TitleForBookmarkNode(node);
  [self updateAccessibilityValues];

  if (node->is_folder()) {
    self.iconView.image = [UIImage imageNamed:@"bookmark_gray_folder_new"];
    [self setAccessoryType:UITableViewCellAccessoryDisclosureIndicator];
  } else {
    [self setAccessoryType:UITableViewCellAccessoryNone];
  }
}

- (void)startEdit {
  self.isTextCommitted = NO;
  self.titleText.userInteractionEnabled = YES;
  self.titleText.enablesReturnKeyAutomatically = YES;
  self.titleText.keyboardType = UIKeyboardTypeDefault;
  self.titleText.returnKeyType = UIReturnKeyDone;
  self.titleText.accessibilityIdentifier = @"bookmark_editing_text";
  self.accessoryType = UITableViewCellAccessoryNone;
  [self.titleText becomeFirstResponder];
  // selectAll doesn't work immediately after calling becomeFirstResponder.
  // Do selectAll on the next run loop.
  dispatch_async(dispatch_get_main_queue(), ^{
    if ([self.titleText isFirstResponder]) {
      [self.titleText selectAll:nil];
    }
  });
  if (![self.accessibilityElements containsObject:self.titleText]) {
    [self.accessibilityElements addObject:self.titleText];
  }
  self.titleText.delegate = self;
}

- (void)stopEdit {
  if (self.isTextCommitted) {
    return;
  }
  self.isTextCommitted = YES;
  [self.textDelegate textDidChangeTo:self.titleText.text];
  self.titleText.userInteractionEnabled = NO;
  [self.titleText endEditing:YES];
  [self.accessibilityElements removeObject:self.titleText];
}

+ (NSString*)reuseIdentifier {
  return @"BookmarkTableCellIdentifier";
}

- (void)setImage:(UIImage*)image {
  [self.iconView setHidden:NO];
  [self.placeholderLabel setHidden:YES];

  [self.iconView setImage:image];
}

- (void)setPlaceholderText:(NSString*)text
                 textColor:(UIColor*)textColor
           backgroundColor:(UIColor*)backgroundColor {
  [self.iconView setHidden:YES];
  [self.placeholderLabel setHidden:NO];

  self.placeholderLabel.backgroundColor = backgroundColor;
  self.placeholderLabel.textColor = textColor;
  self.placeholderLabel.text = text;
}

#pragma mark - Layout

- (void)prepareForReuse {
  self.iconView.image = nil;
  self.placeholderLabel.hidden = YES;
  self.iconView.hidden = NO;
  self.titleText.text = nil;
  self.titleText.accessibilityIdentifier = nil;
  self.titleText.userInteractionEnabled = NO;
  [self.accessibilityElements removeObject:self.titleText];
  self.textDelegate = nil;
  [super prepareForReuse];
}

#pragma mark - Persist placeholder background color

- (void)setHighlighted:(BOOL)highlighted animated:(BOOL)animated {
  // Prevent placeholderLabel's background color from being cleared.
  UIColor* backgroundColor = self.placeholderLabel.backgroundColor;
  [super setHighlighted:highlighted animated:animated];
  self.placeholderLabel.backgroundColor = backgroundColor;
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated {
  // Prevent placeholderLabel's background color from being cleared.
  UIColor* backgroundColor = self.placeholderLabel.backgroundColor;
  [super setSelected:selected animated:animated];
  self.placeholderLabel.backgroundColor = backgroundColor;
  if (selected) {
    self.contentView.accessibilityTraits |= UIAccessibilityTraitSelected;
  } else {
    self.contentView.accessibilityTraits &= ~UIAccessibilityTraitSelected;
  }
}

#pragma mark - Accessibility

- (void)updateAccessibilityValues {
  self.contentView.accessibilityLabel = self.titleText.text;
  self.contentView.accessibilityIdentifier = self.titleText.text;
}

- (NSInteger)accessibilityElementCount {
  return [self.accessibilityElements count];
}

- (id)accessibilityElementAtIndex:(NSInteger)index {
  return [self.accessibilityElements objectAtIndex:index];
}

- (NSInteger)indexOfAccessibilityElement:(id)element {
  return [self.accessibilityElements indexOfObject:element];
}

#pragma mark - UITextFieldDelegate

// This method hides the keyboard when the return key is pressed.
- (BOOL)textFieldShouldReturn:(UITextField*)textField {
  [self stopEdit];
  return YES;
}

// This method is called when titleText resigns its first responder status.
// (when return/dimiss key is pressed, or when navigating away.)
- (void)textFieldDidEndEditing:(UITextField*)textField
                        reason:(UITextFieldDidEndEditingReason)reason {
  [self stopEdit];
}

@end
