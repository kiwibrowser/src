// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/style/typography_provider.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "ui/base/default_style.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/style/typography.h"
#include "ui/views/view.h"

#if defined(OS_MACOSX)
#include "base/mac/mac_util.h"
#endif

namespace views {
namespace {

gfx::Font::Weight GetValueBolderThan(gfx::Font::Weight weight) {
  switch (weight) {
    case gfx::Font::Weight::BOLD:
      return gfx::Font::Weight::EXTRA_BOLD;
    case gfx::Font::Weight::EXTRA_BOLD:
    case gfx::Font::Weight::BLACK:
      return gfx::Font::Weight::BLACK;
    default:
      return gfx::Font::Weight::BOLD;
  }
}

}  // namespace

// static
gfx::Font::Weight TypographyProvider::MediumWeightForUI() {
#if defined(OS_MACOSX)
  // System fonts are not user-configurable on Mac, so there's a simpler check.
  // However, 10.11 do not ship with a MEDIUM weight system font. In that
  // case, trying to use MEDIUM there will give a bold font, which will look
  // worse with the surrounding NORMAL text than just using NORMAL.
  return base::mac::IsOS10_11() ? gfx::Font::Weight::NORMAL
                                : gfx::Font::Weight::MEDIUM;
#else
  // NORMAL may already have at least MEDIUM weight. Return NORMAL in that case
  // since trying to return MEDIUM would actually make the font lighter-weight
  // than the surrounding text. For example, Windows can be configured to use a
  // BOLD font for dialog text; deriving MEDIUM from that would replace the BOLD
  // attribute with something lighter.
  if (ui::ResourceBundle::GetSharedInstance()
          .GetFontListWithDelta(0, gfx::Font::NORMAL, gfx::Font::Weight::NORMAL)
          .GetFontWeight() < gfx::Font::Weight::MEDIUM)
    return gfx::Font::Weight::MEDIUM;
  return gfx::Font::Weight::NORMAL;
#endif
}

const gfx::FontList& DefaultTypographyProvider::GetFont(int context,
                                                        int style) const {
  int size_delta;
  gfx::Font::Weight font_weight;
  GetDefaultFont(context, style, &size_delta, &font_weight);
  return ui::ResourceBundle::GetSharedInstance().GetFontListWithDelta(
      size_delta, gfx::Font::NORMAL, font_weight);
}

SkColor DefaultTypographyProvider::GetColor(const views::View& view,
                                            int context,
                                            int style) const {
  ui::NativeTheme::ColorId color_id =
      ui::NativeTheme::kColorId_LabelEnabledColor;
  if (context == style::CONTEXT_BUTTON_MD) {
    switch (style) {
      case views::style::STYLE_DIALOG_BUTTON_DEFAULT:
        color_id = ui::NativeTheme::kColorId_TextOnProminentButtonColor;
        break;
      case views::style::STYLE_DISABLED:
        color_id = ui::NativeTheme::kColorId_ButtonDisabledColor;
        break;
      default:
        color_id = ui::NativeTheme::kColorId_ButtonEnabledColor;
        break;
    }
  } else if (context == style::CONTEXT_TEXTFIELD) {
    color_id = style == style::STYLE_DISABLED
                   ? ui::NativeTheme::kColorId_TextfieldReadOnlyColor
                   : ui::NativeTheme::kColorId_TextfieldDefaultColor;
  } else if (style == style::STYLE_DISABLED) {
    color_id = ui::NativeTheme::kColorId_LabelDisabledColor;
  }

  const ui::NativeTheme* native_theme = view.GetNativeTheme();
  DCHECK(native_theme);
  return native_theme->GetSystemColor(color_id);
}

int DefaultTypographyProvider::GetLineHeight(int context, int style) const {
  return 0;
}

// static
void DefaultTypographyProvider::GetDefaultFont(int context,
                                               int style,
                                               int* size_delta,
                                               gfx::Font::Weight* font_weight) {
  *font_weight = gfx::Font::Weight::NORMAL;

  switch (context) {
    case style::CONTEXT_BUTTON_MD:
      *size_delta = ui::kLabelFontSizeDelta;
      *font_weight = MediumWeightForUI();
      break;
    case style::CONTEXT_DIALOG_TITLE:
      *size_delta = ui::kTitleFontSizeDelta;
      break;
    case style::CONTEXT_TOUCH_MENU:
      *size_delta = -1;
      break;
    default:
      *size_delta = ui::kLabelFontSizeDelta;
      break;
  }

  switch (style) {
    case style::STYLE_TAB_ACTIVE:
      *font_weight = gfx::Font::Weight::BOLD;
      break;
    case style::STYLE_DIALOG_BUTTON_DEFAULT:
      // Only non-MD default buttons should "increase" in boldness.
      if (context == style::CONTEXT_BUTTON) {
        *font_weight = GetValueBolderThan(
            ui::ResourceBundle::GetSharedInstance()
                .GetFontListWithDelta(*size_delta, gfx::Font::NORMAL,
                                      *font_weight)
                .GetFontWeight());
      }
      break;
  }
}

}  // namespace views
