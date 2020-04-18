// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/palette/tools/capture_screen_action.h"

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/palette/palette_ids.h"
#include "ash/utility/screenshot_controller.h"
#include "ui/base/l10n/l10n_util.h"

namespace ash {

CaptureScreenAction::CaptureScreenAction(Delegate* delegate)
    : CommonPaletteTool(delegate) {}

CaptureScreenAction::~CaptureScreenAction() = default;

PaletteGroup CaptureScreenAction::GetGroup() const {
  return PaletteGroup::ACTION;
}

PaletteToolId CaptureScreenAction::GetToolId() const {
  return PaletteToolId::CAPTURE_SCREEN;
}

void CaptureScreenAction::OnEnable() {
  CommonPaletteTool::OnEnable();

  delegate()->DisableTool(GetToolId());
  delegate()->HidePaletteImmediately();

  Shell::Get()->screenshot_controller()->TakeScreenshotForAllRootWindows();
}

views::View* CaptureScreenAction::CreateView() {
  return CreateDefaultView(
      l10n_util::GetStringUTF16(IDS_ASH_STYLUS_TOOLS_CAPTURE_SCREEN_ACTION));
}

const gfx::VectorIcon& CaptureScreenAction::GetPaletteIcon() const {
  return kPaletteActionCaptureScreenIcon;
}

}  // namespace ash
