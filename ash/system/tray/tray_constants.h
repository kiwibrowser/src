// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_TRAY_CONSTANTS_H_
#define ASH_SYSTEM_TRAY_TRAY_CONSTANTS_H_

#include "ash/ash_export.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"

namespace ash {

extern const int kBubblePaddingHorizontalBottom;

// The size delta between the default font and the font size found in tray
// items like labels and buttons.
extern const int kTrayTextFontSizeIncrease;

ASH_EXPORT extern const int kTrayItemSize;

// Extra padding used beside a single icon in the tray area of the shelf.
constexpr int kTrayImageItemPadding = 3;

extern const int kTrayLabelItemHorizontalPaddingBottomAlignment;
extern const int kTrayLabelItemVerticalPaddingVerticalAlignment;

// The width of the tray menu.
extern const int kTrayMenuWidth;

extern const int kTrayPopupAutoCloseDelayInSeconds;
extern const int kTrayPopupPaddingHorizontal;
extern const int kTrayPopupPaddingBetweenItems;
extern const int kTrayPopupButtonEndMargin;

// The padding used on the left and right of labels. This applies to all labels
// in the system menu.
extern const int kTrayPopupLabelHorizontalPadding;

// The horizontal padding used to properly lay out a slider in a TriView
// container with a FillLayout (such as a volume notification bubble).
extern const int kTrayPopupSliderHorizontalPadding;

// The minimum/default height of the rows in the system tray menu.
extern const int kTrayPopupItemMinHeight;

// The width used for the first region of the row (which holds an image).
extern const int kTrayPopupItemMinStartWidth;

// The width used for the end region of the row (usually a more arrow).
extern const int kTrayPopupItemMinEndWidth;

// When transitioning between a detailed and a default view, this delay is used
// before the transition starts.
ASH_EXPORT extern const int kTrayDetailedViewTransitionDelayMs;

// Padding used on right side of labels to keep minimum distance to the next
// item. This applies to all labels in the system menu.
extern const int kTrayPopupLabelRightPadding;

extern const int kTrayRoundedBorderRadius;

// The width of ToggleButton views including any border padding.
extern const int kTrayToggleButtonWidth;

extern const SkColor kPublicAccountUserCardTextColor;
extern const SkColor kPublicAccountUserCardNameColor;

extern const SkColor kHeaderBackgroundColor;

extern const SkColor kHeaderTextColorNormal;

extern const SkColor kMobileNotConnectedXIconColor;

// Extra padding used to adjust hitting region around tray items.
extern const int kHitRegionPadding;

// Width of a line used to separate tray items in the shelf.
ASH_EXPORT extern const int kSeparatorWidth;

// The color of the separators used in the system menu.
extern const SkColor kMenuSeparatorColor;

// The size and foreground color of the icons appearing in the material design
// system tray.
extern const int kTrayIconSize;
extern const SkColor kTrayIconColor;

// The total visual padding at the start and end of the icon/label section
// of the tray.
constexpr int kTrayEdgePadding = 6;

// The size and foreground color of the icons appearing in the material design
// system menu.
extern const int kMenuIconSize;
extern const SkColor kMenuIconColor;
extern const SkColor kMenuIconColorDisabled;
// The size of buttons in the system menu.
ASH_EXPORT extern const int kMenuButtonSize;
// The vertical padding for the system menu separator.
extern const int kMenuSeparatorVerticalPadding;
// The horizontal padding for the system menu separator.
extern const int kMenuExtraMarginFromLeftEdge;
// The visual padding to the left of icons in the system menu.
extern const int kMenuEdgeEffectivePadding;

// The base color used for all ink drops in the system menu.
extern const SkColor kTrayPopupInkDropBaseColor;

// The opacity of the ink drop ripples for all ink drops in the system menu.
extern const float kTrayPopupInkDropRippleOpacity;

// The opacity of the ink drop ripples for all ink highlights in the system
// menu.
extern const float kTrayPopupInkDropHighlightOpacity;

// The inset applied to clickable surfaces in the system menu that do not have
// the ink drop filling the entire bounds.
extern const int kTrayPopupInkDropInset;

// The radius used to draw the corners of the rounded rect style ink drops.
extern const int kTrayPopupInkDropCornerRadius;

// The height of the system info row.
extern const int kTrayPopupSystemInfoRowHeight;

// The colors used when --enable-features=SystemTrayUnified flag is enabled.
constexpr SkColor kUnifiedMenuBackgroundColor = SkColorSetRGB(0x20, 0x21, 0x24);
constexpr SkColor kUnifiedMenuBackgroundColorWithBlur =
    SkColorSetA(kUnifiedMenuBackgroundColor, 0xB3);
constexpr float kUnifiedMenuBackgroundBlur = 30.f;
constexpr SkColor kUnifiedMenuTextColor = SkColorSetRGB(0xf1, 0xf3, 0xf4);
constexpr SkColor kUnifiedMenuIconColor = SkColorSetRGB(0xf1, 0xf3, 0xf4);
constexpr SkColor kUnifiedMenuSecondaryTextColor =
    SkColorSetA(kUnifiedMenuIconColor, 0xa3);
constexpr SkColor kUnifiedMenuIconColorDisabled =
    SkColorSetA(kUnifiedMenuIconColor, 0xa3);
constexpr SkColor kUnifiedMenuButtonColor =
    SkColorSetA(kUnifiedMenuIconColor, 0x14);
constexpr SkColor kUnifiedMenuButtonColorActive =
    SkColorSetRGB(0x25, 0x81, 0xdf);
constexpr SkColor kUnifiedFeaturePodHoverColor =
    SkColorSetRGB(0xff, 0xff, 0xff);

constexpr gfx::Insets kUnifiedMenuItemPadding(0, 16, 16, 16);
constexpr gfx::Insets kUnifiedSliderPadding(0, 16);

constexpr int kUnifiedNotificationCenterSpacing = 16;
constexpr int kUnifiedTrayCornerRadius = 20;
constexpr int kUnifiedTopShortcutSpacing = 16;
constexpr gfx::Insets kUnifiedTopShortcutPadding(0, 16);

constexpr int kUnifiedSystemInfoHeight = 16;
constexpr int kUnifiedSystemInfoSpacing = 8;

// Constants used in FeaturePodsView of UnifiedSystemTray.
constexpr gfx::Size kUnifiedFeaturePodIconSize(48, 48);
constexpr gfx::Size kUnifiedFeaturePodSize(80, 88);
constexpr gfx::Size kUnifiedFeaturePodCollapsedSize(48, 48);
constexpr gfx::Size kUnifiedFeaturePodHoverSize(80, 36);
constexpr gfx::Insets kUnifiedFeaturePodIconPadding(4);
constexpr gfx::Insets kUnifiedFeaturePodHoverPadding(2);
constexpr int kUnifiedFeaturePodSpacing = 6;
constexpr int kUnifiedFeaturePodHoverRadius = 4;
constexpr int kUnifiedFeaturePodVerticalPadding = 28;
constexpr int kUnifiedFeaturePodHorizontalSidePadding = 28;
constexpr int kUnifiedFeaturePodHorizontalMiddlePadding = 32;
constexpr int kUnifiedFeaturePodCollapsedVerticalPadding = 16;
constexpr int kUnifiedFeaturePodCollapsedHorizontalPadding = 24;
constexpr int kUnifiedFeaturePodItemsInRow = 3;
constexpr int kUnifiedFeaturePodMaxItemsInCollapsed = 5;

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_TRAY_CONSTANTS_H_
