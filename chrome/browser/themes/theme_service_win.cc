// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/themes/theme_service_win.h"

#include "base/bind.h"
#include "base/win/windows_version.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/win/titlebar_config.h"
#include "chrome/grit/theme_resources.h"
#include "skia/ext/skia_utils_win.h"
#include "ui/base/win/shell.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/geometry/safe_integer_conversions.h"

ThemeServiceWin::ThemeServiceWin() {
  // This just checks for Windows 10 instead of calling DwmColorsAllowed()
  // because we want to monitor the frame color even when a custom frame is in
  // use, so that it will be correct if at any time the user switches to the
  // native frame.
  if (base::win::GetVersion() >= base::win::VERSION_WIN10) {
    dwm_key_.reset(new base::win::RegKey(
        HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\DWM", KEY_READ));
    if (dwm_key_->Valid())
      OnDwmKeyUpdated();
    else
      dwm_key_.reset();
  }
}

ThemeServiceWin::~ThemeServiceWin() {
}

bool ThemeServiceWin::ShouldUseNativeFrame() const {
  const bool use_native_frame_if_enabled =
      ShouldCustomDrawSystemTitlebar() || !HasCustomImage(IDR_THEME_FRAME);
  return use_native_frame_if_enabled && ui::win::IsAeroGlassEnabled();
}

SkColor ThemeServiceWin::GetDefaultColor(int id, bool incognito) const {
  if (DwmColorsAllowed()) {
    if (id == ThemeProperties::COLOR_ACCENT_BORDER)
      return dwm_accent_border_color_;

    // When we're custom-drawing the titlebar we want to use either the colors
    // we calculated in OnDwmKeyUpdated() or the default colors. When we're not
    // custom-drawing the titlebar we want to match the color Windows actually
    // uses because some things (like the incognito icon) use this color to
    // decide whether they should draw in light or dark mode. Incognito colors
    // should be the same as non-incognito in all cases here.
    if (id == ThemeProperties::COLOR_FRAME) {
      if (dwm_frame_color_)
        return dwm_frame_color_.value();
      if (!ShouldCustomDrawSystemTitlebar())
        return SK_ColorWHITE;
      // Fall through and use default.
    }
    if (id == ThemeProperties::COLOR_FRAME_INACTIVE) {
      if (!ShouldCustomDrawSystemTitlebar()) {
        return inactive_frame_color_from_registry_
                   ? dwm_inactive_frame_color_.value()
                   : SK_ColorWHITE;
      }
      if (dwm_inactive_frame_color_)
        return dwm_inactive_frame_color_.value();
      // Fall through and use default.
    }
  }

  return ThemeService::GetDefaultColor(id, incognito);
}

bool ThemeServiceWin::DwmColorsAllowed() const {
  return ShouldUseNativeFrame() &&
         (base::win::GetVersion() >= base::win::VERSION_WIN10);
}

void ThemeServiceWin::OnDwmKeyUpdated() {
  DWORD accent_color, color_prevalence;
  bool use_dwm_frame_color =
      dwm_key_->ReadValueDW(L"AccentColor", &accent_color) == ERROR_SUCCESS &&
      dwm_key_->ReadValueDW(L"ColorPrevalence", &color_prevalence) ==
          ERROR_SUCCESS &&
      color_prevalence == 1;
  inactive_frame_color_from_registry_ = false;
  if (use_dwm_frame_color) {
    dwm_frame_color_ = skia::COLORREFToSkColor(accent_color);
    DWORD accent_color_inactive;
    if (dwm_key_->ReadValueDW(L"AccentColorInactive", &accent_color_inactive) ==
        ERROR_SUCCESS) {
      dwm_inactive_frame_color_ =
          skia::COLORREFToSkColor(accent_color_inactive);
      inactive_frame_color_from_registry_ = true;
    } else {
      // Tint to create inactive color. Always use the non-incognito version of
      // the tint, since the frame should look the same in both modes.
      dwm_inactive_frame_color_ = color_utils::HSLShift(
          dwm_frame_color_.value(),
          GetTint(ThemeProperties::TINT_FRAME_INACTIVE, false));
    }
  } else {
    dwm_frame_color_.reset();
    dwm_inactive_frame_color_.reset();
  }

  dwm_accent_border_color_ = SK_ColorWHITE;
  DWORD colorization_color, colorization_color_balance;
  if ((dwm_key_->ReadValueDW(L"ColorizationColor", &colorization_color) ==
       ERROR_SUCCESS) &&
      (dwm_key_->ReadValueDW(L"ColorizationColorBalance",
                             &colorization_color_balance) == ERROR_SUCCESS)) {
    // The accent border color is a linear blend between the colorization
    // color and the neutral #d9d9d9. colorization_color_balance is the
    // percentage of the colorization color in that blend.
    //
    // On Windows version 1611 colorization_color_balance can be 0xfffffff3 if
    // the accent color is taken from the background and either the background
    // is a solid color or was just changed to a slideshow. It's unclear what
    // that value's supposed to mean, so change it to 80 to match Edge's
    // behavior.
    if (colorization_color_balance > 100)
      colorization_color_balance = 80;

    // colorization_color's high byte is not an alpha value, so replace it
    // with 0xff to make an opaque ARGB color.
    SkColor input_color = SkColorSetA(colorization_color, 0xff);

    dwm_accent_border_color_ = color_utils::AlphaBlend(
        input_color, SkColorSetRGB(0xd9, 0xd9, 0xd9),
        gfx::ToRoundedInt(255 * colorization_color_balance / 100.f));
  }

  // Watch for future changes.
  if (!dwm_key_->StartWatching(base::Bind(
          &ThemeServiceWin::OnDwmKeyUpdated, base::Unretained(this))))
    dwm_key_.reset();
}
