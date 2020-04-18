// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/find_bar/find_bar_view.h"

#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/commands/browser_commands.h"
#import "ios/chrome/browser/ui/find_bar/find_bar_touch_forwarding_view.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"
#import "ui/gfx/ios/NSString+CrStringDrawing.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

NSString* const kFindInPageInputFieldId = @"kFindInPageInputFieldId";
NSString* const kFindInPageNextButtonId = @"kFindInPageNextButtonId";
NSString* const kFindInPagePreviousButtonId = @"kFindInPagePreviousButtonId";
NSString* const kFindInPageCloseButtonId = @"kFindInPageCloseButtonId";

@interface FindBarView ()

// The overlay that shows number of results in format "1 of 13".
@property(nonatomic, weak) UILabel* resultsLabel;
@property(nonatomic, weak) UIView* separator;

// Initializes all subviews.
- (void)setupSubviews;
// Sets up appearance of subviews, such as fonts, background colors.
- (void)configureApperance:(BOOL)isDark;
// Convenience method that returns images for light and dark appearances.
- (UIImage*)imageWithName:(NSString*)imageName isDark:(BOOL)isDark;

@end

@implementation FindBarView
@synthesize inputField = _inputField;
@synthesize resultsLabel = _resultsLabel;
@synthesize previousButton = _previousButton;
@synthesize nextButton = _nextButton;
@synthesize closeButton = _closeButton;
@synthesize separator = _separator;

- (instancetype)initWithDarkAppearance:(BOOL)darkAppearance {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    [self setupSubviews];
    [self configureApperance:darkAppearance];
  }
  return self;
}

#pragma mark - Public methods

- (void)updateResultsLabelWithText:(NSString*)text {
  self.resultsLabel.hidden = (text.length == 0);
  self.resultsLabel.text = text;
}

#pragma mark - Internal

- (void)setupSubviews {
  UIView* safeAreaView = [[UIView alloc] initWithFrame:CGRectZero];
  safeAreaView.translatesAutoresizingMaskIntoConstraints = NO;
  [self addSubview:safeAreaView];

  PinToSafeArea(safeAreaView, self);

  [self setBackgroundColor:[UIColor clearColor]];

  // Input field.
  UITextField* inputFieldScoped =
      [[UITextField alloc] initWithFrame:CGRectZero];
  self.inputField = inputFieldScoped;
  self.inputField.backgroundColor = [UIColor clearColor];
  self.inputField.translatesAutoresizingMaskIntoConstraints = NO;
  self.inputField.placeholder =
      l10n_util::GetNSString(IDS_IOS_PLACEHOLDER_FIND_IN_PAGE);

  // Label containing number of found results.
  UILabel* resultsLabelScoped = [[UILabel alloc] initWithFrame:CGRectZero];
  self.resultsLabel = resultsLabelScoped;
  self.resultsLabel.textColor = [UIColor lightGrayColor];
  self.resultsLabel.font = [UIFont systemFontOfSize:14];
  [self.resultsLabel
      setContentCompressionResistancePriority:UILayoutPriorityRequired
                                      forAxis:UILayoutConstraintAxisHorizontal];
  [self.resultsLabel
      setContentHuggingPriority:UILayoutPriorityRequired
                        forAxis:UILayoutConstraintAxisHorizontal];

  // Stack view that holds |inputField| and |resultsLabel|.
  UIStackView* inputStackView = [[UIStackView alloc]
      initWithArrangedSubviews:@[ inputFieldScoped, resultsLabelScoped ]];
  [inputStackView setLayoutMargins:UIEdgeInsetsMake(0, 12, 0, 12)];
  [inputStackView setLayoutMarginsRelativeArrangement:YES];
  [inputStackView setSpacing:12];
  [inputStackView setTranslatesAutoresizingMaskIntoConstraints:NO];
  [safeAreaView addSubview:inputStackView];

  NSMutableArray* constraints = [[NSMutableArray alloc] init];
  [constraints addObjectsFromArray:@[
    [[inputStackView leadingAnchor]
        constraintEqualToAnchor:safeAreaView.leadingAnchor],
    [[inputStackView topAnchor] constraintEqualToAnchor:safeAreaView.topAnchor],
    [[inputStackView bottomAnchor]
        constraintEqualToAnchor:safeAreaView.bottomAnchor],
  ]];

  // Touch-forwarding view is put on top of |inputStackView| to forward touches
  // to |inputField|.
  // Unlike a gesture recognizer, forwarding all touch events allows for using
  // long press, pinch and other manipulatiosn on the target textfield.
  FindBarTouchForwardingView* forwarder =
      [[FindBarTouchForwardingView alloc] init];
  [forwarder setTargetView:self.inputField];
  [safeAreaView addSubview:forwarder];
  [constraints addObjectsFromArray:@[
    [[forwarder leadingAnchor]
        constraintEqualToAnchor:[inputStackView leadingAnchor]],
    [[forwarder topAnchor] constraintEqualToAnchor:[inputStackView topAnchor]],
    [[forwarder bottomAnchor]
        constraintEqualToAnchor:[inputStackView bottomAnchor]],
    [[forwarder trailingAnchor]
        constraintEqualToAnchor:[inputStackView trailingAnchor]],
  ]];
  [forwarder setTranslatesAutoresizingMaskIntoConstraints:NO];

  // Thin line separator between buttons and input.
  UIView* separatorScoped = [[UIView alloc] initWithFrame:CGRectZero];
  UIView* separator = separatorScoped;
  separator.backgroundColor = [UIColor colorWithWhite:0.83 alpha:1];
  [safeAreaView addSubview:separator];
  [constraints addObjectsFromArray:@[
    [separator.widthAnchor constraintEqualToConstant:1],
    [separator.bottomAnchor constraintEqualToAnchor:safeAreaView.bottomAnchor
                                           constant:-8],
    [separator.topAnchor constraintEqualToAnchor:safeAreaView.topAnchor
                                        constant:8],
    [separator.leadingAnchor
        constraintEqualToAnchor:inputStackView.trailingAnchor],
  ]];
  separator.translatesAutoresizingMaskIntoConstraints = NO;
  self.separator = separator;

  // Previous button with an arrow.
  UIButton* previousButtonScoped = [[UIButton alloc] initWithFrame:CGRectZero];
  self.previousButton = previousButtonScoped;
  [safeAreaView addSubview:self.previousButton];
  [constraints addObjectsFromArray:@[
    [self.previousButton.centerYAnchor
        constraintEqualToAnchor:safeAreaView.centerYAnchor],
    [self.previousButton.widthAnchor constraintEqualToConstant:48],
    [self.previousButton.heightAnchor constraintEqualToConstant:56],
    [self.previousButton.leadingAnchor
        constraintEqualToAnchor:separator.trailingAnchor],
  ]];
  self.previousButton.isAccessibilityElement = YES;
  self.previousButton.accessibilityTraits = UIAccessibilityTraitButton;
  self.previousButton.translatesAutoresizingMaskIntoConstraints = NO;

  // Next button with an arrow.
  UIButton* nextButtonScoped = [[UIButton alloc] initWithFrame:CGRectZero];
  self.nextButton = nextButtonScoped;
  [safeAreaView addSubview:self.nextButton];
  [constraints addObjectsFromArray:@[
    [self.nextButton.centerYAnchor
        constraintEqualToAnchor:safeAreaView.centerYAnchor],
    [self.nextButton.widthAnchor constraintEqualToConstant:48],
    [self.nextButton.heightAnchor constraintEqualToConstant:56],
    [self.nextButton.leadingAnchor
        constraintEqualToAnchor:self.previousButton.trailingAnchor],
  ]];
  self.nextButton.translatesAutoresizingMaskIntoConstraints = NO;

  // Close button with a cross.
  UIButton* closeButtonScoped = [[UIButton alloc] initWithFrame:CGRectZero];
  self.closeButton = closeButtonScoped;
  [safeAreaView addSubview:self.closeButton];
  [constraints addObjectsFromArray:@[
    [self.closeButton.centerYAnchor
        constraintEqualToAnchor:safeAreaView.centerYAnchor],
    [self.closeButton.trailingAnchor
        constraintEqualToAnchor:safeAreaView.trailingAnchor
                       constant:-4],
    [self.closeButton.widthAnchor constraintEqualToConstant:48],
    [self.closeButton.heightAnchor constraintEqualToConstant:56],
    [self.closeButton.leadingAnchor
        constraintEqualToAnchor:self.nextButton.trailingAnchor],
  ]];
  self.closeButton.translatesAutoresizingMaskIntoConstraints = NO;

  // A11y labels.
  SetA11yLabelAndUiAutomationName(self.closeButton,
                                  IDS_FIND_IN_PAGE_CLOSE_TOOLTIP,
                                  kFindInPageCloseButtonId);
  SetA11yLabelAndUiAutomationName(self.previousButton,
                                  IDS_FIND_IN_PAGE_PREVIOUS_TOOLTIP,
                                  kFindInPagePreviousButtonId);
  SetA11yLabelAndUiAutomationName(
      self.nextButton, IDS_FIND_IN_PAGE_NEXT_TOOLTIP, kFindInPageNextButtonId);
  self.inputField.accessibilityIdentifier = kFindInPageInputFieldId;

  // Configure fonts.
  [self.inputField setFont:[MDCTypography body1Font]];
  [self.resultsLabel setFont:[MDCTypography body1Font]];

  [NSLayoutConstraint activateConstraints:constraints];
}

- (void)configureApperance:(BOOL)isDark {
  [self.closeButton setImage:[self imageWithName:@"find_close" isDark:isDark]
                    forState:UIControlStateNormal];
  [self.closeButton
      setImage:[self imageWithName:@"find_close_pressed" isDark:isDark]
      forState:UIControlStateHighlighted];

  [self.previousButton setImage:[self imageWithName:@"find_prev" isDark:isDark]
                       forState:UIControlStateNormal];
  [self.previousButton
      setImage:[self imageWithName:@"find_prev_pressed" isDark:isDark]
      forState:UIControlStateHighlighted];
  [self.previousButton
      setImage:[self imageWithName:@"find_prev_disabled" isDark:isDark]
      forState:UIControlStateDisabled];

  [self.nextButton setImage:[self imageWithName:@"find_next" isDark:isDark]
                   forState:UIControlStateNormal];
  [self.nextButton
      setImage:[self imageWithName:@"find_next_pressed" isDark:isDark]
      forState:UIControlStateHighlighted];
  [self.nextButton
      setImage:[self imageWithName:@"find_next_disabled" isDark:isDark]
      forState:UIControlStateDisabled];

  if (!isDark) {
    return;
  }

  // Setup dark appearance.
  [self.inputField setTextColor:[UIColor whiteColor]];
  NSString* placeholder = [self.inputField placeholder];
  UIColor* inputTextColor = [UIColor colorWithWhite:1 alpha:0.7];
  NSDictionary* attributes = @{NSForegroundColorAttributeName : inputTextColor};
  [self.inputField setAttributedPlaceholder:[[NSAttributedString alloc]
                                                initWithString:placeholder
                                                    attributes:attributes]];
  UIColor* resultTextColor = [UIColor colorWithWhite:1 alpha:0.3];
  [self.resultsLabel setTextColor:resultTextColor];
  UIColor* separatorColor = [UIColor colorWithWhite:0 alpha:0.1];
  [self.separator setBackgroundColor:separatorColor];
}

- (UIImage*)imageWithName:(NSString*)imageName isDark:(BOOL)isDark {
  NSString* name =
      isDark ? [imageName stringByAppendingString:@"_incognito"] : imageName;
  return [UIImage imageNamed:name];
}

@end
