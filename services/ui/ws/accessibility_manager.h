// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_ACCESSIBILITY_MANAGER_H_
#define SERVICES_UI_WS_ACCESSIBILITY_MANAGER_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "services/ui/public/interfaces/accessibility_manager.mojom.h"

namespace ui {
namespace ws {

class WindowServer;

class AccessibilityManager : public mojom::AccessibilityManager {
 public:
  explicit AccessibilityManager(WindowServer* window_server);
  ~AccessibilityManager() override;

  void Bind(mojom::AccessibilityManagerRequest request);

 private:
  // mojom::AccessibilityManager:
  void SetHighContrastMode(bool enabled) override;

  WindowServer* window_server_;
  mojo::Binding<mojom::AccessibilityManager> binding_;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityManager);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_ACCESSIBILITY_MANAGER_H_
