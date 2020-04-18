// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_OFFLINE_OFFLINE_INTERNALS_UI_H_
#define CHROME_BROWSER_UI_WEBUI_OFFLINE_OFFLINE_INTERNALS_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

// The WebUI for chrome://offline-internals.
class OfflineInternalsUI : public content::WebUIController {
 public:
  explicit OfflineInternalsUI(content::WebUI* web_ui);
  ~OfflineInternalsUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(OfflineInternalsUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_OFFLINE_OFFLINE_INTERNALS_UI_H_
