// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_NETWORK_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_NETWORK_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

namespace base {
class DictionaryValue;
};

namespace chromeos {

class NetworkUI : public content::WebUIController {
 public:
  explicit NetworkUI(content::WebUI* web_ui);
  ~NetworkUI() override;

  static void GetLocalizedStrings(base::DictionaryValue* localized_strings);

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkUI);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_NETWORK_UI_H_
