// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APP_LIST_SEARCH_SETTINGS_SHORTCUT_SETTINGS_SHORTCUT_METADATA_H_
#define CHROME_BROWSER_UI_APP_LIST_SEARCH_SETTINGS_SHORTCUT_SETTINGS_SHORTCUT_METADATA_H_

#include <vector>

namespace gfx {
struct VectorIcon;
}

namespace app_list {

// Metadata for Settings shortcut.
// TODO(wutao): Add UI controls, e.g. toggle, slider, to change the settings.
struct SettingsShortcut {
  const char* shortcut_id;

  int name_string_resource_id = 0;

  const gfx::VectorIcon& vector_icon;

  // Settings subpage, e.g. bluetooth, network etc.
  const char* subpage;

  // The string used for search query in addition to the name.
  int searchable_string_resource_id = 0;
};

// Returns a list of Settings shortcuts, which are searchable in launcher.
const std::vector<SettingsShortcut>& GetSettingsShortcutList();

}  // namespace app_list

#endif  // CHROME_BROWSER_UI_APP_LIST_SEARCH_SETTINGS_SHORTCUT_SETTINGS_SHORTCUT_METADATA_H_
