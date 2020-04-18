// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/ash_layout_constants.h"

#include "base/logging.h"
#include "ui/base/material_design/material_design_controller.h"

namespace ash {

gfx::Size GetAshLayoutSize(AshLayoutSize size) {
  constexpr int kButtonWidth = 32;
  const int mode = ui::MaterialDesignController::GetMode();
  switch (size) {
    case AshLayoutSize::kBrowserCaptionMaximized: {
      constexpr int kBrowserMaximizedCaptionButtonHeight[] = {29, 33, 41, 29,
                                                              41};
      return gfx::Size(kButtonWidth,
                       kBrowserMaximizedCaptionButtonHeight[mode]);
    }
    case AshLayoutSize::kBrowserCaptionRestored: {
      constexpr int kBrowserRestoredCaptionButtonHeight[] = {36, 40, 48, 36,
                                                             48};
      return gfx::Size(kButtonWidth, kBrowserRestoredCaptionButtonHeight[mode]);
    }
    case AshLayoutSize::kNonBrowserCaption:
      return gfx::Size(kButtonWidth, 32);
  }

  NOTREACHED();
  return gfx::Size();
}

}  // namespace ash
