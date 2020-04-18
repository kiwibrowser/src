// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/shell/shell_delegate_impl.h"

#include <memory>

#include "ash/accessibility/default_accessibility_delegate.h"
#include "ash/keyboard/test_keyboard_ui.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/root_window_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/shell/example_factory.h"
#include "ash/shell/toplevel_window.h"
#include "ash/test_screenshot_delegate.h"
#include "ash/wm/window_state.h"
#include "base/strings/utf_string_conversions.h"
#include "components/user_manager/user_info_impl.h"
#include "ui/aura/window.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"

namespace ash {
namespace shell {

ShellDelegateImpl::ShellDelegateImpl() = default;

ShellDelegateImpl::~ShellDelegateImpl() = default;

::service_manager::Connector* ShellDelegateImpl::GetShellConnector() const {
  return nullptr;
}

bool ShellDelegateImpl::CanShowWindowForUser(aura::Window* window) const {
  return true;
}

void ShellDelegateImpl::PreInit() {}

std::unique_ptr<keyboard::KeyboardUI> ShellDelegateImpl::CreateKeyboardUI() {
  return std::make_unique<TestKeyboardUI>();
}

void ShellDelegateImpl::OpenUrlFromArc(const GURL& url) {}

NetworkingConfigDelegate* ShellDelegateImpl::GetNetworkingConfigDelegate() {
  return nullptr;
}

std::unique_ptr<ash::ScreenshotDelegate>
ShellDelegateImpl::CreateScreenshotDelegate() {
  return std::make_unique<TestScreenshotDelegate>();
}

AccessibilityDelegate* ShellDelegateImpl::CreateAccessibilityDelegate() {
  return new DefaultAccessibilityDelegate;
}

ui::InputDeviceControllerClient*
ShellDelegateImpl::GetInputDeviceControllerClient() {
  return nullptr;
}

}  // namespace shell
}  // namespace ash
