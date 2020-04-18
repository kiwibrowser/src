// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/reading_list_empty_collection_background.h"

#include "base/logging.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Palettes/src/MaterialPalettes.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Images name.
NSString* const kEmptyReadingListBackgroundIcon = @"reading_list_empty_state";
NSString* const kToolbarMenuIcon = @"reading_list_toolbar_icon";
NSString* const kShareMenuIcon = @"reading_list_share_icon";

// Tag in string.
NSString* const kOpenShareMarker = @"SHARE_OPENING_ICON";

// Background view constants.
const CGFloat kTextImageSpacing = 10;
const CGFloat kTextHorizontalMinimumMargin = 32;
const CGFloat kTextMaximalWidth = 255;
const CGFloat kImageWidth = 60;
const CGFloat kImageHeight = 44;
const CGFloat kFontSize = 16;
const CGFloat kIconHeight = 24;
const CGFloat kToolbarMenuWidth = 18;
const CGFloat kShareMenuWidth = 24;
const CGFloat kLineHeight = 24;
const CGFloat kPercentageFromTopForPosition = 0.4;

}  // namespace

@interface ReadingListEmptyCollectionBackground ()

// Attaches the icon named |iconName| to |instructionString| and a |caret|. The
// icon is positionned using the |iconOffset| and with the |attributes| (mainly
// the color). |spaceBeforeCaret| controls whether a space should be added
// between the image and the caret.
- (void)attachIconNamed:(NSString*)iconName
               toString:(NSMutableAttributedString*)instructionString
              withCaret:(NSMutableAttributedString*)caret
                 offset:(CGFloat)iconOffset
                  width:(CGFloat)icondWidth
        imageAttributes:(NSDictionary*)attributes;
// Sets the constraints for this view, positionning the |imageView| in the X
// center and at 40% of the top. The |label| is positionned below.
- (void)setConstraintsToImageView:(UIImageView*)imageView
                         andLabel:(UILabel*)label;
// Returns the caret string to be displayed.
- (NSString*)caretString;

@end

@implementation ReadingListEmptyCollectionBackground

#pragma mark - Public

- (instancetype)init {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    NSString* rawText =
        l10n_util::GetNSString(IDS_IOS_READING_LIST_EMPTY_MESSAGE);
    // Add two spaces between Read Later and the preceding caret.
    NSString* readLater = [@"  "
        stringByAppendingString:l10n_util::GetNSString(
                                    IDS_IOS_SHARE_MENU_READING_LIST_ACTION)];

    id<MDCTypographyFontLoading> fontLoader = [MDCTypography fontLoader];

    UIColor* textColor = [[MDCPalette greyPalette] tint700];
    UIFont* textFont = [fontLoader regularFontOfSize:kFontSize];
    NSMutableParagraphStyle* paragraphStyle =
        [[NSMutableParagraphStyle alloc] init];
    paragraphStyle.lineSpacing = kLineHeight - textFont.lineHeight;

    NSDictionary* textAttributes = @{
      NSFontAttributeName : textFont,
      NSForegroundColorAttributeName : textColor,
      NSParagraphStyleAttributeName : paragraphStyle
    };

    // Offset to vertically center the icons.
    CGFloat iconOffset = (textFont.xHeight - kIconHeight) / 2.0;

    UIFont* instructionFont = [fontLoader boldFontOfSize:kFontSize];
    NSDictionary* instructionAttributes = @{
      NSFontAttributeName : instructionFont,
      NSForegroundColorAttributeName : textColor,
    };

    NSMutableAttributedString* baseAttributedString =
        [[NSMutableAttributedString alloc] initWithString:rawText
                                               attributes:textAttributes];

    NSMutableAttributedString* caret = [[NSMutableAttributedString alloc]
        initWithString:[self caretString]
            attributes:instructionAttributes];

    NSMutableAttributedString* instructionString =
        [[NSMutableAttributedString alloc] init];
    NSString* accessibilityInstructionString = @":";

    // Add the images inside the string.
    if (IsCompactWidth() || !IsIPadIdiom()) {
      // TODO(crbug.com/698726): When the share icon is displayed in the toolbar
      // for landscape iPhone 6+, remove !IsIPadIdiom().
      // If the device has a compact display the share menu is accessed from the
      // toolbar menu. If it is expanded, the share menu is directly accessible.
      [self attachIconNamed:kToolbarMenuIcon
                   toString:instructionString
                  withCaret:caret
                     offset:iconOffset
                      width:kToolbarMenuWidth
            imageAttributes:textAttributes];

      accessibilityInstructionString = [[accessibilityInstructionString
          stringByAppendingString:l10n_util::GetNSString(
                                      IDS_IOS_TOOLBAR_SETTINGS)]
          stringByAppendingString:@", "];

      // Add a space before the share icon.
      [instructionString
          appendAttributedString:[[NSAttributedString alloc]
                                     initWithString:@" "
                                         attributes:instructionAttributes]];
    }

    [self attachIconNamed:kShareMenuIcon
                 toString:instructionString
                withCaret:caret
                   offset:iconOffset
                    width:kShareMenuWidth
          imageAttributes:textAttributes];

    accessibilityInstructionString = [[accessibilityInstructionString
        stringByAppendingString:l10n_util::GetNSString(
                                    IDS_IOS_TOOLS_MENU_SHARE)]
        stringByAppendingString:@", "];

    // Add the "Read Later" string.
    NSAttributedString* shareMenuAction =
        [[NSAttributedString alloc] initWithString:readLater
                                        attributes:instructionAttributes];
    [instructionString appendAttributedString:shareMenuAction];
    accessibilityInstructionString =
        [accessibilityInstructionString stringByAppendingString:readLater];

    // Marker
    NSRange iconRange =
        [[baseAttributedString string] rangeOfString:kOpenShareMarker];
    DCHECK(iconRange.location != NSNotFound);
    [baseAttributedString replaceCharactersInRange:iconRange
                              withAttributedString:instructionString];
    NSString* accessibilityLabel =
        l10n_util::GetNSString(IDS_IOS_READING_LIST_EMPTY_MESSAGE);
    accessibilityLabel = [accessibilityLabel
        stringByReplacingOccurrencesOfString:kOpenShareMarker
                                  withString:accessibilityInstructionString];

    // Attach to the label.
    UILabel* label = [[UILabel alloc] initWithFrame:CGRectZero];
    label.attributedText = baseAttributedString;
    label.numberOfLines = 0;
    label.textAlignment = NSTextAlignmentCenter;
    label.accessibilityLabel = accessibilityLabel;
    label.accessibilityIdentifier =
        [ReadingListEmptyCollectionBackground accessibilityIdentifier];
    [label setTranslatesAutoresizingMaskIntoConstraints:NO];
    [self addSubview:label];

    UIImageView* imageView = [[UIImageView alloc] init];
    imageView.image = [UIImage imageNamed:kEmptyReadingListBackgroundIcon];
    [imageView setTranslatesAutoresizingMaskIntoConstraints:NO];
    [self addSubview:imageView];

    [self setConstraintsToImageView:imageView andLabel:label];
  }
  return self;
}

+ (NSString*)accessibilityIdentifier {
  return @"ReadingListBackgroundViewIdentifier";
}

#pragma mark - Private

- (void)attachIconNamed:(NSString*)iconName
               toString:(NSMutableAttributedString*)instructionString
              withCaret:(NSMutableAttributedString*)caret
                 offset:(CGFloat)iconOffset
                  width:(CGFloat)iconWidth
        imageAttributes:(NSDictionary*)attributes {
  // Add a zero width space to set the attributes for the image.
  [instructionString appendAttributedString:[[NSAttributedString alloc]
                                                initWithString:@"\u200B"
                                                    attributes:attributes]];

  NSTextAttachment* toolbarIcon = [[NSTextAttachment alloc] init];
  toolbarIcon.image = [[UIImage imageNamed:iconName]
      imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
  toolbarIcon.bounds = CGRectMake(0, iconOffset, iconWidth, kIconHeight);
  [instructionString
      appendAttributedString:[NSAttributedString
                                 attributedStringWithAttachment:toolbarIcon]];

    [instructionString appendAttributedString:[[NSAttributedString alloc]
                                                  initWithString:@" "
                                                      attributes:attributes]];

  [instructionString appendAttributedString:caret];
}

- (void)setConstraintsToImageView:(UIImageView*)imageView
                         andLabel:(UILabel*)label {
  [NSLayoutConstraint activateConstraints:@[
    [[imageView heightAnchor] constraintEqualToConstant:kImageHeight],
    [[imageView widthAnchor] constraintEqualToConstant:kImageWidth],
    [[self centerXAnchor] constraintEqualToAnchor:label.centerXAnchor],
    [[self centerXAnchor] constraintEqualToAnchor:imageView.centerXAnchor],
    [label.topAnchor constraintEqualToAnchor:imageView.bottomAnchor
                                    constant:kTextImageSpacing],
    [label.trailingAnchor
        constraintLessThanOrEqualToAnchor:self.trailingAnchor
                                 constant:-kTextHorizontalMinimumMargin],
    [label.leadingAnchor
        constraintGreaterThanOrEqualToAnchor:self.leadingAnchor
                                    constant:kTextHorizontalMinimumMargin]
  ]];

  NSLayoutConstraint* widthConstraint =
      [label.widthAnchor constraintEqualToConstant:kTextMaximalWidth];
  widthConstraint.priority = UILayoutPriorityDefaultHigh;
  widthConstraint.active = YES;

  // Position the top of the image at 40% from the top.
  NSLayoutConstraint* verticalAlignment =
      [NSLayoutConstraint constraintWithItem:imageView
                                   attribute:NSLayoutAttributeTop
                                   relatedBy:NSLayoutRelationEqual
                                      toItem:self
                                   attribute:NSLayoutAttributeBottom
                                  multiplier:kPercentageFromTopForPosition
                                    constant:0];
  [self addConstraints:@[ verticalAlignment ]];
}

- (NSString*)caretString {
  if (UseRTLLayout())
    return @"\u25C2";
  return @"\u25B8";
}

@end
