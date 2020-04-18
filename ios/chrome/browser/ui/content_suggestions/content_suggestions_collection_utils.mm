// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/content_suggestions_collection_utils.h"

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/content_suggestions/cells/content_suggestions_most_visited_cell.h"
#import "ios/chrome/browser/ui/location_bar/location_bar_constants.h"
#import "ios/chrome/browser/ui/ntp/new_tab_page_header_constants.h"
#include "ios/chrome/browser/ui/ui_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/third_party/material_components_ios/src/components/Typography/src/MaterialTypography.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Spacing between tiles.
const CGFloat kHorizontalSpacingRegularXRegular = 19;
const CGFloat kHorizontalSpacingOther = 9;
const CGFloat kVerticalSpacing = 16;
const CGFloat kSpacingIPhone = 16;
const CGFloat kSpacingIPad = 24;

// Width of search field.
const CGFloat kSearchFieldLarge = 432;
const CGFloat kSearchFieldSmall = 343;
const CGFloat kSearchFieldMinMargin = 8;
const CGFloat kMinSearchFieldWidthLegacy = 50;

// Veritcla margin of search hint text.
const CGFloat kSearchHintMargin = 3;

// Offset to align the hint of the fake omnibox with the one in the toolbar.
const CGFloat kSearchHintVerticalOffset = 0.5;
// Maximum margin for the search field.
const CGFloat kMaxSearchFieldFrameMargin = 200;

// Top margin for the doodle.
const CGFloat kDoodleTopMarginRegularXRegular = 162;
const CGFloat kDoodleTopMarginOther = 48;
const CGFloat kDoodleTopMarginIPadLegacy = 82;

// Top margin for the search field
const CGFloat kSearchFieldTopMargin = 32;
const CGFloat kSearchFieldTopMarginIPhoneLegacy = 32;
const CGFloat kSearchFieldTopMarginIPadLegacy = 82;

// Bottom margin for the search field.
const CGFloat kNTPSearchFieldBottomPadding = 16;

// Alpha for search hint text.
const CGFloat kHintAlpha = 0.3;

const CGFloat kTopSpacingMaterial = 24;

const CGFloat kVoiceSearchButtonWidth = 48;

// Height for the doodle frame.
const CGFloat kGoogleSearchDoodleHeight = 120;

// Height for the doodle frame when Google is not the default search engine.
const CGFloat kNonGoogleSearchDoodleHeight = 60;

// Height for the header view on tablet when Google is not the default search
// engine.
const CGFloat kNonGoogleSearchHeaderHeightIPad = 10;

// Returns the width necessary to fit |numberOfItem| items, with no padding on
// the side.
CGFloat widthForNumberOfItem(NSUInteger numberOfItem) {
  return (numberOfItem - 1) *
             content_suggestions::horizontalSpacingBetweenTiles() +
         numberOfItem * [ContentSuggestionsMostVisitedCell defaultSize].width;
}
}

namespace content_suggestions {

const CGFloat kSearchFieldHeight = 50;
const int kSearchFieldBackgroundColor = 0xF1F3F4;
const CGFloat kHintTextScale = 0.85;

const NSUInteger kMostVisitedItemsPerLine = 4;

NSUInteger numberOfTilesForWidth(CGFloat availableWidth) {
  if (IsUIRefreshPhase1Enabled()) {
    return kMostVisitedItemsPerLine;
  }

  if (availableWidth > widthForNumberOfItem(4))
    return 4;
  if (availableWidth > widthForNumberOfItem(3))
    return 3;
  if (availableWidth > widthForNumberOfItem(2))
    return 2;

  return 1;
}

CGFloat horizontalSpacingBetweenTiles() {
  if (IsUIRefreshPhase1Enabled()) {
    return (!IsCompactWidth() && !IsCompactHeight())
               ? kHorizontalSpacingRegularXRegular
               : kHorizontalSpacingOther;
  } else {
    return IsIPadIdiom() ? kSpacingIPad : kSpacingIPhone;
  }
}

CGFloat verticalSpacingBetweenTiles() {
  if (IsUIRefreshPhase1Enabled()) {
    return kVerticalSpacing;
  } else {
    return horizontalSpacingBetweenTiles();
  }
}

CGFloat centeredTilesMarginForWidth(CGFloat width) {
  CGFloat horizontalSpace = horizontalSpacingBetweenTiles();
  NSUInteger columns = numberOfTilesForWidth(width - 2 * horizontalSpace);
  CGFloat whitespace =
      width -
      (columns * [ContentSuggestionsMostVisitedCell defaultSize].width) -
      ((columns - 1) * horizontalSpace);
  CGFloat margin = AlignValueToPixel(whitespace / 2);
  if (IsUIRefreshPhase1Enabled()) {
    // Allow for less spacing as an edge case on smaller devices.
    if (margin < horizontalSpace) {
      DCHECK(width < 400);  // For now this is only expected on small widths.
      return fmaxf(margin, 0);
    }
  } else {
    DCHECK(margin > horizontalSpace);
  }
  return margin;
}

CGFloat doodleHeight(BOOL logoIsShowing) {
  if (!IsRegularXRegularSizeClass() && !logoIsShowing)
    return kNonGoogleSearchDoodleHeight;

  return kGoogleSearchDoodleHeight;
}

CGFloat doodleTopMargin(BOOL toolbarPresent) {
  if (IsUIRefreshPhase1Enabled()) {
    if (!IsCompactWidth() && !IsCompactHeight())
      return kDoodleTopMarginRegularXRegular;
    return StatusBarHeight() + kDoodleTopMarginOther;
  }
  if (IsIPadIdiom())
    return kDoodleTopMarginIPadLegacy;
  return toolbarPresent ? ntp_header::ToolbarHeight() : 0;
}

CGFloat searchFieldTopMargin() {
  if (IsUIRefreshPhase1Enabled()) {
    return kSearchFieldTopMargin;
  }
  if (IsIPadIdiom())
    return kSearchFieldTopMarginIPadLegacy;
  return kSearchFieldTopMarginIPhoneLegacy;
}

CGFloat searchFieldWidth(CGFloat superviewWidth) {
  if (IsUIRefreshPhase1Enabled()) {
    if (!IsCompactWidth() && !IsCompactHeight())
      return kSearchFieldLarge;

    // Special case for narrow sizes.
    return MIN(kSearchFieldSmall, superviewWidth - kSearchFieldMinMargin * 2);
  }
  CGFloat margin = centeredTilesMarginForWidth(superviewWidth);
  if (margin > kMaxSearchFieldFrameMargin)
    margin = kMaxSearchFieldFrameMargin;
  return fmax(superviewWidth - 2 * margin, kMinSearchFieldWidthLegacy);
}

CGFloat heightForLogoHeader(BOOL logoIsShowing,
                            BOOL promoCanShow,
                            BOOL toolbarPresent) {
  CGFloat headerHeight = doodleTopMargin(toolbarPresent) +
                         doodleHeight(logoIsShowing) + searchFieldTopMargin() +
                         kSearchFieldHeight + kNTPSearchFieldBottomPadding;
  if (!IsRegularXRegularSizeClass()) {
    return headerHeight;
  }
  if (!logoIsShowing) {
    return kNonGoogleSearchHeaderHeightIPad;
  }
  if (!promoCanShow) {
    headerHeight += kTopSpacingMaterial;
  }

  return headerHeight;
}

void configureSearchHintLabel(UILabel* searchHintLabel,
                              UIView* hintLabelContainer,
                              UIButton* searchTapTarget) {
  // searchHintLabel is intentionally not using autolayout because it will need
  // to use a CGAffineScale transform that will not work correctly with
  // autolayout.  Instead, |hintLabelContainer| will use autolayout and will
  // contain |searchHintLabel|.
  searchHintLabel.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [hintLabelContainer setTranslatesAutoresizingMaskIntoConstraints:NO];
  [searchTapTarget addSubview:hintLabelContainer];
  [hintLabelContainer addSubview:searchHintLabel];

  CGFloat centerYOffsetConstant =
      IsUIRefreshPhase1Enabled() ? 0 : kSearchHintVerticalOffset;
  [NSLayoutConstraint activateConstraints:@[
    [hintLabelContainer.centerYAnchor
        constraintEqualToAnchor:searchTapTarget.centerYAnchor
                       constant:centerYOffsetConstant],
    [hintLabelContainer.heightAnchor
        constraintEqualToConstant:kSearchFieldHeight - 2 * kSearchHintMargin],
  ]];

  [searchHintLabel setText:l10n_util::GetNSString(IDS_OMNIBOX_EMPTY_HINT)];
  if (base::i18n::IsRTL()) {
    [searchHintLabel setTextAlignment:NSTextAlignmentRight];
  }
  if (IsUIRefreshPhase1Enabled()) {
    [searchHintLabel setTextColor:[UIColor colorWithWhite:0 alpha:kHintAlpha]];
    searchHintLabel.font =
        [UIFont preferredFontForTextStyle:UIFontTextStyleTitle3];
  } else {
    [searchHintLabel
        setTextColor:
            [UIColor colorWithWhite:kiPhoneLocationBarPlaceholderColorBrightness
                              alpha:1.0]];
    [searchHintLabel setFont:[MDCTypography subheadFont]];
  }
}

void configureVoiceSearchButton(UIButton* voiceSearchButton,
                                UIButton* searchTapTarget) {
  UIImage* micImage = [UIImage imageNamed:@"voice_icon"];
  [voiceSearchButton setTranslatesAutoresizingMaskIntoConstraints:NO];
  [searchTapTarget addSubview:voiceSearchButton];

  [NSLayoutConstraint activateConstraints:@[
    [voiceSearchButton.centerYAnchor
        constraintEqualToAnchor:searchTapTarget.centerYAnchor],
    [voiceSearchButton.widthAnchor
        constraintEqualToConstant:kVoiceSearchButtonWidth],
    [voiceSearchButton.heightAnchor
        constraintEqualToAnchor:voiceSearchButton.widthAnchor],
  ]];

  [voiceSearchButton setAdjustsImageWhenHighlighted:NO];
  [voiceSearchButton setImage:micImage forState:UIControlStateNormal];
  [voiceSearchButton setAccessibilityLabel:l10n_util::GetNSString(
                                               IDS_IOS_ACCNAME_VOICE_SEARCH)];
  [voiceSearchButton setAccessibilityIdentifier:@"Voice Search"];
}

UIView* nearestAncestor(UIView* view, Class aClass) {
  if (!view) {
    return nil;
  }
  if ([view isKindOfClass:aClass]) {
    return view;
  }
  return nearestAncestor([view superview], aClass);
}

// Content suggestion dupliations of uikit_ui_util to allow wrapping behind
// the refresh flag.  Post refresh remove these helpers and just check
// IsRxRSC instead.
BOOL IsRegularXRegularSizeClass(id<UITraitEnvironment> environment) {
  return IsUIRefreshPhase1Enabled() ? ::IsRegularXRegularSizeClass(environment)
                                    : IsIPadIdiom();
}

BOOL IsRegularXRegularSizeClass() {
  return IsUIRefreshPhase1Enabled() ? ::IsRegularXRegularSizeClass()
                                    : IsIPadIdiom();
}

}  // namespace content_suggestions
