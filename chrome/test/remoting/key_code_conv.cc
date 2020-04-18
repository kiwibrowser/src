// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/remoting/key_code_conv.h"

#include <stddef.h>

#include "base/macros.h"
#include "chrome/test/remoting/key_code_map.h"

namespace remoting {

ui::KeyboardCode InvalidKeyboardCode() {
  return key_code_map[0].vkey_code;
}

void GetKeyValuesFromChar(
    char c, const char** code, ui::KeyboardCode* vkey_code, bool* shift) {
  *code = NULL;
  *vkey_code = InvalidKeyboardCode();

  for (size_t i = 0; i < arraysize(key_code_map); ++i) {
    if (key_code_map[i].lower_char == c) {
      *code = key_code_map[i].code;
      *vkey_code = key_code_map[i].vkey_code;
      *shift = false;
      return;
    }

    if (key_code_map[i].upper_char == c) {
      *code = key_code_map[i].code;
      *vkey_code = key_code_map[i].vkey_code;
      *shift = true;
    }
  }
}

}  // namespace remoting
