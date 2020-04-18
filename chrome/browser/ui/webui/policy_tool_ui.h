// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_POLICY_TOOL_UI_H_
#define CHROME_BROWSER_UI_WEBUI_POLICY_TOOL_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

namespace content {
class WebUI;
}

// The Web UI controller for the chrome://policy-tool page.
class PolicyToolUI : public content::WebUIController {
 public:
  explicit PolicyToolUI(content::WebUI* web_ui);
  ~PolicyToolUI() override;

  static bool IsEnabled();

 private:
  DISALLOW_COPY_AND_ASSIGN(PolicyToolUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_POLICY_TOOL_UI_H_
