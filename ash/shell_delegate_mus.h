// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_DELEGATE_MUS_H_
#define ASH_SHELL_DELEGATE_MUS_H_

#include <memory>

#include "ash/shell_delegate.h"
#include "base/macros.h"

namespace service_manager {
class Connector;
}

namespace ash {

// TODO(jamescook): Rename to ShellDelegateMash.
class ShellDelegateMus : public ShellDelegate {
 public:
  explicit ShellDelegateMus(service_manager::Connector* connector);
  ~ShellDelegateMus() override;

  // ShellDelegate:
  service_manager::Connector* GetShellConnector() const override;
  bool CanShowWindowForUser(aura::Window* window) const override;
  void PreInit() override;
  std::unique_ptr<keyboard::KeyboardUI> CreateKeyboardUI() override;
  void OpenUrlFromArc(const GURL& url) override;
  NetworkingConfigDelegate* GetNetworkingConfigDelegate() override;
  std::unique_ptr<ScreenshotDelegate> CreateScreenshotDelegate() override;
  AccessibilityDelegate* CreateAccessibilityDelegate() override;
  ui::InputDeviceControllerClient* GetInputDeviceControllerClient() override;

 private:
  // |connector_| may be null in tests.
  service_manager::Connector* connector_;

  std::unique_ptr<ui::InputDeviceControllerClient>
      input_device_controller_client_;

  DISALLOW_COPY_AND_ASSIGN(ShellDelegateMus);
};

}  // namespace ash

#endif  // ASH_SHELL_DELEGATE_MUS_H_
