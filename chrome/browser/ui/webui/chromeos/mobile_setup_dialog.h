// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_MOBILE_SETUP_DIALOG_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_MOBILE_SETUP_DIALOG_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "ui/web_dialogs/web_dialog_ui.h"

class MobileSetupDialog {
 public:
  static void ShowByNetworkId(const std::string& network_id);

 private:
  DISALLOW_COPY_AND_ASSIGN(MobileSetupDialog);
};

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_MOBILE_SETUP_DIALOG_H_
