// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/ntp/recent_tabs/views/panel_bar_view.h"

#import "ios/chrome/browser/ui/ntp/recent_tabs/views/views_utils.h"
#include "ios/chrome/browser/ui/rtl_geometry.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/chrome/browser/ui/util/constraints_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const int kBackgroundColor = 0xf2f2f2;
const CGFloat kFontSize = 20;
const CGFloat kSpacing = 16;

}  // namespace

@interface PanelBarView () {
  UIButton* _closeButton;
  NSLayoutConstraint* _statusBarSpacerConstraint;
}
// Whether the panel view extends throughout the whole screen. For example,
// when presented fullscreen, the panel bar extends to the borders of the app
// and the function returns YES.
// When presented modally as on iPad and iPhone 6 Plus landscape, it returns NO.
- (BOOL)coversFullAppWidth;
@end

@implementation PanelBarView

- (instancetype)init {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    [self setBackgroundColor:UIColorFromRGB(kBackgroundColor)];
    // Create and add the bar's title.
    UILabel* title = [[UILabel alloc] initWithFrame:CGRectZero];
    [title setTranslatesAutoresizingMaskIntoConstraints:NO];
    [title setFont:[[MDCTypography fontLoader] mediumFontOfSize:kFontSize]];
    [title setTextColor:recent_tabs::GetTextColorGray()];
    [title setTextAlignment:NSTextAlignmentNatural];
    [title setText:l10n_util::GetNSString(IDS_IOS_NEW_TAB_RECENT_TABS)];
    [title setBackgroundColor:UIColorFromRGB(kBackgroundColor)];
    [self addSubview:title];

    // Create and add the bar's close button.
    _closeButton = [[UIButton alloc] initWithFrame:CGRectZero];
    [_closeButton setTranslatesAutoresizingMaskIntoConstraints:NO];
    [_closeButton
        setTitle:l10n_util::GetNSString(IDS_IOS_NAVIGATION_BAR_DONE_BUTTON)
                     .uppercaseString
        forState:UIControlStateNormal];
    [_closeButton setTitleColor:recent_tabs::GetTextColorGray()
                       forState:UIControlStateNormal];
    [[_closeButton titleLabel] setFont:[MDCTypography buttonFont]];
    [_closeButton setAccessibilityIdentifier:@"Exit"];
    [self addSubview:_closeButton];

    // Create and add the view that adds vertical padding that matches the
    // status bar's height.
    UIView* statusBarSpacer = [[UIView alloc] initWithFrame:CGRectZero];
    [statusBarSpacer setTranslatesAutoresizingMaskIntoConstraints:NO];
    [self addSubview:statusBarSpacer];

    // Add the constraints on all the subviews.
    NSDictionary* viewsDictionary = @{
      @"title" : title,
      @"closeButton" : _closeButton,
      @"statusBar" : statusBarSpacer,
    };
    NSArray* constraints = @[
      @"V:|-0-[statusBar]-14-[closeButton]-13-|",
      @"H:[title]-(>=0)-[closeButton]",
    ];
    ApplyVisualConstraints(constraints, viewsDictionary);
    id<LayoutGuideProvider> safeAreaLayoutGuide =
        SafeAreaLayoutGuideForView(self);
    [NSLayoutConstraint activateConstraints:@[
      [title.leadingAnchor
          constraintEqualToAnchor:safeAreaLayoutGuide.leadingAnchor
                         constant:kSpacing],
      [_closeButton.trailingAnchor
          constraintEqualToAnchor:safeAreaLayoutGuide.trailingAnchor
                         constant:-kSpacing],
    ]];
    AddSameCenterYConstraint(self, title, _closeButton);
    _statusBarSpacerConstraint =
        [NSLayoutConstraint constraintWithItem:statusBarSpacer
                                     attribute:NSLayoutAttributeHeight
                                     relatedBy:NSLayoutRelationEqual
                                        toItem:nil
                                     attribute:NSLayoutAttributeNotAnAttribute
                                    multiplier:1
                                      constant:0];
    [self addConstraint:_statusBarSpacerConstraint];
  }
  return self;
}

- (void)updateConstraints {
  UIInterfaceOrientation orientation =
      [[UIApplication sharedApplication] statusBarOrientation];
  // On Plus phones in landscape, the modal is not fullscreen. The panel bar
  // doesn't need to take the status bar into account.
  BOOL takeStatusBarIntoAccount = [self coversFullAppWidth] ||
                                  UIInterfaceOrientationIsPortrait(orientation);
  if (takeStatusBarIntoAccount) {
    CGFloat statusBarHeight = StatusBarHeight();
    [_statusBarSpacerConstraint setConstant:statusBarHeight];
  } else {
    [_statusBarSpacerConstraint setConstant:0];
  }
  [super updateConstraints];
}

- (void)setCloseTarget:(id)target action:(SEL)action {
  [_closeButton addTarget:target
                   action:action
         forControlEvents:UIControlEventTouchUpInside];
}

- (void)layoutSubviews {
  [self setNeedsUpdateConstraints];
  [super layoutSubviews];
}

- (BOOL)coversFullAppWidth {
  return self.traitCollection.horizontalSizeClass ==
         self.window.traitCollection.horizontalSizeClass;
}

@end
