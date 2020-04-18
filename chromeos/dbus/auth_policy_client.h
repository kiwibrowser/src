// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_AUTH_POLICY_CLIENT_H_
#define CHROMEOS_DBUS_AUTH_POLICY_CLIENT_H_

#include <string>

#include "base/callback.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/authpolicy/active_directory_info.pb.h"
#include "chromeos/dbus/dbus_client.h"
#include "dbus/object_proxy.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

class AccountId;

namespace chromeos {

// AuthPolicyClient is used to communicate with the org.chromium.AuthPolicy
// sevice. All method should be called from the origin thread (UI thread) which
// initializes the DBusThreadManager instance.
class CHROMEOS_EXPORT AuthPolicyClient : public DBusClient {
 public:
  using AuthCallback = base::OnceCallback<void(
      authpolicy::ErrorType error,
      const authpolicy::ActiveDirectoryAccountInfo& account_info)>;
  using GetUserStatusCallback = base::OnceCallback<void(
      authpolicy::ErrorType error,
      const authpolicy::ActiveDirectoryUserStatus& user_status)>;
  using GetUserKerberosFilesCallback =
      base::OnceCallback<void(authpolicy::ErrorType error,
                              const authpolicy::KerberosFiles& kerberos_files)>;
  using JoinCallback =
      base::OnceCallback<void(authpolicy::ErrorType error,
                              const std::string& machine_domain)>;
  using RefreshPolicyCallback =
      base::OnceCallback<void(authpolicy::ErrorType error)>;

  ~AuthPolicyClient() override;

  // Factory function, creates a new instance and returns ownership.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static AuthPolicyClient* Create();

  // Calls JoinADDomain to join a machine/device to an Active Directory domain.
  // Password is read from the |password_fd|. |callback| is called after getting
  // (or failing to get) D-BUS response.
  virtual void JoinAdDomain(const authpolicy::JoinDomainRequest& request,
                            int password_fd,
                            JoinCallback callback) = 0;

  // Calls AuthenticateUser to authenticate a user against Active Directory.
  // Password is read from the |password_fd|. |callback| is called after getting
  // (or failing to get) D-BUS response.
  virtual void AuthenticateUser(
      const authpolicy::AuthenticateUserRequest& request,
      int password_fd,
      AuthCallback callback) = 0;

  // Calls GetUserStatus. If Active Directory server is online it fetches
  // ActiveDirectoryUserStatus for the user specified by |request|.
  // |callback| is called after getting (or failing to get) D-Bus response.
  virtual void GetUserStatus(const authpolicy::GetUserStatusRequest& request,
                             GetUserStatusCallback callback) = 0;

  // Calls GetUserKerberosFiles. If authpolicyd has Kerberos files for the user
  // specified by |object_guid| it sends them in response: credentials cache and
  // krb5 config files.
  virtual void GetUserKerberosFiles(const std::string& object_guid,
                                    GetUserKerberosFilesCallback callback) = 0;

  // Calls RefreshDevicePolicy - handle policy for the device.
  // Fetch GPO files from Active directory server, parse it, encode it into
  // protobuf and send to SessionManager. Callback is called after that.
  virtual void RefreshDevicePolicy(RefreshPolicyCallback callback) = 0;

  // Calls RefreshUserPolicy - handle policy for the user specified by
  // |account_id|. Similar to RefreshDevicePolicy.
  virtual void RefreshUserPolicy(const AccountId& account_id,
                                 RefreshPolicyCallback callback) = 0;

  // Connects callbacks to D-Bus signal |signal_name| sent by authpolicyd.
  virtual void ConnectToSignal(
      const std::string& signal_name,
      dbus::ObjectProxy::SignalCallback signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) = 0;

 protected:
  // Create() should be used instead.
  AuthPolicyClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(AuthPolicyClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_AUTH_POLICY_CLIENT_H_
