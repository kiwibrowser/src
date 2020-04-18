// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/native_theme/native_theme_mac.h"

#import <Cocoa/Cocoa.h>
#include <stddef.h>

#include "base/mac/mac_util.h"
#include "base/mac/sdk_forward_declarations.h"
#include "base/macros.h"
#import "skia/ext/skia_utils_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/skia_util.h"
#include "ui/native_theme/common_theme.h"

@interface NSWorkspace (Redeclarations)

@property(readonly) BOOL accessibilityDisplayShouldIncreaseContrast;

@end

namespace {

const SkColor kMenuPopupBackgroundColor = SK_ColorWHITE;

// Hardcoded color used for some existing dialogs in Chrome's Cocoa UI.
const SkColor kDialogBackgroundColor = SkColorSetRGB(251, 251, 251);

// Color for the highlighted text in a control when that control doesn't have
// keyboard focus.
const SkColor kUnfocusedSelectedTextBackgroundColor =
    SkColorSetRGB(220, 220, 220);

// Helper to make indexing an array by an enum class easier.
template <class KEY, class VALUE>
struct EnumArray {
  VALUE& operator[](const KEY& key) { return array[static_cast<size_t>(key)]; }
  VALUE array[static_cast<size_t>(KEY::COUNT)];
};

// NSColor has a number of methods that return system colors (i.e. controlled by
// user preferences). This function converts the color given by an NSColor class
// method to an SkColor. Official documentation suggests developers only rely on
// +[NSColor selectedTextBackgroundColor] and +[NSColor selectedControlColor],
// but other colors give a good baseline. For many, a gradient is involved; the
// palette chosen based on the enum value given by +[NSColor currentColorTint].
// Apple's documentation also suggests to use NSColorList, but the system color
// list is just populated with class methods on NSColor.
SkColor NSSystemColorToSkColor(NSColor* color) {
  // System colors use the an NSNamedColorSpace called "System", so first step
  // is to convert the color into something that can be worked with.
  NSColor* device_color =
      [color colorUsingColorSpace:[NSColorSpace deviceRGBColorSpace]];
  if (device_color)
    return skia::NSDeviceColorToSkColor(device_color);

  // Sometimes the conversion is not possible, but we can get an approximation
  // by going through a CGColorRef. Note that simply using NSColor methods for
  // accessing components for system colors results in exceptions like
  // "-numberOfComponents not valid for the NSColor NSNamedColorSpace System
  // windowBackgroundColor; need to first convert colorspace." Hence the
  // conversion first to CGColor.
  CGColorRef cg_color = [color CGColor];
  const size_t component_count = CGColorGetNumberOfComponents(cg_color);
  if (component_count == 4)
    return skia::CGColorRefToSkColor(cg_color);

  CHECK(component_count == 1 || component_count == 2);
  // 1-2 components means a grayscale channel and maybe an alpha channel, which
  // CGColorRefToSkColor will not like. But RGB is additive, so the conversion
  // is easy (RGB to grayscale is less easy).
  const CGFloat* components = CGColorGetComponents(cg_color);
  CGFloat alpha = component_count == 2 ? components[1] : 1.0;
  return SkColorSetARGB(SkScalarRoundToInt(255.0 * alpha),
                        SkScalarRoundToInt(255.0 * components[0]),
                        SkScalarRoundToInt(255.0 * components[0]),
                        SkScalarRoundToInt(255.0 * components[0]));
}

// Converts an SkColor to grayscale by using luminance for all three components.
// Experimentally, this seems to produce a better result than a flat average or
// a min/max average for UI controls.
SkColor ColorToGrayscale(SkColor color) {
  SkScalar luminance = SkColorGetR(color) * 0.21 +
                       SkColorGetG(color) * 0.72 +
                       SkColorGetB(color) * 0.07;
  uint8_t component = SkScalarRoundToInt(luminance);
  return SkColorSetARGB(SkColorGetA(color), component, component, component);
}

}  // namespace

namespace ui {

// static
NativeTheme* NativeTheme::GetInstanceForWeb() {
  return NativeThemeMac::instance();
}

// static
NativeTheme* NativeTheme::GetInstanceForNativeUi() {
  return NativeThemeMac::instance();
}

// static
NativeThemeMac* NativeThemeMac::instance() {
  CR_DEFINE_STATIC_LOCAL(NativeThemeMac, s_native_theme, ());
  return &s_native_theme;
}

// static
SkColor NativeThemeMac::ApplySystemControlTint(SkColor color) {
  if ([NSColor currentControlTint] == NSGraphiteControlTint)
    return ColorToGrayscale(color);
  return color;
}

SkColor NativeThemeMac::GetSystemColor(ColorId color_id) const {
  // Even with --secondary-ui-md, menus use the platform colors and styling, and
  // Mac has a couple of specific color overrides, documented below.
  switch (color_id) {
    case kColorId_EnabledMenuItemForegroundColor:
      return NSSystemColorToSkColor([NSColor controlTextColor]);
    case kColorId_DisabledMenuItemForegroundColor:
      return NSSystemColorToSkColor([NSColor disabledControlTextColor]);
    case kColorId_SelectedMenuItemForegroundColor:
      return SK_ColorBLACK;
    case kColorId_FocusedMenuItemBackgroundColor:
      return gfx::kGoogleGrey200;
    case kColorId_MenuBackgroundColor:
      return kMenuPopupBackgroundColor;
    case kColorId_MenuSeparatorColor:
      return UsesHighContrastColors() ? SK_ColorBLACK
                                      : SkColorSetA(SK_ColorBLACK, 0x26);
    case kColorId_MenuBorderColor:
      return UsesHighContrastColors() ? SK_ColorBLACK
                                      : SkColorSetA(SK_ColorBLACK, 0x60);

    // Mac has a different "pressed button" styling because it doesn't use
    // ripples.
    case kColorId_ButtonPressedShade:
      return SkColorSetA(SK_ColorBLACK, 0x10);

    // There's a system setting General > Highlight color which sets the
    // background color for text selections. We honor that setting.
    // TODO(ellyjones): Listen for NSSystemColorsDidChangeNotification somewhere
    // and propagate it to the View hierarchy.
    case kColorId_LabelTextSelectionBackgroundFocused:
    case kColorId_TextfieldSelectionBackgroundFocused:
      return NSSystemColorToSkColor([NSColor selectedTextBackgroundColor]);

    case kColorId_FocusedBorderColor:
      return NSSystemColorToSkColor([NSColor keyboardFocusIndicatorColor]);

    default:
      break;
  }

  if (ui::MaterialDesignController::IsSecondaryUiMaterial())
    return ApplySystemControlTint(GetAuraColor(color_id, this));

  // TODO(tapted): Add caching for these, and listen for
  // NSSystemColorsDidChangeNotification.
  switch (color_id) {
    case kColorId_WindowBackground:
      return NSSystemColorToSkColor([NSColor windowBackgroundColor]);
    case kColorId_DialogBackground:
      return kDialogBackgroundColor;
    case kColorId_BubbleBackground:
      return SK_ColorWHITE;

    case kColorId_FocusedBorderColor:
      return NSSystemColorToSkColor([NSColor keyboardFocusIndicatorColor]);
    case kColorId_UnfocusedBorderColor:
      return NSSystemColorToSkColor([NSColor controlColor]);

    // Buttons and labels.
    case kColorId_ButtonEnabledColor:
    case kColorId_LabelEnabledColor:
      return NSSystemColorToSkColor([NSColor controlTextColor]);
    // NSColor doesn't offer a color for prominent buttons. Use the Aura color,
    // but apply the system tint. This is a good match for the blue Cocoa uses
    // to draw buttons that are given a \n key equivalent.
    case kColorId_ProminentButtonColor:
      return ApplySystemControlTint(GetAuraColor(color_id, this));
    case kColorId_ButtonDisabledColor:
    case kColorId_LabelDisabledColor:
      return NSSystemColorToSkColor([NSColor disabledControlTextColor]);
    case kColorId_ButtonHoverColor:
      return NSSystemColorToSkColor([NSColor selectedControlTextColor]);
    case kColorId_LabelTextSelectionColor:
      return NSSystemColorToSkColor([NSColor selectedTextColor]);

    // Link.
    case kColorId_LinkDisabled:
      return SK_ColorBLACK;
    case kColorId_LinkEnabled:
      return SK_ColorBLUE;
    case kColorId_LinkPressed:
      return SK_ColorRED;

    // Text fields.
    case kColorId_TextfieldDefaultColor:
    case kColorId_TextfieldReadOnlyColor:
      return NSSystemColorToSkColor([NSColor textColor]);
    case kColorId_TextfieldDefaultBackground:
    case kColorId_TextfieldReadOnlyBackground:
      return NSSystemColorToSkColor([NSColor textBackgroundColor]);
    case kColorId_TextfieldSelectionColor:
      return NSSystemColorToSkColor([NSColor selectedTextColor]);

    // Trees/Tables. For focused text, use the alternate* versions, which
    // NSColor documents as "the table and list view equivalent to the
    // selectedControlTextColor".
    case kColorId_TreeBackground:
    case kColorId_TableBackground:
      return NSSystemColorToSkColor([NSColor controlBackgroundColor]);
    case kColorId_TreeText:
    case kColorId_TableText:
    case kColorId_TableSelectedTextUnfocused:
    case kColorId_TreeSelectedTextUnfocused:
      return NSSystemColorToSkColor([NSColor textColor]);
    case kColorId_TreeSelectedText:
    case kColorId_TableSelectedText:
      return NSSystemColorToSkColor(
          [NSColor alternateSelectedControlTextColor]);
    case kColorId_TreeSelectionBackgroundFocused:
    case kColorId_TableSelectionBackgroundFocused:
      return NSSystemColorToSkColor([NSColor alternateSelectedControlColor]);
    case kColorId_TreeSelectionBackgroundUnfocused:
    case kColorId_TableSelectionBackgroundUnfocused:
      return kUnfocusedSelectedTextBackgroundColor;
    case kColorId_TableGroupingIndicatorColor:
      return SkColorSetRGB(140, 140, 140);

    default:
      // TODO(tapted): Handle all values and remove the default case.
      return GetAuraColor(color_id, this);
  }
}

void NativeThemeMac::PaintMenuPopupBackground(
    cc::PaintCanvas* canvas,
    const gfx::Size& size,
    const MenuBackgroundExtraParams& menu_background) const {
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(kMenuPopupBackgroundColor);
  const SkScalar radius = SkIntToScalar(menu_background.corner_radius);
  SkRect rect = gfx::RectToSkRect(gfx::Rect(size));
  canvas->drawRoundRect(rect, radius, radius, flags);
}

void NativeThemeMac::PaintMenuItemBackground(
    cc::PaintCanvas* canvas,
    State state,
    const gfx::Rect& rect,
    const MenuItemExtraParams& menu_item) const {
  switch (state) {
    case NativeTheme::kNormal:
    case NativeTheme::kDisabled:
      // Draw nothing over the regular background.
      break;
    case NativeTheme::kHovered:
      PaintSelectedMenuItem(canvas, rect);
      break;
    default:
      NOTREACHED();
      break;
  }
}

bool NativeThemeMac::UsesHighContrastColors() const {
  if (NativeThemeBase::UsesHighContrastColors())
    return true;
  NSWorkspace* workspace = [NSWorkspace sharedWorkspace];
  if ([workspace respondsToSelector:@selector
                 (accessibilityDisplayShouldIncreaseContrast)]) {
    return workspace.accessibilityDisplayShouldIncreaseContrast;
  }
  return false;
}

NativeThemeMac::NativeThemeMac() {
}

NativeThemeMac::~NativeThemeMac() {
}

void NativeThemeMac::PaintSelectedMenuItem(cc::PaintCanvas* canvas,
                                           const gfx::Rect& rect) const {
  // Draw the background.
  cc::PaintFlags flags;
  flags.setColor(GetSystemColor(kColorId_FocusedMenuItemBackgroundColor));
  canvas->drawRect(gfx::RectToSkRect(rect), flags);

  // In high contrast, draw a border stroke as well.
  if (UsesHighContrastColors()) {
    constexpr int kStrokeWidth = 2;
    cc::PaintFlags border_flags;
    gfx::Rect border_rect = rect;
    border_rect.Inset(kStrokeWidth / 2, kStrokeWidth / 2);
    border_flags.setColor(GetSystemColor(kColorId_MenuBorderColor));
    border_flags.setStyle(cc::PaintFlags::kStroke_Style);
    border_flags.setStrokeWidth(kStrokeWidth);
    canvas->drawRect(gfx::RectToSkRect(border_rect), border_flags);
  }
}

}  // namespace ui
