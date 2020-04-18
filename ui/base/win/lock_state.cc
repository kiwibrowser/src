// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/win/lock_state.h"

#include <windows.h>

namespace ui {

bool IsWorkstationLocked() {
  bool is_locked = true;
  HDESK input_desk = ::OpenInputDesktop(0, 0, GENERIC_READ);
  if (input_desk) {
    wchar_t name[256] = {0};
    DWORD needed = 0;
    if (::GetUserObjectInformation(
            input_desk, UOI_NAME, name, sizeof(name), &needed)) {
      is_locked = lstrcmpi(name, L"default") != 0;
    }
    ::CloseDesktop(input_desk);
  }
  return is_locked;
}

}  // namespace ui
