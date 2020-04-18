// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <set>

#include "base/stl_util.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/global_keyboard_shortcuts_mac.h"
#include "chrome/browser/ui/views/accelerator_table.h"
#include "chrome/test/views/scoped_macviews_browser_mode.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event_constants.h"
#import "ui/events/keycodes/keyboard_code_conversion_mac.h"

namespace {

void VerifyTableDoesntHaveDuplicates(
    const std::vector<KeyboardShortcutData>& table,
    const std::string& table_name) {
  const std::vector<AcceleratorMapping> accelerators(GetAcceleratorList());

  for (const auto& e : table) {
    int modifiers = 0;
    if (e.command_key)
      modifiers |= ui::EF_COMMAND_DOWN;
    if (e.shift_key)
      modifiers |= ui::EF_SHIFT_DOWN;
    if (e.cntrl_key)
      modifiers |= ui::EF_CONTROL_DOWN;
    if (e.opt_key)
      modifiers |= ui::EF_ALT_DOWN;

    for (const auto& accelerator_entry : accelerators) {
      unichar character;
      unichar shifted_character;
      const int vkey_code = ui::MacKeyCodeForWindowsKeyCode(
          accelerator_entry.keycode, accelerator_entry.modifiers,
          &shifted_character, &character);

      EXPECT_FALSE(modifiers == accelerator_entry.modifiers &&
                   e.chrome_command == accelerator_entry.command_id &&
                   (e.vkey_code ? (e.vkey_code == vkey_code)
                                : (e.key_char == character ||
                                   e.key_char == shifted_character)))
          << "Duplicate command: " << accelerator_entry.command_id
          << " in table " << table_name;
    }
  }
}

}  // namespace

// Vefifies that only the whitelisted accelerators could have Control key
// modifier, while running on macOS.
TEST(AcceleratorTableTest, CheckMacOSControlAccelerators) {
  // Only the accelerators that also work in Cocoa browser are allowed to appear
  // on this whitelist.
  const std::set<int> whitelisted_control_shortcuts = {
      IDC_SELECT_NEXT_TAB,
      IDC_SELECT_PREVIOUS_TAB,
      IDC_FULLSCREEN,
  };

  const std::vector<AcceleratorMapping> accelerators(GetAcceleratorList());

  // Control modifier is rarely used on Mac, and all valid uses must be
  // whitelisted.
  for (const auto& entry : accelerators) {
    if (base::ContainsKey(whitelisted_control_shortcuts, entry.command_id))
      continue;
    EXPECT_FALSE(entry.modifiers & ui::EF_CONTROL_DOWN)
        << "Found non-whitelisted accelerator that contains Control "
           "modifier: " << entry.command_id;
  }

  // Test that whitelist is not outdated.
  for (const auto& whitelist_entry : whitelisted_control_shortcuts) {
    const auto entry =
        std::find_if(accelerators.begin(), accelerators.end(),
                     [whitelist_entry](const AcceleratorMapping& a) {
          return a.command_id == whitelist_entry &&
                 (a.modifiers & ui::EF_CONTROL_DOWN) != 0;
        });
    EXPECT_NE(entry, accelerators.end())
        << "Whitelisted accelerator not found in the actual list: "
        << whitelist_entry;
  }
}

// Verifies that Alt-only (or with just Shift) accelerators are not present in
// the list.
TEST(AcceleratorTableTest, CheckMacOSAltAccelerators) {
  const int kNonShiftMask =
      ui::EF_COMMAND_DOWN | ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN;
  for (const auto& entry : GetAcceleratorList()) {
    EXPECT_FALSE((entry.modifiers & kNonShiftMask) == ui::EF_ALT_DOWN)
        << "Found accelerator that uses solely Alt modifier: "
        << entry.command_id;
  }
}

// Verifies that we're not processing any duplicate accelerators in
// global_keyboard_shortcuts_mac.mm functions.
TEST(AcceleratorTableTest, CheckNoDuplicatesGlobalKeyboardShortcutsMac) {
  test::ScopedMacViewsBrowserMode views_mode_{true};
  VerifyTableDoesntHaveDuplicates(GetWindowKeyboardShortcutTable(),
                                  "WindowKeyboardShortcutTable");
  VerifyTableDoesntHaveDuplicates(GetDelayedWindowKeyboardShortcutTable(),
                                  "DelayedWindowKeyboardShortcutTable");
  VerifyTableDoesntHaveDuplicates(GetBrowserKeyboardShortcutTable(),
                                  "BrowserKeyboardShortcutTable");
}
