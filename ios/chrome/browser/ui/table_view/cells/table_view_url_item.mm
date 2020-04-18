// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/table_view/cells/table_view_url_item.h"

#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/ui/table_view/cells/table_view_cells_constants.h"
#import "ios/chrome/browser/ui/table_view/chrome_table_view_styler.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// The width and height of the favicon ImageView.
const CGFloat kFaviconWidth = 16;
// The width and height of the favicon container view.
const CGFloat kFaviconContainerWidth = 28;
}

@implementation TableViewURLItem

@synthesize metadata = _metadata;
@synthesize title = _title;
@synthesize URL = _URL;

- (instancetype)initWithType:(NSInteger)type {
  self = [super initWithType:type];
  if (self) {
    self.cellClass = [TableViewURLCell class];
  }
  return self;
}

- (void)configureCell:(UITableViewCell*)tableCell
           withStyler:(ChromeTableViewStyler*)styler {
  [super configureCell:tableCell withStyler:styler];

  TableViewURLCell* cell =
      base::mac::ObjCCastStrict<TableViewURLCell>(tableCell);
  // Use the page's title for the label, or its URL if title is empty.
  if (self.title) {
    cell.titleLabel.text = self.title;
    cell.URLLabel.text = base::SysUTF8ToNSString(self.URL.host());
  } else {
    cell.titleLabel.text = base::SysUTF8ToNSString(self.URL.host());
  }
  cell.metadataLabel.text = self.metadata;
  cell.metadataLabel.hidden = ([self.metadata length] == 0);

  cell.cellUniqueIdentifier = self.uniqueIdentifier;
  cell.faviconView.backgroundColor = styler.tableViewBackgroundColor;
  cell.faviconContainerView.backgroundColor = styler.tableViewBackgroundColor;
  cell.titleLabel.backgroundColor = styler.tableViewBackgroundColor;
  cell.URLLabel.backgroundColor = styler.tableViewBackgroundColor;
  cell.metadataLabel.backgroundColor = styler.tableViewBackgroundColor;
}

- (NSString*)uniqueIdentifier {
  return base::SysUTF8ToNSString(self.URL.host());
}

@end

@implementation TableViewURLCell
@synthesize faviconView = _faviconView;
@synthesize faviconContainerView = _faviconContainerView;
@synthesize metadataLabel = _metadataLabel;
@synthesize titleLabel = _titleLabel;
@synthesize URLLabel = _URLLabel;
@synthesize cellUniqueIdentifier = _cellUniqueIdentifier;

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
  if (self) {
    _faviconContainerView = [[UIImageView alloc]
        initWithImage:[UIImage
                          imageNamed:@"table_view_cell_favicon_background"]];
    _faviconView = [[UIImageView alloc] init];
    _faviconView.contentMode = UIViewContentModeScaleAspectFit;
    _faviconView.clipsToBounds = YES;
    [_faviconContainerView addSubview:_faviconView];
    _titleLabel = [[UILabel alloc] init];
    _URLLabel = [[UILabel alloc] init];
    _metadataLabel = [[UILabel alloc] init];

    // Set font sizes using dynamic type.
    _titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    _titleLabel.adjustsFontForContentSizeCategory = YES;
    _URLLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleCaption1];
    _URLLabel.adjustsFontForContentSizeCategory = YES;
    _URLLabel.textColor = [UIColor lightGrayColor];
    _metadataLabel.font =
        [UIFont preferredFontForTextStyle:UIFontTextStyleCaption1];
    _metadataLabel.adjustsFontForContentSizeCategory = YES;

    // Use stack views to layout the subviews except for the favicon.
    UIStackView* verticalStack = [[UIStackView alloc]
        initWithArrangedSubviews:@[ _titleLabel, _URLLabel ]];
    verticalStack.axis = UILayoutConstraintAxisVertical;
    [_metadataLabel setContentHuggingPriority:UILayoutPriorityDefaultHigh
                                      forAxis:UILayoutConstraintAxisHorizontal];
    [_metadataLabel
        setContentCompressionResistancePriority:UILayoutPriorityDefaultHigh
                                        forAxis:
                                            UILayoutConstraintAxisHorizontal];

    // Horizontal stack view holds vertical stack view and favicon.
    UIStackView* horizontalStack = [[UIStackView alloc]
        initWithArrangedSubviews:@[ verticalStack, _metadataLabel ]];
    horizontalStack.axis = UILayoutConstraintAxisHorizontal;
    horizontalStack.spacing = kTableViewSubViewHorizontalSpacing;
    horizontalStack.distribution = UIStackViewDistributionFill;
    horizontalStack.alignment = UIStackViewAlignmentFirstBaseline;

    UIView* contentView = self.contentView;
    _faviconView.translatesAutoresizingMaskIntoConstraints = NO;
    _faviconContainerView.translatesAutoresizingMaskIntoConstraints = NO;
    horizontalStack.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_faviconContainerView];
    [contentView addSubview:horizontalStack];

    [NSLayoutConstraint activateConstraints:@[
      // The favicon view is a fixed size, is pinned to the leading edge of the
      // content view, and is centered vertically.
      [_faviconView.heightAnchor constraintEqualToConstant:kFaviconWidth],
      [_faviconView.widthAnchor constraintEqualToConstant:kFaviconWidth],
      [_faviconView.centerYAnchor
          constraintEqualToAnchor:_faviconContainerView.centerYAnchor],
      [_faviconView.centerXAnchor
          constraintEqualToAnchor:_faviconContainerView.centerXAnchor],
      [_faviconContainerView.heightAnchor
          constraintEqualToConstant:kFaviconContainerWidth],
      [_faviconContainerView.widthAnchor
          constraintEqualToConstant:kFaviconContainerWidth],
      [_faviconContainerView.leadingAnchor
          constraintEqualToAnchor:self.contentView.leadingAnchor
                         constant:kTableViewHorizontalSpacing],
      [_faviconContainerView.centerYAnchor
          constraintEqualToAnchor:self.contentView.centerYAnchor],

      // The stack view fills the remaining space, has an intrinsic height, and
      // is centered vertically.
      [horizontalStack.leadingAnchor
          constraintEqualToAnchor:_faviconContainerView.trailingAnchor
                         constant:kTableViewSubViewHorizontalSpacing],
      [horizontalStack.trailingAnchor
          constraintEqualToAnchor:self.contentView.trailingAnchor
                         constant:-kTableViewHorizontalSpacing],
      [horizontalStack.topAnchor
          constraintEqualToAnchor:self.contentView.topAnchor
                         constant:kTableViewVerticalSpacing],
      [horizontalStack.bottomAnchor
          constraintEqualToAnchor:self.contentView.bottomAnchor
                         constant:-kTableViewVerticalSpacing]
    ]];
  }
  return self;
}

- (void)prepareForReuse {
  [super prepareForReuse];
  self.faviconView.image = nil;
}

@end
