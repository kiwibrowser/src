// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_KEYBOARD_OVERLAY_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_KEYBOARD_OVERLAY_UI_H_

#include "base/macros.h"
#include "ui/web_dialogs/web_dialog_ui.h"

class KeyboardOverlayUI : public ui::WebDialogUI {
 public:
  explicit KeyboardOverlayUI(content::WebUI* web_ui);

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardOverlayUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_KEYBOARD_OVERLAY_UI_H_
