// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SYSTEM_INFO_UI_H_
#define CHROME_BROWSER_UI_WEBUI_SYSTEM_INFO_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

class SystemInfoUI : public content::WebUIController {
 public:
  explicit SystemInfoUI(content::WebUI* web_ui);

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemInfoUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SYSTEM_INFO_UI_H_
