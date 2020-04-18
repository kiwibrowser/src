// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_HANDLER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/webui/chromeos/login/base_webui_handler.h"

namespace chromeos {

class AssistantOptInHandler : public BaseWebUIHandler {
 public:
  explicit AssistantOptInHandler(JSCallsContainer* js_calls_container);
  ~AssistantOptInHandler() override;

  // BaseWebUIHandler:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void RegisterMessages() override;
  void Initialize() override;

  // Send message and consent data to the page.
  void ReloadContent(const base::DictionaryValue& dict);
  void AddSettingZippy(const base::ListValue& data);

 private:
  // Handler for JS WebUI message.
  void HandleInitialized();

  DISALLOW_COPY_AND_ASSIGN(AssistantOptInHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_HANDLER_H_
