// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell_delegate_mus.h"

#include <memory>
#include <utility>

#include "ash/accessibility/default_accessibility_delegate.h"
#include "ash/screenshot_delegate.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "components/user_manager/user_info_impl.h"
#include "services/ui/public/cpp/input_devices/input_device_controller_client.h"
#include "ui/gfx/image/image.h"
#include "ui/keyboard/keyboard_ui.h"

namespace ash {
namespace {

// TODO(jamescook): Replace with a mojo-compatible ScreenshotClient.
class ScreenshotDelegateMash : public ScreenshotDelegate {
 public:
  ScreenshotDelegateMash() = default;
  ~ScreenshotDelegateMash() override = default;

  // ScreenshotDelegate:
  void HandleTakeScreenshotForAllRootWindows() override { NOTIMPLEMENTED(); }
  void HandleTakePartialScreenshot(aura::Window* window,
                                   const gfx::Rect& rect) override {
    NOTIMPLEMENTED();
  }
  void HandleTakeWindowScreenshot(aura::Window* window) override {
    NOTIMPLEMENTED();
  }
  bool CanTakeScreenshot() override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(ScreenshotDelegateMash);
};

}  // namespace

ShellDelegateMus::ShellDelegateMus(service_manager::Connector* connector)
    : connector_(connector) {}

ShellDelegateMus::~ShellDelegateMus() = default;

service_manager::Connector* ShellDelegateMus::GetShellConnector() const {
  return connector_;
}

bool ShellDelegateMus::CanShowWindowForUser(aura::Window* window) const {
  NOTIMPLEMENTED_LOG_ONCE();
  return true;
}

void ShellDelegateMus::PreInit() {
  NOTIMPLEMENTED_LOG_ONCE();
}

std::unique_ptr<keyboard::KeyboardUI> ShellDelegateMus::CreateKeyboardUI() {
  NOTIMPLEMENTED_LOG_ONCE();
  return nullptr;
}

void ShellDelegateMus::OpenUrlFromArc(const GURL& url) {
  NOTIMPLEMENTED_LOG_ONCE();
}

NetworkingConfigDelegate* ShellDelegateMus::GetNetworkingConfigDelegate() {
  // TODO(mash): Provide a real implementation, perhaps by folding its behavior
  // into an ash-side network information cache. http://crbug.com/651157
  NOTIMPLEMENTED_LOG_ONCE();
  return nullptr;
}

std::unique_ptr<ScreenshotDelegate>
ShellDelegateMus::CreateScreenshotDelegate() {
  return std::make_unique<ScreenshotDelegateMash>();
}

AccessibilityDelegate* ShellDelegateMus::CreateAccessibilityDelegate() {
  return new DefaultAccessibilityDelegate;
}

ui::InputDeviceControllerClient*
ShellDelegateMus::GetInputDeviceControllerClient() {
  if (!connector_)
    return nullptr;  // Happens in tests.

  if (!input_device_controller_client_) {
    input_device_controller_client_ =
        std::make_unique<ui::InputDeviceControllerClient>(connector_);
  }
  return input_device_controller_client_.get();
}

}  // namespace ash
