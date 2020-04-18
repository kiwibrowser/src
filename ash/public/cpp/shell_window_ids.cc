// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/shell_window_ids.h"

#include "base/macros.h"

namespace ash {

// NOTE: this list is ordered by activation order. That is, windows in
// containers appearing earlier in the list are activated before windows in
// containers appearing later in the list.
const int32_t kActivatableShellWindowIds[] = {
    kShellWindowId_OverlayContainer, kShellWindowId_LockSystemModalContainer,
    kShellWindowId_AccessibilityPanelContainer,
    kShellWindowId_SettingBubbleContainer, kShellWindowId_PowerMenuContainer,
    kShellWindowId_LockActionHandlerContainer,
    kShellWindowId_LockScreenContainer, kShellWindowId_SystemModalContainer,
    kShellWindowId_AlwaysOnTopContainer, kShellWindowId_AppListContainer,
    kShellWindowId_DefaultContainer, kShellWindowId_AppListTabletModeContainer,

    // Panel, launcher and status are intentionally checked after other
    // containers even though these layers are higher. The user expects their
    // windows to be focused before these elements.
    kShellWindowId_PanelContainer, kShellWindowId_ShelfContainer,
    kShellWindowId_StatusContainer,
};

const size_t kNumActivatableShellWindowIds =
    arraysize(kActivatableShellWindowIds);

bool IsActivatableShellWindowId(int32_t id) {
  for (size_t i = 0; i < kNumActivatableShellWindowIds; i++) {
    if (id == kActivatableShellWindowIds[i])
      return true;
  }
  return false;
}

}  // namespace ash
