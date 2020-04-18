// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_UI_H_

#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/arc/voice_interaction/voice_interaction_controller_client.h"
#include "chrome/browser/ui/webui/chromeos/assistant_optin/assistant_optin_handler.h"
#include "chrome/browser/ui/webui/chromeos/assistant_optin/assistant_optin_screen_exit_code.h"
#include "chrome/browser/ui/webui/chromeos/login/base_webui_handler.h"
#include "chrome/browser/ui/webui/chromeos/system_web_dialog_delegate.h"
#include "chromeos/services/assistant/public/mojom/settings.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/web_dialogs/web_dialog_ui.h"

namespace chromeos {

// Controller for chrome://assistant-optin/ page.
class AssistantOptInUI
    : public ui::WebDialogUI,
      public arc::VoiceInteractionControllerClient::Observer {
 public:
  explicit AssistantOptInUI(content::WebUI* web_ui);
  ~AssistantOptInUI() override;

  // arc::VoiceInteractionControllerClient::Observer overrides
  void OnStateChanged(ash::mojom::VoiceInteractionState state) override;

 private:
  // Initilize connection to settings manager.
  void Initialize();

  // Add message handler for optin screens.
  void AddScreenHandler(std::unique_ptr<BaseWebUIHandler> handler);

  // Called by a screen when user's done with it.
  void OnExit(AssistantOptInScreenExitCode exit_code);

  // Handle response from the settings manager.
  void OnGetSettingsResponse(const std::string& settings);
  void OnUpdateSettingsResponse(const std::string& settings);

  // Consent token used to complete the opt-in.
  std::string consent_token_;

  AssistantOptInHandler* assistant_handler_ = nullptr;
  std::unique_ptr<JSCallsContainer> js_calls_container_;
  assistant::mojom::AssistantSettingsManagerPtr settings_manager_;
  std::vector<BaseWebUIHandler*> screen_handlers_;
  base::WeakPtrFactory<AssistantOptInUI> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AssistantOptInUI);
};

// Dialog delegate for the assistant optin page.
class AssistantOptInDialog : public SystemWebDialogDelegate {
 public:
  // Shows the assistant optin dialog.
  static void Show();

  // Returns whether the dialog is being shown.
  static bool IsActive();

 protected:
  AssistantOptInDialog();
  ~AssistantOptInDialog() override;

  // ui::WebDialogDelegate
  void GetDialogSize(gfx::Size* size) const override;
  std::string GetDialogArgs() const override;
  bool ShouldShowDialogTitle() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AssistantOptInDialog);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_ASSISTANT_OPTIN_ASSISTANT_OPTIN_UI_H_
