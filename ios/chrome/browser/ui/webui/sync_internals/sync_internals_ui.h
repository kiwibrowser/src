// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_WEBUI_SYNC_INTERNALS_SYNC_INTERNALS_UI_H_
#define IOS_CHROME_BROWSER_UI_WEBUI_SYNC_INTERNALS_SYNC_INTERNALS_UI_H_

#include "base/macros.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"

namespace web {
class WebUIIOS;
}

// The implementation for the chrome://sync-internals page.
class SyncInternalsUI : public web::WebUIIOSController {
 public:
  explicit SyncInternalsUI(web::WebUIIOS* web_ui);
  ~SyncInternalsUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SyncInternalsUI);
};

#endif  // IOS_CHROME_BROWSER_UI_WEBUI_SYNC_INTERNALS_SYNC_INTERNALS_UI_H_
