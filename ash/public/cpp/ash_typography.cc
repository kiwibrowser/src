// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/ash_typography.h"

namespace ash {

void ApplyAshFontStyles(int context,
                        int style,
                        int* size_delta,
                        gfx::Font::Weight* font_weight) {
  switch (context) {
    case CONTEXT_LAUNCHER_BUTTON:
      *size_delta = 2;
      break;
    case CONTEXT_TOAST_OVERLAY:
      *size_delta = 3;
      break;
    case CONTEXT_TRAY_POPUP_BUTTON:
      *font_weight = gfx::Font::Weight::MEDIUM;
      break;
  }
}

}  // namespace ash
