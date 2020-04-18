// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/public/cpp/gamepad.h"

namespace device {

Gamepad::Gamepad()
    : connected(false),
      timestamp(0),
      axes_length(0),
      buttons_length(0),
      display_id(0) {
  id[0] = 0;
  mapping[0] = 0;
}

Gamepad::Gamepad(const Gamepad& other) = default;

}  // namespace device
