// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_WEBUI_NET_EXPORT_NET_EXPORT_UI_H_
#define IOS_CHROME_BROWSER_UI_WEBUI_NET_EXPORT_NET_EXPORT_UI_H_

#include "base/macros.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"

// The C++ back-end for the chrome://net-export webui page.
class NetExportUI : public web::WebUIIOSController {
 public:
  explicit NetExportUI(web::WebUIIOS* web_ui);

 private:
  DISALLOW_COPY_AND_ASSIGN(NetExportUI);
};

#endif  // IOS_CHROME_BROWSER_UI_WEBUI_NET_EXPORT_NET_EXPORT_UI_H_
