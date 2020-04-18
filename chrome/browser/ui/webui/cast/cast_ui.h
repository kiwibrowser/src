// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CAST_CAST_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CAST_CAST_UI_H_

#include "base/macros.h"
#include "base/values.h"
#include "content/public/browser/web_ui_controller.h"

// The WebUI for chrome://cast
class CastUI : public content::WebUIController {
 public:
  explicit CastUI(content::WebUI* web_ui);
  ~CastUI() override;
 private:
  DISALLOW_COPY_AND_ASSIGN(CastUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_CAST_CAST_UI_H_
