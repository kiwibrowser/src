// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/scoped_root_window_for_new_windows.h"

#include "ash/shell.h"
#include "base/logging.h"

namespace ash {

ScopedRootWindowForNewWindows::ScopedRootWindowForNewWindows(
    aura::Window* new_root) {
  DCHECK(new_root);
  Shell::Get()->scoped_root_window_for_new_windows_ = new_root;
}

ScopedRootWindowForNewWindows::~ScopedRootWindowForNewWindows() {
  Shell::Get()->scoped_root_window_for_new_windows_ = nullptr;
}

}  // namespace ash
