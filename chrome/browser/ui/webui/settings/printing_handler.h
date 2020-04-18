// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_PRINTING_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_PRINTING_HANDLER_H_

#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "printing/buildflags/buildflags.h"

#if defined(OS_CHROMEOS)
#error "Not for use on ChromeOS"
#endif

#if !BUILDFLAG(ENABLE_PRINTING)
#error "Printing must be enabled"
#endif

namespace settings {

// UI handler for Chrome printing setting subpage on operating systems other
// than Chrome OS.
class PrintingHandler : public SettingsPageUIHandler {
 public:
  PrintingHandler();
  ~PrintingHandler() override;

  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

 private:
  void HandleOpenSystemPrintDialog(const base::ListValue* args);

  DISALLOW_COPY_AND_ASSIGN(PrintingHandler);
};

}  // namespace settings

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_PRINTING_HANDLER_H_
