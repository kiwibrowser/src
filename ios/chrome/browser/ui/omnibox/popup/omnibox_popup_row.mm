// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/omnibox/popup/omnibox_popup_row.h"

#include "base/logging.h"

#import "ios/chrome/browser/ui/omnibox/truncating_attributed_label.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_theme_resources.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kImageDimensionLength = 19.0;
const CGFloat kLeadingPaddingIpad = 164;
const CGFloat kLeadingPaddingIpadCompact = 71;
const CGFloat kAppendButtonTrailingMargin = 4;
const CGFloat kAppendButtonSize = 48.0;
}

@interface OmniboxPopupRow () {
  BOOL _incognito;
}

// Set the append button normal and highlighted images.
- (void)updateAppendButtonImages;

@end

@implementation OmniboxPopupRow

@synthesize textTruncatingLabel = _textTruncatingLabel;
@synthesize detailTruncatingLabel = _detailTruncatingLabel;
@synthesize detailAnswerLabel = _detailAnswerLabel;
@synthesize appendButton = _appendButton;
@synthesize answerImageView = _answerImageView;
@synthesize imageView = _imageView;
@synthesize rowHeight = _rowHeight;

- (instancetype)initWithStyle:(UITableViewCellStyle)style
              reuseIdentifier:(NSString*)reuseIdentifier {
  return [self initWithIncognito:NO];
}

- (instancetype)initWithIncognito:(BOOL)incognito {
  self = [super initWithStyle:UITableViewCellStyleDefault
              reuseIdentifier:@"OmniboxPopupRow"];
  if (self) {
    self.isAccessibilityElement = YES;
    self.backgroundColor = [UIColor clearColor];
    _incognito = incognito;

    _textTruncatingLabel =
        [[OmniboxPopupTruncatingLabel alloc] initWithFrame:CGRectZero];
    _textTruncatingLabel.userInteractionEnabled = NO;
    [self.contentView addSubview:_textTruncatingLabel];

    _detailTruncatingLabel =
        [[OmniboxPopupTruncatingLabel alloc] initWithFrame:CGRectZero];
    _detailTruncatingLabel.userInteractionEnabled = NO;
    [self.contentView addSubview:_detailTruncatingLabel];

    // Answers use a UILabel with NSLineBreakByTruncatingTail to produce a
    // truncation with an ellipse instead of fading on multi-line text.
    _detailAnswerLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _detailAnswerLabel.userInteractionEnabled = NO;
    _detailAnswerLabel.lineBreakMode = NSLineBreakByTruncatingTail;
    [self.contentView addSubview:_detailAnswerLabel];

    _appendButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [_appendButton setContentMode:UIViewContentModeRight];
    [self updateAppendButtonImages];
    // TODO(justincohen): Consider using the UITableViewCell's accessory view.
    // The current implementation is from before using a UITableViewCell.
    [self.contentView addSubview:_appendButton];

    // Leading icon is only displayed on iPad.
    if (IsIPadIdiom()) {
      _imageView = [[UIImageView alloc] initWithFrame:CGRectZero];
      _imageView.userInteractionEnabled = NO;
      _imageView.contentMode = UIViewContentModeCenter;

      // TODO(justincohen): Consider using the UITableViewCell's image view.
      // The current implementation is from before using a UITableViewCell.
      [self.contentView addSubview:_imageView];
    }

    _answerImageView = [[UIImageView alloc] initWithFrame:CGRectZero];
    _answerImageView.userInteractionEnabled = NO;
    _answerImageView.contentMode = UIViewContentModeScaleAspectFit;
    [self.contentView addSubview:_answerImageView];
  }
  return self;
}

- (void)layoutSubviews {
  [super layoutSubviews];
  [self layoutAccessoryViews];
}

- (void)layoutAccessoryViews {
  LayoutRect imageViewLayout = LayoutRectMake(
      IsCompactTablet() ? kLeadingPaddingIpadCompact : kLeadingPaddingIpad,
      CGRectGetWidth(self.contentView.bounds),
      floor((_rowHeight - kImageDimensionLength) / 2), kImageDimensionLength,
      kImageDimensionLength);
  _imageView.frame = LayoutRectGetRect(imageViewLayout);

  LayoutRect trailingAccessoryLayout =
      LayoutRectMake(CGRectGetWidth(self.contentView.bounds) -
                         kAppendButtonSize - kAppendButtonTrailingMargin,
                     CGRectGetWidth(self.contentView.bounds),
                     floor((_rowHeight - kAppendButtonSize) / 2),
                     kAppendButtonSize, kAppendButtonSize);
  _appendButton.frame = LayoutRectGetRect(trailingAccessoryLayout);
}

- (void)updateLeadingImage:(int)imageID {
  _imageView.image = NativeImage(imageID);

  _imageView.accessibilityIdentifier =
      [NSString stringWithFormat:@"leading image id %d", imageID];

  // Adjust the vertical position based on the current size of the row.
  CGRect frame = _imageView.frame;
  frame.origin.y = floor((_rowHeight - kImageDimensionLength) / 2);
  _imageView.frame = frame;
}

- (void)updateHighlightBackground:(BOOL)highlighted {
  // Set the background color to match the color of selected table view cells
  // when their selection style is UITableViewCellSelectionStyleGray.
  if (highlighted) {
    self.backgroundColor = _incognito ? [UIColor colorWithWhite:1 alpha:0.1]
                                      : [UIColor colorWithWhite:0 alpha:0.05];
  } else {
    self.backgroundColor = [UIColor clearColor];
  }
}

- (void)setHighlighted:(BOOL)highlighted animated:(BOOL)animated {
  [super setHighlighted:highlighted animated:animated];
  [self updateHighlightBackground:highlighted];
}

- (void)setHighlighted:(BOOL)highlighted {
  [super setHighlighted:highlighted];
  [self updateHighlightBackground:highlighted];
}

- (void)updateAppendButtonImages {
  int appendResourceID = _incognito
                             ? IDR_IOS_OMNIBOX_KEYBOARD_VIEW_APPEND_INCOGNITO
                             : IDR_IOS_OMNIBOX_KEYBOARD_VIEW_APPEND;
  UIImage* appendImage = NativeReversableImage(appendResourceID, YES);

  [_appendButton setImage:appendImage forState:UIControlStateNormal];
  int appendSelectedResourceID =
      _incognito ? IDR_IOS_OMNIBOX_KEYBOARD_VIEW_APPEND_INCOGNITO_HIGHLIGHTED
                 : IDR_IOS_OMNIBOX_KEYBOARD_VIEW_APPEND_HIGHLIGHTED;
  UIImage* appendImageSelected =
      NativeReversableImage(appendSelectedResourceID, YES);
  [_appendButton setImage:appendImageSelected
                 forState:UIControlStateHighlighted];
}

- (NSString*)accessibilityLabel {
  return _textTruncatingLabel.attributedText.string;
}

- (NSString*)accessibilityValue {
  return _detailTruncatingLabel.hidden
             ? _detailAnswerLabel.attributedText.string
             : _detailTruncatingLabel.attributedText.string;
}

@end
