// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/themes/theme_properties.h"

#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "chrome/browser/themes/browser_theme_pack.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace {

// ----------------------------------------------------------------------------
// Defaults for properties which are stored in the browser theme pack. If you
// change these defaults, you must increment the version number in
// browser_theme_pack.cc.

const SkColor kDefaultColorFrame = SkColorSetRGB(0xCC, 0xCC, 0xCC);
const SkColor kDefaultColorFrameInactive = SkColorSetRGB(0xF5, 0xF5, 0xF5);

#if defined(OS_MACOSX)
const SkColor kDefaultColorFrameIncognito =
    SkColorSetARGB(0xE6, 0x14, 0x16, 0x18);
const SkColor kDefaultColorFrameIncognitoInactive =
    SkColorSetRGB(0x1E, 0x1E, 0x1E);
#else
const SkColor kDefaultColorFrameIncognito = SkColorSetRGB(0x28, 0x2B, 0x2D);
const SkColor kDefaultColorFrameIncognitoInactive =
    SkColorSetRGB(0x38, 0x3B, 0x3D);
#endif

const SkColor kDefaultColorToolbar = SkColorSetRGB(0xF2, 0xF2, 0xF2);
const SkColor kDefaultColorToolbarIncognito = SkColorSetRGB(0x50, 0x50, 0x50);

const SkColor kDefaultDetachedBookmarkBarBackground = SK_ColorWHITE;
const SkColor kDefaultDetachedBookmarkBarBackgroundIncognito =
    SkColorSetRGB(0x32, 0x32, 0x32);

// "Toolbar" text is used for active tabs and the bookmarks bar.
constexpr SkColor kDefaultColorToolbarText = SK_ColorBLACK;
constexpr SkColor kDefaultColorToolbarTextIncognito = SK_ColorWHITE;
constexpr SkColor kDefaultColorBackgroundTabText = SK_ColorBLACK;
constexpr SkColor kDefaultColorBackgroundTabTextIncognito = SK_ColorWHITE;

const SkColor kDefaultColorBookmarkInstructionsText =
    SkColorSetRGB(0x64, 0x64, 0x64);
const SkColor kDefaultColorBookmarkInstructionsTextIncognito =
    SkColorSetA(SK_ColorWHITE, 0x8A);

#if defined(OS_WIN)
const SkColor kDefaultColorNTPBackground =
    color_utils::GetSysSkColor(COLOR_WINDOW);
const SkColor kDefaultColorNTPText =
    color_utils::GetSysSkColor(COLOR_WINDOWTEXT);
const SkColor kDefaultColorNTPLink = color_utils::GetSysSkColor(COLOR_HOTLIGHT);
#else
// TODO(beng): source from theme provider.
constexpr SkColor kDefaultColorNTPBackground = SK_ColorWHITE;
constexpr SkColor kDefaultColorNTPText = SK_ColorBLACK;
const SkColor kDefaultColorNTPLink = SkColorSetRGB(0x06, 0x37, 0x74);
#endif  // OS_WIN

// Then new MD Incognito NTP uses a slightly different shade of black.
// TODO(msramek): Remove the old entry when the new NTP fully launches.
const SkColor kDefaultColorNTPBackgroundIncognito =
    SkColorSetRGB(0x30, 0x30, 0x30);

const SkColor kDefaultColorNTPHeader = SkColorSetRGB(0x96, 0x96, 0x96);
constexpr SkColor kDefaultColorButtonBackground = SK_ColorTRANSPARENT;

// Default tints.
constexpr color_utils::HSL kDefaultTintButtons = {-1, -1, -1};
constexpr color_utils::HSL kDefaultTintButtonsIncognito = {-1, -1, 0.85};
constexpr color_utils::HSL kDefaultTintFrame = {-1, -1, -1};
constexpr color_utils::HSL kDefaultTintFrameInactive = {-1, -1, 0.75};
constexpr color_utils::HSL kDefaultTintFrameIncognito = {-1, 0.2, 0.35};
constexpr color_utils::HSL kDefaultTintFrameIncognitoInactive = {-1, 0.3, 0.6};
constexpr color_utils::HSL kDefaultTintBackgroundTab = {-1, -1, 0.75};

constexpr SkColor kDefaultColorTabAlertRecordingIcon =
    SkColorSetRGB(0xC5, 0x39, 0x29);
constexpr SkColor kDefaultColorTabAlertCapturingIcon =
    SkColorSetRGB(0x42, 0x85, 0xF4);

// ----------------------------------------------------------------------------
// Defaults for properties which are not stored in the browser theme pack.

constexpr SkColor kDefaultColorControlBackground = SK_ColorWHITE;
const SkColor kDefaultToolbarBottomSeparator = SkColorSetRGB(0xB6, 0xB4, 0xB6);
const SkColor kDefaultToolbarBottomSeparatorIncognito =
    SkColorSetRGB(0x28, 0x28, 0x28);
const SkColor kDefaultToolbarTopSeparator = SkColorSetA(SK_ColorBLACK, 0x40);

#if defined(OS_MACOSX)
const SkColor kDefaultColorFrameVibrancyOverlay =
    SkColorSetA(SK_ColorBLACK, 0x19);
const SkColor kDefaultColorFrameVibrancyOverlayIncognito =
    SkColorSetARGB(0xE6, 0x14, 0x16, 0x18);
const SkColor kDefaultColorToolbarInactive = SkColorSetRGB(0xF6, 0xF6, 0xF6);
const SkColor kDefaultColorToolbarInactiveIncognito =
    SkColorSetRGB(0x2D, 0x2D, 0x2D);
const SkColor kDefaultColorTabBackgroundInactive =
    SkColorSetRGB(0xEC, 0xEC, 0xEC);
const SkColor kDefaultColorTabBackgroundInactiveIncognito =
    SkColorSetRGB(0x28, 0x28, 0x28);
const SkColor kDefaultColorToolbarButtonStroke =
    SkColorSetARGB(0x4B, 0x51, 0x51, 0x51);
const SkColor kDefaultColorToolbarButtonStrokeInactive =
    SkColorSetARGB(0x4B, 0x63, 0x63, 0x63);
const SkColor kDefaultColorToolbarBezel = SkColorSetRGB(0xCC, 0xCC, 0xCC);
const SkColor kDefaultColorToolbarStroke = SkColorSetA(SK_ColorBLACK, 0x4C);
const SkColor kDefaultColorToolbarStrokeInactive =
    SkColorSetRGB(0xA3, 0xA3, 0xA3);
const SkColor kDefaultColorToolbarIncognitoStroke =
    SkColorSetA(SK_ColorBLACK, 0x3F);
const SkColor kDefaultColorToolbarStrokeTheme =
    SkColorSetA(SK_ColorWHITE, 0x66);
const SkColor kDefaultColorToolbarStrokeThemeInactive =
    SkColorSetARGB(0x66, 0x4C, 0x4C, 0x4C);
#endif  // OS_MACOSX

// Strings used in alignment properties.
constexpr char kAlignmentCenter[] = "center";
constexpr char kAlignmentTop[] = "top";
constexpr char kAlignmentBottom[] = "bottom";
constexpr char kAlignmentLeft[] = "left";
constexpr char kAlignmentRight[] = "right";

// Strings used in background tiling repetition properties.
constexpr char kTilingNoRepeat[] = "no-repeat";
constexpr char kTilingRepeatX[] = "repeat-x";
constexpr char kTilingRepeatY[] = "repeat-y";
constexpr char kTilingRepeat[] = "repeat";

// Returns a |nullopt| if the newer material UI is not enabled (MD refresh or
// touch-optimized UI).
base::Optional<SkColor> MaybeGetDefaultColorForNewerMaterialUi(int id,
                                                               bool incognito) {
  if (!ui::MaterialDesignController::IsNewerMaterialUi())
    return base::nullopt;

  switch (id) {
    case ThemeProperties::COLOR_FRAME:
    case ThemeProperties::COLOR_FRAME_INACTIVE:
    case ThemeProperties::COLOR_BACKGROUND_TAB:
      return incognito ? gfx::kGoogleGrey900 : gfx::kGoogleGrey200;
    case ThemeProperties::COLOR_TOOLBAR:
      return incognito ? SkColorSetRGB(0x32, 0x36, 0x39) : SK_ColorWHITE;

    case ThemeProperties::COLOR_TAB_TEXT:
      return incognito ? gfx::kGoogleGrey100 : gfx::kGoogleGrey800;

    case ThemeProperties::COLOR_BOOKMARK_TEXT:
      return incognito ? gfx::kGoogleGrey100 : gfx::kGoogleGrey700;

    case ThemeProperties::COLOR_TAB_CLOSE_BUTTON_ACTIVE:
    case ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON:
      return incognito ? gfx::kGoogleGrey100 : gfx::kChromeIconGrey;

    case ThemeProperties::COLOR_BACKGROUND_TAB_TEXT:
    case ThemeProperties::COLOR_TAB_CLOSE_BUTTON_INACTIVE:
    case ThemeProperties::COLOR_TAB_ALERT_AUDIO:
      return incognito ? gfx::kGoogleGrey400 : gfx::kChromeIconGrey;

    case ThemeProperties::COLOR_TAB_CLOSE_BUTTON_BACKGROUND_HOVER:
      return incognito ? gfx::kGoogleGrey700 : gfx::kGoogleGrey200;
    case ThemeProperties::COLOR_TAB_CLOSE_BUTTON_BACKGROUND_PRESSED:
      return incognito ? gfx::kGoogleGrey600 : gfx::kGoogleGrey300;
    case ThemeProperties::COLOR_TAB_ALERT_RECORDING:
      return incognito ? gfx::kGoogleGrey400 : gfx::kGoogleRed600;
    case ThemeProperties::COLOR_TAB_PIP_PLAYING:
      return incognito ? gfx::kGoogleGrey400 : gfx::kGoogleBlue600;
    case ThemeProperties::COLOR_TAB_ALERT_CAPTURING:
      return incognito ? gfx::kGoogleGrey400 : gfx::kGoogleBlue600;

    default:
      return base::nullopt;
  }
}

}  // namespace

// static
int ThemeProperties::StringToAlignment(const std::string& alignment) {
  int alignment_mask = 0;
  for (const std::string& component : base::SplitString(
           alignment, base::kWhitespaceASCII,
           base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    if (base::LowerCaseEqualsASCII(component, kAlignmentTop))
      alignment_mask |= ALIGN_TOP;
    else if (base::LowerCaseEqualsASCII(component, kAlignmentBottom))
      alignment_mask |= ALIGN_BOTTOM;
    else if (base::LowerCaseEqualsASCII(component, kAlignmentLeft))
      alignment_mask |= ALIGN_LEFT;
    else if (base::LowerCaseEqualsASCII(component, kAlignmentRight))
      alignment_mask |= ALIGN_RIGHT;
  }
  return alignment_mask;
}

// static
int ThemeProperties::StringToTiling(const std::string& tiling) {
  if (base::LowerCaseEqualsASCII(tiling, kTilingRepeatX))
    return REPEAT_X;
  if (base::LowerCaseEqualsASCII(tiling, kTilingRepeatY))
    return REPEAT_Y;
  if (base::LowerCaseEqualsASCII(tiling, kTilingRepeat))
    return REPEAT;
  // NO_REPEAT is the default choice.
  return NO_REPEAT;
}

// static
std::string ThemeProperties::AlignmentToString(int alignment) {
  // Convert from an AlignmentProperty back into a string.
  std::string vertical_string(kAlignmentCenter);
  std::string horizontal_string(kAlignmentCenter);

  if (alignment & ALIGN_TOP)
    vertical_string = kAlignmentTop;
  else if (alignment & ALIGN_BOTTOM)
    vertical_string = kAlignmentBottom;

  if (alignment & ALIGN_LEFT)
    horizontal_string = kAlignmentLeft;
  else if (alignment & ALIGN_RIGHT)
    horizontal_string = kAlignmentRight;

  return horizontal_string + " " + vertical_string;
}

// static
std::string ThemeProperties::TilingToString(int tiling) {
  // Convert from a TilingProperty back into a string.
  if (tiling == REPEAT_X)
    return kTilingRepeatX;
  if (tiling == REPEAT_Y)
    return kTilingRepeatY;
  if (tiling == REPEAT)
    return kTilingRepeat;
  return kTilingNoRepeat;
}

// static
color_utils::HSL ThemeProperties::GetDefaultTint(int id, bool incognito) {
  switch (id) {
    case TINT_FRAME:
      return incognito ? kDefaultTintFrameIncognito : kDefaultTintFrame;
    case TINT_FRAME_INACTIVE:
      return incognito ? kDefaultTintFrameIncognitoInactive
                       : kDefaultTintFrameInactive;
    case TINT_BUTTONS:
      return incognito ? kDefaultTintButtonsIncognito : kDefaultTintButtons;
    case TINT_BACKGROUND_TAB:
      return kDefaultTintBackgroundTab;
    case TINT_FRAME_INCOGNITO:
    case TINT_FRAME_INCOGNITO_INACTIVE:
      NOTREACHED() << "These values should be queried via their respective "
                      "non-incognito equivalents and an appropriate "
                      "|incognito| value.";
      FALLTHROUGH;
    default:
      return {-1, -1, -1};
  }
}

// static
SkColor ThemeProperties::GetDefaultColor(int id, bool incognito) {
  const base::Optional<SkColor> color =
      MaybeGetDefaultColorForNewerMaterialUi(id, incognito);
  if (color)
    return color.value();

  switch (id) {
    // Properties stored in theme pack.
    case COLOR_FRAME:
      return incognito ? kDefaultColorFrameIncognito : kDefaultColorFrame;
    case COLOR_FRAME_INACTIVE:
      return incognito ? kDefaultColorFrameIncognitoInactive
                       : kDefaultColorFrameInactive;
    case COLOR_TOOLBAR:
      return incognito ? kDefaultColorToolbarIncognito : kDefaultColorToolbar;
    case COLOR_TAB_TEXT:
    case COLOR_BOOKMARK_TEXT:
      return incognito ? kDefaultColorToolbarTextIncognito
                       : kDefaultColorToolbarText;
    case COLOR_BACKGROUND_TAB_TEXT:
      return incognito ? kDefaultColorBackgroundTabTextIncognito
                       : kDefaultColorBackgroundTabText;
    case COLOR_NTP_BACKGROUND:
      if (!incognito)
        return kDefaultColorNTPBackground;
      return kDefaultColorNTPBackgroundIncognito;
    case COLOR_NTP_TEXT:
      return kDefaultColorNTPText;
    case COLOR_NTP_LINK:
      return kDefaultColorNTPLink;
    case COLOR_NTP_HEADER:
      return kDefaultColorNTPHeader;
    case COLOR_BUTTON_BACKGROUND:
      return kDefaultColorButtonBackground;

    // Properties not stored in theme pack.
    case COLOR_CONTROL_BACKGROUND:
      return kDefaultColorControlBackground;
    case COLOR_BOOKMARK_BAR_INSTRUCTIONS_TEXT:
      return incognito ? kDefaultColorBookmarkInstructionsTextIncognito
                       : kDefaultColorBookmarkInstructionsText;
    case COLOR_DETACHED_BOOKMARK_BAR_SEPARATOR:
      // We shouldn't reach this case because the color is calculated from
      // others.
      NOTREACHED();
      return gfx::kPlaceholderColor;
    case COLOR_DETACHED_BOOKMARK_BAR_BACKGROUND:
      return incognito ? kDefaultDetachedBookmarkBarBackgroundIncognito
                       : kDefaultDetachedBookmarkBarBackground;
    case COLOR_TOOLBAR_BOTTOM_SEPARATOR:
      return incognito ? kDefaultToolbarBottomSeparatorIncognito
                       : kDefaultToolbarBottomSeparator;
    case COLOR_TOOLBAR_TOP_SEPARATOR:
    case COLOR_TOOLBAR_TOP_SEPARATOR_INACTIVE:
      return kDefaultToolbarTopSeparator;
    case COLOR_TAB_ALERT_RECORDING:
      return kDefaultColorTabAlertRecordingIcon;
    case COLOR_TAB_ALERT_CAPTURING:
      return kDefaultColorTabAlertCapturingIcon;
#if defined(OS_MACOSX)
    case COLOR_FRAME_VIBRANCY_OVERLAY:
      return incognito ? kDefaultColorFrameVibrancyOverlayIncognito
                       : kDefaultColorFrameVibrancyOverlay;
    case COLOR_TOOLBAR_INACTIVE:
      return incognito ? kDefaultColorToolbarInactiveIncognito
                       : kDefaultColorToolbarInactive;
    case COLOR_BACKGROUND_TAB_INACTIVE:
      return incognito ? kDefaultColorTabBackgroundInactiveIncognito
                       : kDefaultColorTabBackgroundInactive;
    case COLOR_TOOLBAR_BUTTON_STROKE:
      return kDefaultColorToolbarButtonStroke;
    case COLOR_TOOLBAR_BUTTON_STROKE_INACTIVE:
      return kDefaultColorToolbarButtonStrokeInactive;
    case COLOR_TOOLBAR_BEZEL:
      return kDefaultColorToolbarBezel;
    case COLOR_TOOLBAR_STROKE:
      return incognito ? kDefaultColorToolbarIncognitoStroke
                       : kDefaultColorToolbarStroke;
    case COLOR_TOOLBAR_STROKE_INACTIVE:
      return kDefaultColorToolbarStrokeInactive;
    case COLOR_TOOLBAR_STROKE_THEME:
      return kDefaultColorToolbarStrokeTheme;
    case COLOR_TOOLBAR_STROKE_THEME_INACTIVE:
      return kDefaultColorToolbarStrokeThemeInactive;
#endif
#if defined(OS_WIN)
    case COLOR_ACCENT_BORDER:
      NOTREACHED();
      return gfx::kPlaceholderColor;
#endif

    case COLOR_FRAME_INCOGNITO:
    case COLOR_FRAME_INCOGNITO_INACTIVE:
      NOTREACHED() << "These values should be queried via their respective "
                      "non-incognito equivalents and an appropriate "
                      "|incognito| value.";
      return gfx::kPlaceholderColor;
  }

  return gfx::kPlaceholderColor;
}
