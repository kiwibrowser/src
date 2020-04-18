// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_INTERNET_CONFIG_DIALOG_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_INTERNET_CONFIG_DIALOG_H_

#include <string>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "chrome/browser/ui/webui/chromeos/system_web_dialog_delegate.h"
#include "ui/web_dialogs/web_dialog_ui.h"

namespace chromeos {

class NetworkState;

class InternetConfigDialog : public SystemWebDialogDelegate {
 public:
  // Shows a network configuration dialog for |network_id|. Does nothing if
  // there is no NetworkState matching |network_id|.
  static void ShowDialogForNetworkId(const std::string& network_id);
  // Shows a network configuration dialog for a new network of |network_type|.
  static void ShowDialogForNetworkType(const std::string& network_type);

 protected:
  InternetConfigDialog(const std::string& network_type,
                       const std::string& network_id);
  ~InternetConfigDialog() override;

  // ui::WebDialogDelegate
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;

 private:
  std::string network_type_;
  std::string network_id_;

  DISALLOW_COPY_AND_ASSIGN(InternetConfigDialog);
};

// A WebUI to host the network configuration UI in a dialog, used in the
// login screen and when a new network is configured from the system tray.
class InternetConfigDialogUI : public ui::WebDialogUI {
 public:
  explicit InternetConfigDialogUI(content::WebUI* web_ui);
  ~InternetConfigDialogUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(InternetConfigDialogUI);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_INTERNET_CONFIG_DIALOG_H_
