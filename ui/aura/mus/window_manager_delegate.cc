// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/window_manager_delegate.h"

namespace aura {

void WindowManagerDelegate::OnWmConnected() {}

ui::mojom::EventResult WindowManagerDelegate::OnAccelerator(
    uint32_t id,
    const ui::Event& event,
    base::flat_map<std::string, std::vector<uint8_t>>* properties) {
  return ui::mojom::EventResult::UNHANDLED;
}

void WindowManagerDelegate::OnWmPerformAction(Window* window,
                                              const std::string& action) {}

void WindowManagerDelegate::OnEventBlockedByModalWindow(Window* window) {}

}  // namespace aura
