// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_AUTHPOLICY_AUTH_POLICY_CREDENTIALS_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_AUTHPOLICY_AUTH_POLICY_CREDENTIALS_MANAGER_H_

#include <set>

#include "base/cancelable_callback.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/dbus/authpolicy/active_directory_info.pb.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "components/account_id/account_id.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_member.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

class Profile;

namespace authpolicy {
class ActiveDirectoryUserStatus;
}

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace dbus {
class Signal;
}

namespace chromeos {

// Kerberos defaults for canonicalization SPN. (see
// https://web.mit.edu/kerberos/krb5-1.12/doc/admin/conf_files/krb5_conf.html)
// Exported for browsertests.
extern const char* kKrb5CnameSettings;

// A service responsible for tracking user credential status. Created for each
// Active Directory user profile.
class AuthPolicyCredentialsManager
    : public KeyedService,
      public chromeos::NetworkStateHandlerObserver {
 public:
  explicit AuthPolicyCredentialsManager(Profile* profile);
  ~AuthPolicyCredentialsManager() override;

  // KeyedService overrides.
  void Shutdown() override;

  // chromeos::NetworkStateHandlerObserver overrides.
  void DefaultNetworkChanged(const chromeos::NetworkState* network) override;
  void NetworkConnectionStateChanged(
      const chromeos::NetworkState* network) override;
  void OnShuttingDown() override;

 private:
  friend class AuthPolicyCredentialsManagerTest;
  // Calls AuthPolicyClient::GetUserStatus method.
  void GetUserStatus();

  // See AuthPolicyClient::GetUserStatusCallback.
  void OnGetUserStatusCallback(
      authpolicy::ErrorType error,
      const authpolicy::ActiveDirectoryUserStatus& user_status);

  // Calls AuthPolicyClient::GetUserKerberosFiles.
  void GetUserKerberosFiles();

  // See AuthPolicyClient::GetUserKerberosFilesCallback.
  void OnGetUserKerberosFilesCallback(
      authpolicy::ErrorType error,
      const authpolicy::KerberosFiles& kerberos_files);

  // Post delayed task to call GetUserStatus in the future.
  void ScheduleGetUserStatus();

  // Add itself as network observer.
  void StartObserveNetwork();
  // Remove itself as network observer.
  void StopObserveNetwork();

  // Update display and given name in case it has changed.
  void UpdateDisplayAndGivenName(
      const authpolicy::ActiveDirectoryAccountInfo& account_info);

  // Shows user notification to sign out/sign in.
  void ShowNotification(int message_id);

  // Call GetUserStatus if |network_state| is connected and the previous call
  // failed.
  void GetUserStatusIfConnected(const chromeos::NetworkState* network_state);

  // Callback for 'UserKerberosFilesChanged' D-Bus signal sent by authpolicyd.
  void OnUserKerberosFilesChangedCallback(dbus::Signal* signal);

  // Called after connected to 'UserKerberosFilesChanged' signal.
  void OnSignalConnectedCallback(const std::string& interface_name,
                                 const std::string& signal_name,
                                 bool success);

  // Called whenever prefs::kDisableAuthNegotiateCnameLookup is changed.
  void OnDisabledAuthNegotiateCnameLookupChanged();

  Profile* const profile_;
  AccountId account_id_;
  std::string display_name_;
  std::string given_name_;
  bool is_get_status_in_progress_ = false;
  bool rerun_get_status_on_error_ = false;
  bool is_observing_network_ = false;

  // Stores message ids of shown notifications. Each notification is shown at
  // most once.
  std::set<int> shown_notifications_;
  authpolicy::ErrorType last_error_ = authpolicy::ERROR_NONE;
  base::CancelableClosure scheduled_get_user_status_call_;
  PrefMember<bool> negotiate_disable_cname_lookup_;

  base::WeakPtrFactory<AuthPolicyCredentialsManager> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(AuthPolicyCredentialsManager);
};

// Singleton that owns all AuthPolicyCredentialsManagers and associates them
// with BrowserContexts.
class AuthPolicyCredentialsManagerFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static AuthPolicyCredentialsManagerFactory* GetInstance();

  // Returns nullptr in case profile is not Active Directory. Otherwise returns
  // valid AuthPolicyCredentialsManager. Lifetime is managed by
  // BrowserContextKeyedServiceFactory.
  static KeyedService* BuildForProfileIfActiveDirectory(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<
      AuthPolicyCredentialsManagerFactory>;

  AuthPolicyCredentialsManagerFactory();
  ~AuthPolicyCredentialsManagerFactory() override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(AuthPolicyCredentialsManagerFactory);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_AUTHPOLICY_AUTH_POLICY_CREDENTIALS_MANAGER_H_
