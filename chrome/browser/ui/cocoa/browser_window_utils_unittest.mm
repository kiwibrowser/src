// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/stringprintf.h"
#import "chrome/browser/ui/cocoa/browser_window_utils.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/events/keycodes/keyboard_codes.h"

using blink::WebInputEvent;

const struct {
  ui::KeyboardCode key_code;
  int modifiers;
  bool is_text_editing_event;
} kTextEditingEventTestCases[] = {
    {ui::VKEY_A, WebInputEvent::kMetaKey, true},
    {ui::VKEY_V, WebInputEvent::kMetaKey, true},
    {ui::VKEY_C, WebInputEvent::kMetaKey, true},
    {ui::VKEY_X, WebInputEvent::kMetaKey, true},
    {ui::VKEY_Z, WebInputEvent::kMetaKey, true},

    {ui::VKEY_A, WebInputEvent::kShiftKey, false},
    {ui::VKEY_G, WebInputEvent::kMetaKey, false},
};

TEST(BrowserWindowUtilsTest, TestIsTextEditingEvent) {
  content::NativeWebKeyboardEvent event(
      WebInputEvent::kChar, WebInputEvent::kNoModifiers,
      WebInputEvent::GetStaticTimeStampForTests());
  EXPECT_FALSE([BrowserWindowUtils isTextEditingEvent:event]);

  for (const auto& test : kTextEditingEventTestCases) {
    SCOPED_TRACE(base::StringPrintf("key = %c, modifiers = %d",
                 test.key_code, test.modifiers));
    content::NativeWebKeyboardEvent event(
        WebInputEvent::kChar, test.modifiers,
        WebInputEvent::GetStaticTimeStampForTests());
    event.windows_key_code = test.key_code;
    EXPECT_EQ(test.is_text_editing_event,
              [BrowserWindowUtils isTextEditingEvent:event]);
  }
}
