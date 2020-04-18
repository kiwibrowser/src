// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CONFLICTS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CONFLICTS_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/win/enumerate_modules_model.h"
#include "content/public/browser/web_ui_message_handler.h"

// This class takes care of sending the list of all loaded modules to the
// chrome://conflicts WebUI page when it is requested.
class ConflictsHandler : public content::WebUIMessageHandler,
                         public EnumerateModulesModel::Observer {
 public:
  ConflictsHandler();
  ~ConflictsHandler() override;

 private:
  // content::WebUIMessageHandler:
  void RegisterMessages() override;

  // EnumerateModulesModel::Observer:
  void OnScanCompleted() override;

  // Callback for the "requestModuleList" message.
  void HandleRequestModuleList(const base::ListValue* args);

  // Sends the module list back to the WebUI page.
  void SendModuleList();

  ScopedObserver<EnumerateModulesModel, EnumerateModulesModel::Observer>
      observer_;

  // The ID of the callback that will get invoked with the module list.
  std::string module_list_callback_id_;

  DISALLOW_COPY_AND_ASSIGN(ConflictsHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_CONFLICTS_HANDLER_H_
