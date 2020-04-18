// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_TABLET_MODE_SCOPED_DISABLE_INTERNAL_MOUSE_AND_KEYBOARD_H_
#define ASH_WM_TABLET_MODE_SCOPED_DISABLE_INTERNAL_MOUSE_AND_KEYBOARD_H_

namespace ash {

class ScopedDisableInternalMouseAndKeyboard {
 public:
  virtual ~ScopedDisableInternalMouseAndKeyboard() {}
};

}  // namespace ash

#endif  // ASH_WM_TABLET_MODE_SCOPED_DISABLE_INTERNAL_MOUSE_AND_KEYBOARD_H_
