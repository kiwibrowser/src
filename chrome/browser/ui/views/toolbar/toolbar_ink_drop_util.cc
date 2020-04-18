// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"

#include "ui/gfx/color_palette.h"

SkColor GetToolbarInkDropBaseColor(const views::View* host_view) {
  const auto* theme_provider = host_view->GetThemeProvider();
  // There may be no theme provider in unit tests.
  if (theme_provider) {
    return color_utils::BlendTowardOppositeLuma(
        theme_provider->GetColor(ThemeProperties::COLOR_TOOLBAR),
        SK_AlphaOPAQUE);
  }

  return gfx::kPlaceholderColor;
}
