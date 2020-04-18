// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELL_DELEGATE_H_
#define ASH_SHELL_DELEGATE_H_

#include <memory>
#include <string>

#include "ash/ash_export.h"
#include "base/callback.h"
#include "base/strings/string16.h"

class GURL;

namespace aura {
class Window;
}

namespace keyboard {
class KeyboardUI;
}

namespace service_manager {
class Connector;
}

namespace ui {
class InputDeviceControllerClient;
}

namespace ash {

class AccessibilityDelegate;
class NetworkingConfigDelegate;
class ScreenshotDelegate;

// Delegate of the Shell.
class ASH_EXPORT ShellDelegate {
 public:
  // The Shell owns the delegate.
  virtual ~ShellDelegate() {}

  // Returns the connector for the mojo service manager. Returns null in tests.
  virtual service_manager::Connector* GetShellConnector() const = 0;

  // Returns true if |window| can be shown for the delegate's concept of current
  // user.
  virtual bool CanShowWindowForUser(aura::Window* window) const = 0;

  // Called before processing |Shell::Init()| so that the delegate
  // can perform tasks necessary before the shell is initialized.
  virtual void PreInit() = 0;

  // Create a shell-specific keyboard::KeyboardUI.
  virtual std::unique_ptr<keyboard::KeyboardUI> CreateKeyboardUI() = 0;

  // Opens the |url| in a new browser tab.
  virtual void OpenUrlFromArc(const GURL& url) = 0;

  // Returns the delegate. May be null in tests.
  virtual NetworkingConfigDelegate* GetNetworkingConfigDelegate() = 0;

  // TODO(jamescook): Replace with a mojo-compatible interface.
  virtual std::unique_ptr<ScreenshotDelegate> CreateScreenshotDelegate() = 0;

  // Creates a accessibility delegate. Shell takes ownership of the delegate.
  virtual AccessibilityDelegate* CreateAccessibilityDelegate() = 0;

  virtual void OpenKeyboardShortcutHelpPage() const {}

  // Creator of Shell owns this; it's assumed this outlives Shell.
  virtual ui::InputDeviceControllerClient* GetInputDeviceControllerClient() = 0;
};

}  // namespace ash

#endif  // ASH_SHELL_DELEGATE_H_
