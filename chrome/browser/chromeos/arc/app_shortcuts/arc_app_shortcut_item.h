// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_APP_SHORTCUTS_ARC_APP_SHORTCUT_ITEM_H_
#define CHROME_BROWSER_CHROMEOS_ARC_APP_SHORTCUTS_ARC_APP_SHORTCUT_ITEM_H_

#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "ui/gfx/image/image_skia.h"

namespace arc {

// Describes app shortcut that is published by Android's ShortcutManager.
struct ArcAppShortcutItem {
  // The ID of this shortcut. Unique within each publisher app and stable across
  // devices.
  std::string shortcut_id;

  // The short description of this shortcut.
  base::string16 short_label;

  // The icon for this shortcut.
  gfx::ImageSkia icon;
};

using ArcAppShortcutItems = std::vector<ArcAppShortcutItem>;

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_APP_SHORTCUTS_ARC_APP_SHORTCUT_ITEM_H_
