// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_CHROME_SHELL_DELEGATE_H_
#define CHROME_BROWSER_UI_ASH_CHROME_SHELL_DELEGATE_H_

#include <memory>
#include <string>

#include "ash/shell_delegate.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace keyboard {
class KeyboardUI;
}

class ChromeShellDelegate : public ash::ShellDelegate,
                            public content::NotificationObserver {
 public:
  ChromeShellDelegate();
  ~ChromeShellDelegate() override;

  // ash::ShellDelegate overrides;
  service_manager::Connector* GetShellConnector() const override;
  bool CanShowWindowForUser(aura::Window* window) const override;
  void PreInit() override;
  std::unique_ptr<keyboard::KeyboardUI> CreateKeyboardUI() override;
  void OpenUrlFromArc(const GURL& url) override;
  ash::NetworkingConfigDelegate* GetNetworkingConfigDelegate() override;
  std::unique_ptr<ash::ScreenshotDelegate> CreateScreenshotDelegate() override;
  ash::AccessibilityDelegate* CreateAccessibilityDelegate() override;
  void OpenKeyboardShortcutHelpPage() const override;
  ui::InputDeviceControllerClient* GetInputDeviceControllerClient() override;

  // content::NotificationObserver override:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 private:
  void PlatformInit();

  content::NotificationRegistrar registrar_;

  std::unique_ptr<ash::NetworkingConfigDelegate> networking_config_delegate_;

  DISALLOW_COPY_AND_ASSIGN(ChromeShellDelegate);
};

#endif  // CHROME_BROWSER_UI_ASH_CHROME_SHELL_DELEGATE_H_
