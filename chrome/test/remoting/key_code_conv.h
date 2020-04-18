// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_REMOTING_KEY_CODE_CONV_H_
#define CHROME_TEST_REMOTING_KEY_CODE_CONV_H_

#include "ui/events/keycodes/keyboard_codes.h"

namespace remoting {

// Find out the key(s) that need to be pressed to type the desired character.
// The information can be used to simulate a key press event.
// The information returned includes:
//   1. The UIEvents (aka: DOM4Events) |code| value as defined in:
//      http://www.w3.org/TR/uievents/
//   2. The virtual key code (ui::KeyboardCode)
//   3. The shift state.
// This function assumes US keyboard layout.
void GetKeyValuesFromChar(
    char c, const char** code, ui::KeyboardCode* vkey_code, bool* shift);

}  // namespace remoting

#endif  // CHROME_TEST_REMOTING_KEY_CODE_CONV_H_
