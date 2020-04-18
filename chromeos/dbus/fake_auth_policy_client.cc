// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_auth_policy_client.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/md5.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/chromeos_paths.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "chromeos/login/auth/authpolicy_login_helper.h"
#include "components/account_id/account_id.h"
#include "components/policy/proto/cloud_policy.pb.h"
#include "dbus/message.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace em = enterprise_management;

namespace {

constexpr size_t kMaxMachineNameLength = 15;
constexpr char kInvalidMachineNameCharacters[] = "\\/:*?\"<>|";

void OnStorePolicy(chromeos::AuthPolicyClient::RefreshPolicyCallback callback,
                   bool success) {
  const authpolicy::ErrorType error =
      success ? authpolicy::ERROR_NONE : authpolicy::ERROR_STORE_POLICY_FAILED;
  std::move(callback).Run(error);
}

// Posts |closure| on the ThreadTaskRunner with |delay|.
void PostDelayedClosure(base::OnceClosure closure,
                        const base::TimeDelta& delay) {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, std::move(closure), delay);
}

// Runs |signal_callback| with Signal*. Needed to own Signal object.
void RunSignalCallback(const std::string& interface_name,
                       const std::string& method_name,
                       dbus::ObjectProxy::SignalCallback signal_callback) {
  signal_callback.Run(
      std::make_unique<dbus::Signal>(interface_name, method_name).get());
}

void StoreDevicePolicy(
    const em::ChromeDeviceSettingsProto& device_policy,
    const std::string& machine_name,
    const std::string& dm_token,
    chromeos::AuthPolicyClient::RefreshPolicyCallback callback) {
  std::string payload;
  CHECK(device_policy.SerializeToString(&payload));

  em::PolicyFetchResponse response;
  em::PolicyData policy_data;
  policy_data.set_policy_type("google/chromeos/device");
  policy_data.set_device_id(machine_name);
  policy_data.set_request_token(dm_token);
  policy_data.set_policy_value(payload);
  policy_data.set_timestamp(base::Time::Now().ToJavaTime());
  response.set_policy_data(policy_data.SerializeAsString());
  chromeos::SessionManagerClient* session_manager_client =
      chromeos::DBusThreadManager::Get()->GetSessionManagerClient();
  session_manager_client->StoreDevicePolicy(
      response.SerializeAsString(),
      base::BindOnce(&OnStorePolicy, std::move(callback)));
}

}  // namespace

namespace chromeos {

FakeAuthPolicyClient::FakeAuthPolicyClient() = default;

FakeAuthPolicyClient::~FakeAuthPolicyClient() = default;

void FakeAuthPolicyClient::Init(dbus::Bus* bus) {}

void FakeAuthPolicyClient::JoinAdDomain(
    const authpolicy::JoinDomainRequest& request,
    int password_fd,
    JoinCallback callback) {
  DCHECK(!AuthPolicyLoginHelper::IsAdLocked());
  authpolicy::ErrorType error = authpolicy::ERROR_NONE;
  std::string machine_domain;
  if (!started_) {
    LOG(ERROR) << "authpolicyd not started";
    error = authpolicy::ERROR_DBUS_FAILURE;
  } else if (request.machine_name().size() > kMaxMachineNameLength) {
    error = authpolicy::ERROR_MACHINE_NAME_TOO_LONG;
  } else if (request.machine_name().empty() ||
             request.machine_name().find_first_of(
                 kInvalidMachineNameCharacters) != std::string::npos) {
    error = authpolicy::ERROR_INVALID_MACHINE_NAME;
  } else if (request.kerberos_encryption_types() ==
             authpolicy::KerberosEncryptionTypes::ENC_TYPES_LEGACY) {
    // Pretend that server does not support legacy types.
    error = authpolicy::ERROR_KDC_DOES_NOT_SUPPORT_ENCRYPTION_TYPE;
  } else {
    std::vector<std::string> parts =
        base::SplitString(request.user_principal_name(), "@",
                          base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (parts.size() != 2 || parts[0].empty() || parts[1].empty()) {
      error = authpolicy::ERROR_PARSE_UPN_FAILED;
    } else {
      machine_domain = parts[1];
    }
  }

  if (error == authpolicy::ERROR_NONE)
    machine_name_ = request.machine_name();
  if (error != authpolicy::ERROR_NONE)
    machine_domain.clear();
  else if (request.has_machine_domain() && !request.machine_domain().empty())
    machine_domain = request.machine_domain();
  PostDelayedClosure(base::BindOnce(std::move(callback), error, machine_domain),
                     dbus_operation_delay_);
}

void FakeAuthPolicyClient::AuthenticateUser(
    const authpolicy::AuthenticateUserRequest& request,
    int password_fd,
    AuthCallback callback) {
  DCHECK(AuthPolicyLoginHelper::IsAdLocked());
  authpolicy::ErrorType error = authpolicy::ERROR_NONE;
  authpolicy::ActiveDirectoryAccountInfo account_info;
  if (!started_) {
    LOG(ERROR) << "authpolicyd not started";
    error = authpolicy::ERROR_DBUS_FAILURE;
  } else {
    if (auth_error_ == authpolicy::ERROR_NONE) {
      if (request.account_id().empty())
        account_info.set_account_id(
            base::MD5String(request.user_principal_name()));
      else
        account_info.set_account_id(request.account_id());
    }
    error = auth_error_;
  }
  PostDelayedClosure(base::BindOnce(std::move(callback), error, account_info),
                     dbus_operation_delay_);
}

void FakeAuthPolicyClient::GetUserStatus(
    const authpolicy::GetUserStatusRequest& request,
    GetUserStatusCallback callback) {
  authpolicy::ActiveDirectoryUserStatus user_status;
  user_status.set_password_status(password_status_);
  user_status.set_tgt_status(tgt_status_);

  authpolicy::ActiveDirectoryAccountInfo* const account_info =
      user_status.mutable_account_info();
  account_info->set_account_id(request.account_id());
  if (!display_name_.empty())
    account_info->set_display_name(display_name_);
  if (!given_name_.empty())
    account_info->set_given_name(given_name_);

  PostDelayedClosure(
      base::BindOnce(std::move(callback), authpolicy::ERROR_NONE, user_status),
      dbus_operation_delay_);
  if (!on_get_status_closure_.is_null())
    PostDelayedClosure(std::move(on_get_status_closure_),
                       dbus_operation_delay_);
}

void FakeAuthPolicyClient::GetUserKerberosFiles(
    const std::string& object_guid,
    GetUserKerberosFilesCallback callback) {
  authpolicy::KerberosFiles files;
  files.set_krb5cc("credentials");
  files.set_krb5conf("configuration");
  PostDelayedClosure(
      base::BindOnce(std::move(callback), authpolicy::ERROR_NONE, files),
      dbus_operation_delay_);
}

void FakeAuthPolicyClient::RefreshDevicePolicy(RefreshPolicyCallback callback) {
  if (!started_) {
    LOG(ERROR) << "authpolicyd not started";
    std::move(callback).Run(authpolicy::ERROR_DBUS_FAILURE);
    return;
  }

  if (!AuthPolicyLoginHelper::IsAdLocked()) {
    // Pretend that policy was fetched and cached inside authpolicyd.
    std::move(callback).Run(
        authpolicy::ERROR_DEVICE_POLICY_CACHED_BUT_NOT_SENT);
    return;
  }

  SessionManagerClient* session_manager_client =
      DBusThreadManager::Get()->GetSessionManagerClient();

  if (machine_name_.empty() || dm_token_.empty()) {
    // We need to set a new timestamp below. So we fetch the policy to get the
    // machine name and dm_token. So we could set it as well.
    session_manager_client->RetrieveDevicePolicy(
        base::BindOnce(&FakeAuthPolicyClient::OnDevicePolicyRetrieved,
                       weak_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }

  StoreDevicePolicy(device_policy_, machine_name_, dm_token_,
                    std::move(callback));
}

void FakeAuthPolicyClient::RefreshUserPolicy(const AccountId& account_id,
                                             RefreshPolicyCallback callback) {
  DCHECK(AuthPolicyLoginHelper::IsAdLocked());
  if (!started_) {
    LOG(ERROR) << "authpolicyd not started";
    std::move(callback).Run(authpolicy::ERROR_DBUS_FAILURE);
    return;
  }

  SessionManagerClient* session_manager_client =
      DBusThreadManager::Get()->GetSessionManagerClient();

  em::CloudPolicySettings policy;
  std::string payload;
  CHECK(policy.SerializeToString(&payload));

  em::PolicyFetchResponse response;
  em::PolicyData policy_data;
  policy_data.set_policy_type("google/chromeos/user");
  policy_data.set_username(account_id.GetUserEmail());
  policy_data.set_device_id(account_id.GetObjGuid());
  policy_data.set_timestamp(base::Time::Now().ToJavaTime());
  policy_data.set_policy_value(payload);
  response.set_policy_data(policy_data.SerializeAsString());
  session_manager_client->StorePolicyForUser(
      cryptohome::Identification(account_id), response.SerializeAsString(),
      base::BindOnce(&OnStorePolicy, std::move(callback)));
}

void FakeAuthPolicyClient::ConnectToSignal(
    const std::string& signal_name,
    dbus::ObjectProxy::SignalCallback signal_callback,
    dbus::ObjectProxy::OnConnectedCallback on_connected_callback) {
  std::move(on_connected_callback)
      .Run(authpolicy::kAuthPolicyInterface, signal_name, true /* success */);
  PostDelayedClosure(
      base::BindOnce(RunSignalCallback, authpolicy::kAuthPolicyInterface,
                     signal_name, signal_callback),
      dbus_operation_delay_);
}

void FakeAuthPolicyClient::OnDevicePolicyRetrieved(
    RefreshPolicyCallback callback,
    SessionManagerClient::RetrievePolicyResponseType response_type,
    const std::string& protobuf) {
  if (response_type !=
      SessionManagerClient::RetrievePolicyResponseType::SUCCESS) {
    std::move(callback).Run(authpolicy::ERROR_DBUS_FAILURE);
    return;
  }
  em::PolicyFetchResponse response;
  response.ParseFromString(protobuf);
  em::PolicyData policy_data;
  policy_data.ParseFromString(response.policy_data());
  if (policy_data.has_device_id())
    machine_name_ = policy_data.device_id();
  if (policy_data.has_request_token())
    dm_token_ = policy_data.request_token();
  StoreDevicePolicy(device_policy_, machine_name_, dm_token_,
                    std::move(callback));
}

}  // namespace chromeos
