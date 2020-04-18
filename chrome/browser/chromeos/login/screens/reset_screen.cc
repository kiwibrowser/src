// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/reset_screen.h"

#include "base/command_line.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/screens/error_screen.h"
#include "chrome/browser/chromeos/login/screens/network_error.h"
#include "chrome/browser/chromeos/login/screens/reset_view.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/reset/metrics.h"
#include "chrome/browser/chromeos/tpm_firmware_update.h"
#include "chrome/common/pref_names.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager_client.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {
namespace {

constexpr const char kUserActionCancelReset[] = "cancel-reset";
constexpr const char kUserActionResetRestartPressed[] = "restart-pressed";
constexpr const char kUserActionResetPowerwashPressed[] = "powerwash-pressed";
constexpr const char kUserActionResetLearnMorePressed[] = "learn-more-link";
constexpr const char kUserActionResetRollbackToggled[] = "rollback-toggled";
constexpr const char kUserActionResetShowConfirmationPressed[] =
    "show-confirmation";
constexpr const char kUserActionResetResetConfirmationDismissed[] =
    "reset-confirm-dismissed";
constexpr const char kUserActionTPMFirmwareUpdateLearnMore[] =
    "tpm-firmware-update-learn-more-link";

constexpr const char kContextKeyIsRollbackAvailable[] = "rollback-available";
constexpr const char kContextKeyIsRollbackChecked[] = "rollback-checked";
constexpr const char kContextKeyIsTPMFirmwareUpdateAvailable[] =
    "tpm-firmware-update-available";
constexpr const char kContextKeyIsTPMFirmwareUpdateChecked[] =
    "tpm-firmware-update-checked";
constexpr const char kContextKeyIsTPMFirmwareUpdateEditable[] =
    "tpm-firmware-update-editable";
constexpr const char kContextKeyTPMFirmwareUpdateMode[] =
    "tpm-firmware-update-mode";
constexpr const char kContextKeyIsConfirmational[] = "is-confirmational-view";
constexpr const char kContextKeyIsOfficialBuild[] = "is-official-build";
constexpr const char kContextKeyScreenState[] = "screen-state";

void StartTPMFirmwareUpdate(
    tpm_firmware_update::Mode requested_mode,
    const std::set<tpm_firmware_update::Mode>& available_modes) {
  if (available_modes.count(requested_mode) == 0) {
    // This should not happen, except for edge cases such as hijacked
    // UI, device policy changing while the dialog was up, etc.
    LOG(ERROR) << "Firmware update no longer available?";
    return;
  }

  std::string mode_string;
  switch (requested_mode) {
    case tpm_firmware_update::Mode::kNone:
      // Error handled below.
      break;
    case tpm_firmware_update::Mode::kPowerwash:
      mode_string = "first_boot";
      break;
    case tpm_firmware_update::Mode::kPreserveDeviceState:
      mode_string = "preserve_stateful";
      break;
  }

  if (mode_string.empty()) {
    LOG(ERROR) << "Invalid mode " << static_cast<int>(requested_mode);
    return;
  }

  DBusThreadManager::Get()->GetSessionManagerClient()->StartTPMFirmwareUpdate(
      mode_string);
}

}  // namespace

ResetScreen::ResetScreen(BaseScreenDelegate* base_screen_delegate,
                         ResetView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_OOBE_RESET),
      view_(view),
      weak_ptr_factory_(this) {
  DCHECK(view_);
  if (view_)
    view_->Bind(this);
  context_.SetInteger(kContextKeyScreenState, STATE_RESTART_REQUIRED);
  context_.SetBoolean(kContextKeyIsRollbackAvailable, false);
  context_.SetBoolean(kContextKeyIsRollbackChecked, false);
  context_.SetBoolean(kContextKeyIsTPMFirmwareUpdateAvailable, false);
  context_.SetBoolean(kContextKeyIsTPMFirmwareUpdateChecked, false);
  context_.SetBoolean(kContextKeyIsTPMFirmwareUpdateEditable, true);
  context_.SetInteger(kContextKeyTPMFirmwareUpdateMode,
                      static_cast<int>(tpm_firmware_update::Mode::kPowerwash));
  context_.SetBoolean(kContextKeyIsConfirmational, false);
  context_.SetBoolean(kContextKeyIsOfficialBuild, false);
#if defined(OFFICIAL_BUILD)
  context_.SetBoolean(kContextKeyIsOfficialBuild, true);
#endif
}

ResetScreen::~ResetScreen() {
  if (view_)
    view_->Unbind();
  DBusThreadManager::Get()->GetUpdateEngineClient()->RemoveObserver(this);
}

// static
void ResetScreen::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(prefs::kFactoryResetRequested, false);
  registry->RegisterIntegerPref(
      prefs::kFactoryResetTPMFirmwareUpdateMode,
      static_cast<int>(tpm_firmware_update::Mode::kNone));
}

void ResetScreen::Show() {
  if (view_)
    view_->Show();

  reset::DialogViewType dialog_type =
      reset::DIALOG_VIEW_TYPE_SIZE;  // used by UMA metrics.

  ContextEditor context_editor = GetContextEditor();

  bool restart_required = !base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kFirstExecAfterBoot);
  if (restart_required) {
    context_editor.SetInteger(kContextKeyScreenState, STATE_RESTART_REQUIRED);
    dialog_type = reset::DIALOG_SHORTCUT_RESTART_REQUIRED;
  } else {
    context_editor.SetInteger(kContextKeyScreenState, STATE_POWERWASH_PROPOSAL);
  }

  // Set availability of Rollback feature.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableRollbackOption)) {
    context_editor.SetBoolean(kContextKeyIsRollbackAvailable, false);
    dialog_type = reset::DIALOG_SHORTCUT_OFFERING_ROLLBACK_UNAVAILABLE;
  } else {
    chromeos::DBusThreadManager::Get()
        ->GetUpdateEngineClient()
        ->CanRollbackCheck(base::Bind(&ResetScreen::OnRollbackCheck,
                                      weak_ptr_factory_.GetWeakPtr()));
  }

  if (dialog_type < reset::DIALOG_VIEW_TYPE_SIZE) {
    UMA_HISTOGRAM_ENUMERATION("Reset.ChromeOS.PowerwashDialogShown",
                              dialog_type, reset::DIALOG_VIEW_TYPE_SIZE);
  }

  // Set availability of TPM firmware update.
  PrefService* prefs = g_browser_process->local_state();
  bool tpm_firmware_update_requested =
      prefs->HasPrefPath(prefs::kFactoryResetTPMFirmwareUpdateMode);
  if (tpm_firmware_update_requested) {
    // If an update has been requested previously, rely on the earlier update
    // availability test to initialize the dialog. This avoids a race condition
    // where the powerwash dialog gets shown immediately after reboot before the
    // init job to determine update availability has completed.
    context_editor.SetBoolean(kContextKeyIsTPMFirmwareUpdateAvailable, true);
    context_editor.SetInteger(
        kContextKeyTPMFirmwareUpdateMode,
        prefs->GetInteger(prefs::kFactoryResetTPMFirmwareUpdateMode));
  } else {
    // If a TPM firmware update hasn't previously been requested, check the
    // system to see whether to offer the checkbox to update TPM firmware. Note
    // that due to the asynchronous availability check, the decision might not
    // be available immediately, so set a timeout of a couple seconds.
    tpm_firmware_update::GetAvailableUpdateModes(
        base::BindOnce(&ResetScreen::OnTPMFirmwareUpdateAvailableCheck,
                       weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromSeconds(10));
  }

  context_editor.SetBoolean(kContextKeyIsTPMFirmwareUpdateChecked,
                            tpm_firmware_update_requested);
  context_editor.SetBoolean(kContextKeyIsTPMFirmwareUpdateEditable,
                            !tpm_firmware_update_requested);

  // Clear prefs so the reset screen isn't triggered again the next time the
  // device is about to show the login screen.
  prefs->ClearPref(prefs::kFactoryResetRequested);
  prefs->ClearPref(prefs::kFactoryResetTPMFirmwareUpdateMode);
  prefs->CommitPendingWrite();
}

void ResetScreen::Hide() {
  if (view_)
    view_->Hide();
}

void ResetScreen::OnViewDestroyed(ResetView* view) {
  if (view_ == view)
    view_ = nullptr;
}

void ResetScreen::OnUserAction(const std::string& action_id) {
  if (action_id == kUserActionCancelReset)
    OnCancel();
  else if (action_id == kUserActionResetRestartPressed)
    OnRestart();
  else if (action_id == kUserActionResetPowerwashPressed)
    OnPowerwash();
  else if (action_id == kUserActionResetLearnMorePressed)
    ShowHelpArticle(HelpAppLauncher::HELP_POWERWASH);
  else if (action_id == kUserActionResetRollbackToggled)
    OnToggleRollback();
  else if (action_id == kUserActionResetShowConfirmationPressed)
    OnShowConfirm();
  else if (action_id == kUserActionResetResetConfirmationDismissed)
    OnConfirmationDismissed();
  else if (action_id == kUserActionTPMFirmwareUpdateLearnMore)
    ShowHelpArticle(HelpAppLauncher::HELP_TPM_FIRMWARE_UPDATE);
  else
    BaseScreen::OnUserAction(action_id);
}

void ResetScreen::OnCancel() {
  if (context_.GetInteger(kContextKeyScreenState, STATE_RESTART_REQUIRED) ==
      STATE_REVERT_PROMISE)
    return;
  // Hide Rollback view for the next show.
  if (context_.GetBoolean(kContextKeyIsRollbackAvailable) &&
      context_.GetBoolean(kContextKeyIsRollbackChecked))
    OnToggleRollback();
  Finish(ScreenExitCode::RESET_CANCELED);
  DBusThreadManager::Get()->GetUpdateEngineClient()->RemoveObserver(this);
}

void ResetScreen::OnPowerwash() {
  if (context_.GetInteger(kContextKeyScreenState, 0) !=
      STATE_POWERWASH_PROPOSAL)
    return;

  GetContextEditor().SetBoolean(kContextKeyIsConfirmational, false);
  CommitContextChanges();

  if (context_.GetBoolean(kContextKeyIsRollbackChecked) &&
      !context_.GetBoolean(kContextKeyIsRollbackAvailable)) {
    NOTREACHED()
        << "Rollback was checked but not available. Starting powerwash.";
  }

  if (context_.GetBoolean(kContextKeyIsRollbackAvailable) &&
      context_.GetBoolean(kContextKeyIsRollbackChecked)) {
    GetContextEditor().SetInteger(kContextKeyScreenState, STATE_REVERT_PROMISE);
    DBusThreadManager::Get()->GetUpdateEngineClient()->AddObserver(this);
    VLOG(1) << "Starting Rollback";
    DBusThreadManager::Get()->GetUpdateEngineClient()->Rollback();
  } else if (context_.GetBoolean(kContextKeyIsTPMFirmwareUpdateChecked)) {
    VLOG(1) << "Starting TPM firmware update";
    // Re-check availability with a couple seconds timeout. This addresses the
    // case where the powerwash dialog gets shown immediately after reboot and
    // the decision on whether the update is available is not known immediately.
    tpm_firmware_update::GetAvailableUpdateModes(
        base::BindOnce(
            &StartTPMFirmwareUpdate,
            static_cast<tpm_firmware_update::Mode>(
                context_.GetInteger(kContextKeyTPMFirmwareUpdateMode))),
        base::TimeDelta::FromSeconds(10));
  } else {
    VLOG(1) << "Starting Powerwash";
    DBusThreadManager::Get()->GetSessionManagerClient()->StartDeviceWipe();
  }
}

void ResetScreen::OnRestart() {
  PrefService* prefs = g_browser_process->local_state();
  prefs->SetBoolean(prefs::kFactoryResetRequested, true);
  if (context_.GetBoolean(kContextKeyIsTPMFirmwareUpdateChecked)) {
    prefs->SetInteger(prefs::kFactoryResetTPMFirmwareUpdateMode,
                      static_cast<int>(tpm_firmware_update::Mode::kPowerwash));
  } else {
    prefs->ClearPref(prefs::kFactoryResetTPMFirmwareUpdateMode);
  }
  prefs->CommitPendingWrite();

  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RequestRestart(
      power_manager::REQUEST_RESTART_FOR_USER, "login reset screen restart");
}

void ResetScreen::OnToggleRollback() {
  // Hide Rollback if visible.
  if (context_.GetBoolean(kContextKeyIsRollbackAvailable) &&
      context_.GetBoolean(kContextKeyIsRollbackChecked)) {
    VLOG(1) << "Hiding rollback view on reset screen";
    GetContextEditor().SetBoolean(kContextKeyIsRollbackChecked, false);
    return;
  }

  // Show Rollback if available.
  VLOG(1) << "Requested rollback availability"
          << context_.GetBoolean(kContextKeyIsRollbackAvailable);
  if (context_.GetBoolean(kContextKeyIsRollbackAvailable) &&
      !context_.GetBoolean(kContextKeyIsRollbackChecked)) {
    UMA_HISTOGRAM_ENUMERATION(
        "Reset.ChromeOS.PowerwashDialogShown",
        reset::DIALOG_SHORTCUT_OFFERING_ROLLBACK_AVAILABLE,
        reset::DIALOG_VIEW_TYPE_SIZE);
    GetContextEditor().SetBoolean(kContextKeyIsRollbackChecked, true);
  }
}

void ResetScreen::OnShowConfirm() {
  reset::DialogViewType dialog_type =
      context_.GetBoolean(kContextKeyIsRollbackChecked)
          ? reset::DIALOG_SHORTCUT_CONFIRMING_POWERWASH_AND_ROLLBACK
          : reset::DIALOG_SHORTCUT_CONFIRMING_POWERWASH_ONLY;
  UMA_HISTOGRAM_ENUMERATION("Reset.ChromeOS.PowerwashDialogShown", dialog_type,
                            reset::DIALOG_VIEW_TYPE_SIZE);

  GetContextEditor().SetBoolean(kContextKeyIsConfirmational, true);
}

void ResetScreen::OnConfirmationDismissed() {
  GetContextEditor().SetBoolean(kContextKeyIsConfirmational, false);
}

void ResetScreen::ShowHelpArticle(HelpAppLauncher::HelpTopic topic) {
#if defined(OFFICIAL_BUILD)
  VLOG(1) << "Trying to view help article " << topic;
  if (!help_app_.get()) {
    help_app_ = new HelpAppLauncher(
        LoginDisplayHost::default_host()->GetNativeWindow());
  }
  help_app_->ShowHelpTopic(topic);
#endif
}

void ResetScreen::UpdateStatusChanged(
    const UpdateEngineClient::Status& status) {
  VLOG(1) << "Update status change to " << status.status;
  if (status.status == UpdateEngineClient::UPDATE_STATUS_ERROR ||
      status.status ==
          UpdateEngineClient::UPDATE_STATUS_REPORTING_ERROR_EVENT) {
    GetContextEditor().SetInteger(kContextKeyScreenState, STATE_ERROR);
    // Show error screen.
    GetErrorScreen()->SetUIState(NetworkError::UI_STATE_ROLLBACK_ERROR);
    get_base_screen_delegate()->ShowErrorScreen();
  } else if (status.status ==
             UpdateEngineClient::UPDATE_STATUS_UPDATED_NEED_REBOOT) {
    DBusThreadManager::Get()->GetPowerManagerClient()->RequestRestart(
        power_manager::REQUEST_RESTART_FOR_UPDATE, "login reset screen update");
  }
}

// Invoked from call to CanRollbackCheck upon completion of the DBus call.
void ResetScreen::OnRollbackCheck(bool can_rollback) {
  VLOG(1) << "Callback from CanRollbackCheck, result " << can_rollback;
  reset::DialogViewType dialog_type =
      can_rollback ? reset::DIALOG_SHORTCUT_OFFERING_ROLLBACK_AVAILABLE
                   : reset::DIALOG_SHORTCUT_OFFERING_ROLLBACK_UNAVAILABLE;
  UMA_HISTOGRAM_ENUMERATION("Reset.ChromeOS.PowerwashDialogShown", dialog_type,
                            reset::DIALOG_VIEW_TYPE_SIZE);

  GetContextEditor().SetBoolean(kContextKeyIsRollbackAvailable, can_rollback);
}

void ResetScreen::OnTPMFirmwareUpdateAvailableCheck(
    const std::set<tpm_firmware_update::Mode>& modes) {
  bool available = modes.count(tpm_firmware_update::Mode::kPowerwash) > 0;
  ContextEditor context_editor = GetContextEditor();
  context_editor.SetBoolean(kContextKeyIsTPMFirmwareUpdateAvailable, available);
  if (available) {
    context_editor.SetInteger(
        kContextKeyTPMFirmwareUpdateMode,
        static_cast<int>(tpm_firmware_update::Mode::kPowerwash));
  }
}

ErrorScreen* ResetScreen::GetErrorScreen() {
  return get_base_screen_delegate()->GetErrorScreen();
}

}  // namespace chromeos
