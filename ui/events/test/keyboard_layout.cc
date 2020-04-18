// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/test/keyboard_layout.h"

#include "base/logging.h"

namespace ui {

ScopedKeyboardLayout::ScopedKeyboardLayout(KeyboardLayout layout) {
  original_layout_ = GetActiveLayout();
  ActivateLayout(GetPlatformKeyboardLayout(layout));
}

ScopedKeyboardLayout::~ScopedKeyboardLayout() {
  ActivateLayout(original_layout_);
}

#if !defined(OS_WIN) && !(defined(OS_MACOSX) && !defined(OS_IOS))
PlatformKeyboardLayout ScopedKeyboardLayout::GetActiveLayout() {
  NOTIMPLEMENTED();
  return nullptr;
}

void ScopedKeyboardLayout::ActivateLayout(PlatformKeyboardLayout layout) {
  NOTIMPLEMENTED();
}

PlatformKeyboardLayout GetPlatformKeyboardLayout(KeyboardLayout layout) {
  NOTIMPLEMENTED();
  return nullptr;
}
#endif

}  // namespace ui
