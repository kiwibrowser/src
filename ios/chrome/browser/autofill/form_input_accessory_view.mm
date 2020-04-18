// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/autofill/form_input_accessory_view.h"

#import <QuartzCore/QuartzCore.h>

#include "base/i18n/rtl.h"
#include "base/logging.h"
#import "ios/chrome/browser/autofill/form_input_accessory_view_delegate.h"
#import "ios/chrome/browser/ui/image_util/image_util.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// The alpha value of the background color.
const CGFloat kBackgroundColorAlpha = 1.0;

// The width of the separators of the previous and next buttons.
const CGFloat kNavigationButtonSeparatorWidth = 1;

// The width of the shadow part of the navigation area separator.
const CGFloat kNavigationAreaSeparatorShadowWidth = 2;

// The width of the navigation area / custom view separator asset.
const CGFloat kNavigationAreaSeparatorWidth = 1;

}  // namespace

@interface FormInputAccessoryView ()

// Returns a view that shows navigation buttons.
- (UIView*)viewForNavigationButtonsUsingDelegate:
    (id<FormInputAccessoryViewDelegate>)delegate;

// Adds a navigation button for Autofill in |view| that has |normalImage| for
// state UIControlStateNormal, a |pressedImage| for states
// UIControlStateSelected and UIControlStateHighlighted, and an optional
// |disabledImage| for UIControlStateDisabled.
- (UIButton*)addKeyboardNavButtonWithNormalImage:(UIImage*)normalImage
                                    pressedImage:(UIImage*)pressedImage
                                   disabledImage:(UIImage*)disabledImage
                                          target:(id)target
                                          action:(SEL)action
                                         enabled:(BOOL)enabled
                                          inView:(UIView*)view;

// Adds a background image to |view|. The supplied image is stretched to fit the
// space by stretching the content its horizontal and vertical centers.
+ (void)addBackgroundImageInView:(UIView*)view
                   withImageName:(NSString*)imageName;

// Adds an image view in |view| with an image named |imageName|.
+ (UIView*)createImageViewWithImageName:(NSString*)imageName
                                 inView:(UIView*)view;

@end

@implementation FormInputAccessoryView

- (void)setUpWithCustomView:(UIView*)customView {
  [self addSubview:customView];
  customView.translatesAutoresizingMaskIntoConstraints = NO;
  AddSameConstraints(self, customView);

  [[self class] addBackgroundImageInView:self
                           withImageName:@"autofill_keyboard_background"];
}

- (void)setUpWithNavigationDelegate:(id<FormInputAccessoryViewDelegate>)delegate
                         customView:(UIView*)customView {
  self.translatesAutoresizingMaskIntoConstraints = NO;
  UIView* customViewContainer = [[UIView alloc] init];
  customViewContainer.translatesAutoresizingMaskIntoConstraints = NO;
  [self addSubview:customViewContainer];
  UIView* navView = [[UIView alloc] init];
  navView.translatesAutoresizingMaskIntoConstraints = NO;
  [self addSubview:navView];

  [customViewContainer addSubview:customView];
  customView.translatesAutoresizingMaskIntoConstraints = NO;
  AddSameConstraints(customViewContainer, customView);

  UIView* navViewContent =
      [self viewForNavigationButtonsUsingDelegate:delegate];
  [navView addSubview:navViewContent];

    AddSameConstraints(navView, navViewContent);

    [[self class] addBackgroundImageInView:self
                             withImageName:@"autofill_keyboard_background"];

    id<LayoutGuideProvider> layoutGuide = SafeAreaLayoutGuideForView(self);
    [NSLayoutConstraint activateConstraints:@[
      [customViewContainer.topAnchor
          constraintEqualToAnchor:layoutGuide.topAnchor],
      [customViewContainer.bottomAnchor
          constraintEqualToAnchor:layoutGuide.bottomAnchor],
      [customViewContainer.leadingAnchor
          constraintEqualToAnchor:layoutGuide.leadingAnchor],
      [customViewContainer.trailingAnchor
          constraintEqualToAnchor:navView.leadingAnchor
                         constant:kNavigationAreaSeparatorShadowWidth],
      [navView.trailingAnchor
          constraintEqualToAnchor:layoutGuide.trailingAnchor],
      [navView.topAnchor constraintEqualToAnchor:layoutGuide.topAnchor],
      [navView.bottomAnchor constraintEqualToAnchor:layoutGuide.bottomAnchor],
    ]];
}

#pragma mark -
#pragma mark UIInputViewAudioFeedback

- (BOOL)enableInputClicksWhenVisible {
  return YES;
}

#pragma mark -
#pragma mark Private Methods

UIImage* ButtonImage(NSString* name) {
  UIImage* rawImage = [UIImage imageNamed:name];
  return StretchableImageFromUIImage(rawImage, 1, 0);
}

- (UIView*)viewForNavigationButtonsUsingDelegate:
    (id<FormInputAccessoryViewDelegate>)delegate {
  UIView* navView = [[UIView alloc] init];
  navView.translatesAutoresizingMaskIntoConstraints = NO;

  UIView* separator =
      [[self class] createImageViewWithImageName:@"autofill_left_sep"
                                          inView:navView];

  UIButton* previousButton = [self
      addKeyboardNavButtonWithNormalImage:ButtonImage(@"autofill_prev")
                             pressedImage:ButtonImage(@"autofill_prev_pressed")
                            disabledImage:ButtonImage(@"autofill_prev_inactive")
                                   target:delegate
                                   action:@selector
                                   (selectPreviousElementWithButtonPress)
                                  enabled:NO
                                   inView:navView];
  [previousButton
      setAccessibilityLabel:l10n_util::GetNSString(
                                IDS_IOS_AUTOFILL_ACCNAME_PREVIOUS_FIELD)];

  // Add internal separator.
  UIView* internalSeparator =
      [[self class] createImageViewWithImageName:@"autofill_middle_sep"
                                          inView:navView];

  UIButton* nextButton = [self
      addKeyboardNavButtonWithNormalImage:ButtonImage(@"autofill_next")
                             pressedImage:ButtonImage(@"autofill_next_pressed")
                            disabledImage:ButtonImage(@"autofill_next_inactive")
                                   target:delegate
                                   action:@selector
                                   (selectNextElementWithButtonPress)
                                  enabled:NO
                                   inView:navView];
  [nextButton setAccessibilityLabel:l10n_util::GetNSString(
                                        IDS_IOS_AUTOFILL_ACCNAME_NEXT_FIELD)];

  [delegate fetchPreviousAndNextElementsPresenceWithCompletionHandler:^(
                BOOL hasPreviousElement, BOOL hasNextElement) {
    previousButton.enabled = hasPreviousElement;
    nextButton.enabled = hasNextElement;
  }];

  // Add internal separator.
  UIView* internalSeparator2 =
      [[self class] createImageViewWithImageName:@"autofill_middle_sep"
                                          inView:navView];

  UIButton* closeButton = [self
      addKeyboardNavButtonWithNormalImage:ButtonImage(@"autofill_close")
                             pressedImage:ButtonImage(@"autofill_close_pressed")
                            disabledImage:nil
                                   target:delegate
                                   action:@selector
                                   (closeKeyboardWithButtonPress)
                                  enabled:YES
                                   inView:navView];
  [closeButton
      setAccessibilityLabel:l10n_util::GetNSString(
                                IDS_IOS_AUTOFILL_ACCNAME_HIDE_KEYBOARD)];

  ApplyVisualConstraintsWithMetrics(
      @[
        (@"H:|[separator1(==areaSeparatorWidth)][previousButton][separator2(=="
         @"buttonSeparatorWidth)][nextButton][internalSeparator2("
         @"buttonSeparatorWidth)][closeButton]|"),
        @"V:|-(topPadding)-[separator1]|",
        @"V:|-(topPadding)-[previousButton]|",
        @"V:|-(topPadding)-[previousButton]|",
        @"V:|-(topPadding)-[separator2]|", @"V:|-(topPadding)-[nextButton]|",
        @"V:|-(topPadding)-[internalSeparator2]|",
        @"V:|-(topPadding)-[closeButton]|"
      ],
      @{
        @"separator1" : separator,
        @"previousButton" : previousButton,
        @"separator2" : internalSeparator,
        @"nextButton" : nextButton,
        @"internalSeparator2" : internalSeparator2,
        @"closeButton" : closeButton
      },
      @{

        @"areaSeparatorWidth" : @(kNavigationAreaSeparatorWidth),
        @"buttonSeparatorWidth" : @(kNavigationButtonSeparatorWidth),
        @"topPadding" : @(1)
      });

  return navView;
}

- (UIButton*)addKeyboardNavButtonWithNormalImage:(UIImage*)normalImage
                                    pressedImage:(UIImage*)pressedImage
                                   disabledImage:(UIImage*)disabledImage
                                          target:(id)target
                                          action:(SEL)action
                                         enabled:(BOOL)enabled
                                          inView:(UIView*)view {
  UIButton* button = [UIButton buttonWithType:UIButtonTypeCustom];
  button.translatesAutoresizingMaskIntoConstraints = NO;

  [button setBackgroundImage:normalImage forState:UIControlStateNormal];
  [button setBackgroundImage:pressedImage forState:UIControlStateSelected];
  [button setBackgroundImage:pressedImage forState:UIControlStateHighlighted];
  if (disabledImage)
    [button setBackgroundImage:disabledImage forState:UIControlStateDisabled];

  CALayer* layer = [button layer];
  layer.borderWidth = 0;
  layer.borderColor = [[UIColor blackColor] CGColor];
  button.enabled = enabled;
  [button addTarget:target
                action:action
      forControlEvents:UIControlEventTouchUpInside];
  [button.heightAnchor constraintEqualToAnchor:button.widthAnchor].active = YES;
  [view addSubview:button];
  return button;
}

+ (void)addBackgroundImageInView:(UIView*)view
                   withImageName:(NSString*)imageName {
  UIImage* backgroundImage = StretchableImageNamed(imageName);

  UIImageView* backgroundImageView = [[UIImageView alloc] init];
  backgroundImageView.translatesAutoresizingMaskIntoConstraints = NO;
  [backgroundImageView setImage:backgroundImage];
  [backgroundImageView setAlpha:kBackgroundColorAlpha];
  [view addSubview:backgroundImageView];
  [view sendSubviewToBack:backgroundImageView];
  AddSameConstraints(view, backgroundImageView);
}

+ (UIView*)createImageViewWithImageName:(NSString*)imageName
                                 inView:(UIView*)view {
  UIImage* image =
      StretchableImageFromUIImage([UIImage imageNamed:imageName], 0, 0);
  UIImageView* imageView = [[UIImageView alloc] initWithImage:image];
  imageView.translatesAutoresizingMaskIntoConstraints = NO;
  [view addSubview:imageView];
  return imageView;
}

@end
