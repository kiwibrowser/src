// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/keyboard_event_counter.h"

#include "base/logging.h"

namespace media {

KeyboardEventCounter::KeyboardEventCounter() : total_key_presses_(0) {}

KeyboardEventCounter::~KeyboardEventCounter() = default;

void KeyboardEventCounter::OnKeyboardEvent(ui::EventType event,
                                           ui::KeyboardCode key_code) {
  // Updates the pressed keys and the total count of key presses.
  if (event == ui::ET_KEY_PRESSED) {
    if (pressed_keys_.find(key_code) != pressed_keys_.end())
      return;
    pressed_keys_.insert(key_code);
    ++total_key_presses_;
  } else {
    DCHECK_EQ(ui::ET_KEY_RELEASED, event);
    pressed_keys_.erase(key_code);
  }
}

uint32_t KeyboardEventCounter::GetKeyPressCount() const {
  return total_key_presses_.load();
}

}  // namespace media
