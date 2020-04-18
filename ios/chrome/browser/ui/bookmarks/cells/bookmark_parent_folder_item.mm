// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bookmarks/cells/bookmark_parent_folder_item.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/experimental_flags.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_ui_constants.h"
#import "ios/chrome/browser/ui/bookmarks/bookmark_utils_ios.h"
#import "ios/chrome/browser/ui/icons/chrome_icon.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - BookmarkParentFolderItem

@implementation BookmarkParentFolderItem

@synthesize title = _title;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.accessibilityIdentifier = @"Change Folder";
    if (experimental_flags::IsBookmarksUIRebootEnabled()) {
      self.cellClass = [BookmarkParentFolderCell class];
    } else {
      self.cellClass = [LegacyBookmarkParentFolderCell class];
    }
  }
  return self;
}

#pragma mark TableViewItem

- (void)configureCell:(UITableViewCell*)tableCell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:tableCell withStyler:styler];
  if (experimental_flags::IsBookmarksUIRebootEnabled()) {
    BookmarkParentFolderCell* cell =
        base::mac::ObjCCastStrict<BookmarkParentFolderCell>(tableCell);
    cell.parentFolderNameLabel.text = self.title;
  } else {
    LegacyBookmarkParentFolderCell* cell =
        base::mac::ObjCCastStrict<LegacyBookmarkParentFolderCell>(tableCell);
    cell.parentFolderNameLabel.text = self.title;
  }
}

@end

#pragma mark - BookmarkParentFolderCell

@interface BookmarkParentFolderCell ()
@property(nonatomic, readwrite, strong) UILabel* parentFolderNameLabel;
@end

@implementation BookmarkParentFolderCell
@synthesize parentFolderNameLabel = _parentFolderNameLabel;

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (!self)
    return nil;

  self.isAccessibilityElement = YES;
  self.accessibilityTraits |= UIAccessibilityTraitButton;

  // "Folder" decoration label.
  UILabel* titleLabel = [[UILabel alloc] init];
  titleLabel.text = l10n_util::GetNSString(IDS_IOS_BOOKMARK_GROUP_BUTTON);
  titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
  titleLabel.adjustsFontForContentSizeCategory = YES;
  [titleLabel setContentHuggingPriority:UILayoutPriorityDefaultHigh
                                forAxis:UILayoutConstraintAxisHorizontal];
  [titleLabel
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisHorizontal];

  // Parent Folder name label.
  self.parentFolderNameLabel = [[UILabel alloc] init];
  self.parentFolderNameLabel.font =
      [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
  self.parentFolderNameLabel.adjustsFontForContentSizeCategory = YES;
  self.parentFolderNameLabel.textColor = [UIColor lightGrayColor];
  self.parentFolderNameLabel.textAlignment = NSTextAlignmentRight;
  [self.parentFolderNameLabel
      setContentHuggingPriority:UILayoutPriorityDefaultLow
                        forAxis:UILayoutConstraintAxisHorizontal];

  // Container StackView.
  UIStackView* horizontalStack = [[UIStackView alloc]
      initWithArrangedSubviews:@[ titleLabel, self.parentFolderNameLabel ]];
  horizontalStack.axis = UILayoutConstraintAxisHorizontal;
  horizontalStack.spacing = kBookmarkCellViewSpacing;
  horizontalStack.distribution = UIStackViewDistributionFill;
  horizontalStack.alignment = UIStackViewAlignmentCenter;
  horizontalStack.translatesAutoresizingMaskIntoConstraints = NO;
  [self.contentView addSubview:horizontalStack];

  // Set up constraints.
  [NSLayoutConstraint activateConstraints:@[
    [horizontalStack.topAnchor
        constraintEqualToAnchor:self.contentView.topAnchor
                       constant:kBookmarkCellVerticalInset],
    [horizontalStack.bottomAnchor
        constraintEqualToAnchor:self.contentView.bottomAnchor
                       constant:-kBookmarkCellVerticalInset],
    [horizontalStack.leadingAnchor
        constraintEqualToAnchor:self.contentView.leadingAnchor
                       constant:kBookmarkCellHorizontalLeadingInset],
    [horizontalStack.trailingAnchor
        constraintEqualToAnchor:self.contentView.trailingAnchor
                       constant:-kBookmarkCellHorizontalAccessoryViewSpacing],
  ]];

  // Chevron accessory view.
  UIImageView* navigationChevronImage = [[UIImageView alloc]
      initWithImage:[UIImage imageNamed:@"table_view_cell_chevron"]];
  self.accessoryView = navigationChevronImage;

  return self;
}

- (void)prepareForReuse {
  [super prepareForReuse];
  self.parentFolderNameLabel.text = nil;
}

- (NSString*)accessibilityLabel {
  return self.parentFolderNameLabel.text;
}

- (NSString*)accessibilityHint {
  return l10n_util::GetNSString(
      IDS_IOS_BOOKMARK_EDIT_PARENT_FOLDER_BUTTON_HINT);
}

@end

#pragma mark - LegacyBookmarkParentFolderCell

@interface LegacyBookmarkParentFolderCell ()
@property(nonatomic, readwrite, strong) UILabel* parentFolderNameLabel;
@property(nonatomic, strong) UILabel* decorationLabel;
@end

@implementation LegacyBookmarkParentFolderCell

@synthesize parentFolderNameLabel = _parentFolderNameLabel;
@synthesize decorationLabel = _decorationLabel;

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (!self)
    return nil;

  self.isAccessibilityElement = YES;
  self.accessibilityTraits |= UIAccessibilityTraitButton;

  const CGFloat kHorizontalPadding = 15;
  const CGFloat kVerticalPadding = 8;
  const CGFloat kParentFolderLabelTopPadding = 7;

  UIView* containerView = [[UIView alloc] initWithFrame:CGRectZero];
  containerView.translatesAutoresizingMaskIntoConstraints = NO;

  _decorationLabel = [[UILabel alloc] init];
  _decorationLabel.translatesAutoresizingMaskIntoConstraints = NO;
  _decorationLabel.text = l10n_util::GetNSString(IDS_IOS_BOOKMARK_GROUP_BUTTON);
  _decorationLabel.font = [[MDCTypography fontLoader] regularFontOfSize:12];
  _decorationLabel.textColor = bookmark_utils_ios::lightTextColor();
  [containerView addSubview:_decorationLabel];

  _parentFolderNameLabel = [[UILabel alloc] init];
  _parentFolderNameLabel.translatesAutoresizingMaskIntoConstraints = NO;
  _parentFolderNameLabel.font =
      [[MDCTypography fontLoader] regularFontOfSize:16];
  _parentFolderNameLabel.textColor =
      [UIColor colorWithWhite:33.0 / 255.0 alpha:1.0];
  _parentFolderNameLabel.textAlignment = NSTextAlignmentNatural;
  [containerView addSubview:_parentFolderNameLabel];

  UIImageView* navigationChevronImage = [[UIImageView alloc] init];
  UIImage* image = TintImage([ChromeIcon chevronIcon], [UIColor grayColor]);
  navigationChevronImage.image = image;
  navigationChevronImage.translatesAutoresizingMaskIntoConstraints = NO;
  [containerView addSubview:navigationChevronImage];

  [self.contentView addSubview:containerView];

  // Set up the constraints.
  [NSLayoutConstraint activateConstraints:@[
    [_decorationLabel.topAnchor
        constraintEqualToAnchor:containerView.topAnchor],
    [_decorationLabel.leadingAnchor
        constraintEqualToAnchor:containerView.leadingAnchor],
    [_parentFolderNameLabel.topAnchor
        constraintEqualToAnchor:_decorationLabel.bottomAnchor
                       constant:kParentFolderLabelTopPadding],
    [_parentFolderNameLabel.leadingAnchor
        constraintEqualToAnchor:_decorationLabel.leadingAnchor],
    [_parentFolderNameLabel.bottomAnchor
        constraintEqualToAnchor:containerView.bottomAnchor],
    [navigationChevronImage.centerYAnchor
        constraintEqualToAnchor:_parentFolderNameLabel.centerYAnchor],
    [navigationChevronImage.leadingAnchor
        constraintEqualToAnchor:_parentFolderNameLabel.trailingAnchor],
    [navigationChevronImage.widthAnchor
        constraintEqualToConstant:navigationChevronImage.image.size.width],
    [navigationChevronImage.trailingAnchor
        constraintEqualToAnchor:containerView.trailingAnchor],
    [containerView.leadingAnchor
        constraintEqualToAnchor:self.contentView.leadingAnchor
                       constant:kHorizontalPadding],
    [containerView.trailingAnchor
        constraintEqualToAnchor:self.contentView.trailingAnchor
                       constant:-kHorizontalPadding],
    [containerView.topAnchor constraintEqualToAnchor:self.contentView.topAnchor
                                            constant:kVerticalPadding],
    [containerView.bottomAnchor
        constraintEqualToAnchor:self.contentView.bottomAnchor
                       constant:-kVerticalPadding],
  ]];

  return self;
}

- (void)prepareForReuse {
  [super prepareForReuse];
  self.parentFolderNameLabel.text = nil;
}

- (NSString*)accessibilityLabel {
  return self.parentFolderNameLabel.text;
}

- (NSString*)accessibilityHint {
  return l10n_util::GetNSString(
      IDS_IOS_BOOKMARK_EDIT_PARENT_FOLDER_BUTTON_HINT);
}

@end
