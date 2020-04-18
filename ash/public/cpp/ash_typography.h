// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_ASH_TYPOGRAPHY_H_
#define ASH_PUBLIC_CPP_ASH_TYPOGRAPHY_H_

#include "ash/public/cpp/ash_public_export.h"
#include "ui/gfx/font.h"
#include "ui/views/style/typography.h"

namespace ash {

enum AshTextContext {
  ASH_TEXT_CONTEXT_START = views::style::VIEWS_TEXT_CONTEXT_END,

  // A button that appears in the launcher's status area.
  CONTEXT_LAUNCHER_BUTTON = ASH_TEXT_CONTEXT_START,

  // Buttons and labels that appear in the fullscreen toast overlay UI.
  CONTEXT_TOAST_OVERLAY,

  // A button that appears within a row of the tray popup.
  CONTEXT_TRAY_POPUP_BUTTON,

  ASH_TEXT_CONTEXT_END
};

// Sets the |size_delta| and |font_weight| for ash-specific text contexts.
// Values are only set for contexts specific to ash.
void ASH_PUBLIC_EXPORT ApplyAshFontStyles(int context,
                                          int style,
                                          int* size_delta,
                                          gfx::Font::Weight* font_weight);

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_ASH_TYPOGRAPHY_H_
