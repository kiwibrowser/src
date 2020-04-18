// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_AUTH_POLICY_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_AUTH_POLICY_CLIENT_H_

#include <string>
#include <utility>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"

#include "chromeos/dbus/auth_policy_client.h"
#include "chromeos/dbus/session_manager_client.h"
#include "components/policy/proto/chrome_device_policy.pb.h"
#include "components/policy/proto/device_management_backend.pb.h"

class AccountId;

namespace chromeos {

class CHROMEOS_EXPORT FakeAuthPolicyClient : public AuthPolicyClient {
 public:
  FakeAuthPolicyClient();
  ~FakeAuthPolicyClient() override;

  // DBusClient overrides.
  void Init(dbus::Bus* bus) override;
  // AuthPolicyClient overrides.

  // Performs basic checks on |request.machine_name| and
  // |request.user_principal_name|. Could fail with ERROR_MACHINE_NAME_TOO_LONG,
  // ERROR_INVALID_MACHINE_NAME or ERROR_PARSE_UPN_FAILED. Otherwise succeeds.
  void JoinAdDomain(const authpolicy::JoinDomainRequest& request,
                    int password_fd,
                    JoinCallback callback) override;

  // Runs |callback| with |auth_error_|.
  void AuthenticateUser(const authpolicy::AuthenticateUserRequest& request,
                        int password_fd,
                        AuthCallback callback) override;

  // Runs |callback| with |password_status_| and |tgt_status_|. Also calls
  // |on_get_status_closure_| after that.
  void GetUserStatus(const authpolicy::GetUserStatusRequest& request,
                     GetUserStatusCallback callback) override;

  // Runs |callback| with Kerberos files.
  void GetUserKerberosFiles(const std::string& object_guid,
                            GetUserKerberosFilesCallback callback) override;

  // Writes device policy file and runs callback.
  void RefreshDevicePolicy(RefreshPolicyCallback callback) override;

  // Writes user policy file and runs callback.
  void RefreshUserPolicy(const AccountId& account_id,
                         RefreshPolicyCallback callback) override;

  // Runs |on_connected_callback| with success. Then runs |signal_callback|
  // once.
  void ConnectToSignal(
      const std::string& signal_name,
      dbus::ObjectProxy::SignalCallback signal_callback,
      dbus::ObjectProxy::OnConnectedCallback on_connected_callback) override;

  // Mark service as started. It's getting started by the
  // UpstartClient::StartAuthPolicyService on the Active Directory managed
  // devices.
  void set_started(bool started) { started_ = started; }

  bool started() const { return started_; }

  void set_auth_error(authpolicy::ErrorType auth_error) {
    auth_error_ = auth_error;
  }

  void set_display_name(const std::string& display_name) {
    display_name_ = display_name;
  }

  void set_given_name(const std::string& given_name) {
    given_name_ = given_name;
  }

  void set_password_status(
      authpolicy::ActiveDirectoryUserStatus::PasswordStatus password_status) {
    password_status_ = password_status;
  }

  void set_tgt_status(
      authpolicy::ActiveDirectoryUserStatus::TgtStatus tgt_status) {
    tgt_status_ = tgt_status;
  }

  void set_on_get_status_closure(base::OnceClosure on_get_status_closure) {
    on_get_status_closure_ = std::move(on_get_status_closure);
  }

  void set_device_policy(
      const enterprise_management::ChromeDeviceSettingsProto& device_policy) {
    device_policy_ = device_policy;
  }

  void DisableOperationDelayForTesting() {
    dbus_operation_delay_ = disk_operation_delay_ =
        base::TimeDelta::FromSeconds(0);
  }

 protected:
  authpolicy::ErrorType auth_error_ = authpolicy::ERROR_NONE;

 private:
  void OnDevicePolicyRetrieved(
      RefreshPolicyCallback callback,
      SessionManagerClient::RetrievePolicyResponseType response_type,
      const std::string& protobuf);
  bool started_ = false;
  // If valid called after GetUserStatusCallback is called.
  base::OnceClosure on_get_status_closure_;
  std::string display_name_;
  std::string given_name_;
  std::string machine_name_;
  std::string dm_token_;
  authpolicy::ActiveDirectoryUserStatus::PasswordStatus password_status_ =
      authpolicy::ActiveDirectoryUserStatus::PASSWORD_VALID;
  authpolicy::ActiveDirectoryUserStatus::TgtStatus tgt_status_ =
      authpolicy::ActiveDirectoryUserStatus::TGT_VALID;
  // Delay operations to be more realistic.
  base::TimeDelta dbus_operation_delay_ = base::TimeDelta::FromSeconds(3);
  base::TimeDelta disk_operation_delay_ =
      base::TimeDelta::FromMilliseconds(100);
  enterprise_management::ChromeDeviceSettingsProto device_policy_;

  base::WeakPtrFactory<FakeAuthPolicyClient> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(FakeAuthPolicyClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_AUTH_POLICY_CLIENT_H_
