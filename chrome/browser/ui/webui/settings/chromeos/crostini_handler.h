// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_CROSTINI_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_CROSTINI_HANDLER_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

namespace crostini {
enum class ConciergeClientResult;
}

namespace chromeos {
namespace settings {

class CrostiniHandler : public ::settings::SettingsPageUIHandler {
 public:
  CrostiniHandler();
  ~CrostiniHandler() override;

  // SettingsPageUIHandler
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

 private:
  void HandleRequestCrostiniInstallerView(const base::ListValue* args);
  void HandleRequestRemoveCrostini(const base::ListValue* args);

  // weak_ptr_factory_ should always be last member.
  base::WeakPtrFactory<CrostiniHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CrostiniHandler);
};

}  // namespace settings
}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_CROSTINI_HANDLER_H_
