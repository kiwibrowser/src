// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_NACL_UI_H_
#define CHROME_BROWSER_UI_WEBUI_NACL_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

// The Web UI handler for about:nacl.
class NaClUI : public content::WebUIController {
 public:
  explicit NaClUI(content::WebUI* web_ui);

 private:
  DISALLOW_COPY_AND_ASSIGN(NaClUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_NACL_UI_H_
