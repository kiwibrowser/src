// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_LOGIN_AUTH_AUTHPOLICY_LOGIN_HELPER_H_
#define CHROMEOS_LOGIN_AUTH_AUTHPOLICY_LOGIN_HELPER_H_

#include <string>

#include "base/callback.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/dbus/auth_policy_client.h"

namespace chromeos {

// Helper class to use AuthPolicyClient. For Active Directory domain join and
// authenticate users this class should be used instead of AuthPolicyClient.
// Allows canceling all pending calls and restarting AuthPolicy service. Used
// for enrollment and login UI to proper cancel the flows.
class CHROMEOS_EXPORT AuthPolicyLoginHelper {
 public:
  using AuthCallback = AuthPolicyClient::AuthCallback;
  using JoinCallback = AuthPolicyClient::JoinCallback;

  AuthPolicyLoginHelper();
  ~AuthPolicyLoginHelper();

  // Try to get Kerberos TGT. To get an error code of this call one should use
  // last_auth_error_ returned from AuthPolicyClient::GetUserStatus afterwards.
  // (see https://crbug.com/710452).
  static void TryAuthenticateUser(const std::string& username,
                                  const std::string& object_guid,
                                  const std::string& password);

  // Restarts AuthPolicy service.
  static void Restart();

  // Checks if device is locked for Active Directory management.
  static bool IsAdLocked();

  // Sets install attributes for Active Directory managed device. Persists it on
  // disk.
  static bool LockDeviceActiveDirectoryForTesting(const std::string& realm);

  // Packs arguments and calls AuthPolicyClient::JoinAdDomain. Joins machine to
  // Active directory domain. Then it calls RefreshDevicePolicy to cache the
  // policy on the authpolicyd side. |machine_name| is a name for a local
  // machine. If |distinguished_name| is not empty |machine| would be put into
  // that domain or/and organizational unit structure. Otherwise |machine| would
  // be joined to domain of the |username|. |username|, |password| are
  // credentials of the Active directory account which has right to join the
  // machine to the domain. |callback| is called after getting (or failing to
  // get) D-BUS response.
  void JoinAdDomain(const std::string& machine_name,
                    const std::string& distinguished_name,
                    int encryption_types,
                    const std::string& username,
                    const std::string& password,
                    JoinCallback callback);

  // Packs arguments and calls AuthPolicyClient::AuthenticateUser. Authenticates
  // user against Active Directory server. |username|, |password| are
  // credentials of the Active Directory account. |username| should be in the
  // user@example.domain.com format. |object_guid| is the user's LDAP GUID. If
  // specified, it is used instead of |username|. The GUID is guaranteed to be
  // stable, the user's name can change on the server.
  void AuthenticateUser(const std::string& username,
                        const std::string& object_guid,
                        const std::string& password,
                        AuthCallback callback);

  // Cancel pending requests and restarts AuthPolicy service.
  void CancelRequestsAndRestart();

  // Sets the DM token. Will be sent to authpolicy with the domain join call.
  // Authpolicy would set it in the device policy.
  void set_dm_token(const std::string& dm_token) { dm_token_ = dm_token; }

 private:
  // Called from AuthPolicyClient::JoinAdDomain.
  void OnJoinCallback(JoinCallback callback,
                      authpolicy::ErrorType error,
                      const std::string& machine_domain);

  // Called from AuthPolicyClient::RefreshDevicePolicy. This is used only once
  // during device enrollment with the first device policy refresh.
  void OnFirstPolicyRefreshCallback(JoinCallback callback,
                                    const std::string& machine_domain,
                                    authpolicy::ErrorType error);

  // Called from AuthPolicyClient::AuthenticateUser.
  void OnAuthCallback(
      AuthCallback callback,
      authpolicy::ErrorType error,
      const authpolicy::ActiveDirectoryAccountInfo& account_info);

  std::string dm_token_;

  base::WeakPtrFactory<AuthPolicyLoginHelper> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AuthPolicyLoginHelper);
};

}  // namespace chromeos

#endif  // CHROMEOS_LOGIN_AUTH_AUTHPOLICY_LOGIN_HELPER_H_
