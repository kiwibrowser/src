// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <string>

#include "ash/events/keyboard_driven_event_rewriter.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/event.h"

namespace ash {

class KeyboardDrivenEventRewriterTest : public testing::Test {
 public:
  KeyboardDrivenEventRewriterTest() {}

  ~KeyboardDrivenEventRewriterTest() override {}

 protected:
  std::string GetRewrittenEventAsString(ui::KeyboardCode ui_keycode,
                                        int ui_flags,
                                        ui::EventType ui_type) {
    ui::KeyEvent keyevent(ui_type, ui_keycode, ui_flags);
    std::unique_ptr<ui::Event> rewritten_event;
    ui::EventRewriteStatus status =
        rewriter_.RewriteForTesting(keyevent, &rewritten_event);
    return base::StringPrintf(
        "ui_flags=%d status=%d",
        rewritten_event ? rewritten_event->flags() : keyevent.flags(),
        status);
  }

  std::string GetExpectedResultAsString(int ui_flags,
                                        ui::EventRewriteStatus status) {
    return base::StringPrintf("ui_flags=%d status=%u", ui_flags, status);
  }

  KeyboardDrivenEventRewriter rewriter_;

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardDrivenEventRewriterTest);
};

TEST_F(KeyboardDrivenEventRewriterTest, PassThrough) {
  struct {
    ui::KeyboardCode ui_keycode;
    int ui_flags;
  } kTests[] = {
    { ui::VKEY_A, ui::EF_NONE },
    { ui::VKEY_A, ui::EF_CONTROL_DOWN },
    { ui::VKEY_A, ui::EF_ALT_DOWN },
    { ui::VKEY_A, ui::EF_SHIFT_DOWN },
    { ui::VKEY_A, ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN },
    { ui::VKEY_A, ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN | ui::EF_SHIFT_DOWN },

    { ui::VKEY_LEFT, ui::EF_NONE },
    { ui::VKEY_LEFT, ui::EF_CONTROL_DOWN },
    { ui::VKEY_LEFT, ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN },

    { ui::VKEY_RIGHT, ui::EF_NONE },
    { ui::VKEY_RIGHT, ui::EF_CONTROL_DOWN },
    { ui::VKEY_RIGHT, ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN },

    { ui::VKEY_UP, ui::EF_NONE },
    { ui::VKEY_UP, ui::EF_CONTROL_DOWN },
    { ui::VKEY_UP, ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN },

    { ui::VKEY_DOWN, ui::EF_NONE },
    { ui::VKEY_DOWN, ui::EF_CONTROL_DOWN },
    { ui::VKEY_DOWN, ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN },

    { ui::VKEY_RETURN, ui::EF_NONE },
    { ui::VKEY_RETURN, ui::EF_CONTROL_DOWN },
    { ui::VKEY_RETURN, ui::EF_CONTROL_DOWN | ui::EF_ALT_DOWN },
  };

  for (size_t i = 0; i < arraysize(kTests); ++i) {
    EXPECT_EQ(GetExpectedResultAsString(kTests[i].ui_flags,
                                        ui::EVENT_REWRITE_CONTINUE),
              GetRewrittenEventAsString(kTests[i].ui_keycode,
                                        kTests[i].ui_flags,
                                        ui::ET_KEY_PRESSED))
    << "Test case " << i;
  }
}

TEST_F(KeyboardDrivenEventRewriterTest, Rewrite) {
  const int kModifierMask = ui::EF_SHIFT_DOWN;

  struct {
    ui::KeyboardCode ui_keycode;
    int ui_flags;
  } kTests[] = {
    { ui::VKEY_LEFT, kModifierMask },
    { ui::VKEY_RIGHT, kModifierMask },
    { ui::VKEY_UP, kModifierMask },
    { ui::VKEY_DOWN, kModifierMask },
    { ui::VKEY_RETURN, kModifierMask },
    { ui::VKEY_F6, kModifierMask },
  };

  for (size_t i = 0; i < arraysize(kTests); ++i) {
    EXPECT_EQ(GetExpectedResultAsString(ui::EF_NONE,
                                        ui::EVENT_REWRITE_REWRITTEN),
              GetRewrittenEventAsString(kTests[i].ui_keycode,
                                        kTests[i].ui_flags,
                                        ui::ET_KEY_PRESSED))
    << "Test case " << i;
  }
}

}  // namespace ash
