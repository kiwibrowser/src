// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screens/network_screen.h"

#include <utility>

#include "base/location.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/base/locale_util.h"
#include "chrome/browser/chromeos/customization/customization_document.h"
#include "chrome/browser/chromeos/login/help_app_launcher.h"
#include "chrome/browser/chromeos/login/helper.h"
#include "chrome/browser/chromeos/login/screen_manager.h"
#include "chrome/browser/chromeos/login/screens/base_screen_delegate.h"
#include "chrome/browser/chromeos/login/screens/network_view.h"
#include "chrome/browser/chromeos/login/ui/input_events_blocker.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/chromeos/system/timezone_util.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/webui/chromeos/login/l10n_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state_handler.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// Time in seconds for connection timeout.
const int kConnectionTimeoutSec = 40;

constexpr const char kUserActionContinueButtonClicked[] = "continue";
constexpr const char kUserActionConnectDebuggingFeaturesClicked[] =
    "connect-debugging-features";
constexpr const char kContextKeyLocale[] = "locale";
constexpr const char kContextKeyInputMethod[] = "input-method";
constexpr const char kContextKeyTimezone[] = "timezone";
constexpr const char kContextKeyContinueButtonEnabled[] =
    "continue-button-enabled";

}  // namespace

namespace chromeos {

///////////////////////////////////////////////////////////////////////////////
// NetworkScreen, public:

// static
NetworkScreen* NetworkScreen::Get(ScreenManager* manager) {
  return static_cast<NetworkScreen*>(
      manager->GetScreen(OobeScreen::SCREEN_OOBE_NETWORK));
}

NetworkScreen::NetworkScreen(BaseScreenDelegate* base_screen_delegate,
                             Delegate* delegate,
                             NetworkView* view)
    : BaseScreen(base_screen_delegate, OobeScreen::SCREEN_OOBE_NETWORK),
      view_(view),
      delegate_(delegate),
      network_state_helper_(new login::NetworkStateHelper),
      weak_factory_(this) {
  if (view_)
    view_->Bind(this);

  input_method::InputMethodManager::Get()->AddObserver(this);
  InitializeTimezoneObserver();
  OnSystemTimezoneChanged();
  UpdateLanguageList();
}

NetworkScreen::~NetworkScreen() {
  if (view_)
    view_->Unbind();
  connection_timer_.Stop();
  UnsubscribeNetworkNotification();

  input_method::InputMethodManager::Get()->RemoveObserver(this);
}

////////////////////////////////////////////////////////////////////////////////
// NetworkScreen, public API, setters and getters for input method and timezone.

void NetworkScreen::OnViewDestroyed(NetworkView* view) {
  if (view_ == view) {
    view_ = nullptr;
    timezone_subscription_.reset();
    // Ownership of NetworkScreen is complicated; ensure that we remove
    // this as a NetworkStateHandler observer when the view is destroyed.
    UnsubscribeNetworkNotification();
  }
}

void NetworkScreen::UpdateLanguageList() {
  ScheduleResolveLanguageList(
      std::unique_ptr<locale_util::LanguageSwitchResult>());
}

void NetworkScreen::SetApplicationLocaleAndInputMethod(
    const std::string& locale,
    const std::string& input_method) {
  const std::string& app_locale = g_browser_process->GetApplicationLocale();
  if (app_locale == locale || locale.empty()) {
    // If the locale doesn't change, set input method directly.
    SetInputMethod(input_method);
    return;
  }

  // Block UI while resource bundle is being reloaded.
  // (InputEventsBlocker will live until callback is finished.)
  locale_util::SwitchLanguageCallback callback(base::Bind(
      &NetworkScreen::OnLanguageChangedCallback, weak_factory_.GetWeakPtr(),
      base::Owned(new chromeos::InputEventsBlocker), input_method));
  locale_util::SwitchLanguage(locale, true /* enableLocaleKeyboardLayouts */,
                              true /* login_layouts_only */, callback,
                              ProfileManager::GetActiveUserProfile());
}

std::string NetworkScreen::GetApplicationLocale() {
  return g_browser_process->GetApplicationLocale();
}

std::string NetworkScreen::GetInputMethod() const {
  return input_method_;
}

void NetworkScreen::SetTimezone(const std::string& timezone_id) {
  if (timezone_id.empty())
    return;

  timezone_ = timezone_id;
  chromeos::system::SetSystemAndSigninScreenTimezone(timezone_id);
}

std::string NetworkScreen::GetTimezone() const {
  return timezone_;
}

void NetworkScreen::GetConnectedWifiNetwork(std::string* out_onc_spec) {
  // Currently We can only transfer unsecured WiFi configuration from shark to
  // remora. There is no way to get password for a secured Wifi network in Cros
  // for security reasons.
  network_state_helper_->GetConnectedWifiNetwork(out_onc_spec);
}

void NetworkScreen::CreateAndConnectNetworkFromOnc(
    const std::string& onc_spec,
    const base::Closure& success_callback,
    const network_handler::ErrorCallback& error_callback) {
  network_state_helper_->CreateAndConnectNetworkFromOnc(
      onc_spec, success_callback, error_callback);
}

void NetworkScreen::AddObserver(Observer* observer) {
  if (observer)
    observers_.AddObserver(observer);
}

void NetworkScreen::RemoveObserver(Observer* observer) {
  if (observer)
    observers_.RemoveObserver(observer);
}

////////////////////////////////////////////////////////////////////////////////
// BaseScreen implementation:

void NetworkScreen::Show() {
  Refresh();

  // Here we should handle default locales, for which we do not have UI
  // resources. This would load fallback, but properly show "selected" locale
  // in the UI.
  if (selected_language_code_.empty()) {
    const StartupCustomizationDocument* startup_manifest =
        StartupCustomizationDocument::GetInstance();
    SetApplicationLocale(startup_manifest->initial_locale_default());
  }

  if (!timezone_subscription_)
    InitializeTimezoneObserver();

  if (view_)
    view_->Show();
}

void NetworkScreen::Hide() {
  timezone_subscription_.reset();
  if (view_)
    view_->Hide();
}

void NetworkScreen::OnUserAction(const std::string& action_id) {
  if (action_id == kUserActionContinueButtonClicked) {
    OnContinueButtonPressed();
  } else if (action_id == kUserActionConnectDebuggingFeaturesClicked) {
    if (delegate_)
      delegate_->OnEnableDebuggingScreenRequested();
  } else {
    BaseScreen::OnUserAction(action_id);
  }
}

void NetworkScreen::OnContextKeyUpdated(
    const ::login::ScreenContext::KeyType& key) {
  if (key == kContextKeyLocale)
    SetApplicationLocale(context_.GetString(kContextKeyLocale));
  else if (key == kContextKeyInputMethod)
    SetInputMethod(context_.GetString(kContextKeyInputMethod));
  else if (key == kContextKeyTimezone)
    SetTimezone(context_.GetString(kContextKeyTimezone));
  else
    BaseScreen::OnContextKeyUpdated(key);
}

////////////////////////////////////////////////////////////////////////////////
// NetworkScreen, NetworkStateHandlerObserver implementation:

void NetworkScreen::NetworkConnectionStateChanged(const NetworkState* network) {
  UpdateStatus();
}

void NetworkScreen::DefaultNetworkChanged(const NetworkState* network) {
  UpdateStatus();
}

////////////////////////////////////////////////////////////////////////////////
// NetworkScreen, InputMethodManager::Observer implementation:

void NetworkScreen::InputMethodChanged(
    input_method::InputMethodManager* manager,
    Profile* /* proflie */,
    bool /* show_message */) {
  GetContextEditor().SetString(
      kContextKeyInputMethod,
      manager->GetActiveIMEState()->GetCurrentInputMethod().id());
}

////////////////////////////////////////////////////////////////////////////////
// NetworkScreen, private:

void NetworkScreen::SetApplicationLocale(const std::string& locale) {
  const std::string& app_locale = g_browser_process->GetApplicationLocale();
  if (app_locale == locale || locale.empty())
    return;

  // Block UI while resource bundle is being reloaded.
  // (InputEventsBlocker will live until callback is finished.)
  locale_util::SwitchLanguageCallback callback(base::Bind(
      &NetworkScreen::OnLanguageChangedCallback, weak_factory_.GetWeakPtr(),
      base::Owned(new chromeos::InputEventsBlocker), std::string()));
  locale_util::SwitchLanguage(locale, true /* enableLocaleKeyboardLayouts */,
                              true /* login_layouts_only */, callback,
                              ProfileManager::GetActiveUserProfile());
}

void NetworkScreen::SetInputMethod(const std::string& input_method) {
  const std::vector<std::string>& input_methods =
      input_method::InputMethodManager::Get()
          ->GetActiveIMEState()
          ->GetActiveInputMethodIds();
  if (input_method.empty() ||
      !base::ContainsValue(input_methods, input_method)) {
    LOG(WARNING) << "The input method is empty or ineligible!";
    return;
  }
  input_method_ = input_method;
  input_method::InputMethodManager::Get()
      ->GetActiveIMEState()
      ->ChangeInputMethod(input_method_, false /* show_message */);
}

void NetworkScreen::InitializeTimezoneObserver() {
  timezone_subscription_ = CrosSettings::Get()->AddSettingsObserver(
      kSystemTimezone, base::Bind(&NetworkScreen::OnSystemTimezoneChanged,
                                  base::Unretained(this)));
}

void NetworkScreen::Refresh() {
  SubscribeNetworkNotification();
  UpdateStatus();
}

void NetworkScreen::SetNetworkStateHelperForTest(
    login::NetworkStateHelper* helper) {
  network_state_helper_.reset(helper);
}

void NetworkScreen::SubscribeNetworkNotification() {
  if (!is_network_subscribed_) {
    is_network_subscribed_ = true;
    NetworkHandler::Get()->network_state_handler()->AddObserver(this,
                                                                FROM_HERE);
  }
}

void NetworkScreen::UnsubscribeNetworkNotification() {
  if (is_network_subscribed_) {
    is_network_subscribed_ = false;
    NetworkHandler::Get()->network_state_handler()->RemoveObserver(this,
                                                                   FROM_HERE);
  }
}

void NetworkScreen::NotifyOnConnection() {
  // TODO(nkostylev): Check network connectivity.
  UnsubscribeNetworkNotification();
  connection_timer_.Stop();
  Finish(ScreenExitCode::NETWORK_CONNECTED);
}

void NetworkScreen::OnConnectionTimeout() {
  StopWaitingForConnection(network_id_);
  if (!network_state_helper_->IsConnected() && view_) {
    // Show error bubble.
    view_->ShowError(l10n_util::GetStringFUTF16(
        IDS_NETWORK_SELECTION_ERROR,
        l10n_util::GetStringUTF16(IDS_SHORT_PRODUCT_OS_NAME), network_id_));
  }
}

void NetworkScreen::UpdateStatus() {
  if (!view_)
    return;

  bool is_connected = network_state_helper_->IsConnected();
  if (is_connected)
    view_->ClearErrors();

  base::string16 network_name = network_state_helper_->GetCurrentNetworkName();
  if (is_connected)
    StopWaitingForConnection(network_name);
  else if (network_state_helper_->IsConnecting())
    WaitForConnection(network_name);
  else
    StopWaitingForConnection(network_id_);
}

void NetworkScreen::StopWaitingForConnection(const base::string16& network_id) {
  bool is_connected = network_state_helper_->IsConnected();
  if (is_connected && continue_pressed_) {
    NotifyOnConnection();
    return;
  }

  continue_pressed_ = false;
  connection_timer_.Stop();

  network_id_ = network_id;
  if (view_)
    view_->ShowConnectingStatus(false, network_id_);

  GetContextEditor().SetBoolean(kContextKeyContinueButtonEnabled, is_connected);

  // Automatically continue if we are using Hands-Off Enrollment.
  if (is_connected && continue_attempts_ == 0 &&
      WizardController::UsingHandsOffEnrollment()) {
    OnContinueButtonPressed();
  }
}

void NetworkScreen::WaitForConnection(const base::string16& network_id) {
  if (network_id_ != network_id || !connection_timer_.IsRunning()) {
    connection_timer_.Stop();
    connection_timer_.Start(FROM_HERE,
                            base::TimeDelta::FromSeconds(kConnectionTimeoutSec),
                            this, &NetworkScreen::OnConnectionTimeout);
  }

  network_id_ = network_id;
  if (view_)
    view_->ShowConnectingStatus(continue_pressed_, network_id_);

  GetContextEditor().SetBoolean(kContextKeyContinueButtonEnabled, false);
}

void NetworkScreen::OnContinueButtonPressed() {
  ++continue_attempts_;
  if (view_) {
    view_->StopDemoModeDetection();
    view_->ClearErrors();
  }
  if (network_state_helper_->IsConnected()) {
    NotifyOnConnection();
  } else {
    continue_pressed_ = true;
    WaitForConnection(network_id_);
  }
}

void NetworkScreen::OnLanguageChangedCallback(
    const InputEventsBlocker* /* input_events_blocker */,
    const std::string& input_method,
    const locale_util::LanguageSwitchResult& result) {
  if (!selected_language_code_.empty()) {
    // We still do not have device owner, so owner settings are not applied.
    // But Guest session can be started before owner is created, so we need to
    // save locale settings directly here.
    g_browser_process->local_state()->SetString(prefs::kApplicationLocale,
                                                selected_language_code_);
  }
  ScheduleResolveLanguageList(
      std::unique_ptr<locale_util::LanguageSwitchResult>(
          new locale_util::LanguageSwitchResult(result)));

  AccessibilityManager::Get()->OnLocaleChanged();
  SetInputMethod(input_method);
}

void NetworkScreen::ScheduleResolveLanguageList(
    std::unique_ptr<locale_util::LanguageSwitchResult> language_switch_result) {
  UILanguageListResolvedCallback callback = base::Bind(
      &NetworkScreen::OnLanguageListResolved, weak_factory_.GetWeakPtr());
  ResolveUILanguageList(std::move(language_switch_result), callback);
}

void NetworkScreen::OnLanguageListResolved(
    std::unique_ptr<base::ListValue> new_language_list,
    const std::string& new_language_list_locale,
    const std::string& new_selected_language) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  language_list_ = std::move(new_language_list);
  language_list_locale_ = new_language_list_locale;
  selected_language_code_ = new_selected_language;

  g_browser_process->local_state()->SetString(prefs::kApplicationLocale,
                                              selected_language_code_);
  if (view_)
    view_->ReloadLocalizedContent();
  for (auto& observer : observers_)
    observer.OnLanguageListReloaded();
}

void NetworkScreen::OnSystemTimezoneChanged() {
  std::string current_timezone_id;
  CrosSettings::Get()->GetString(kSystemTimezone, &current_timezone_id);
  GetContextEditor().SetString(kContextKeyTimezone, current_timezone_id);
}

}  // namespace chromeos
