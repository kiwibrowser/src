// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtkui/native_theme_gtk2.h"

#include <gtk/gtk.h>

#include "chrome/browser/ui/libgtkui/chrome_gtk_frame.h"
#include "chrome/browser/ui/libgtkui/chrome_gtk_menu_subclasses.h"
#include "chrome/browser/ui/libgtkui/gtk_ui.h"
#include "chrome/browser/ui/libgtkui/gtk_util.h"
#include "chrome/browser/ui/libgtkui/skia_utils_gtk.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/path.h"
#include "ui/gfx/skia_util.h"
#include "ui/native_theme/common_theme.h"
#include "ui/native_theme/native_theme_aura.h"
#include "ui/native_theme/native_theme_dark_aura.h"

namespace libgtkui {

namespace {

enum WidgetState {
  NORMAL = 0,
  ACTIVE = 1,
  PRELIGHT = 2,
  SELECTED = 3,
  INSENSITIVE = 4,
};

// Same order as enum WidgetState above
const GtkStateType stateMap[] = {
    GTK_STATE_NORMAL,   GTK_STATE_ACTIVE,      GTK_STATE_PRELIGHT,
    GTK_STATE_SELECTED, GTK_STATE_INSENSITIVE,
};

SkColor GetFgColor(GtkWidget* widget, WidgetState state) {
  return GdkColorToSkColor(gtk_rc_get_style(widget)->fg[stateMap[state]]);
}
SkColor GetBgColor(GtkWidget* widget, WidgetState state) {
  return GdkColorToSkColor(gtk_rc_get_style(widget)->bg[stateMap[state]]);
}

SkColor GetTextColor(GtkWidget* widget, WidgetState state) {
  return GdkColorToSkColor(gtk_rc_get_style(widget)->text[stateMap[state]]);
}
SkColor GetTextAAColor(GtkWidget* widget, WidgetState state) {
  return GdkColorToSkColor(gtk_rc_get_style(widget)->text_aa[stateMap[state]]);
}
SkColor GetBaseColor(GtkWidget* widget, WidgetState state) {
  return GdkColorToSkColor(gtk_rc_get_style(widget)->base[stateMap[state]]);
}

}  // namespace

// static
NativeThemeGtk2* NativeThemeGtk2::instance() {
  CR_DEFINE_STATIC_LOCAL(NativeThemeGtk2, s_native_theme, ());
  return &s_native_theme;
}

// Constructors automatically called
NativeThemeGtk2::NativeThemeGtk2() {}
// This doesn't actually get called
NativeThemeGtk2::~NativeThemeGtk2() {}

void NativeThemeGtk2::PaintMenuPopupBackground(
    cc::PaintCanvas* canvas,
    const gfx::Size& size,
    const MenuBackgroundExtraParams& menu_background) const {
  if (menu_background.corner_radius > 0) {
    cc::PaintFlags flags;
    flags.setStyle(cc::PaintFlags::kFill_Style);
    flags.setAntiAlias(true);
    flags.setColor(GetSystemColor(kColorId_MenuBackgroundColor));

    gfx::Path path;
    SkRect rect = SkRect::MakeWH(SkIntToScalar(size.width()),
                                 SkIntToScalar(size.height()));
    SkScalar radius = SkIntToScalar(menu_background.corner_radius);
    SkScalar radii[8] = {radius, radius, radius, radius,
                         radius, radius, radius, radius};
    path.addRoundRect(rect, radii);

    canvas->drawPath(path, flags);
  } else {
    canvas->drawColor(GetSystemColor(kColorId_MenuBackgroundColor),
                      SkBlendMode::kSrc);
  }
}

void NativeThemeGtk2::PaintMenuItemBackground(
    cc::PaintCanvas* canvas,
    State state,
    const gfx::Rect& rect,
    const MenuItemExtraParams& menu_item) const {
  SkColor color;
  cc::PaintFlags flags;
  switch (state) {
    case NativeTheme::kNormal:
    case NativeTheme::kDisabled:
      color = GetSystemColor(NativeTheme::kColorId_MenuBackgroundColor);
      flags.setColor(color);
      break;
    case NativeTheme::kHovered:
      color =
          GetSystemColor(NativeTheme::kColorId_FocusedMenuItemBackgroundColor);
      flags.setColor(color);
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

SkColor NativeThemeGtk2::GetSystemColor(ColorId color_id) const {
  const SkColor kPositiveTextColor = SkColorSetRGB(0x0b, 0x80, 0x43);
  const SkColor kNegativeTextColor = SkColorSetRGB(0xc5, 0x39, 0x29);

  switch (color_id) {
    // Windows
    case kColorId_WindowBackground:
      return GetBgColor(GetWindow(), SELECTED);

    // Dialogs
    case kColorId_DialogBackground:
    case kColorId_BubbleBackground:
      return GetBgColor(GetWindow(), NORMAL);

    // FocusableBorder
    case kColorId_FocusedBorderColor:
      return GetBgColor(GetEntry(), SELECTED);
    case kColorId_UnfocusedBorderColor:
      return GetTextAAColor(GetEntry(), NORMAL);

    // MenuItem
    case kColorId_SelectedMenuItemForegroundColor:
      return GetTextColor(GetMenuItem(), SELECTED);
    case kColorId_FocusedMenuItemBackgroundColor:
      return GetBgColor(GetMenuItem(), SELECTED);

    case kColorId_EnabledMenuItemForegroundColor:
      return GetTextColor(GetMenuItem(), NORMAL);
    case kColorId_MenuItemMinorTextColor:
    case kColorId_DisabledMenuItemForegroundColor:
      return GetTextColor(GetMenuItem(), INSENSITIVE);
    case kColorId_MenuBorderColor:
    case kColorId_MenuSeparatorColor:
      return GetTextColor(GetMenuItem(), INSENSITIVE);
    case kColorId_MenuBackgroundColor:
      return GetBgColor(GetMenu(), NORMAL);

    // Label
    case kColorId_LabelEnabledColor:
      return GetTextColor(GetEntry(), NORMAL);
    case kColorId_LabelDisabledColor:
      return GetTextColor(GetLabel(), INSENSITIVE);
    case kColorId_LabelTextSelectionColor:
      return GetTextColor(GetLabel(), SELECTED);
    case kColorId_LabelTextSelectionBackgroundFocused:
      return GetBaseColor(GetLabel(), SELECTED);

    // Link
    case kColorId_LinkDisabled:
      return SkColorSetA(GetSystemColor(kColorId_LinkEnabled), 0xBB);
    case kColorId_LinkEnabled: {
      SkColor link_color = SK_ColorTRANSPARENT;
      GdkColor* style_color = nullptr;
      gtk_widget_style_get(GetWindow(), "link-color", &style_color, nullptr);
      if (style_color) {
        link_color = GdkColorToSkColor(*style_color);
        gdk_color_free(style_color);
      }
      if (link_color != SK_ColorTRANSPARENT)
        return link_color;
      // Default color comes from gtklinkbutton.c.
      return SkColorSetRGB(0x00, 0x00, 0xEE);
    }
    case kColorId_LinkPressed:
      return SK_ColorRED;

    // Separator
    case kColorId_SeparatorColor:
      return GetFgColor(GetSeparator(), INSENSITIVE);

    // Button
    case kColorId_ButtonEnabledColor:
      return GetTextColor(GetButton(), NORMAL);
    case kColorId_BlueButtonEnabledColor:
      return GetTextColor(GetBlueButton(), NORMAL);
    case kColorId_ButtonDisabledColor:
      return GetTextColor(GetButton(), INSENSITIVE);
    case kColorId_BlueButtonDisabledColor:
      return GetTextColor(GetBlueButton(), INSENSITIVE);
    case kColorId_ButtonHoverColor:
      return GetTextColor(GetButton(), PRELIGHT);
    case kColorId_BlueButtonHoverColor:
      return GetTextColor(GetBlueButton(), PRELIGHT);
    case kColorId_BlueButtonPressedColor:
      return GetTextColor(GetBlueButton(), ACTIVE);
    case kColorId_BlueButtonShadowColor:
      return SK_ColorTRANSPARENT;
    case kColorId_ProminentButtonColor:
      return GetSystemColor(kColorId_LinkEnabled);
    case kColorId_TextOnProminentButtonColor:
      return GetTextColor(GetLabel(), SELECTED);
    case kColorId_ButtonPressedShade:
      return SK_ColorTRANSPARENT;

    // TabbedPane
    case ui::NativeTheme::kColorId_TabTitleColorActive:
      return GetTextColor(GetEntry(), NORMAL);
    case ui::NativeTheme::kColorId_TabTitleColorInactive:
      return GetTextColor(GetLabel(), INSENSITIVE);
    case ui::NativeTheme::kColorId_TabBottomBorder:
      return GetTextColor(GetEntry(), NORMAL);

    // Textfield
    case kColorId_TextfieldDefaultColor:
      return GetTextColor(GetEntry(), NORMAL);
    case kColorId_TextfieldDefaultBackground:
      return GetBaseColor(GetEntry(), NORMAL);

    case kColorId_TextfieldReadOnlyColor:
      return GetTextColor(GetEntry(), ACTIVE);
    case kColorId_TextfieldReadOnlyBackground:
      return GetBaseColor(GetEntry(), ACTIVE);
    case kColorId_TextfieldSelectionColor:
      return GetTextColor(GetEntry(), SELECTED);
    case kColorId_TextfieldSelectionBackgroundFocused:
      return GetBaseColor(GetEntry(), SELECTED);

    // Tooltips
    case kColorId_TooltipBackground:
      return GetBgColor(GetTooltip(), NORMAL);
    case kColorId_TooltipText:
      return GetFgColor(GetTooltip(), NORMAL);

    // Trees and Tables (implemented on GTK using the same class)
    case kColorId_TableBackground:
    case kColorId_TreeBackground:
      return GetBgColor(GetTree(), NORMAL);
    case kColorId_TableText:
    case kColorId_TreeText:
      return GetTextColor(GetTree(), NORMAL);
    case kColorId_TableSelectedText:
    case kColorId_TableSelectedTextUnfocused:
    case kColorId_TreeSelectedText:
    case kColorId_TreeSelectedTextUnfocused:
      return GetTextColor(GetTree(), SELECTED);
    case kColorId_TableSelectionBackgroundFocused:
    case kColorId_TableSelectionBackgroundUnfocused:
    case kColorId_TreeSelectionBackgroundFocused:
    case kColorId_TreeSelectionBackgroundUnfocused:
      return GetBgColor(GetTree(), SELECTED);
    case kColorId_TableGroupingIndicatorColor:
      return GetTextAAColor(GetTree(), NORMAL);

    // Table Headers
    case kColorId_TableHeaderText:
      return GetTextColor(GetTree(), NORMAL);
    case kColorId_TableHeaderBackground:
      return GetBgColor(GetTree(), NORMAL);
    case kColorId_TableHeaderSeparator:
      return GetFgColor(GetSeparator(), INSENSITIVE);

    // Results Table
    case kColorId_ResultsTableNormalBackground:
      return GetSystemColor(kColorId_TextfieldDefaultBackground);
    case kColorId_ResultsTableHoveredBackground:
      return color_utils::AlphaBlend(
          GetSystemColor(kColorId_TextfieldDefaultBackground),
          GetSystemColor(kColorId_TextfieldSelectionBackgroundFocused), 0x80);
    case kColorId_ResultsTableSelectedBackground:
      return GetSystemColor(kColorId_TextfieldSelectionBackgroundFocused);
    case kColorId_ResultsTableNormalText:
    case kColorId_ResultsTableHoveredText:
      return GetSystemColor(kColorId_TextfieldDefaultColor);
    case kColorId_ResultsTableSelectedText:
      return GetSystemColor(kColorId_TextfieldSelectionColor);
    case kColorId_ResultsTableNormalDimmedText:
    case kColorId_ResultsTableHoveredDimmedText:
      return color_utils::AlphaBlend(
          GetSystemColor(kColorId_TextfieldDefaultColor),
          GetSystemColor(kColorId_TextfieldDefaultBackground), 0x80);
    case kColorId_ResultsTableSelectedDimmedText:
      return color_utils::AlphaBlend(
          GetSystemColor(kColorId_TextfieldSelectionColor),
          GetSystemColor(kColorId_TextfieldDefaultBackground), 0x80);
    case kColorId_ResultsTableNormalUrl:
    case kColorId_ResultsTableHoveredUrl:
      return NormalURLColor(GetSystemColor(kColorId_TextfieldDefaultColor));

    case kColorId_ResultsTableSelectedUrl:
      return SelectedURLColor(
          GetSystemColor(kColorId_TextfieldSelectionColor),
          GetSystemColor(kColorId_TextfieldSelectionBackgroundFocused));

    case kColorId_ResultsTablePositiveText: {
      return color_utils::GetReadableColor(kPositiveTextColor,
                                           GetBaseColor(GetEntry(), NORMAL));
    }
    case kColorId_ResultsTablePositiveHoveredText: {
      return color_utils::GetReadableColor(kPositiveTextColor,
                                           GetBaseColor(GetEntry(), PRELIGHT));
    }
    case kColorId_ResultsTablePositiveSelectedText: {
      return color_utils::GetReadableColor(kPositiveTextColor,
                                           GetBaseColor(GetEntry(), SELECTED));
    }
    case kColorId_ResultsTableNegativeText: {
      return color_utils::GetReadableColor(kNegativeTextColor,
                                           GetBaseColor(GetEntry(), NORMAL));
    }
    case kColorId_ResultsTableNegativeHoveredText: {
      return color_utils::GetReadableColor(kNegativeTextColor,
                                           GetBaseColor(GetEntry(), PRELIGHT));
    }
    case kColorId_ResultsTableNegativeSelectedText: {
      return color_utils::GetReadableColor(kNegativeTextColor,
                                           GetBaseColor(GetEntry(), SELECTED));
    }

    // Throbber
    case kColorId_ThrobberSpinningColor:
    case kColorId_ThrobberLightColor:
      return GetSystemColor(kColorId_TextfieldSelectionBackgroundFocused);

    case kColorId_ThrobberWaitingColor:
      return color_utils::AlphaBlend(
          GetSystemColor(kColorId_TextfieldSelectionBackgroundFocused),
          GetBgColor(GetWindow(), NORMAL), 0x80);

    // Alert icons
    // Just fall back to the same colors as Aura.
    case kColorId_AlertSeverityLow:
    case kColorId_AlertSeverityMedium:
    case kColorId_AlertSeverityHigh: {
      ui::NativeTheme* fallback_theme =
          color_utils::IsDark(GetTextColor(GetEntry(), NORMAL))
              ? ui::NativeTheme::GetInstanceForNativeUi()
              : ui::NativeThemeDarkAura::instance();
      return fallback_theme->GetSystemColor(color_id);
    }

    case kColorId_NumColors:
      NOTREACHED();
      break;
  }

  return kInvalidColorIdColor;
}

GtkWidget* NativeThemeGtk2::GetWindow() const {
  static GtkWidget* fake_window = nullptr;

  if (!fake_window) {
    fake_window = chrome_gtk_frame_new();
    gtk_widget_realize(fake_window);
  }

  return fake_window;
}

GtkWidget* NativeThemeGtk2::GetEntry() const {
  static GtkWidget* fake_entry = nullptr;

  if (!fake_entry) {
    fake_entry = gtk_entry_new();

    // The fake entry needs to be in the window so it can be realized so we can
    // use the computed parts of the style.
    gtk_container_add(GTK_CONTAINER(GetWindow()), fake_entry);
    gtk_widget_realize(fake_entry);
  }

  return fake_entry;
}

GtkWidget* NativeThemeGtk2::GetLabel() const {
  static GtkWidget* fake_label = nullptr;

  if (!fake_label)
    fake_label = gtk_label_new("");

  return fake_label;
}

GtkWidget* NativeThemeGtk2::GetButton() const {
  static GtkWidget* fake_button = nullptr;

  if (!fake_button)
    fake_button = gtk_button_new();

  return fake_button;
}

GtkWidget* NativeThemeGtk2::GetBlueButton() const {
  static GtkWidget* fake_bluebutton = nullptr;

  if (!fake_bluebutton) {
    fake_bluebutton = gtk_button_new();
    TurnButtonBlue(fake_bluebutton);
  }

  return fake_bluebutton;
}

GtkWidget* NativeThemeGtk2::GetTree() const {
  static GtkWidget* fake_tree = nullptr;

  if (!fake_tree)
    fake_tree = gtk_tree_view_new();

  return fake_tree;
}

GtkWidget* NativeThemeGtk2::GetTooltip() const {
  static GtkWidget* fake_tooltip = nullptr;

  if (!fake_tooltip) {
    fake_tooltip = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(fake_tooltip, "gtk-tooltip");
    gtk_widget_realize(fake_tooltip);
  }

  return fake_tooltip;
}

GtkWidget* NativeThemeGtk2::GetMenu() const {
  static GtkWidget* fake_menu = nullptr;

  if (!fake_menu)
    fake_menu = gtk_custom_menu_new();

  return fake_menu;
}

GtkWidget* NativeThemeGtk2::GetMenuItem() const {
  static GtkWidget* fake_menu_item = nullptr;

  if (!fake_menu_item) {
    fake_menu_item = gtk_custom_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(GetMenu()), fake_menu_item);
  }

  return fake_menu_item;
}

GtkWidget* NativeThemeGtk2::GetSeparator() const {
  static GtkWidget* fake_separator = nullptr;

  if (!fake_separator)
    fake_separator = gtk_hseparator_new();

  return fake_separator;
}

}  // namespace libgtkui
