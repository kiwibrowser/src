// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/harmony/chrome_typography.h"

#include "build/build_config.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "ui/base/default_style.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_features.h"
#include "ui/gfx/platform_font.h"

// Mac doesn't use LocationBarView (yet).
#if !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)

int GetFontSizeDeltaBoundedByAvailableHeight(int available_height,
                                             int desired_font_size) {
  int size_delta = desired_font_size - gfx::PlatformFont::kDefaultBaseFontSize;
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
  gfx::FontList base_font = bundle.GetFontListWithDelta(size_delta);

  // The ResourceBundle's default font may not actually be kDefaultBaseFontSize
  // if, for example, the user has changed their system font sizes or the
  // current locale has been overridden to use a different default font size.
  // Adjust for the difference in default font sizes.
  int user_or_locale_delta = 0;
  if (base_font.GetFontSize() != desired_font_size) {
    user_or_locale_delta = desired_font_size - base_font.GetFontSize();
    base_font = bundle.GetFontListWithDelta(size_delta + user_or_locale_delta);
  }
  DCHECK_EQ(desired_font_size, base_font.GetFontSize());

  // Shrink large fonts to ensure they fit. Default fonts should fit already.
  // TODO(tapted): Move DeriveWithHeightUpperBound() to ui::ResourceBundle to
  // take advantage of the font cache.
  base_font = base_font.DeriveWithHeightUpperBound(available_height);

  // To ensure a subsequent request from the ResourceBundle ignores the delta
  // due to user or locale settings, include it here.
  return base_font.GetFontSize() - gfx::PlatformFont::kDefaultBaseFontSize +
         user_or_locale_delta;
}

#endif  // OS_MACOSX || MAC_VIEWS_BROWSER

void ApplyCommonFontStyles(int context,
                           int style,
                           int* size_delta,
                           gfx::Font::Weight* weight) {
  switch (context) {
#if !defined(OS_MACOSX) || BUILDFLAG(MAC_VIEWS_BROWSER)
    case CONTEXT_OMNIBOX_PRIMARY: {
      constexpr int kDesiredFontSizeRegular = 14;
      constexpr int kDesiredFontSizeTouchable = 15;
      static const int omnibox_primary_delta =
          GetFontSizeDeltaBoundedByAvailableHeight(
              LocationBarView::GetAvailableTextHeight(),
              ui::MaterialDesignController::IsTouchOptimizedUiEnabled()
                  ? kDesiredFontSizeTouchable
                  : kDesiredFontSizeRegular);
      *size_delta = omnibox_primary_delta;
      break;
    }
    case CONTEXT_OMNIBOX_DECORATION: {
      // Use 11 for both touchable and non-touchable. The touchable spec
      // specifies 11 explicitly. Historically, non-touchable would take the
      // primary omnibox font and incrementally reduce its size until it fit.
      // In default configurations, it would obtain 11. Deriving fonts is slow,
      // so don't bother starting at 14.
      constexpr int kDesiredFontSizeDecoration = 11;
      static const int omnibox_decoration_delta =
          GetFontSizeDeltaBoundedByAvailableHeight(
              LocationBarView::GetAvailableDecorationTextHeight(),
              kDesiredFontSizeDecoration);
      *size_delta = omnibox_decoration_delta;
      break;
    }
#endif  // !OS_MACOSX || MAC_VIEWS_BROWSER
#if defined(OS_WIN)
    case CONTEXT_WINDOWS10_NATIVE:
      // Adjusts default font size up to match Win10 modern UI.
      *size_delta = 15 - gfx::PlatformFont::kDefaultBaseFontSize;
      break;
#endif
  }
}

const gfx::FontList& LegacyTypographyProvider::GetFont(int context,
                                                       int style) const {
  constexpr int kHeadlineDelta = 8;
  constexpr int kDialogMessageDelta = 1;

  int size_delta;
  gfx::Font::Weight font_weight;
  GetDefaultFont(context, style, &size_delta, &font_weight);

#if defined(OS_CHROMEOS)
  ash::ApplyAshFontStyles(context, style, &size_delta, &font_weight);
#endif

  ApplyCommonFontStyles(context, style, &size_delta, &font_weight);

  switch (context) {
    case CONTEXT_HEADLINE:
      size_delta = kHeadlineDelta;
      break;
    case CONTEXT_BODY_TEXT_LARGE:
      // Note: Not using ui::kMessageFontSizeDelta, so 13pt in most cases.
      size_delta = kDialogMessageDelta;
      break;
    case CONTEXT_BODY_TEXT_SMALL:
      size_delta = ui::kLabelFontSizeDelta;
      break;
    case CONTEXT_DEPRECATED_SMALL:
      size_delta = ui::ResourceBundle::kSmallFontDelta;
      break;
  }

  switch (style) {
    case STYLE_EMPHASIZED:
    case STYLE_EMPHASIZED_SECONDARY:
      font_weight = gfx::Font::Weight::BOLD;
      break;
  }
  constexpr gfx::Font::FontStyle kFontStyle = gfx::Font::NORMAL;
  return ui::ResourceBundle::GetSharedInstance().GetFontListWithDelta(
      size_delta, kFontStyle, font_weight);
}

SkColor LegacyTypographyProvider::GetColor(const views::View& view,
                                           int context,
                                           int style) const {
  // Use "disabled grey" for HINT and SECONDARY when Harmony is disabled.
  if (style == STYLE_HINT || style == STYLE_SECONDARY ||
      style == STYLE_EMPHASIZED_SECONDARY) {
    style = views::style::STYLE_DISABLED;
  }

  return DefaultTypographyProvider::GetColor(view, context, style);
}
