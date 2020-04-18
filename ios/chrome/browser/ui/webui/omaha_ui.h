// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_WEBUI_OMAHA_UI_H_
#define IOS_CHROME_BROWSER_UI_WEBUI_OMAHA_UI_H_

#include "base/macros.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"

class OmahaUI : public web::WebUIIOSController {
 public:
  explicit OmahaUI(web::WebUIIOS* web_ui);
  ~OmahaUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(OmahaUI);
};

#endif  // IOS_CHROME_BROWSER_UI_WEBUI_OMAHA_UI_H_
