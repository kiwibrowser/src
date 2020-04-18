// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/windows/windows_window_manager.h"

namespace ui {

WindowsWindowManager::WindowsWindowManager() = default;

WindowsWindowManager::~WindowsWindowManager() {}

int32_t WindowsWindowManager::AddWindow(WindowsWindow* window) {
  return windows_.Add(window);
}

void WindowsWindowManager::RemoveWindow(int32_t window_id,
                                        WindowsWindow* window) {
  DCHECK_EQ(window, windows_.Lookup(window_id));
  windows_.Remove(window_id);
}

WindowsWindow* WindowsWindowManager::GetWindow(int32_t window_id) {
  return windows_.Lookup(window_id);
}

}  // namespace ui
