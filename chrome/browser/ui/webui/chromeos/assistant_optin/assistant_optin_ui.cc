// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/assistant_optin/assistant_optin_ui.h"

#include <memory>

#include "base/bind.h"
#include "base/macros.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/webui/chromeos/assistant_optin/value_prop_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chromeos/services/assistant/public/mojom/constants.mojom.h"
#include "chromeos/services/assistant/public/proto/settings_ui.pb.h"
#include "components/arc/arc_prefs.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/service_manager/public/cpp/connector.h"

namespace chromeos {

namespace {

bool is_active = false;

constexpr int kAssistantOptInDialogWidth = 576;
constexpr int kAssistantOptInDialogHeight = 480;

// Construct SettingsUiSelector for the ConsentFlow UI.
assistant::SettingsUiSelector GetSettingsUiSelector() {
  assistant::SettingsUiSelector selector;
  assistant::ConsentFlowUiSelector* consent_flow_ui =
      selector.mutable_consent_flow_ui_selector();
  consent_flow_ui->set_flow_id(assistant::ActivityControlSettingsUiSelector::
                                   ASSISTANT_SUW_ONBOARDING_ON_CHROME_OS);
  return selector;
}

// Construct SettingsUiUpdate for user opt-in.
assistant::SettingsUiUpdate GetSettingsUiUpdate(
    const std::string& consent_token) {
  assistant::SettingsUiUpdate update;
  assistant::ConsentFlowUiUpdate* consent_flow_update =
      update.mutable_consent_flow_ui_update();
  consent_flow_update->set_flow_id(
      assistant::ActivityControlSettingsUiSelector::
          ASSISTANT_SUW_ONBOARDING_ON_CHROME_OS);
  consent_flow_update->set_consent_token(consent_token);

  return update;
}

}  // namespace

AssistantOptInUI::AssistantOptInUI(content::WebUI* web_ui)
    : ui::WebDialogUI(web_ui), weak_factory_(this) {
  // Set up the chrome://assistant-optin source.
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIAssistantOptInHost);

  js_calls_container_ = std::make_unique<JSCallsContainer>();

  auto base_handler =
      std::make_unique<AssistantOptInHandler>(js_calls_container_.get());
  assistant_handler_ = base_handler.get();
  AddScreenHandler(std::move(base_handler));

  AddScreenHandler(std::make_unique<ValuePropScreenHandler>(
      base::BindOnce(&AssistantOptInUI::OnExit, weak_factory_.GetWeakPtr())));

  base::DictionaryValue localized_strings;
  for (auto* handler : screen_handlers_)
    handler->GetLocalizedStrings(&localized_strings);
  source->AddLocalizedStrings(localized_strings);

  source->SetJsonPath("strings.js");
  source->AddResourcePath("assistant_optin.js", IDR_ASSISTANT_OPTIN_JS);
  source->SetDefaultResource(IDR_ASSISTANT_OPTIN_HTML);
  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui), source);

  if (arc::VoiceInteractionControllerClient::Get()->voice_interaction_state() !=
      ash::mojom::VoiceInteractionState::RUNNING) {
    arc::VoiceInteractionControllerClient::Get()->AddObserver(this);
  } else {
    Initialize();
  }
}

AssistantOptInUI::~AssistantOptInUI() {
  arc::VoiceInteractionControllerClient::Get()->RemoveObserver(this);
}

void AssistantOptInUI::OnStateChanged(ash::mojom::VoiceInteractionState state) {
  if (state == ash::mojom::VoiceInteractionState::RUNNING)
    Initialize();
}

void AssistantOptInUI::Initialize() {
  if (settings_manager_.is_bound())
    return;

  // Set up settings mojom.
  Profile* const profile = Profile::FromWebUI(web_ui());
  service_manager::Connector* connector =
      content::BrowserContext::GetConnectorFor(profile);
  connector->BindInterface(assistant::mojom::kServiceName,
                           mojo::MakeRequest(&settings_manager_));

  // Send GetSettings request for the ConsentFlow UI.
  assistant::SettingsUiSelector selector = GetSettingsUiSelector();
  settings_manager_->GetSettings(
      selector.SerializeAsString(),
      base::BindOnce(&AssistantOptInUI::OnGetSettingsResponse,
                     weak_factory_.GetWeakPtr()));
}

void AssistantOptInUI::AddScreenHandler(
    std::unique_ptr<BaseWebUIHandler> handler) {
  screen_handlers_.push_back(handler.get());
  web_ui()->AddMessageHandler(std::move(handler));
}

void AssistantOptInUI::OnExit(AssistantOptInScreenExitCode exit_code) {
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  switch (exit_code) {
    case AssistantOptInScreenExitCode::VALUE_PROP_SKIPPED:
      prefs->SetBoolean(arc::prefs::kArcVoiceInteractionValuePropAccepted,
                        false);
      prefs->SetBoolean(arc::prefs::kVoiceInteractionEnabled, false);
      CloseDialog(nullptr);
      break;
    case AssistantOptInScreenExitCode::VALUE_PROP_ACCEPTED:
      // Send the update to complete user opt-in.
      settings_manager_->UpdateSettings(
          GetSettingsUiUpdate(consent_token_).SerializeAsString(),
          base::BindOnce(&AssistantOptInUI::OnUpdateSettingsResponse,
                         weak_factory_.GetWeakPtr()));
      break;
    default:
      NOTREACHED();
  }
}

void AssistantOptInUI::OnGetSettingsResponse(const std::string& settings) {
  assistant::SettingsUi settings_ui;
  assistant::ConsentFlowUi::ConsentUi::ActivityControlUi activity_control_ui;
  settings_ui.ParseFromString(settings);

  DCHECK(settings_ui.has_consent_flow_ui());
  activity_control_ui =
      settings_ui.consent_flow_ui().consent_ui().activity_control_ui();
  consent_token_ = activity_control_ui.consent_token();

  base::ListValue zippy_data;
  if (activity_control_ui.setting_zippy().size() == 0) {
    // No need to consent. Close the dialog for now.
    CloseDialog(nullptr);
    return;
  }
  for (auto& setting_zippy : activity_control_ui.setting_zippy()) {
    base::DictionaryValue data;
    data.SetString("title", setting_zippy.title());
    data.SetString("description", setting_zippy.description_paragraph(0));
    data.SetString("additionalInfo",
                   setting_zippy.additional_info_paragraph(0));
    data.SetString("iconUri", setting_zippy.icon_uri());
    zippy_data.GetList().push_back(std::move(data));
  }
  assistant_handler_->AddSettingZippy(zippy_data);

  base::DictionaryValue dictionary;
  dictionary.SetString("valuePropIntro",
                       activity_control_ui.intro_text_paragraph(0));
  dictionary.SetString("valuePropIdentity", activity_control_ui.identity());
  dictionary.SetString("valuePropFooter",
                       activity_control_ui.footer_paragraph(0));
  dictionary.SetString(
      "valuePropNextButton",
      settings_ui.consent_flow_ui().consent_ui().accept_button_text());
  dictionary.SetString(
      "valuePropSkipButton",
      settings_ui.consent_flow_ui().consent_ui().reject_button_text());
  assistant_handler_->ReloadContent(dictionary);
}

void AssistantOptInUI::OnUpdateSettingsResponse(const std::string& result) {
  assistant::SettingsUiUpdateResult ui_result;
  ui_result.ParseFromString(result);

  DCHECK(ui_result.has_consent_flow_update_result());
  if (ui_result.consent_flow_update_result().update_status() !=
      assistant::ConsentFlowUiUpdateResult::SUCCESS) {
    // TODO(updowndta): Handle consent update failure.
    LOG(ERROR) << "Consent udpate error.";
  }

  // More screens to be added. Close the dialog for now.
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  prefs->SetBoolean(arc::prefs::kArcVoiceInteractionValuePropAccepted, true);
  prefs->SetBoolean(arc::prefs::kVoiceInteractionEnabled, true);
  CloseDialog(nullptr);
}

// AssistantOptInDialog

// static
void AssistantOptInDialog::Show() {
  DCHECK(!is_active);
  AssistantOptInDialog* dialog = new AssistantOptInDialog();
  dialog->ShowSystemDialog(true);
}

// static
bool AssistantOptInDialog::IsActive() {
  return is_active;
}

AssistantOptInDialog::AssistantOptInDialog()
    : SystemWebDialogDelegate(GURL(chrome::kChromeUIAssistantOptInURL),
                              base::string16()) {
  DCHECK(!is_active);
  is_active = true;
}

AssistantOptInDialog::~AssistantOptInDialog() {
  is_active = false;
}

void AssistantOptInDialog::GetDialogSize(gfx::Size* size) const {
  size->SetSize(kAssistantOptInDialogWidth, kAssistantOptInDialogHeight);
}

std::string AssistantOptInDialog::GetDialogArgs() const {
  return std::string();
}

bool AssistantOptInDialog::ShouldShowDialogTitle() const {
  return false;
}

}  // namespace chromeos
