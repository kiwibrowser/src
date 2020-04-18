// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/accessibility_manager.h"

#include "services/ui/ws/window_server.h"

namespace ui {
namespace ws {

AccessibilityManager::AccessibilityManager(WindowServer* window_server)
    : window_server_(window_server), binding_(this) {
  DCHECK(window_server_);
}

AccessibilityManager::~AccessibilityManager() {}

void AccessibilityManager::Bind(mojom::AccessibilityManagerRequest request) {
  binding_.Close();
  binding_.Bind(std::move(request));
}

void AccessibilityManager::SetHighContrastMode(bool enabled) {
  window_server_->SetHighContrastMode(enabled);
}

}  // namespace ws
}  // namespace ui
