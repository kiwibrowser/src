// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/kiosk_app_menu_handler.h"

#include <stddef.h>

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/sys_info.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/app_mode/arc/arc_kiosk_app_manager.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_launch_error.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/screens/network_error.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/chromeos_switches.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_ui.h"
#include "extensions/grit/extensions_browser_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/web_ui_util.h"

namespace chromeos {

namespace {

// JS functions that define new and old kiosk UI API.
const char kKioskSetAppsNewAPI[] = "login.AccountPickerScreen.setApps";
const char kKioskSetAppsOldAPI[] = "login.AppsMenuButton.setApps";
const char kKioskShowErrorNewAPI[] = "login.AccountPickerScreen.showAppError";
const char kKioskShowErrorOldAPI[] = "login.AppsMenuButton.showError";

}  // namespace

KioskAppMenuHandler::KioskAppMenuHandler(
    const scoped_refptr<NetworkStateInformer>& network_state_informer)
    : is_webui_initialized_(false),
      network_state_informer_(network_state_informer),
      weak_ptr_factory_(this) {
  KioskAppManager::Get()->AddObserver(this);
  network_state_informer_->AddObserver(this);
  ArcKioskAppManager::Get()->AddObserver(this);
}

KioskAppMenuHandler::~KioskAppMenuHandler() {
  KioskAppManager::Get()->RemoveObserver(this);
  network_state_informer_->RemoveObserver(this);
  ArcKioskAppManager::Get()->RemoveObserver(this);
}

void KioskAppMenuHandler::GetLocalizedStrings(
    base::DictionaryValue* localized_strings) {
  localized_strings->SetString(
      "showApps",
      l10n_util::GetStringUTF16(IDS_KIOSK_APPS_BUTTON));
  localized_strings->SetString(
      "confirmKioskAppDiagnosticModeFormat",
      l10n_util::GetStringUTF16(IDS_LOGIN_CONFIRM_KIOSK_DIAGNOSTIC_FORMAT));
  localized_strings->SetString(
      "confirmKioskAppDiagnosticModeYes",
      l10n_util::GetStringUTF16(IDS_CONFIRM_MESSAGEBOX_YES_BUTTON_LABEL));
  localized_strings->SetString(
      "confirmKioskAppDiagnosticModeNo",
      l10n_util::GetStringUTF16(IDS_CONFIRM_MESSAGEBOX_NO_BUTTON_LABEL));
  localized_strings->SetBoolean(
      "kioskAppHasLaunchError",
      KioskAppLaunchError::Get() != KioskAppLaunchError::NONE);
}

void KioskAppMenuHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "initializeKioskApps",
      base::BindRepeating(&KioskAppMenuHandler::HandleInitializeKioskApps,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "kioskAppsLoaded",
      base::BindRepeating(&KioskAppMenuHandler::HandleKioskAppsLoaded,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "checkKioskAppLaunchError",
      base::BindRepeating(&KioskAppMenuHandler::HandleCheckKioskAppLaunchError,
                          base::Unretained(this)));
}

// static
bool KioskAppMenuHandler::EnableNewKioskUI() {
  // Turn off new kiosk UI for M34/35.
  // TODO(xiyuan, nkostylev): Revist for http://crbug.com/362062.
  return false;
}

void KioskAppMenuHandler::SendKioskApps() {
  if (!is_webui_initialized_)
    return;

  KioskAppManager::Apps apps;
  KioskAppManager::Get()->GetApps(&apps);

  base::ListValue apps_list;
  for (size_t i = 0; i < apps.size(); ++i) {
    const KioskAppManager::App& app_data = apps[i];

    std::unique_ptr<base::DictionaryValue> app_info(
        new base::DictionaryValue());
    app_info->SetBoolean("isApp", true);
    app_info->SetString("id", app_data.app_id);
    app_info->SetBoolean("isAndroidApp", false);
    // Unused for native apps. Added for consistency with Android apps.
    app_info->SetString("account_email", app_data.account_id.GetUserEmail());
    app_info->SetString("label", app_data.name);

    std::string icon_url;
    if (app_data.icon.isNull()) {
      icon_url =
          webui::GetBitmapDataUrl(*ui::ResourceBundle::GetSharedInstance()
                                       .GetImageNamed(IDR_APP_DEFAULT_ICON)
                                       .ToSkBitmap());
    } else {
      icon_url = webui::GetBitmapDataUrl(*app_data.icon.bitmap());
    }
    app_info->SetString("iconUrl", icon_url);

    apps_list.Append(std::move(app_info));
  }

  ArcKioskAppManager::Apps arc_apps;
  ArcKioskAppManager::Get()->GetAllApps(&arc_apps);
  for (size_t i = 0; i < arc_apps.size(); ++i) {
    std::unique_ptr<base::DictionaryValue> app_info(
        new base::DictionaryValue());
    app_info->SetBoolean("isApp", true);
    app_info->SetBoolean("isAndroidApp", true);
    app_info->SetString("id", arc_apps[i]->app_id());
    app_info->SetString("account_email",
                        arc_apps[i]->account_id().GetUserEmail());
    app_info->SetString("label", arc_apps[i]->name());

    std::string icon_url;
    if (arc_apps[i]->icon().isNull()) {
      icon_url =
          webui::GetBitmapDataUrl(*ui::ResourceBundle::GetSharedInstance()
                                       .GetImageNamed(IDR_APP_DEFAULT_ICON)
                                       .ToSkBitmap());
    } else {
      icon_url = webui::GetBitmapDataUrl(*arc_apps[i]->icon().bitmap());
    }
    app_info->SetString("iconUrl", icon_url);

    apps_list.Append(std::move(app_info));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      EnableNewKioskUI() ? kKioskSetAppsNewAPI : kKioskSetAppsOldAPI,
      apps_list);
}

void KioskAppMenuHandler::HandleInitializeKioskApps(
    const base::ListValue* args) {
  is_webui_initialized_ = true;
  SendKioskApps();
  UpdateState(NetworkError::ERROR_REASON_UPDATE);
}

void KioskAppMenuHandler::HandleKioskAppsLoaded(
    const base::ListValue* args) {
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_KIOSK_APPS_LOADED,
      content::NotificationService::AllSources(),
      content::NotificationService::NoDetails());
}

void KioskAppMenuHandler::HandleCheckKioskAppLaunchError(
    const base::ListValue* args) {
  KioskAppLaunchError::Error error = KioskAppLaunchError::Get();
  if (error == KioskAppLaunchError::NONE)
    return;
  KioskAppLaunchError::RecordMetricAndClear();

  const std::string error_message = KioskAppLaunchError::GetErrorMessage(error);
  bool new_kiosk_ui = EnableNewKioskUI();
  web_ui()->CallJavascriptFunctionUnsafe(
      new_kiosk_ui ? kKioskShowErrorNewAPI : kKioskShowErrorOldAPI,
      base::Value(error_message));
}

void KioskAppMenuHandler::OnKioskAppsSettingsChanged() {
  SendKioskApps();
}

void KioskAppMenuHandler::OnKioskAppDataChanged(const std::string& app_id) {
  SendKioskApps();
}

void KioskAppMenuHandler::OnKioskAppDataLoadFailure(const std::string& app_id) {
  SendKioskApps();
}

void KioskAppMenuHandler::UpdateState(NetworkError::ErrorReason reason) {
  if (network_state_informer_->state() == NetworkStateInformer::ONLINE)
    KioskAppManager::Get()->RetryFailedAppDataFetch();
}

void KioskAppMenuHandler::OnArcKioskAppsChanged() {
  SendKioskApps();
}

}  // namespace chromeos
