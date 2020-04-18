// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MODULE_DATABASE_CONFLICTS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_MODULE_DATABASE_CONFLICTS_HANDLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/conflicts/module_database_observer_win.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class Listvalue;
}

// This class takes care of sending the list of all loaded modules to the
// chrome://conflicts WebUI page when it is requested. It replaces
// ConflictsHandler when the ModuleDatabase feature is enabled.
class ModuleDatabaseConflictsHandler : public content::WebUIMessageHandler,
                                       public ModuleDatabaseObserver {
 public:
  ModuleDatabaseConflictsHandler();
  ~ModuleDatabaseConflictsHandler() override;

 private:
  // content::WebUIMessageHandler:
  void RegisterMessages() override;

  // ModuleDatabaseObserver:
  void OnNewModuleFound(const ModuleInfoKey& module_key,
                        const ModuleInfoData& module_data) override;
  void OnModuleDatabaseIdle() override;

  // Callback for the "requestModuleList" message.
  void HandleRequestModuleList(const base::ListValue* args);

  // The ID of the callback that will get invoked with the module list.
  std::string module_list_callback_id_;

  // Temporarily holds the module list while the modules are being
  // enumerated.
  std::unique_ptr<base::ListValue> module_list_;

  DISALLOW_COPY_AND_ASSIGN(ModuleDatabaseConflictsHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_MODULE_DATABASE_CONFLICTS_HANDLER_H_
