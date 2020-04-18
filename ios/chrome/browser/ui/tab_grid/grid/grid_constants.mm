// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/tab_grid/grid/grid_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Accessibility identifier prefix of a grid cell.
NSString* const kGridCellIdentifierPrefix = @"GridCellIdentifierPrefix";

// Accessibility identifier for the close button in a grid cell.
NSString* const kGridCellCloseButtonIdentifier =
    @"GridCellCloseButtonIdentifier";

// Grid styling.
const int kGridBackgroundColor = 0x222222;

// Definition of limited width for applicable size classes. The first refers to
// the horizontal size class; the second to the vertical.
const CGFloat kGridLayoutCompactCompactLimitedWidth = 666.0f;
const CGFloat kGridLayoutCompactRegularLimitedWidth = 374.0f;
// Insets for size classes. The first refers to the horizontal size class; the
// second to the vertical.
const UIEdgeInsets kGridLayoutInsetsCompactCompact =
    UIEdgeInsets{20.0f, 20.0f, 20.0f, 20.0f};
const UIEdgeInsets kGridLayoutInsetsCompactCompactLimitedWidth =
    UIEdgeInsets{22.0f, 44.0f, 22.0f, 44.0f};
const UIEdgeInsets kGridLayoutInsetsCompactRegular =
    UIEdgeInsets{13.0f, 13.0f, 13.0f, 13.0f};
const UIEdgeInsets kGridLayoutInsetsCompactRegularLimitedWidth =
    UIEdgeInsets{28.0f, 10.0f, 28.0f, 10.0f};
const UIEdgeInsets kGridLayoutInsetsRegularCompact =
    UIEdgeInsets{32.0f, 32.0f, 32.0f, 32.0f};
const UIEdgeInsets kGridLayoutInsetsRegularRegular =
    UIEdgeInsets{28.0f, 28.0f, 28.0f, 28.0f};
// Minimum line spacing for size classes. The first refers to the horizontal
// size class; the second to the vertical.
const CGFloat kGridLayoutLineSpacingCompactCompact = 17.0f;
const CGFloat kGridLayoutLineSpacingCompactCompactLimitedWidth = 22.0f;
const CGFloat kGridLayoutLineSpacingCompactRegular = 13.0f;
const CGFloat kGridLayoutLineSpacingCompactRegularLimitedWidth = 15.0f;
const CGFloat kGridLayoutLineSpacingRegularCompact = 32.0f;
const CGFloat kGridLayoutLineSpacingRegularRegular = 14.0f;

// GridCell styling.
// Common colors.
const int kGridCellIconBackgroundColor = 0xF1F3F4;
const int kGridCellSnapshotBackgroundColor = 0xE8EAED;
// Light theme colors.
const int kGridLightThemeCellTitleColor = 0x000000;
const int kGridLightThemeCellHeaderColor = 0xF8F9FA;
const int kGridLightThemeCellSelectionColor = 0x1A73E8;
const int kGridLightThemeCellCloseButtonTintColor = 0x3C4043;
// Dark theme colors.
const int kGridDarkThemeCellTitleColor = 0xFFFFFF;
const int kGridDarkThemeCellHeaderColor = 0x5F6368;
const int kGridDarkThemeCellSelectionColor = 0x9AA0A6;
const int kGridDarkThemeCellCloseButtonTintColor = 0xFFFFFF;

// GridCell dimensions.
const CGSize kGridCellSizeSmall = CGSize{144.0f, 168.0f};
const CGSize kGridCellSizeMedium = CGSize{168.0f, 202.0f};
const CGSize kGridCellSizeLarge = CGSize{228.0f, 256.0f};
const CGFloat kGridCellCornerRadius = 13.0f;
const CGFloat kGridCellIconCornerRadius = 3.0f;
// The cell header contains the icon, title, and close button.
const CGFloat kGridCellHeaderHeight = 32.0f;
const CGFloat kGridCellHeaderLeadingInset = 5.0f;
const CGFloat kGridCellCloseButtonContentInset = 8.5f;
const CGFloat kGridCellIconDiameter = 22.0f;
const CGFloat kGridCellSelectionRingGapWidth = 2.0f;
const CGFloat kGridCellSelectionRingTintWidth = 5.0f;
