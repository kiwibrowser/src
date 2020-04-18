// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_BUTTONS_TOOLBAR_CONSTANTS_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_BUTTONS_TOOLBAR_CONSTANTS_H_

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#include "ios/chrome/browser/ui/rtl_geometry.h"

// All kxxxColor constants are RGB values stored in a Hex integer. These will be
// converted into UIColors using the UIColorFromRGB() function, from
// uikit_ui_util.h

// Toolbar styling.
extern const CGFloat kToolbarBackgroundColor;
extern const CGFloat kIncognitoToolbarBackgroundColor;
// The brightness of the toolbar's background color (visible on NTPs when the
// background view is hidden).
extern const CGFloat kNTPBackgroundColorBrightnessIncognito;

// Stackview constraints.
extern const CGFloat kTopButtonsBottomMargin;
extern const CGFloat kBottomButtonsBottomMargin;
extern const CGFloat kAdaptiveToolbarMargin;
extern const CGFloat kAdaptiveToolbarStackViewSpacing;
// TODO(crbug.com/800266): Remove those properties.
extern const CGFloat kButtonVerticalMargin;
extern const CGFloat kLocationBarVerticalMargin;
extern const CGFloat kLeadingMarginIPad;
extern const CGFloat kHorizontalMargin;
extern const CGFloat kStackViewSpacing;

// Location bar styling.
extern const CGFloat kLocationBarBorderWidth;
extern const CGFloat kLocationBarBorderColor;
extern const CGFloat kIncognitoLocationBarBorderColor;
extern const CGFloat kLocationBarCornerRadius;
extern const CGFloat kLocationBarShadowHeight;
extern const CGFloat kLocationBarShadowInset;
extern const CGFloat kIncognitoLocationBackgroundColor;

// Progress Bar Height.
extern const CGFloat kProgressBarHeight;

// Toolbar Buttons.
extern const CGFloat kToolsMenuButtonWidth;
extern const CGFloat kAdaptiveToolbarButtonHeight;
extern const CGFloat kAdaptiveToolbarButtonWidth;
extern const CGFloat kOmniboxButtonWidth;
extern const CGFloat kToolbarButtonWidth;
extern const CGFloat kLeadingLocationBarButtonWidth;
extern const CGFloat kToolbarButtonTitleNormalColor;
extern const CGFloat kToolbarButtonTitleHighlightedColor;
extern const CGFloat kIncognitoToolbarButtonTitleNormalColor;
extern const CGFloat kIncognitoToolbarButtonTitleHighlightedColor;
extern const CGFloat kBackButtonImageInset;
extern const CGFloat kForwardButtonImageInset;
extern const CGFloat kLeadingLocationBarButtonImageInset;
extern const CGFloat kCancelButtonHorizontalInset;

// Background color of the blur view.
extern const CGFloat kBlurBackgroundGrayscaleComponent;
extern const CGFloat kBlurBackgroundAlpha;

// Alpha for the tint color of the buttons.
extern const CGFloat kToolbarButtonTintColorAlpha;
// Alpha for the tint color of the buttons when in the highlighted state.
extern const CGFloat kToolbarButtonTintColorAlphaHighlighted;
extern const CGFloat kIncognitoToolbarButtonTintColorAlphaHighlighted;
// Alpha for the spotlight view's background, when the toolbar is dimmed or not.
extern const CGFloat kToolbarSpotlightAlpha;
extern const CGFloat kDimmedToolbarSpotlightAlpha;

// Maximum number of tabs displayed by the button containing the tab count.
extern const NSInteger kShowTabStripButtonMaxTabCount;

// Animation constants.
extern const LayoutOffset kToolbarButtonAnimationOffset;

// Adaptive toolbar position constants.
extern const CGFloat kExpandedLocationBarHorizontalMargin;
extern const CGFloat kContractedLocationBarHorizontalMargin;

// Adaptive Location bar constants.
extern const CGFloat kAdaptiveLocationBarCornerRadius;
extern const CGFloat kAdaptiveLocationBarBackgroundAlpha;
extern const CGFloat kAdaptiveLocationBarVerticalMargin;

// Height of the toolbar.
extern const CGFloat kToolbarHeight;
// Top margin of the top toolbar when the adaptive toolbar is unsplit.
extern const CGFloat kTopToolbarUnsplitMargin;
// Height of the adaptive toolbars.
extern const CGFloat kAdaptiveToolbarHeight;
// Height of the toolbar when in fullscreen.
extern const CGFloat kToolbarHeightFullscreen;

// Accessibility identifier of the tools menu button.
extern NSString* const kToolbarToolsMenuButtonIdentifier;
// Accessibility identifier of the stack button.
extern NSString* const kToolbarStackButtonIdentifier;
// Accessibility identifier of the share button.
extern NSString* const kToolbarShareButtonIdentifier;
// Accessibility identifier of the omnibox button.
extern NSString* const kToolbarOmniboxButtonIdentifier;
// Accessibility identifier of the cancel omnibox edit button.
extern NSString* const kToolbarCancelOmniboxEditButtonIdentifier;

// The maximum number to display in the tab switcher button.
extern NSInteger const kStackButtonMaxTabCount;

// Font size for the TabGrid button containing the tab count.
extern const NSInteger kTabGridButtonFontSize;

// TODO(crbug.com/800266): Remove those properties.
// Font sizes for the button containing the tab count
extern const NSInteger kFontSizeFewerThanTenTabs;
extern const NSInteger kFontSizeTenTabsOrMore;

// Height of the shadow displayed below the toolbar when the omnibox is
// contracted.
extern const CGFloat kToolbarShadowHeight;
// Height of the shadow displayed below the toolbar when the omnibox is
// expanded.
extern const CGFloat kToolbarFullBleedShadowHeight;

// Toolbar style. Determines which button images are used.
enum ToolbarControllerStyle {
  ToolbarControllerStyleLightMode = 0,
  ToolbarControllerStyleDarkMode,
  ToolbarControllerStyleIncognitoMode,
  ToolbarControllerStyleMaxStyles
};

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_BUTTONS_TOOLBAR_CONSTANTS_H_
