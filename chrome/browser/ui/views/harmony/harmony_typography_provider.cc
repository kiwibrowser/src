// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/harmony/harmony_typography_provider.h"

#include "chrome/browser/ui/views/harmony/chrome_typography.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/platform_font.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/view.h"

#if defined(OS_WIN)
#include "base/win/windows_version.h"
#include "ui/gfx/platform_font_win.h"
#include "ui/native_theme/native_theme_win.h"
#endif

#if defined(OS_CHROMEOS)
// gn check complains on Linux Ozone.
#include "ash/public/cpp/ash_typography.h"  // nogncheck
#endif

namespace {

// If the default foreground color from the native theme isn't black, the rest
// of the Harmony spec isn't going to work. Also skip Harmony if a Windows
// High Contrast theme is enabled. One of the four standard High Contrast themes
// in Windows 10 still has black text, but (since the user wants high contrast)
// the grey text shades in Harmony should not be used.
bool ShouldIgnoreHarmonySpec(const ui::NativeTheme& theme) {
  if (theme.UsesHighContrastColors())
    return true;

  constexpr auto kTestColorId = ui::NativeTheme::kColorId_LabelEnabledColor;
  return theme.GetSystemColor(kTestColorId) != SK_ColorBLACK;
}

// Returns a color for a possibly inverted or high-contrast OS color theme.
SkColor GetHarmonyTextColorForNonStandardNativeTheme(
    int context,
    int style,
    const ui::NativeTheme& theme) {
  // At the time of writing, very few UI surfaces need typography for a Chrome-
  // provided theme. Typically just incognito browser windows (when the native
  // theme is NativeThemeDarkAura). Instead, this method is consulted when the
  // actual OS theme is configured in a special way. So pick from a small number
  // of NativeTheme constants that are known to adapt properly to distinct
  // colors when configuring the OS to use a high-contrast theme. For example,
  // ::GetSysColor() on Windows has 8 text colors: BTNTEXT, CAPTIONTEXT,
  // GRAYTEXT, HIGHLIGHTTEXT, INACTIVECAPTIONTEXT, INFOTEXT (tool tips),
  // MENUTEXT, and WINDOWTEXT. There's also hyperlinks: COLOR_HOTLIGHT.
  // Diverging from these risks using a color that doesn't match user
  // expectations.

  const bool inverted_scheme = color_utils::IsInvertedColorScheme();

  ui::NativeTheme::ColorId color_id =
      (context == views::style::CONTEXT_BUTTON ||
       context == views::style::CONTEXT_BUTTON_MD)
          ? ui::NativeTheme::kColorId_ButtonEnabledColor
          : ui::NativeTheme::kColorId_TextfieldDefaultColor;
  switch (style) {
    case views::style::STYLE_DIALOG_BUTTON_DEFAULT:
      // This is just white in Harmony and, even in inverted themes, prominent
      // buttons have a dark background, so white will maximize contrast.
      return SK_ColorWHITE;
    case views::style::STYLE_DISABLED:
      color_id = ui::NativeTheme::kColorId_LabelDisabledColor;
      break;
    case views::style::STYLE_LINK:
      color_id = ui::NativeTheme::kColorId_LinkEnabled;
      break;
    case STYLE_RED:
      return inverted_scheme ? gfx::kGoogleRed300 : gfx::kGoogleRed700;
    case STYLE_GREEN:
      return inverted_scheme ? gfx::kGoogleGreen300 : gfx::kGoogleGreen700;
  }
  return theme.GetSystemColor(color_id);
}

}  // namespace

#if defined(OS_WIN)
// static
int HarmonyTypographyProvider::GetPlatformFontHeight(int font_context) {
  const bool direct_write_enabled =
      gfx::PlatformFontWin::IsDirectWriteEnabled();
  const bool windows_10 = base::win::GetVersion() >= base::win::VERSION_WIN10;
  switch (font_context) {
    case CONTEXT_HEADLINE:
      return windows_10 && direct_write_enabled ? 27 : 28;
    case views::style::CONTEXT_DIALOG_TITLE:
      return windows_10 || !direct_write_enabled ? 20 : 21;
    case CONTEXT_BODY_TEXT_LARGE:
    case views::style::CONTEXT_MESSAGE_BOX_BODY_TEXT:
      return direct_write_enabled ? 18 : 17;
    case CONTEXT_BODY_TEXT_SMALL:
      return windows_10 && direct_write_enabled ? 16 : 15;
  }
  NOTREACHED();
  return 0;
}
#endif

const gfx::FontList& HarmonyTypographyProvider::GetFont(int context,
                                                        int style) const {
  // "Target" font size constants from the Harmony spec.
  constexpr int kHeadlineSize = 20;
  constexpr int kTitleSize = 15;
  constexpr int kBodyTextLargeSize = 13;
  constexpr int kDefaultSize = 12;

  int size_delta = kDefaultSize - gfx::PlatformFont::kDefaultBaseFontSize;
  gfx::Font::Weight font_weight = gfx::Font::Weight::NORMAL;

#if defined(OS_CHROMEOS)
  ash::ApplyAshFontStyles(context, style, &size_delta, &font_weight);
#endif

  ApplyCommonFontStyles(context, style, &size_delta, &font_weight);

  switch (context) {
    case views::style::CONTEXT_BUTTON_MD:
      font_weight = MediumWeightForUI();
      break;
    case views::style::CONTEXT_DIALOG_TITLE:
      size_delta = kTitleSize - gfx::PlatformFont::kDefaultBaseFontSize;
      break;
    case CONTEXT_BODY_TEXT_LARGE:
    case views::style::CONTEXT_MESSAGE_BOX_BODY_TEXT:
      size_delta = kBodyTextLargeSize - gfx::PlatformFont::kDefaultBaseFontSize;
      break;
    case CONTEXT_HEADLINE:
      size_delta = kHeadlineSize - gfx::PlatformFont::kDefaultBaseFontSize;
      break;
    default:
      break;
  }

  // Use a bold style for emphasized text in body contexts, and ignore |style|
  // otherwise.
  if (style == STYLE_EMPHASIZED || style == STYLE_EMPHASIZED_SECONDARY) {
    switch (context) {
      case CONTEXT_BODY_TEXT_SMALL:
      case CONTEXT_BODY_TEXT_LARGE:
      case views::style::CONTEXT_MESSAGE_BOX_BODY_TEXT:
        font_weight = gfx::Font::Weight::BOLD;
        break;

      default:
        break;
    }
  }

  return ui::ResourceBundle::GetSharedInstance().GetFontListWithDelta(
      size_delta, gfx::Font::NORMAL, font_weight);
}

SkColor HarmonyTypographyProvider::GetColor(const views::View& view,
                                            int context,
                                            int style) const {
  const ui::NativeTheme* native_theme = view.GetNativeTheme();
  DCHECK(native_theme);
  if (ShouldIgnoreHarmonySpec(*native_theme)) {
    return GetHarmonyTextColorForNonStandardNativeTheme(context, style,
                                                        *native_theme);
  }

  if (context == views::style::CONTEXT_BUTTON_MD) {
    switch (style) {
      case views::style::STYLE_DIALOG_BUTTON_DEFAULT:
        return SK_ColorWHITE;
      case views::style::STYLE_DISABLED:
        return SkColorSetRGB(0x9e, 0x9e, 0x9e);
      default:
        return SkColorSetRGB(0x75, 0x75, 0x75);
    }
  }

  // Use the secondary style instead of primary for message box body text.
  if (context == views::style::CONTEXT_MESSAGE_BOX_BODY_TEXT &&
      style == views::style::STYLE_PRIMARY) {
    style = STYLE_SECONDARY;
  }

  switch (style) {
    case views::style::STYLE_DIALOG_BUTTON_DEFAULT:
      return SK_ColorWHITE;
    case views::style::STYLE_DISABLED:
      return SkColorSetRGB(0x9e, 0x9e, 0x9e);
    case views::style::STYLE_LINK:
      return gfx::kGoogleBlue700;
    case STYLE_SECONDARY:
    case STYLE_EMPHASIZED_SECONDARY:
    case STYLE_HINT:
      return SkColorSetRGB(0x75, 0x75, 0x75);
    case STYLE_RED:
      return gfx::kGoogleRed700;
    case STYLE_GREEN:
      return gfx::kGoogleGreen700;
  }

  // Use GoogleGrey900 as primary color for everything else.
  return gfx::kGoogleGrey900;
}

int HarmonyTypographyProvider::GetLineHeight(int context, int style) const {
  // "Target" line height constants from the Harmony spec. A default OS
  // configuration should use these heights. However, if the user overrides OS
  // defaults, then GetLineHeight() should return the height that would add the
  // same extra space between lines as the default configuration would have.
  constexpr int kHeadlineHeight = 32;
  constexpr int kTitleHeight = 22;
  constexpr int kBodyHeight = 20;  // For both large and small.

  // Button text should always use the minimum line height for a font to avoid
  // unnecessarily influencing the height of a button.
  constexpr int kButtonAbsoluteHeight = 0;

// The platform-specific heights (i.e. gfx::Font::GetHeight()) that result when
// asking for the target size constants in HarmonyTypographyProvider::GetFont()
// in a default OS configuration.
#if defined(OS_MACOSX)
  constexpr int kHeadlinePlatformHeight = 25;
  constexpr int kTitlePlatformHeight = 19;
  constexpr int kBodyTextLargePlatformHeight = 16;
  constexpr int kBodyTextSmallPlatformHeight = 15;
#elif defined(OS_WIN)
  static const int kHeadlinePlatformHeight =
      GetPlatformFontHeight(CONTEXT_HEADLINE);
  static const int kTitlePlatformHeight =
      GetPlatformFontHeight(views::style::CONTEXT_DIALOG_TITLE);
  static const int kBodyTextLargePlatformHeight =
      GetPlatformFontHeight(CONTEXT_BODY_TEXT_LARGE);
  static const int kBodyTextSmallPlatformHeight =
      GetPlatformFontHeight(CONTEXT_BODY_TEXT_SMALL);
#else
  constexpr int kHeadlinePlatformHeight = 24;
  constexpr int kTitlePlatformHeight = 18;
  constexpr int kBodyTextLargePlatformHeight = 17;
  constexpr int kBodyTextSmallPlatformHeight = 15;
#endif

  // The style of the system font used to determine line heights.
  constexpr int kTemplateStyle = views::style::STYLE_PRIMARY;

  // TODO(tapted): These statics should be cleared out when something invokes
  // ui::ResourceBundle::ReloadFonts(). Currently that only happens on ChromeOS.
  // See http://crbug.com/708943.
  static const int headline_height =
      GetFont(CONTEXT_HEADLINE, kTemplateStyle).GetHeight() -
      kHeadlinePlatformHeight + kHeadlineHeight;
  static const int title_height =
      GetFont(views::style::CONTEXT_DIALOG_TITLE, kTemplateStyle).GetHeight() -
      kTitlePlatformHeight + kTitleHeight;
  static const int body_large_height =
      GetFont(CONTEXT_BODY_TEXT_LARGE, kTemplateStyle).GetHeight() -
      kBodyTextLargePlatformHeight + kBodyHeight;
  static const int default_height =
      GetFont(CONTEXT_BODY_TEXT_SMALL, kTemplateStyle).GetHeight() -
      kBodyTextSmallPlatformHeight + kBodyHeight;

  switch (context) {
    case views::style::CONTEXT_BUTTON:
    case views::style::CONTEXT_BUTTON_MD:
      return kButtonAbsoluteHeight;
    case views::style::CONTEXT_DIALOG_TITLE:
      return title_height;
    case CONTEXT_BODY_TEXT_LARGE:
    case views::style::CONTEXT_MESSAGE_BOX_BODY_TEXT:
    case views::style::CONTEXT_TABLE_ROW:
      return body_large_height;
    case CONTEXT_HEADLINE:
      return headline_height;
    default:
      return default_height;
  }
}
