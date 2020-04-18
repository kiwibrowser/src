// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/cells/content_suggestions_cell.h"

#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#import "ios/chrome/browser/ui/favicon/favicon_view.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#import "ios/chrome/browser/ui/util/i18n_string.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kImageSize = 80;
const CGFloat kImageSizeLegacy = 62;
const CGFloat kStandardSpacing = 13;
const CGFloat kStandardSpacingLegacy = 16;
const CGFloat kSmallSpacing = 7;
const CGFloat kSmallSpacingLegacy = 8;

// Size of the favicon view.
const CGFloat kFaviconSize = 16;
// Size of the icon displayed when there is not image but one should be
// displayed.
const CGFloat kIconSize = 24;
// Name of the icon displayed when there is not image but one should be
// displayed.
NSString* const kNoImageIconName = @"content_suggestions_no_image";
// No image icon percentage of white.
const CGFloat kNoImageIconWhite = 0.38;
// No image background percentage of white.
const CGFloat kNoImageBackgroundWhite = 0.95;
// Duration of the animation to display the image.
const CGFloat kAnimationDuration = 0.3;
}

@interface ContentSuggestionsCell ()

@property(nonatomic, strong) UILabel* additionalInformationLabel;
// Contains the no-image icon or the image.
@property(nonatomic, strong) UIView* imageContainer;
// The no-image icon displayed when there is no image.
@property(nonatomic, strong) UIImageView* noImageIcon;
// Displays the image associated with this suggestion. It is added to the
// imageContainer only if there is an image to display, hiding the no-image
// icon.
@property(nonatomic, strong) UIImageView* contentImageView;
// Constraint for the size of the image.
@property(nonatomic, strong) NSLayoutConstraint* imageSizeConstraint;
// Constraint for the distance between the texts and the image.
@property(nonatomic, strong) NSLayoutConstraint* imageTitleSpacing;

// Applies the constraints on the elements. Called in the init.
- (void)applyConstraints;

// Helpers to account for legacy sizes
+ (CGFloat)imageSize;
+ (CGFloat)smallSpacing;

@end

@implementation ContentSuggestionsCell

@synthesize titleLabel = _titleLabel;
@synthesize imageContainer = _imageContainer;
@synthesize noImageIcon = _noImageIcon;
@synthesize additionalInformationLabel = _additionalInformationLabel;
@synthesize contentImageView = _contentImageView;
@synthesize faviconView = _faviconView;
@synthesize imageSizeConstraint = _imageSizeConstraint;
@synthesize imageTitleSpacing = _imageTitleSpacing;
@synthesize displayImage = _displayImage;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    _titleLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _imageContainer = [[UIView alloc] initWithFrame:CGRectZero];
    if (IsUIRefreshPhase1Enabled()) {
      _imageContainer.layer.cornerRadius = 11;
      _imageContainer.layer.masksToBounds = YES;
    }
    _noImageIcon = [[UIImageView alloc] initWithFrame:CGRectZero];
    _additionalInformationLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    _contentImageView = [[UIImageView alloc] initWithFrame:CGRectZero];
    _faviconView = [[FaviconViewNew alloc] init];

    _contentImageView.contentMode = UIViewContentModeScaleAspectFill;
    _contentImageView.clipsToBounds = YES;
    _contentImageView.hidden = YES;

    _imageContainer.translatesAutoresizingMaskIntoConstraints = NO;
    _noImageIcon.translatesAutoresizingMaskIntoConstraints = NO;
    _titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    _additionalInformationLabel.translatesAutoresizingMaskIntoConstraints = NO;
    _contentImageView.translatesAutoresizingMaskIntoConstraints = NO;
    _faviconView.translatesAutoresizingMaskIntoConstraints = NO;

    [self.contentView addSubview:_imageContainer];
    [self.contentView addSubview:_titleLabel];
    [self.contentView addSubview:_additionalInformationLabel];
    [self.contentView addSubview:_faviconView];

    [_imageContainer addSubview:_noImageIcon];
    [_imageContainer addSubview:_contentImageView];

    _imageContainer.backgroundColor =
        [UIColor colorWithWhite:kNoImageBackgroundWhite alpha:1];
    _noImageIcon.image = [[UIImage imageNamed:kNoImageIconName]
        imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    [_noImageIcon
        setTintColor:[UIColor colorWithWhite:kNoImageIconWhite alpha:1]];

    [[self class] configureTitleLabel:_titleLabel];
    _additionalInformationLabel.font = [[self class] additionalInformationFont];
    _faviconView.font = [[MDCTypography fontLoader] mediumFontOfSize:10];
    if (IsUIRefreshPhase1Enabled()) {
      _additionalInformationLabel.textColor =
          [UIColor colorWithWhite:0 alpha:0.4];
    } else {
      _additionalInformationLabel.textColor =
          [[MDCPalette greyPalette] tint700];
    }

    [self applyConstraints];
  }
  return self;
}

- (void)setContentImage:(UIImage*)image animated:(BOOL)animated {
  if (!image) {
    self.contentImageView.hidden = YES;
    return;
  }

  self.contentImageView.image = image;
  self.contentImageView.hidden = NO;

  if (!animated) {
    return;
  }

  self.contentImageView.alpha = 0;

  [UIView animateWithDuration:kAnimationDuration
                   animations:^{
                     self.contentImageView.alpha = 1;
                   }];
}

- (void)setAdditionalInformationWithPublisherName:(NSString*)publisherName
                                             date:(NSString*)date {
  self.additionalInformationLabel.text =
      [[self class] stringForPublisher:publisherName date:date];
}

- (void)setDisplayImage:(BOOL)displayImage {
  if (displayImage) {
    self.imageTitleSpacing.constant = [[self class] standardSpacing];
    self.imageSizeConstraint.constant = [[self class] imageSize];
    self.imageContainer.hidden = NO;
  } else {
    self.imageTitleSpacing.constant = 0;
    self.imageSizeConstraint.constant = 0;
    self.imageContainer.hidden = YES;
  }
  _displayImage = displayImage;
}

+ (CGFloat)heightForWidth:(CGFloat)width
                withImage:(BOOL)hasImage
                    title:(NSString*)title
            publisherName:(NSString*)publisherName
          publicationDate:(NSString*)publicationDate {
    UILabel* titleLabel = [[UILabel alloc] init];
    [self configureTitleLabel:titleLabel];
    titleLabel.text = title;

    UILabel* additionalInfoLabel = [[UILabel alloc] init];
    additionalInfoLabel.font = [self additionalInformationFont];
    additionalInfoLabel.text =
        [self stringForPublisher:publisherName date:publicationDate];

    CGSize sizeForLabels =
        CGSizeMake(width - [self labelMarginWithImage:hasImage], 500);

    if (IsUIRefreshPhase1Enabled()) {
      CGFloat minimalHeight =
          [[self class] imageSize] + [[self class] standardSpacing];

      CGFloat labelHeight = [[self class] standardSpacing];
      labelHeight += [titleLabel sizeThatFits:sizeForLabels].height;
      labelHeight += [[self class] smallSpacing];
      CGFloat additionalInfoHeight =
          [additionalInfoLabel sizeThatFits:sizeForLabels].height;
      labelHeight += MAX(additionalInfoHeight, kFaviconSize);
      return MAX(minimalHeight, labelHeight);
    } else {
      CGFloat labelHeight = 3 * [[self class] standardSpacing];
      labelHeight += [titleLabel sizeThatFits:sizeForLabels].height;
      CGFloat additionalInfoHeight =
          [additionalInfoLabel sizeThatFits:sizeForLabels].height;
      labelHeight += MAX(additionalInfoHeight, kFaviconSize);

      CGFloat minimalHeight = hasImage ? [[self class] imageSize] : 0;
      minimalHeight += 2 * [[self class] standardSpacing];
      return MAX(minimalHeight, labelHeight);
  }
}

#pragma mark - UICollectionViewCell

- (void)prepareForReuse {
  [super prepareForReuse];
  self.titleLabel.text = nil;
  self.displayImage = NO;
}

#pragma mark - UIView

- (void)traitCollectionDidChange:(UITraitCollection*)previousTraitCollection {
  [super traitCollectionDidChange:previousTraitCollection];
  if (IsUIRefreshPhase1Enabled() &&
      self.traitCollection.preferredContentSizeCategory !=
          previousTraitCollection.preferredContentSizeCategory) {
    [[self class] configureTitleLabel:_titleLabel];
    _additionalInformationLabel.font = [[self class] additionalInformationFont];
  }
}

// Implements -layoutSubviews as per instructions in documentation for
// +[MDCCollectionViewCell cr_preferredHeightForWidth:forItem:].
- (void)layoutSubviews {
  [super layoutSubviews];

  // Adjust the text label preferredMaxLayoutWidth when the parent's width
  // changes, for instance on screen rotation.
  CGFloat parentWidth = CGRectGetWidth(self.contentView.bounds);

  self.titleLabel.preferredMaxLayoutWidth =
      parentWidth - [[self class] labelMarginWithImage:self.displayImage];
  self.additionalInformationLabel.preferredMaxLayoutWidth =
      parentWidth - kFaviconSize - [[self class] smallSpacing] -
      2 * [[self class] standardSpacing];

  // Re-layout with the new preferred width to allow the label to adjust its
  // height.
  [super layoutSubviews];
}

#pragma mark - Private

- (void)applyConstraints {
  _imageSizeConstraint = [_imageContainer.heightAnchor
      constraintEqualToConstant:[[self class] imageSize]];
  _imageTitleSpacing = [_titleLabel.leadingAnchor
      constraintEqualToAnchor:_imageContainer.trailingAnchor
                     constant:[[self class] standardSpacing]];

  if (IsUIRefreshPhase1Enabled()) {
    [NSLayoutConstraint activateConstraints:@[
      [_imageContainer.bottomAnchor
          constraintLessThanOrEqualToAnchor:_faviconView.bottomAnchor],
      [_faviconView.topAnchor
          constraintGreaterThanOrEqualToAnchor:self.titleLabel.bottomAnchor
                                      constant:[[self class] smallSpacing]]
    ]];
  } else {
    [NSLayoutConstraint activateConstraints:@[
      [_imageContainer.bottomAnchor
          constraintLessThanOrEqualToAnchor:self.contentView.bottomAnchor
                                   constant:-[[self class] standardSpacing]],
      [_additionalInformationLabel.topAnchor
          constraintGreaterThanOrEqualToAnchor:_titleLabel.bottomAnchor
                                      constant:[[self class] standardSpacing]],
      [_additionalInformationLabel.bottomAnchor
          constraintLessThanOrEqualToAnchor:self.contentView.bottomAnchor
                                   constant:-[[self class] standardSpacing]],
      [_faviconView.topAnchor
          constraintGreaterThanOrEqualToAnchor:_titleLabel.bottomAnchor
                                      constant:[[self class] standardSpacing]],
      [_faviconView.bottomAnchor
          constraintLessThanOrEqualToAnchor:self.contentView.bottomAnchor
                                   constant:-[[self class] standardSpacing]],
    ]];
  }

  [NSLayoutConstraint activateConstraints:@[
    // Image.
    _imageSizeConstraint,
    [_imageContainer.widthAnchor
        constraintEqualToAnchor:_imageContainer.heightAnchor],
    [_imageContainer.topAnchor constraintEqualToAnchor:_titleLabel.topAnchor],

    // Text.
    _imageTitleSpacing,

    // Additional Information.
    [_additionalInformationLabel.trailingAnchor
        constraintEqualToAnchor:_titleLabel.trailingAnchor],

    // Favicon.
    [_faviconView.leadingAnchor
        constraintEqualToAnchor:_titleLabel.leadingAnchor],
    [_faviconView.centerYAnchor
        constraintEqualToAnchor:_additionalInformationLabel.centerYAnchor],
    [_faviconView.heightAnchor constraintEqualToConstant:kFaviconSize],
    [_faviconView.widthAnchor
        constraintEqualToAnchor:_faviconView.heightAnchor],

    // No image icon.
    [_noImageIcon.centerXAnchor
        constraintEqualToAnchor:_imageContainer.centerXAnchor],
    [_noImageIcon.centerYAnchor
        constraintEqualToAnchor:_imageContainer.centerYAnchor],
    [_noImageIcon.widthAnchor constraintEqualToConstant:kIconSize],
    [_noImageIcon.heightAnchor constraintEqualToAnchor:_noImageIcon.widthAnchor]
  ]];

  AddSameConstraints(_contentImageView, _imageContainer);

  ApplyVisualConstraintsWithMetrics(
      @[
        @"H:[title]-(space)-|",
        @"H:|-(space)-[image]",
        @"V:|-(space)-[title]",
        @"H:[favicon]-(small)-[additional]",
      ],
      @{
        @"image" : _imageContainer,
        @"title" : _titleLabel,
        @"additional" : _additionalInformationLabel,
        @"favicon" : _faviconView,
      },
      @{
        @"space" : @([[self class] standardSpacing]),
        @"small" : @([[self class] smallSpacing])
      });
}

+ (CGFloat)imageSize {
  return IsUIRefreshPhase1Enabled() ? kImageSize : kImageSizeLegacy;
}

+ (CGFloat)standardSpacing {
  return IsUIRefreshPhase1Enabled() ? kStandardSpacing : kStandardSpacingLegacy;
}

+ (CGFloat)smallSpacing {
  return IsUIRefreshPhase1Enabled() ? kSmallSpacing : kSmallSpacingLegacy;
}

// Configures the |titleLabel|.
+ (void)configureTitleLabel:(UILabel*)titleLabel {
  if (IsUIRefreshPhase1Enabled()) {
    titleLabel.textColor = [UIColor colorWithWhite:0 alpha:0.8];
    UIFontDescriptor* descriptor = [[UIFontDescriptor
        preferredFontDescriptorWithTextStyle:UIFontTextStyleSubheadline]
        fontDescriptorWithSymbolicTraits:UIFontDescriptorTraitBold];
    titleLabel.font = [UIFont fontWithDescriptor:descriptor size:0];
    titleLabel.numberOfLines = 3;
  } else {
    titleLabel.font = [MDCTypography subheadFont];
    titleLabel.numberOfLines = 2;
  }
}

// Returns the font used to display the additional informations.
+ (UIFont*)additionalInformationFont {
  if (IsUIRefreshPhase1Enabled()) {
    return [UIFont preferredFontForTextStyle:UIFontTextStyleCaption2];
  } else {
    return [MDCTypography captionFont];
  }
}

// Returns the margin for the labels, depending if the cell |hasImage|.
+ (CGFloat)labelMarginWithImage:(BOOL)hasImage {
  CGFloat offset =
      hasImage ? [[self class] imageSize] + [[self class] standardSpacing] : 0;
  return 2 * [[self class] standardSpacing] + offset;
}

// Returns the attributed string to be displayed.
+ (NSString*)stringForPublisher:(NSString*)publisherName date:(NSString*)date {
  NSString* sep = IsUIRefreshPhase1Enabled() ? @"•" : @"-";
  return AdjustStringForLocaleDirection(
      [NSString stringWithFormat:@"%@ %@ %@ ", publisherName, sep, date]);
}

@end
