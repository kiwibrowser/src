// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_NETWORK_SCREEN_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_NETWORK_SCREEN_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/login/screens/base_screen.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chromeos/network/network_handler_callbacks.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "ui/base/ime/chromeos/input_method_manager.h"

namespace chromeos {

class InputEventsBlocker;
class NetworkView;
class ScreenManager;

namespace locale_util {
struct LanguageSwitchResult;
}

namespace login {
class NetworkStateHelper;
}

class NetworkScreen : public BaseScreen,
                      public NetworkStateHandlerObserver,
                      public input_method::InputMethodManager::Observer {
 public:
  class Delegate {
   public:
    virtual ~Delegate() {}

    // Called when enable debugging screen is requested.
    virtual void OnEnableDebuggingScreenRequested() = 0;
  };

  class Observer {
   public:
    virtual ~Observer() {}

    // Called when language list is reloaded.
    virtual void OnLanguageListReloaded() = 0;
  };

  NetworkScreen(BaseScreenDelegate* base_screen_delegate,
                Delegate* delegate,
                NetworkView* view);
  ~NetworkScreen() override;

  static NetworkScreen* Get(ScreenManager* manager);

  // Called when |view| has been destroyed. If this instance is destroyed before
  // the |view| it should call view->Unbind().
  void OnViewDestroyed(NetworkView* view);

  const std::string& language_list_locale() const {
    return language_list_locale_;
  }
  const base::ListValue* language_list() const { return language_list_.get(); }

  void UpdateLanguageList();

  // Set locale and input method. If |locale| is empty or doesn't change, set
  // the |input_method| directly. If |input_method| is empty or ineligible, we
  // don't change the current |input_method|.
  void SetApplicationLocaleAndInputMethod(const std::string& locale,
                                          const std::string& input_method);
  std::string GetApplicationLocale();
  std::string GetInputMethod() const;

  void SetTimezone(const std::string& timezone_id);
  std::string GetTimezone() const;

  // Currently We can only get unsecured Wifi network configuration from shark
  // that can be applied to remora. Returns the network ONC configuration.
  void GetConnectedWifiNetwork(std::string* out_onc_spec);
  void CreateAndConnectNetworkFromOnc(
      const std::string& onc_spec,
      const base::Closure& success_callback,
      const network_handler::ErrorCallback& error_callback);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  friend class NetworkScreenTest;
  friend class NetworkScreenUnitTest;
  FRIEND_TEST_ALL_PREFIXES(NetworkScreenTest, Timeout);
  FRIEND_TEST_ALL_PREFIXES(NetworkScreenTest, CanConnect);
  FRIEND_TEST_ALL_PREFIXES(NetworkScreenUnitTest, ContinuesAutomatically);
  FRIEND_TEST_ALL_PREFIXES(NetworkScreenUnitTest, ContinuesOnlyOnce);

  // BaseScreen implementation:
  void Show() override;
  void Hide() override;
  void OnUserAction(const std::string& action_id) override;
  void OnContextKeyUpdated(const ::login::ScreenContext::KeyType& key) override;

  // NetworkStateHandlerObserver implementation:
  void NetworkConnectionStateChanged(const NetworkState* network) override;
  void DefaultNetworkChanged(const NetworkState* network) override;

  // InputMethodManager::Observer implementation:
  void InputMethodChanged(input_method::InputMethodManager* manager,
                          Profile* profile,
                          bool show_message) override;

  void SetApplicationLocale(const std::string& locale);
  void SetInputMethod(const std::string& input_method);

  // Subscribe to timezone changes.
  void InitializeTimezoneObserver();

  // Subscribes NetworkScreen to the network change notification,
  // forces refresh of current network state.
  void Refresh();

  // Sets the NetworkStateHelper for use in tests. This
  // class will take ownership of the pointed object.
  void SetNetworkStateHelperForTest(login::NetworkStateHelper* helper);

  // Subscribes to network change notifications.
  void SubscribeNetworkNotification();

  // Unsubscribes from network change notifications.
  void UnsubscribeNetworkNotification();

  // Notifies wizard on successful connection.
  void NotifyOnConnection();

  // Called by |connection_timer_| when connection to the network timed out.
  void OnConnectionTimeout();

  // Update UI based on current network status.
  void UpdateStatus();

  // Stops waiting for network to connect.
  void StopWaitingForConnection(const base::string16& network_id);

  // Starts waiting for network connection. Shows spinner.
  void WaitForConnection(const base::string16& network_id);

  // Called when continue button is pressed.
  void OnContinueButtonPressed();

  // Async callback after ReloadResourceBundle(locale) completed.
  void OnLanguageChangedCallback(
      const InputEventsBlocker* input_events_blocker,
      const std::string& input_method,
      const locale_util::LanguageSwitchResult& result);

  // Starts resolving language list on BlockingPool.
  void ScheduleResolveLanguageList(
      std::unique_ptr<locale_util::LanguageSwitchResult>
          language_switch_result);

  // Callback for chromeos::ResolveUILanguageList() (from l10n_util).
  void OnLanguageListResolved(
      std::unique_ptr<base::ListValue> new_language_list,
      const std::string& new_language_list_locale,
      const std::string& new_selected_language);

  // Callback when the system timezone settings is changed.
  void OnSystemTimezoneChanged();

  // True if subscribed to network change notification.
  bool is_network_subscribed_ = false;

  // ID of the the network that we are waiting for.
  base::string16 network_id_;

  // Keeps track of the number of times OnContinueButtonPressed was called.
  // OnContinueButtonPressed is called either in response to the user
  // pressing the continue button, or automatically during hands-off enrollment
  // after a network connection is established.
  int continue_attempts_ = 0;

  // True if the user pressed the continue button in the UI.
  // Indicates that we should proceed with OOBE as soon as we are connected.
  bool continue_pressed_ = false;

  // Timer for connection timeout.
  base::OneShotTimer connection_timer_;

  std::unique_ptr<CrosSettings::ObserverSubscription> timezone_subscription_;

  NetworkView* view_ = nullptr;
  Delegate* delegate_ = nullptr;
  std::unique_ptr<login::NetworkStateHelper> network_state_helper_;

  std::string input_method_;
  std::string timezone_;

  // Creation of language list happens on Blocking Pool, so we cache
  // resolved data.
  std::string language_list_locale_;
  std::unique_ptr<base::ListValue> language_list_;

  // The exact language code selected by user in the menu.
  std::string selected_language_code_;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<NetworkScreen> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkScreen);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SCREENS_NETWORK_SCREEN_H_
