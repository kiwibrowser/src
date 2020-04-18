// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_DEVICE_DISABLED_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_DEVICE_DISABLED_SCREEN_HANDLER_H_

#include "base/macros.h"
#include "chrome/browser/chromeos/login/screens/device_disabled_screen_view.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"

namespace chromeos {

// WebUI implementation of DeviceDisabledScreenActor.
class DeviceDisabledScreenHandler : public DeviceDisabledScreenView,
                                    public BaseScreenHandler {
 public:
  DeviceDisabledScreenHandler();
  ~DeviceDisabledScreenHandler() override;

  // DeviceDisabledScreenActor:
  void Show() override;
  void Hide() override;
  void SetDelegate(Delegate* delegate) override;
  void UpdateMessage(const std::string& message) override;

  // BaseScreenHandler:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void Initialize() override;

 private:
  // WebUIMessageHandler:
  void RegisterMessages() override;

  Delegate* delegate_ = nullptr;

  // Indicates whether the screen should be shown right after initialization.
  bool show_on_init_ = false;

  DISALLOW_COPY_AND_ASSIGN(DeviceDisabledScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_DEVICE_DISABLED_SCREEN_HANDLER_H_

