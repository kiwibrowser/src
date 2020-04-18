// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/content/keyboard_overlay/keyboard_overlay_view.h"

#include <algorithm>

#include "ash/content/keyboard_overlay/keyboard_overlay_delegate.h"
#include "ash/content/shell_content_state.h"
#include "ash/public/cpp/accelerators.h"
#include "ash/test/ash_test_base.h"
#include "base/stl_util.h"
#include "ui/web_dialogs/test/test_web_contents_handler.h"
#include "ui/web_dialogs/test/test_web_dialog_delegate.h"

namespace ash {

using KeyboardOverlayViewTest = AshTestBase;

bool operator==(const KeyboardOverlayView::KeyEventData& lhs,
                const KeyboardOverlayView::KeyEventData& rhs) {
  return (lhs.key_code == rhs.key_code) && (lhs.flags == rhs.flags);
}

// Verifies that the accelerators that open the keyboard overlay close it.
TEST_F(KeyboardOverlayViewTest, OpenAcceleratorsClose) {
  ui::test::TestWebDialogDelegate delegate(GURL("chrome://keyboardoverlay"));
  KeyboardOverlayView view(
      ShellContentState::GetInstance()->GetActiveBrowserContext(), &delegate,
      new ui::test::TestWebContentsHandler);
  for (size_t i = 0; i < kAcceleratorDataLength; ++i) {
    if (kAcceleratorData[i].action != SHOW_KEYBOARD_OVERLAY)
      continue;
    const AcceleratorData& open_key_data = kAcceleratorData[i];
    ui::KeyEvent open_key(open_key_data.trigger_on_press ? ui::ET_KEY_PRESSED
                                                         : ui::ET_KEY_RELEASED,
                          open_key_data.keycode, open_key_data.modifiers);
    EXPECT_TRUE(view.IsCancelingKeyEvent(&open_key));
  }
}

// Test modifiers that might exist in a KeyEvent but they shouldn't be
// considered in an accelerator comparison to determine if a KeyEvent is a
// canceling key.
TEST_F(KeyboardOverlayViewTest, TestCancelingKeysWithNonModifierFlags) {
  ui::test::TestWebDialogDelegate delegate(GURL("chrome://keyboardoverlay"));
  KeyboardOverlayView view(
      ShellContentState::GetInstance()->GetActiveBrowserContext(), &delegate,
      new ui::test::TestWebContentsHandler);

  const int kNonModifierFlags = ui::EF_IS_SYNTHESIZED | ui::EF_NUM_LOCK_ON |
                                ui::EF_IME_FABRICATED_KEY | ui::EF_IS_REPEAT;

  std::vector<KeyboardOverlayView::KeyEventData> canceling_keys;
  KeyboardOverlayView::GetCancelingKeysForTesting(&canceling_keys);
  for (const auto& key_data : canceling_keys) {
    ui::KeyEvent key_event(ui::ET_KEY_PRESSED, key_data.key_code,
                           key_data.flags | kNonModifierFlags);
    EXPECT_TRUE(view.IsCancelingKeyEvent(&key_event));
  }
}

// Verifies that there are no redunduant keys in the canceling keys.
TEST_F(KeyboardOverlayViewTest, NoRedundantCancelingKeys) {
  std::vector<KeyboardOverlayView::KeyEventData> open_keys;
  for (size_t i = 0; i < kAcceleratorDataLength; ++i) {
    if (kAcceleratorData[i].action != SHOW_KEYBOARD_OVERLAY)
      continue;
    // Escape is used just for canceling.
    KeyboardOverlayView::KeyEventData open_key = {
        kAcceleratorData[i].keycode, kAcceleratorData[i].modifiers,
    };
    open_keys.push_back(open_key);
  }

  std::vector<KeyboardOverlayView::KeyEventData> canceling_keys;
  KeyboardOverlayView::GetCancelingKeysForTesting(&canceling_keys);

  // Escape is used just for canceling, so exclude it from the comparison with
  // open keys.
  KeyboardOverlayView::KeyEventData escape = {ui::VKEY_ESCAPE, ui::EF_NONE};
  std::vector<KeyboardOverlayView::KeyEventData>::iterator escape_itr =
      std::find(canceling_keys.begin(), canceling_keys.end(), escape);
  canceling_keys.erase(escape_itr);

  // Other canceling keys should be same as opening keys.
  EXPECT_EQ(open_keys.size(), canceling_keys.size());
  for (size_t i = 0; i < canceling_keys.size(); ++i)
    EXPECT_TRUE(base::ContainsValue(open_keys, canceling_keys[i]));
}

}  // namespace ash
