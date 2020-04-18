// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/table_view/cells/table_view_accessory_item.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_cells_constants.h"
#import "ios/chrome/browser/ui/table_view/chrome_table_view_styler.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// The width and height of the ImageView.
const float kImageWidth = 28.0f;
}

@implementation TableViewAccessoryItem

@synthesize image = _image;
@synthesize title = _title;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [TableViewAccessoryCell class];
  }
  return self;
}

- (void)configureCell:(UITableViewCell*)tableCell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:tableCell withStyler:styler];

  TableViewAccessoryCell* cell =
      base::mac::ObjCCastStrict<TableViewAccessoryCell>(tableCell);
  if (self.image) {
    cell.imageView.hidden = NO;
    cell.imageView.image = self.image;
  } else {
    // No image. Hide imageView.
    cell.imageView.hidden = YES;
  }

  cell.titleLabel.text = self.title;

  cell.imageView.backgroundColor = styler.tableViewBackgroundColor;
  cell.titleLabel.backgroundColor = styler.tableViewBackgroundColor;
}

@end

@implementation TableViewAccessoryCell
@synthesize imageView = _imageView;
@synthesize titleLabel = _titleLabel;

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self) {
    _imageView = [[UIImageView alloc] init];
    _titleLabel = [[UILabel alloc] init];

    // The favicon image is smaller than its UIImageView's bounds, so center
    // it.
    _imageView.contentMode = UIViewContentModeCenter;

    // Set font size using dynamic type.
    _titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    _titleLabel.adjustsFontForContentSizeCategory = YES;

    // Disclosure ImageView.
    UIImageView* disclosureImageView = [[UIImageView alloc]
        initWithImage:[UIImage imageNamed:@"table_view_cell_chevron"]];
    [disclosureImageView
        setContentHuggingPriority:UILayoutPriorityDefaultHigh
                          forAxis:UILayoutConstraintAxisHorizontal];

    // Horizontal stack view holds favicon, title, and disclosureView.
    UIStackView* horizontalStack =
        [[UIStackView alloc] initWithArrangedSubviews:@[
          _imageView, _titleLabel, disclosureImageView
        ]];
    horizontalStack.axis = UILayoutConstraintAxisHorizontal;
    horizontalStack.spacing = kTableViewSubViewHorizontalSpacing;
    horizontalStack.distribution = UIStackViewDistributionFill;
    horizontalStack.alignment = UIStackViewAlignmentCenter;
    horizontalStack.translatesAutoresizingMaskIntoConstraints = NO;

    [self.contentView addSubview:horizontalStack];

    [NSLayoutConstraint activateConstraints:@[
      // The favicon view is a fixed size.
      [_imageView.heightAnchor constraintEqualToConstant:kImageWidth],
      [_imageView.widthAnchor constraintEqualToConstant:kImageWidth],
      // Horizontal Stack constraints.
      [horizontalStack.leadingAnchor
          constraintEqualToAnchor:self.contentView.leadingAnchor
                         constant:kTableViewHorizontalSpacing],
      [horizontalStack.trailingAnchor
          constraintEqualToAnchor:self.contentView.trailingAnchor
                         constant:-kTableViewHorizontalSpacing],
      [horizontalStack.topAnchor
          constraintEqualToAnchor:self.contentView.topAnchor
                         constant:kTableViewVerticalSpacing],
      [horizontalStack.bottomAnchor
          constraintEqualToAnchor:self.contentView.bottomAnchor
                         constant:-kTableViewVerticalSpacing],
    ]];
  }
  return self;
}

@end
