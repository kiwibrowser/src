// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/error_screen.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/app_mode/certificate_manager_dialog.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager.h"
#include "chrome/browser/chromeos/login/auth/chrome_login_performer.h"
#include "chrome/browser/chromeos/login/chrome_restart_request.h"
#include "chrome/browser/chromeos/login/screens/network_error_view.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/ui/captive_portal_window_proxy.h"
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#include "chrome/browser/chromeos/login/ui/webui_login_view.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "chrome/browser/extensions/component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager_client.h"
#include "chromeos/dbus/session_manager_client.h"
#include "chromeos/network/portal_detector/network_portal_detector.h"
#include "chromeos/network/portal_detector/network_portal_detector_strategy.h"
#include "components/session_manager/core/session_manager.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "third_party/cros_system_api/dbus/service_constants.h"
#include "ui/gfx/native_widget_types.h"

namespace chromeos {

namespace {

constexpr const char kContextKeyErrorStateCode[] = "error-state-code";
constexpr const char kContextKeyErrorStateNetwork[] = "error-state-network";
constexpr const char kContextKeyGuestSigninAllowed[] = "guest-signin-allowed";
constexpr const char kContextKeyOfflineSigninAllowed[] =
    "offline-signin-allowed";
constexpr const char kContextKeyShowConnectingIndicator[] =
    "show-connecting-indicator";
constexpr const char kContextKeyUIState[] = "ui-state";

// Returns the current running kiosk app profile in a kiosk session. Otherwise,
// returns nullptr.
Profile* GetAppProfile() {
  return chrome::IsRunningInForcedAppMode()
             ? ProfileManager::GetActiveUserProfile()
             : nullptr;
}

}  // namespace

constexpr const char ErrorScreen::kUserActionConfigureCertsButtonClicked[] =
    "configure-certs";
constexpr const char ErrorScreen::kUserActionDiagnoseButtonClicked[] =
    "diagnose";
constexpr const char ErrorScreen::kUserActionLaunchOobeGuestSessionClicked[] =
    "launch-oobe-guest";
constexpr const char
    ErrorScreen::kUserActionLocalStateErrorPowerwashButtonClicked[] =
        "local-state-error-powerwash";
constexpr const char ErrorScreen::kUserActionRebootButtonClicked[] = "reboot";
constexpr const char ErrorScreen::kUserActionShowCaptivePortalClicked[] =
    "show-captive-portal";
constexpr const char ErrorScreen::kUserActionConnectRequested[] =
    "connect-requested";

ErrorScreen::ErrorScreen(BaseScreenDelegate* base_screen_delegate,
                         NetworkErrorView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_ERROR_MESSAGE),
      view_(view),
      weak_factory_(this) {
  network_state_informer_ = new NetworkStateInformer();
  network_state_informer_->Init();
  if (view_)
    view_->Bind(this);
}

ErrorScreen::~ErrorScreen() {
  if (view_)
    view_->Unbind();
}

void ErrorScreen::AllowGuestSignin(bool allowed) {
  GetContextEditor().SetBoolean(kContextKeyGuestSigninAllowed, allowed);
}

void ErrorScreen::AllowOfflineLogin(bool allowed) {
  GetContextEditor().SetBoolean(kContextKeyOfflineSigninAllowed, allowed);
}

void ErrorScreen::FixCaptivePortal() {
  if (!captive_portal_window_proxy_.get()) {
    content::WebContents* web_contents =
        LoginDisplayHost::default_host()->GetWebUILoginView()->GetWebContents();
    captive_portal_window_proxy_.reset(new CaptivePortalWindowProxy(
        network_state_informer_.get(), web_contents));
  }
  captive_portal_window_proxy_->ShowIfRedirected();
}

NetworkError::UIState ErrorScreen::GetUIState() const {
  return ui_state_;
}

NetworkError::ErrorState ErrorScreen::GetErrorState() const {
  return error_state_;
}

OobeScreen ErrorScreen::GetParentScreen() const {
  return parent_screen_;
}

void ErrorScreen::HideCaptivePortal() {
  if (captive_portal_window_proxy_.get())
    captive_portal_window_proxy_->Close();
}

void ErrorScreen::OnViewDestroyed(NetworkErrorView* view) {
  if (view_ == view)
    view_ = nullptr;
}

void ErrorScreen::SetUIState(NetworkError::UIState ui_state) {
  ui_state_ = ui_state;
  GetContextEditor().SetInteger(kContextKeyUIState,
                                static_cast<int>(ui_state_));
}

void ErrorScreen::SetErrorState(NetworkError::ErrorState error_state,
                                const std::string& network) {
  error_state_ = error_state;
  GetContextEditor()
      .SetInteger(kContextKeyErrorStateCode, static_cast<int>(error_state_))
      .SetString(kContextKeyErrorStateNetwork, network);
}

void ErrorScreen::SetParentScreen(OobeScreen parent_screen) {
  parent_screen_ = parent_screen;
  // Not really used on JS side yet so no need to propagate to screen context.
}

void ErrorScreen::SetHideCallback(const base::Closure& on_hide) {
  on_hide_callback_.reset(new base::Closure(on_hide));
}

void ErrorScreen::ShowCaptivePortal() {
  // This call is an explicit user action
  // i.e. clicking on link so force dialog show.
  FixCaptivePortal();
  captive_portal_window_proxy_->Show();
}

void ErrorScreen::ShowConnectingIndicator(bool show) {
  GetContextEditor().SetBoolean(kContextKeyShowConnectingIndicator, show);
}

ErrorScreen::ConnectRequestCallbackSubscription
ErrorScreen::RegisterConnectRequestCallback(const base::Closure& callback) {
  return connect_request_callbacks_.Add(callback);
}

void ErrorScreen::Show() {
  if (!on_hide_callback_) {
    SetHideCallback(base::Bind(&ErrorScreen::DefaultHideCallback,
                               weak_factory_.GetWeakPtr()));
  }
  if (view_)
    view_->Show();
}

void ErrorScreen::Hide() {
  if (view_)
    view_->Hide();
}

void ErrorScreen::OnShow() {
  LOG(WARNING) << "Network error screen message is shown";
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_LOGIN_NETWORK_ERROR_SHOWN,
      content::NotificationService::AllSources(),
      content::NotificationService::NoDetails());
  network_portal_detector::GetInstance()->SetStrategy(
      PortalDetectorStrategy::STRATEGY_ID_ERROR_SCREEN);
}

void ErrorScreen::OnHide() {
  LOG(WARNING) << "Network error screen message is hidden";
  if (on_hide_callback_) {
    on_hide_callback_->Run();
    on_hide_callback_.reset();
  }
  network_portal_detector::GetInstance()->SetStrategy(
      PortalDetectorStrategy::STRATEGY_ID_LOGIN_SCREEN);
}

void ErrorScreen::OnUserAction(const std::string& action_id) {
  if (action_id == kUserActionShowCaptivePortalClicked)
    ShowCaptivePortal();
  else if (action_id == kUserActionConfigureCertsButtonClicked)
    OnConfigureCerts();
  else if (action_id == kUserActionDiagnoseButtonClicked)
    OnDiagnoseButtonClicked();
  else if (action_id == kUserActionLaunchOobeGuestSessionClicked)
    OnLaunchOobeGuestSession();
  else if (action_id == kUserActionLocalStateErrorPowerwashButtonClicked)
    OnLocalStateErrorPowerwashButtonClicked();
  else if (action_id == kUserActionRebootButtonClicked)
    OnRebootButtonClicked();
  else if (action_id == kUserActionConnectRequested)
    OnConnectRequested();
  else
    BaseScreen::OnUserAction(action_id);
}

void ErrorScreen::OnAuthFailure(const AuthFailure& error) {
  // The only condition leading here is guest mount failure, which should not
  // happen in practice. For now, just log an error so this situation is visible
  // in logs if it ever occurs.
  NOTREACHED() << "Guest login failed.";
  guest_login_performer_.reset();
}

void ErrorScreen::OnAuthSuccess(const UserContext& user_context) {
  LOG(FATAL);
}

void ErrorScreen::OnOffTheRecordAuthSuccess() {
  // Restart Chrome to enter the guest session.
  const base::CommandLine& browser_command_line =
      *base::CommandLine::ForCurrentProcess();
  base::CommandLine command_line(browser_command_line.GetProgram());
  GetOffTheRecordCommandLine(GURL(), StartupUtils::IsOobeCompleted(),
                             browser_command_line, &command_line);
  RestartChrome(command_line);
}

void ErrorScreen::OnPasswordChangeDetected() {
  LOG(FATAL);
}

void ErrorScreen::WhiteListCheckFailed(const std::string& email) {
  LOG(FATAL);
}

void ErrorScreen::PolicyLoadFailed() {
  LOG(FATAL);
}

void ErrorScreen::SetAuthFlowOffline(bool offline) {
  LOG(FATAL);
}

void ErrorScreen::DefaultHideCallback() {
  if (parent_screen_ != OobeScreen::SCREEN_UNKNOWN && view_)
    view_->ShowOobeScreen(parent_screen_);

  // TODO(antrim): Due to potential race with GAIA reload and hiding network
  // error UI we can't just reset parent screen to SCREEN_UNKNOWN here.
}

void ErrorScreen::OnConfigureCerts() {
  gfx::NativeWindow native_window =
      LoginDisplayHost::default_host()->GetNativeWindow();
  CertificateManagerDialog* dialog =
      new CertificateManagerDialog(GetAppProfile(), NULL, native_window);
  dialog->Show();
}

void ErrorScreen::OnDiagnoseButtonClicked() {
  Profile* profile = GetAppProfile();
  ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(profile)->extension_service();

  std::string extension_id = extension_service->component_loader()->Add(
      IDR_CONNECTIVITY_DIAGNOSTICS_MANIFEST,
      base::FilePath(extension_misc::kConnectivityDiagnosticsPath));

  const extensions::Extension* extension =
      extension_service->GetExtensionById(extension_id, true);
  OpenApplication(AppLaunchParams(
      profile, extension, extensions::LAUNCH_CONTAINER_WINDOW,
      WindowOpenDisposition::NEW_WINDOW, extensions::SOURCE_CHROME_INTERNAL));
  KioskAppManager::Get()->InitSession(profile, extension_id);

  LoginDisplayHost::default_host()->Finalize(base::BindOnce(
      [] { session_manager::SessionManager::Get()->SessionStarted(); }));
}

void ErrorScreen::OnLaunchOobeGuestSession() {
  DeviceSettingsService::Get()->GetOwnershipStatusAsync(
      base::Bind(&ErrorScreen::StartGuestSessionAfterOwnershipCheck,
                 weak_factory_.GetWeakPtr()));
}

void ErrorScreen::OnLocalStateErrorPowerwashButtonClicked() {
  chromeos::DBusThreadManager::Get()
      ->GetSessionManagerClient()
      ->StartDeviceWipe();
}

void ErrorScreen::OnRebootButtonClicked() {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RequestRestart(
      power_manager::REQUEST_RESTART_FOR_USER, "login error screen");
}

void ErrorScreen::OnConnectRequested() {
  connect_request_callbacks_.Notify();
}

void ErrorScreen::StartGuestSessionAfterOwnershipCheck(
    DeviceSettingsService::OwnershipStatus ownership_status) {
  // Make sure to disallow guest login if it's explicitly disabled.
  CrosSettingsProvider::TrustedStatus trust_status =
      CrosSettings::Get()->PrepareTrustedValues(
          base::Bind(&ErrorScreen::StartGuestSessionAfterOwnershipCheck,
                     weak_factory_.GetWeakPtr(), ownership_status));
  switch (trust_status) {
    case CrosSettingsProvider::TEMPORARILY_UNTRUSTED:
      // Wait for a callback.
      return;
    case CrosSettingsProvider::PERMANENTLY_UNTRUSTED:
      // Only allow guest sessions if there is no owner yet.
      if (ownership_status == DeviceSettingsService::OWNERSHIP_NONE)
        break;
      return;
    case CrosSettingsProvider::TRUSTED: {
      // Honor kAccountsPrefAllowGuest.
      bool allow_guest = false;
      CrosSettings::Get()->GetBoolean(kAccountsPrefAllowGuest, &allow_guest);
      if (allow_guest)
        break;
      return;
    }
  }

  if (guest_login_performer_)
    return;

  guest_login_performer_.reset(new ChromeLoginPerformer(this));
  guest_login_performer_->LoginOffTheRecord();
}

}  // namespace chromeos
