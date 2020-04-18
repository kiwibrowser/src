// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_MULTIDEVICE_SETUP_MULTIDEVICE_SETUP_DIALOG_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_MULTIDEVICE_SETUP_MULTIDEVICE_SETUP_DIALOG_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/webui/chromeos/system_web_dialog_delegate.h"
#include "ui/web_dialogs/web_dialog_ui.h"

namespace chromeos {

namespace multidevice_setup {

// Dialog which displays the multi-device setup flow which allows users to
// enable features involving communication between multiple devices (e.g., a
// Chromebook and a phone).
class MultiDeviceSetupDialog : public SystemWebDialogDelegate {
 public:
  // Shows the dialog; if the dialog is already displayed, this function is a
  // no-op.
  static void Show();

 protected:
  MultiDeviceSetupDialog();
  ~MultiDeviceSetupDialog() override;

  // ui::WebDialogDelegate
  void GetDialogSize(gfx::Size* size) const override;
  void OnDialogClosed(const std::string& json_retval) override;

 private:
  static MultiDeviceSetupDialog* current_instance_;

  DISALLOW_COPY_AND_ASSIGN(MultiDeviceSetupDialog);
};

class MultiDeviceSetupDialogUI : public ui::WebDialogUI {
 public:
  explicit MultiDeviceSetupDialogUI(content::WebUI* web_ui);
  ~MultiDeviceSetupDialogUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MultiDeviceSetupDialogUI);
};

}  // namespace multidevice_setup

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_MULTIDEVICE_SETUP_MULTIDEVICE_SETUP_DIALOG_H_
