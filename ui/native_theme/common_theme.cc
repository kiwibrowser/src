// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/native_theme/common_theme.h"

#include "base/logging.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/skia_util.h"

namespace ui {

SkColor GetAuraColor(NativeTheme::ColorId color_id,
                     const NativeTheme* base_theme) {
  // High contrast overrides the normal colors for certain ColorIds to be much
  // darker or lighter.
  if (base_theme->UsesHighContrastColors()) {
    switch (color_id) {
      case NativeTheme::kColorId_ButtonEnabledColor:
      case NativeTheme::kColorId_ButtonHoverColor:
        return SK_ColorBLACK;
      case NativeTheme::kColorId_MenuBorderColor:
      case NativeTheme::kColorId_MenuSeparatorColor:
        return SK_ColorBLACK;
      case NativeTheme::kColorId_SeparatorColor:
        return SK_ColorBLACK;
      case NativeTheme::kColorId_FocusedBorderColor:
        return gfx::kGoogleBlue900;
      case NativeTheme::kColorId_UnfocusedBorderColor:
        return SK_ColorBLACK;
      case NativeTheme::kColorId_TabBottomBorder:
        return SK_ColorBLACK;
      case NativeTheme::kColorId_ProminentButtonColor:
        return gfx::kGoogleBlue900;
      default:
        break;
    }
  }

  // Second wave of MD colors (colors that only appear in secondary UI).
  if (ui::MaterialDesignController::IsSecondaryUiMaterial()) {
    static const SkColor kPrimaryTextColor = SK_ColorBLACK;

    switch (color_id) {
      // Labels
      case NativeTheme::kColorId_LabelEnabledColor:
        return kPrimaryTextColor;
      case NativeTheme::kColorId_LabelDisabledColor:
        return SkColorSetA(
            base_theme->GetSystemColor(NativeTheme::kColorId_LabelEnabledColor),
            gfx::kDisabledControlAlpha);

      // FocusableBorder
      case NativeTheme::kColorId_UnfocusedBorderColor:
        return SkColorSetA(SK_ColorBLACK, 0x4e);

      // Textfields
      case NativeTheme::kColorId_TextfieldDefaultColor:
        return kPrimaryTextColor;
      case NativeTheme::kColorId_TextfieldDefaultBackground:
        return base_theme->GetSystemColor(
            NativeTheme::kColorId_DialogBackground);
      case NativeTheme::kColorId_TextfieldReadOnlyColor:
        return SkColorSetA(base_theme->GetSystemColor(
                               NativeTheme::kColorId_TextfieldDefaultColor),
                           gfx::kDisabledControlAlpha);

      default:
        break;
    }
  }

  // Shared constant for disabled text.
  static const SkColor kDisabledTextColor = SkColorSetRGB(0xA1, 0xA1, 0x92);

  // Dialogs:
  static const SkColor kDialogBackgroundColor = SK_ColorWHITE;
  // Buttons:
  static const SkColor kButtonEnabledColor = gfx::kChromeIconGrey;
  static const SkColor kProminentButtonColor = gfx::kGoogleBlue500;
  static const SkColor kProminentButtonTextColor = SK_ColorWHITE;
  static const SkColor kBlueButtonTextColor = SK_ColorWHITE;
  static const SkColor kBlueButtonShadowColor = SkColorSetRGB(0x53, 0x8C, 0xEA);
  // MenuItem:
  static const SkColor kTouchableMenuItemLabelColor =
      SkColorSetRGB(0x20, 0x21, 0x24);
  static const SkColor kActionableSubmenuVerticalSeparatorColor =
      SkColorSetARGB(0x24, 0x20, 0x21, 0x24);
  static const SkColor kMenuBackgroundColor = SK_ColorWHITE;
  static const SkColor kMenuHighlightBackgroundColor =
      SkColorSetA(SK_ColorBLACK, 0x14);
  static const SkColor kSelectedMenuItemForegroundColor = SK_ColorBLACK;
  static const SkColor kMenuBorderColor = SkColorSetRGB(0xBA, 0xBA, 0xBA);
  static const SkColor kMenuSeparatorColor = SkColorSetRGB(0xE9, 0xE9, 0xE9);
  static const SkColor kEnabledMenuItemForegroundColor = SK_ColorBLACK;
  static const SkColor kMenuItemMinorTextColor =
      SkColorSetA(SK_ColorBLACK, 0x89);
  // Separator:
  static const SkColor kSeparatorColor = SkColorSetRGB(0xE9, 0xE9, 0xE9);
  // Link:
  static const SkColor kLinkEnabledColor = gfx::kGoogleBlue700;
  // Text selection colors:
  static const SkColor kTextSelectionBackgroundFocused =
      SkColorSetARGB(0x54, 0x60, 0xA8, 0xEB);
  static const SkColor kTextSelectionColor = color_utils::AlphaBlend(
      SK_ColorBLACK, kTextSelectionBackgroundFocused, 0xdd);
  // Textfield:
  static const SkColor kTextfieldDefaultColor = SK_ColorBLACK;
  static const SkColor kTextfieldDefaultBackground = SK_ColorWHITE;
  static const SkColor kTextfieldReadOnlyColor = kDisabledTextColor;
  static const SkColor kTextfieldReadOnlyBackground = SK_ColorWHITE;
  // Results tables:
  static const SkColor kResultsTableText = SK_ColorBLACK;
  static const SkColor kResultsTableDimmedText =
      SkColorSetRGB(0x64, 0x64, 0x64);
  static const SkColor kResultsTableHoveredBackground = color_utils::AlphaBlend(
      kTextSelectionBackgroundFocused, kTextfieldDefaultBackground, 0x40);
  const SkColor kPositiveTextColor = SkColorSetRGB(0x0b, 0x80, 0x43);
  const SkColor kNegativeTextColor = SkColorSetRGB(0xc5, 0x39, 0x29);
  static const SkColor kResultsTablePositiveText = color_utils::AlphaBlend(
      kPositiveTextColor, kTextfieldDefaultBackground, 0xDD);
  static const SkColor kResultsTablePositiveHoveredText =
      color_utils::AlphaBlend(kPositiveTextColor,
                              kResultsTableHoveredBackground, 0xDD);
  static const SkColor kResultsTablePositiveSelectedText =
      color_utils::AlphaBlend(kPositiveTextColor,
                              kTextSelectionBackgroundFocused, 0xDD);
  static const SkColor kResultsTableNegativeText = color_utils::AlphaBlend(
      kNegativeTextColor, kTextfieldDefaultBackground, 0xDD);
  static const SkColor kResultsTableNegativeHoveredText =
      color_utils::AlphaBlend(kNegativeTextColor,
                              kResultsTableHoveredBackground, 0xDD);
  static const SkColor kResultsTableNegativeSelectedText =
      color_utils::AlphaBlend(kNegativeTextColor,
                              kTextSelectionBackgroundFocused, 0xDD);
  // Tooltip:
  static const SkColor kTooltipBackground = SkColorSetA(SK_ColorBLACK, 0xCC);
  static const SkColor kTooltipTextColor = SkColorSetA(SK_ColorWHITE, 0xDE);
  // Tree:
  static const SkColor kTreeBackground = SK_ColorWHITE;
  static const SkColor kTreeTextColor = SK_ColorBLACK;
  static const SkColor kTreeSelectedTextColor = SK_ColorBLACK;
  static const SkColor kTreeSelectionBackgroundColor =
      SkColorSetRGB(0xEE, 0xEE, 0xEE);
  // Table:
  static const SkColor kTableBackground = SK_ColorWHITE;
  static const SkColor kTableTextColor = SK_ColorBLACK;
  static const SkColor kTableSelectedTextColor = SK_ColorBLACK;
  static const SkColor kTableSelectionBackgroundColor =
      SkColorSetRGB(0xEE, 0xEE, 0xEE);
  static const SkColor kTableGroupingIndicatorColor =
      SkColorSetRGB(0xCC, 0xCC, 0xCC);
  // Material spinner/throbber:
  static const SkColor kThrobberSpinningColor = gfx::kGoogleBlue500;
  static const SkColor kThrobberWaitingColor = SkColorSetRGB(0xA6, 0xA6, 0xA6);
  static const SkColor kThrobberLightColor = SkColorSetRGB(0xF4, 0xF8, 0xFD);

  switch (color_id) {
    // Dialogs
    case NativeTheme::kColorId_WindowBackground:
    case NativeTheme::kColorId_DialogBackground:
    case NativeTheme::kColorId_BubbleBackground:
      return kDialogBackgroundColor;

    // Buttons
    case NativeTheme::kColorId_ButtonEnabledColor:
    case NativeTheme::kColorId_ButtonHoverColor:
      return kButtonEnabledColor;
    // TODO(estade): remove the BlueButton colors.
    case NativeTheme::kColorId_BlueButtonEnabledColor:
    case NativeTheme::kColorId_BlueButtonDisabledColor:
    case NativeTheme::kColorId_BlueButtonPressedColor:
    case NativeTheme::kColorId_BlueButtonHoverColor:
      return kBlueButtonTextColor;
    case NativeTheme::kColorId_BlueButtonShadowColor:
      return kBlueButtonShadowColor;
    case NativeTheme::kColorId_ProminentButtonColor:
      return kProminentButtonColor;
    case NativeTheme::kColorId_TextOnProminentButtonColor:
      return kProminentButtonTextColor;
    case NativeTheme::kColorId_ButtonPressedShade:
      return SK_ColorTRANSPARENT;
    case NativeTheme::kColorId_ButtonDisabledColor:
      return kDisabledTextColor;

    // MenuItem
    case NativeTheme::kColorId_TouchableMenuItemLabelColor:
      return kTouchableMenuItemLabelColor;
    case NativeTheme::kColorId_ActionableSubmenuVerticalSeparatorColor:
      return kActionableSubmenuVerticalSeparatorColor;
    case NativeTheme::kColorId_SelectedMenuItemForegroundColor:
      return kSelectedMenuItemForegroundColor;
    case NativeTheme::kColorId_MenuBorderColor:
      return kMenuBorderColor;
    case NativeTheme::kColorId_MenuSeparatorColor:
      return kMenuSeparatorColor;
    case NativeTheme::kColorId_MenuBackgroundColor:
      return kMenuBackgroundColor;
    case NativeTheme::kColorId_FocusedMenuItemBackgroundColor:
      return kMenuHighlightBackgroundColor;
    case NativeTheme::kColorId_EnabledMenuItemForegroundColor:
      return kEnabledMenuItemForegroundColor;
    case NativeTheme::kColorId_DisabledMenuItemForegroundColor:
      return kDisabledTextColor;
    case NativeTheme::kColorId_MenuItemMinorTextColor:
      return kMenuItemMinorTextColor;

    // Label
    case NativeTheme::kColorId_LabelEnabledColor:
      return kButtonEnabledColor;
    case NativeTheme::kColorId_LabelDisabledColor:
      return base_theme->GetSystemColor(
          NativeTheme::kColorId_ButtonDisabledColor);
    case NativeTheme::kColorId_LabelTextSelectionColor:
      return kTextSelectionColor;
    case NativeTheme::kColorId_LabelTextSelectionBackgroundFocused:
      return kTextSelectionBackgroundFocused;

    // Link
    // TODO(estade): where, if anywhere, do we use disabled links in Chrome?
    case NativeTheme::kColorId_LinkDisabled:
      return SK_ColorBLACK;

    case NativeTheme::kColorId_LinkEnabled:
    case NativeTheme::kColorId_LinkPressed:
      return kLinkEnabledColor;

    // Separator
    case NativeTheme::kColorId_SeparatorColor:
      return kSeparatorColor;

    // TabbedPane
    case NativeTheme::kColorId_TabTitleColorActive:
      return SkColorSetRGB(0x42, 0x85, 0xF4);
    case NativeTheme::kColorId_TabTitleColorInactive:
      return SkColorSetRGB(0x75, 0x75, 0x75);
    case NativeTheme::kColorId_TabBottomBorder:
      return SkColorSetA(SK_ColorBLACK, 0x1E);

    // Textfield
    case NativeTheme::kColorId_TextfieldDefaultColor:
      return kTextfieldDefaultColor;
    case NativeTheme::kColorId_TextfieldDefaultBackground:
      return kTextfieldDefaultBackground;
    case NativeTheme::kColorId_TextfieldReadOnlyColor:
      return kTextfieldReadOnlyColor;
    case NativeTheme::kColorId_TextfieldReadOnlyBackground:
      return kTextfieldReadOnlyBackground;
    case NativeTheme::kColorId_TextfieldSelectionColor:
      return kTextSelectionColor;
    case NativeTheme::kColorId_TextfieldSelectionBackgroundFocused:
      return kTextSelectionBackgroundFocused;

    // Tooltip
    case NativeTheme::kColorId_TooltipBackground:
      return kTooltipBackground;
    case NativeTheme::kColorId_TooltipText:
      return kTooltipTextColor;

    // Tree
    case NativeTheme::kColorId_TreeBackground:
      return kTreeBackground;
    case NativeTheme::kColorId_TreeText:
      return kTreeTextColor;
    case NativeTheme::kColorId_TreeSelectedText:
    case NativeTheme::kColorId_TreeSelectedTextUnfocused:
      return kTreeSelectedTextColor;
    case NativeTheme::kColorId_TreeSelectionBackgroundFocused:
    case NativeTheme::kColorId_TreeSelectionBackgroundUnfocused:
      return kTreeSelectionBackgroundColor;

    // Table
    case NativeTheme::kColorId_TableBackground:
      return kTableBackground;
    case NativeTheme::kColorId_TableText:
      return kTableTextColor;
    case NativeTheme::kColorId_TableSelectedText:
    case NativeTheme::kColorId_TableSelectedTextUnfocused:
      return kTableSelectedTextColor;
    case NativeTheme::kColorId_TableSelectionBackgroundFocused:
    case NativeTheme::kColorId_TableSelectionBackgroundUnfocused:
      return kTableSelectionBackgroundColor;
    case NativeTheme::kColorId_TableGroupingIndicatorColor:
      return kTableGroupingIndicatorColor;

    // Table Header
    case NativeTheme::kColorId_TableHeaderText:
      return base_theme->GetSystemColor(
          NativeTheme::kColorId_EnabledMenuItemForegroundColor);
    case NativeTheme::kColorId_TableHeaderBackground:
      return base_theme->GetSystemColor(
          NativeTheme::kColorId_MenuBackgroundColor);
    case NativeTheme::kColorId_TableHeaderSeparator:
      return base_theme->GetSystemColor(NativeTheme::kColorId_MenuBorderColor);

    // FocusableBorder
    case NativeTheme::kColorId_FocusedBorderColor:
      return gfx::kGoogleBlue500;
    case NativeTheme::kColorId_UnfocusedBorderColor:
      return SkColorSetA(SK_ColorBLACK, 0x66);

    // Results Tables
    case NativeTheme::kColorId_ResultsTableNormalBackground:
      return kTextfieldDefaultBackground;
    case NativeTheme::kColorId_ResultsTableHoveredBackground:
      return SkColorSetA(base_theme->GetSystemColor(
                             NativeTheme::kColorId_ResultsTableNormalText),
                         0x0D);
    case NativeTheme::kColorId_ResultsTableSelectedBackground:
      return SkColorSetA(base_theme->GetSystemColor(
                             NativeTheme::kColorId_ResultsTableNormalText),
                         0x14);
    case NativeTheme::kColorId_ResultsTableNormalText:
    case NativeTheme::kColorId_ResultsTableHoveredText:
    case NativeTheme::kColorId_ResultsTableSelectedText:
      return kResultsTableText;
    case NativeTheme::kColorId_ResultsTableNormalDimmedText:
    case NativeTheme::kColorId_ResultsTableHoveredDimmedText:
    case NativeTheme::kColorId_ResultsTableSelectedDimmedText:
      return kResultsTableDimmedText;
    case NativeTheme::kColorId_ResultsTableNormalUrl:
    case NativeTheme::kColorId_ResultsTableHoveredUrl:
    case NativeTheme::kColorId_ResultsTableSelectedUrl:
      return base_theme->GetSystemColor(NativeTheme::kColorId_LinkEnabled);
    case NativeTheme::kColorId_ResultsTablePositiveText:
      return kResultsTablePositiveText;
    case NativeTheme::kColorId_ResultsTablePositiveHoveredText:
      return kResultsTablePositiveHoveredText;
    case NativeTheme::kColorId_ResultsTablePositiveSelectedText:
      return kResultsTablePositiveSelectedText;
    case NativeTheme::kColorId_ResultsTableNegativeText:
      return kResultsTableNegativeText;
    case NativeTheme::kColorId_ResultsTableNegativeHoveredText:
      return kResultsTableNegativeHoveredText;
    case NativeTheme::kColorId_ResultsTableNegativeSelectedText:
      return kResultsTableNegativeSelectedText;

    // Material spinner/throbber
    case NativeTheme::kColorId_ThrobberSpinningColor:
      return kThrobberSpinningColor;
    case NativeTheme::kColorId_ThrobberWaitingColor:
      return kThrobberWaitingColor;
    case NativeTheme::kColorId_ThrobberLightColor:
      return kThrobberLightColor;

    // Alert icon colors
    case NativeTheme::kColorId_AlertSeverityLow:
      return gfx::kGoogleGreen700;
    case NativeTheme::kColorId_AlertSeverityMedium:
      return gfx::kGoogleYellow700;
    case NativeTheme::kColorId_AlertSeverityHigh:
      return gfx::kGoogleRed700;

    case NativeTheme::kColorId_NumColors:
      break;
  }

  NOTREACHED();
  return gfx::kPlaceholderColor;
}

void CommonThemePaintMenuItemBackground(
    const NativeTheme* theme,
    cc::PaintCanvas* canvas,
    NativeTheme::State state,
    const gfx::Rect& rect,
    const NativeTheme::MenuItemExtraParams& menu_item) {
  cc::PaintFlags flags;
  switch (state) {
    case NativeTheme::kNormal:
    case NativeTheme::kDisabled:
      flags.setColor(
          theme->GetSystemColor(NativeTheme::kColorId_MenuBackgroundColor));
      break;
    case NativeTheme::kHovered:
      flags.setColor(theme->GetSystemColor(
          NativeTheme::kColorId_FocusedMenuItemBackgroundColor));
      break;
    default:
      NOTREACHED() << "Invalid state " << state;
      break;
  }
  if (menu_item.corner_radius > 0) {
    const SkScalar radius = SkIntToScalar(menu_item.corner_radius);
    canvas->drawRoundRect(gfx::RectToSkRect(rect), radius, radius, flags);
    return;
  }
  canvas->drawRect(gfx::RectToSkRect(rect), flags);
}

}  // namespace ui
