// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_ACCOUNT_MANAGER_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_ACCOUNT_MANAGER_HANDLER_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "chromeos/account_manager/account_manager.h"

class AccountTrackerService;

namespace chromeos {
namespace settings {

class AccountManagerUIHandler : public ::settings::SettingsPageUIHandler {
 public:
  // Accepts non-owning pointers to |AccountManager| and
  // |AccountTrackerService|. Both of these must outlive |this| instance.
  AccountManagerUIHandler(AccountManager* account_manager,
                          AccountTrackerService* account_tracker_service);
  ~AccountManagerUIHandler() override;

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

 private:
  // WebUI "getAccounts" message callback.
  void HandleGetAccounts(const base::ListValue* args);

  // |AccountManager::GetAccounts| callback.
  void GetAccountsCallbackHandler(
      base::Value callback_id,
      std::vector<AccountManager::AccountKey> account_keys);

  // A non-owning pointer to |AccountManager|.
  AccountManager* const account_manager_;

  // A non-owning pointer to |AccountTrackerService|.
  AccountTrackerService* const account_tracker_service_;

  base::WeakPtrFactory<AccountManagerUIHandler> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AccountManagerUIHandler);
};

}  // namespace settings
}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_SETTINGS_CHROMEOS_ACCOUNT_MANAGER_HANDLER_H_
