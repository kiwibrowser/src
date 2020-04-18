// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/autofill/form_suggestion_label.h"

#import <QuartzCore/QuartzCore.h>
#include <stddef.h>

#include <cmath>

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/browser/autofill_data_util.h"
#include "components/autofill/core/browser/credit_card.h"
#import "components/autofill/ios/browser/form_suggestion.h"
#import "ios/chrome/browser/autofill/form_suggestion_view_client.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// The button corner radius.
const CGFloat kCornerRadius = 2.0f;

// Font size of button titles.
const CGFloat kIpadFontSize = 15.0f;
const CGFloat kIphoneFontSize = 14.0f;

// The alpha values of the suggestion's main and description labels.
const CGFloat kMainLabelAlpha = 0.87f;
const CGFloat kDescriptionLabelAlpha = 0.55f;

// The horizontal space between the edge of the background and the text.
const CGFloat kBorderWidth = 8.0f;
// The space between items in the label.
const CGFloat kSpacing = 4.0f;

// RGB button color when the button is not pressed.
const int kBackgroundNormalColor = 0xeceff1;
// RGB button color when the button is pressed.
const int kBackgroundPressedColor = 0xc4cbcf;

// Structure that record the image for each icon.
struct IconImageMap {
  const char* const icon_name;
  NSString* image_name;
};

// Creates a label with the given |text| and |alpha| suitable for use in a
// suggestion button in the keyboard accessory view.
UILabel* TextLabel(NSString* text, CGFloat alpha, BOOL bold) {
  UILabel* label = [[UILabel alloc] init];
  [label setText:text];
  CGFloat fontSize = IsIPadIdiom() ? kIpadFontSize : kIphoneFontSize;
  UIFont* font = bold ? [UIFont boldSystemFontOfSize:fontSize]
                      : [UIFont systemFontOfSize:fontSize];
  [label setFont:font];
  [label setTextColor:[UIColor colorWithWhite:0.0f alpha:alpha]];
  [label setBackgroundColor:[UIColor clearColor]];
  [label sizeToFit];
  return label;
}

}  // namespace

@implementation FormSuggestionLabel {
  // Client of this view.
  __weak id<FormSuggestionViewClient> client_;
  FormSuggestion* suggestion_;
}

- (id)initWithSuggestion:(FormSuggestion*)suggestion
           proposedFrame:(CGRect)proposedFrame
                   index:(NSUInteger)index
          numSuggestions:(NSUInteger)numSuggestions
                  client:(id<FormSuggestionViewClient>)client {
  // TODO(jimblackler): implement sizeThatFits: and layoutSubviews, and perform
  // layout in those methods instead of in the designated initializer.
  self = [super initWithFrame:CGRectZero];
  if (self) {
    suggestion_ = suggestion;
    client_ = client;

    const CGFloat frameHeight = CGRectGetHeight(proposedFrame);
    CGFloat currentX = kBorderWidth;

    if (suggestion.icon.length > 0) {
      const int iconImageID = autofill::data_util::GetPaymentRequestData(
                                  base::SysNSStringToUTF8(suggestion.icon))
                                  .icon_resource_id;
      UIImage* iconImage = NativeImage(iconImageID);
      UIImageView* iconView = [[UIImageView alloc] initWithImage:iconImage];
      const CGFloat iconY =
          std::floor((frameHeight - iconImage.size.height) / 2.0f);
      iconView.frame = CGRectMake(currentX, iconY, iconImage.size.width,
                                  iconImage.size.height);
      [self addSubview:iconView];
      currentX += CGRectGetWidth(iconView.frame) + kSpacing;
    }

    UILabel* label = TextLabel(suggestion.value, kMainLabelAlpha, YES);
    const CGFloat labelY =
        std::floor(frameHeight / 2.0f - CGRectGetMidY(label.frame));
    label.frame = CGRectMake(currentX, labelY, CGRectGetWidth(label.frame),
                             CGRectGetHeight(label.frame));
    [self addSubview:label];
    currentX += CGRectGetWidth(label.frame);

    if ([suggestion.displayDescription length] > 0) {
      currentX += kSpacing;
      UILabel* description =
          TextLabel(suggestion.displayDescription, kDescriptionLabelAlpha, NO);
      const CGFloat descriptionY =
          std::floor(frameHeight / 2.0f - CGRectGetMidY(description.frame));
      description.frame =
          CGRectMake(currentX, descriptionY, CGRectGetWidth(description.frame),
                     CGRectGetHeight(description.frame));
      [self addSubview:description];
      currentX += CGRectGetWidth(description.frame);
    }

    currentX += kBorderWidth;

    self.frame = CGRectMake(proposedFrame.origin.x, proposedFrame.origin.y,
                            currentX, proposedFrame.size.height);
    [self setBackgroundColor:UIColorFromRGB(kBackgroundNormalColor)];
    [[self layer] setCornerRadius:kCornerRadius];

    [self setClipsToBounds:YES];
    [self setUserInteractionEnabled:YES];
    [self setIsAccessibilityElement:YES];
    [self setAccessibilityLabel:l10n_util::GetNSStringF(
                                    IDS_IOS_AUTOFILL_ACCNAME_SUGGESTION,
                                    base::SysNSStringToUTF16(suggestion.value),
                                    base::SysNSStringToUTF16(
                                        suggestion.displayDescription),
                                    base::IntToString16(index + 1),
                                    base::IntToString16(numSuggestions))];
  }

  return self;
}

- (id)initWithFrame:(CGRect)frame {
  NOTREACHED();
  return nil;
}

#pragma mark -
#pragma mark UIResponder

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event {
  [self setBackgroundColor:UIColorFromRGB(kBackgroundPressedColor)];
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event {
  [self setBackgroundColor:UIColorFromRGB(kBackgroundNormalColor)];
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event {
  [self setBackgroundColor:UIColorFromRGB(kBackgroundNormalColor)];
  [client_ didSelectSuggestion:suggestion_];
}

@end
