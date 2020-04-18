// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/session_manager_client.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/chromeos_paths.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/blocking_method_caller.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "chromeos/dbus/login_manager/arc.pb.h"
#include "chromeos/dbus/login_manager/policy_descriptor.pb.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "dbus/scoped_dbus_error.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

using RetrievePolicyResponseType =
    SessionManagerClient::RetrievePolicyResponseType;

constexpr char kEmptyAccountId[] = "";

// Helper to get the enum type of RetrievePolicyResponseType based on error
// name.
RetrievePolicyResponseType GetPolicyResponseTypeByError(
    base::StringPiece error_name) {
  if (error_name == login_manager::dbus_error::kNone) {
    return RetrievePolicyResponseType::SUCCESS;
  } else if (error_name == login_manager::dbus_error::kGetServiceFail ||
             error_name == login_manager::dbus_error::kSessionDoesNotExist) {
    // TODO(crbug.com/765644, ljusten): Remove kSessionDoesNotExist case once
    // Chrome OS has switched to kGetServiceFail.
    return RetrievePolicyResponseType::GET_SERVICE_FAIL;
  } else if (error_name == login_manager::dbus_error::kSigEncodeFail) {
    return RetrievePolicyResponseType::POLICY_ENCODE_ERROR;
  }
  return RetrievePolicyResponseType::OTHER_ERROR;
}

// Logs UMA stat for retrieve policy request, corresponding to D-Bus method name
// used.
void LogPolicyResponseUma(login_manager::PolicyAccountType account_type,
                          RetrievePolicyResponseType response) {
  switch (account_type) {
    case login_manager::ACCOUNT_TYPE_DEVICE:
      UMA_HISTOGRAM_ENUMERATION("Enterprise.RetrievePolicyResponse.Device",
                                response, RetrievePolicyResponseType::COUNT);
      break;
    case login_manager::ACCOUNT_TYPE_DEVICE_LOCAL_ACCOUNT:
      UMA_HISTOGRAM_ENUMERATION(
          "Enterprise.RetrievePolicyResponse.DeviceLocalAccount", response,
          RetrievePolicyResponseType::COUNT);
      break;
    case login_manager::ACCOUNT_TYPE_USER:
      UMA_HISTOGRAM_ENUMERATION("Enterprise.RetrievePolicyResponse.User",
                                response, RetrievePolicyResponseType::COUNT);
      break;
    case login_manager::ACCOUNT_TYPE_SESSIONLESS_USER:
      UMA_HISTOGRAM_ENUMERATION(
          "Enterprise.RetrievePolicyResponse.UserDuringLogin", response,
          RetrievePolicyResponseType::COUNT);
      break;
  }
}

// Creates a PolicyDescriptor object to store/retrieve Chrome policy.
login_manager::PolicyDescriptor MakeChromePolicyDescriptor(
    login_manager::PolicyAccountType account_type,
    const std::string& account_id) {
  login_manager::PolicyDescriptor descriptor;
  descriptor.set_account_type(account_type);
  descriptor.set_account_id(account_id);
  descriptor.set_domain(login_manager::POLICY_DOMAIN_CHROME);
  return descriptor;
}

// Creates a pipe that contains the given data. The data will be prefixed by a
// size_t sized variable containing the size of the data to read.
base::ScopedFD CreatePasswordPipe(const std::string& data) {
  int pipe_fds[2];
  if (!base::CreateLocalNonBlockingPipe(pipe_fds)) {
    DLOG(ERROR) << "Failed to create pipe";
    return base::ScopedFD();
  }
  base::ScopedFD pipe_read_end(pipe_fds[0]);
  base::ScopedFD pipe_write_end(pipe_fds[1]);

  const size_t data_size = data.size();

  base::WriteFileDescriptor(pipe_write_end.get(),
                            reinterpret_cast<const char*>(&data_size),
                            sizeof(data_size));
  base::WriteFileDescriptor(pipe_write_end.get(), data.c_str(), data.size());

  return pipe_read_end;
}

}  // namespace

// The SessionManagerClient implementation used in production.
class SessionManagerClientImpl : public SessionManagerClient {
 public:
  SessionManagerClientImpl() : weak_ptr_factory_(this) {}

  ~SessionManagerClientImpl() override = default;

  // SessionManagerClient overrides:
  void SetStubDelegate(StubDelegate* delegate) override {
    // Do nothing; this isn't a stub implementation.
  }

  void AddObserver(Observer* observer) override {
    observers_.AddObserver(observer);
  }

  void RemoveObserver(Observer* observer) override {
    observers_.RemoveObserver(observer);
  }

  bool HasObserver(const Observer* observer) const override {
    return observers_.HasObserver(observer);
  }

  bool IsScreenLocked() const override { return screen_is_locked_; }

  void EmitLoginPromptVisible() override {
    SimpleMethodCallToSessionManager(
        login_manager::kSessionManagerEmitLoginPromptVisible);
    for (auto& observer : observers_)
      observer.EmitLoginPromptVisibleCalled();
  }

  void EmitAshInitialized() override {
    SimpleMethodCallToSessionManager(
        login_manager::kSessionManagerEmitAshInitialized);
  }

  void RestartJob(int socket_fd,
                  const std::vector<std::string>& argv,
                  VoidDBusMethodCallback callback) override {
    dbus::MethodCall method_call(login_manager::kSessionManagerInterface,
                                 login_manager::kSessionManagerRestartJob);
    dbus::MessageWriter writer(&method_call);
    writer.AppendFileDescriptor(socket_fd);
    writer.AppendArrayOfStrings(argv);
    session_manager_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SessionManagerClientImpl::OnVoidMethod,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void SaveLoginPassword(const std::string& password) override {
    dbus::MethodCall method_call(
        login_manager::kSessionManagerInterface,
        login_manager::kSessionManagerSaveLoginPassword);
    dbus::MessageWriter writer(&method_call);

    base::ScopedFD fd = CreatePasswordPipe(password);
    if (fd.get() == -1) {
      LOG(WARNING) << "Could not create password pipe.";
      return;
    }

    writer.AppendFileDescriptor(fd.get());

    session_manager_proxy_->CallMethod(&method_call,
                                       dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                                       base::DoNothing());
  }

  void StartSession(const cryptohome::Identification& cryptohome_id) override {
    dbus::MethodCall method_call(login_manager::kSessionManagerInterface,
                                 login_manager::kSessionManagerStartSession);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(cryptohome_id.id());
    writer.AppendString("");  // Unique ID is deprecated
    session_manager_proxy_->CallMethod(&method_call,
                                       dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                                       base::DoNothing());
  }

  void StopSession() override {
    dbus::MethodCall method_call(login_manager::kSessionManagerInterface,
                                 login_manager::kSessionManagerStopSession);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString("");  // Unique ID is deprecated
    session_manager_proxy_->CallMethod(&method_call,
                                       dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                                       base::DoNothing());
  }

  void StartDeviceWipe() override {
    SimpleMethodCallToSessionManager(
        login_manager::kSessionManagerStartDeviceWipe);
  }

  void StartTPMFirmwareUpdate(const std::string& update_mode) override {
    dbus::MethodCall method_call(
        login_manager::kSessionManagerInterface,
        login_manager::kSessionManagerStartTPMFirmwareUpdate);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(update_mode);
    session_manager_proxy_->CallMethod(&method_call,
                                       dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                                       base::DoNothing());
  }

  void RequestLockScreen() override {
    SimpleMethodCallToSessionManager(login_manager::kSessionManagerLockScreen);
  }

  void NotifyLockScreenShown() override {
    SimpleMethodCallToSessionManager(
        login_manager::kSessionManagerHandleLockScreenShown);
  }

  void NotifyLockScreenDismissed() override {
    SimpleMethodCallToSessionManager(
        login_manager::kSessionManagerHandleLockScreenDismissed);
  }

  void NotifySupervisedUserCreationStarted() override {
    SimpleMethodCallToSessionManager(
        login_manager::kSessionManagerHandleSupervisedUserCreationStarting);
  }

  void NotifySupervisedUserCreationFinished() override {
    SimpleMethodCallToSessionManager(
        login_manager::kSessionManagerHandleSupervisedUserCreationFinished);
  }

  void RetrieveActiveSessions(ActiveSessionsCallback callback) override {
    dbus::MethodCall method_call(
        login_manager::kSessionManagerInterface,
        login_manager::kSessionManagerRetrieveActiveSessions);

    session_manager_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SessionManagerClientImpl::OnRetrieveActiveSessions,
                       weak_ptr_factory_.GetWeakPtr(),
                       login_manager::kSessionManagerRetrieveActiveSessions,
                       std::move(callback)));
  }

  void RetrieveDevicePolicy(RetrievePolicyCallback callback) override {
    login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
        login_manager::ACCOUNT_TYPE_DEVICE, kEmptyAccountId);
    CallRetrievePolicy(descriptor, std::move(callback));
  }

  RetrievePolicyResponseType BlockingRetrieveDevicePolicy(
      std::string* policy_out) override {
    login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
        login_manager::ACCOUNT_TYPE_DEVICE, kEmptyAccountId);
    return BlockingRetrievePolicy(descriptor, policy_out);
  }

  void RetrievePolicyForUser(const cryptohome::Identification& cryptohome_id,
                             RetrievePolicyCallback callback) override {
    login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
        login_manager::ACCOUNT_TYPE_USER, cryptohome_id.id());
    CallRetrievePolicy(descriptor, std::move(callback));
  }

  RetrievePolicyResponseType BlockingRetrievePolicyForUser(
      const cryptohome::Identification& cryptohome_id,
      std::string* policy_out) override {
    login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
        login_manager::ACCOUNT_TYPE_USER, cryptohome_id.id());
    return BlockingRetrievePolicy(descriptor, policy_out);
  }

  void RetrievePolicyForUserWithoutSession(
      const cryptohome::Identification& cryptohome_id,
      RetrievePolicyCallback callback) override {
    login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
        login_manager::ACCOUNT_TYPE_SESSIONLESS_USER, cryptohome_id.id());
    CallRetrievePolicy(descriptor, std::move(callback));
  }

  void RetrieveDeviceLocalAccountPolicy(
      const std::string& account_name,
      RetrievePolicyCallback callback) override {
    login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
        login_manager::ACCOUNT_TYPE_DEVICE_LOCAL_ACCOUNT, account_name);
    CallRetrievePolicy(descriptor, std::move(callback));
  }

  RetrievePolicyResponseType BlockingRetrieveDeviceLocalAccountPolicy(
      const std::string& account_name,
      std::string* policy_out) override {
    login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
        login_manager::ACCOUNT_TYPE_DEVICE_LOCAL_ACCOUNT, account_name);
    return BlockingRetrievePolicy(descriptor, policy_out);
  }

  void RetrievePolicy(const login_manager::PolicyDescriptor& descriptor,
                      RetrievePolicyCallback callback) override {
    CallRetrievePolicy(descriptor, std::move(callback));
  }

  RetrievePolicyResponseType BlockingRetrievePolicy(
      const login_manager::PolicyDescriptor& descriptor,
      std::string* policy_out) override {
    return CallBlockingRetrievePolicy(descriptor, policy_out);
  }

  void StoreDevicePolicy(const std::string& policy_blob,
                         VoidDBusMethodCallback callback) override {
    login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
        login_manager::ACCOUNT_TYPE_DEVICE, kEmptyAccountId);
    CallStorePolicy(descriptor, policy_blob, std::move(callback));
  }

  void StorePolicyForUser(const cryptohome::Identification& cryptohome_id,
                          const std::string& policy_blob,
                          VoidDBusMethodCallback callback) override {
    login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
        login_manager::ACCOUNT_TYPE_USER, cryptohome_id.id());
    CallStorePolicy(descriptor, policy_blob, std::move(callback));
  }

  void StoreDeviceLocalAccountPolicy(const std::string& account_name,
                                     const std::string& policy_blob,
                                     VoidDBusMethodCallback callback) override {
    login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
        login_manager::ACCOUNT_TYPE_DEVICE_LOCAL_ACCOUNT, account_name);
    CallStorePolicy(descriptor, policy_blob, std::move(callback));
  }

  void StorePolicy(const login_manager::PolicyDescriptor& descriptor,
                   const std::string& policy_blob,
                   VoidDBusMethodCallback callback) override {
    CallStorePolicy(descriptor, policy_blob, std::move(callback));
  }

  bool SupportsRestartToApplyUserFlags() const override { return true; }

  void SetFlagsForUser(const cryptohome::Identification& cryptohome_id,
                       const std::vector<std::string>& flags) override {
    dbus::MethodCall method_call(login_manager::kSessionManagerInterface,
                                 login_manager::kSessionManagerSetFlagsForUser);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(cryptohome_id.id());
    writer.AppendArrayOfStrings(flags);
    session_manager_proxy_->CallMethod(&method_call,
                                       dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                                       base::DoNothing());
  }

  void GetServerBackedStateKeys(StateKeysCallback callback) override {
    dbus::MethodCall method_call(
        login_manager::kSessionManagerInterface,
        login_manager::kSessionManagerGetServerBackedStateKeys);

    // Infinite timeout needed because the state keys are not generated as long
    // as the time sync hasn't been done (which requires network).
    // TODO(igorcov): Since this is a resource allocated that could last a long
    // time, we will need to change the behavior to either listen to
    // LastSyncInfo event from tlsdated or communicate through signals with
    // session manager in this particular flow.
    session_manager_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_INFINITE,
        base::BindOnce(&SessionManagerClientImpl::OnGetServerBackedStateKeys,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void StartArcMiniContainer(
      const login_manager::StartArcMiniContainerRequest& request,
      StartArcMiniContainerCallback callback) override {
    DCHECK(!callback.is_null());
    dbus::MethodCall method_call(
        login_manager::kSessionManagerInterface,
        login_manager::kSessionManagerStartArcMiniContainer);
    dbus::MessageWriter writer(&method_call);

    writer.AppendProtoAsArrayOfBytes(request);

    session_manager_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SessionManagerClientImpl::OnStartArcMiniContainer,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void UpgradeArcContainer(
      const login_manager::UpgradeArcContainerRequest& request,
      UpgradeArcContainerCallback success_callback,
      UpgradeErrorCallback error_callback) override {
    DCHECK(!success_callback.is_null());
    DCHECK(!error_callback.is_null());
    dbus::MethodCall method_call(
        login_manager::kSessionManagerInterface,
        login_manager::kSessionManagerUpgradeArcContainer);
    dbus::MessageWriter writer(&method_call);

    writer.AppendProtoAsArrayOfBytes(request);

    session_manager_proxy_->CallMethodWithErrorResponse(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SessionManagerClientImpl::OnUpgradeArcContainer,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::move(success_callback), std::move(error_callback)));
  }

  void StopArcInstance(VoidDBusMethodCallback callback) override {
    dbus::MethodCall method_call(login_manager::kSessionManagerInterface,
                                 login_manager::kSessionManagerStopArcInstance);
    session_manager_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SessionManagerClientImpl::OnVoidMethod,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void SetArcCpuRestriction(
      login_manager::ContainerCpuRestrictionState restriction_state,
      VoidDBusMethodCallback callback) override {
    dbus::MethodCall method_call(
        login_manager::kSessionManagerInterface,
        login_manager::kSessionManagerSetArcCpuRestriction);
    dbus::MessageWriter writer(&method_call);
    writer.AppendUint32(restriction_state);
    session_manager_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SessionManagerClientImpl::OnVoidMethod,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void EmitArcBooted(const cryptohome::Identification& cryptohome_id,
                     VoidDBusMethodCallback callback) override {
    dbus::MethodCall method_call(login_manager::kSessionManagerInterface,
                                 login_manager::kSessionManagerEmitArcBooted);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(cryptohome_id.id());
    session_manager_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SessionManagerClientImpl::OnVoidMethod,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void GetArcStartTime(DBusMethodCallback<base::TimeTicks> callback) override {
    dbus::MethodCall method_call(
        login_manager::kSessionManagerInterface,
        login_manager::kSessionManagerGetArcStartTimeTicks);

    session_manager_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SessionManagerClientImpl::OnGetArcStartTime,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  void RemoveArcData(const cryptohome::Identification& cryptohome_id,
                     VoidDBusMethodCallback callback) override {
    dbus::MethodCall method_call(login_manager::kSessionManagerInterface,
                                 login_manager::kSessionManagerRemoveArcData);
    dbus::MessageWriter writer(&method_call);
    writer.AppendString(cryptohome_id.id());
    session_manager_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SessionManagerClientImpl::OnVoidMethod,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

 protected:
  void Init(dbus::Bus* bus) override {
    session_manager_proxy_ = bus->GetObjectProxy(
        login_manager::kSessionManagerServiceName,
        dbus::ObjectPath(login_manager::kSessionManagerServicePath));
    blocking_method_caller_.reset(
        new BlockingMethodCaller(bus, session_manager_proxy_));

    // Signals emitted on the session manager's interface.
    session_manager_proxy_->ConnectToSignal(
        login_manager::kSessionManagerInterface,
        login_manager::kOwnerKeySetSignal,
        base::Bind(&SessionManagerClientImpl::OwnerKeySetReceived,
                   weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(&SessionManagerClientImpl::SignalConnected,
                       weak_ptr_factory_.GetWeakPtr()));
    session_manager_proxy_->ConnectToSignal(
        login_manager::kSessionManagerInterface,
        login_manager::kPropertyChangeCompleteSignal,
        base::Bind(&SessionManagerClientImpl::PropertyChangeCompleteReceived,
                   weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(&SessionManagerClientImpl::SignalConnected,
                       weak_ptr_factory_.GetWeakPtr()));
    session_manager_proxy_->ConnectToSignal(
        login_manager::kSessionManagerInterface,
        login_manager::kScreenIsLockedSignal,
        base::Bind(&SessionManagerClientImpl::ScreenIsLockedReceived,
                   weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(&SessionManagerClientImpl::SignalConnected,
                       weak_ptr_factory_.GetWeakPtr()));
    session_manager_proxy_->ConnectToSignal(
        login_manager::kSessionManagerInterface,
        login_manager::kScreenIsUnlockedSignal,
        base::Bind(&SessionManagerClientImpl::ScreenIsUnlockedReceived,
                   weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(&SessionManagerClientImpl::SignalConnected,
                       weak_ptr_factory_.GetWeakPtr()));
    session_manager_proxy_->ConnectToSignal(
        login_manager::kSessionManagerInterface,
        login_manager::kArcInstanceStopped,
        base::Bind(&SessionManagerClientImpl::ArcInstanceStoppedReceived,
                   weak_ptr_factory_.GetWeakPtr()),
        base::BindOnce(&SessionManagerClientImpl::SignalConnected,
                       weak_ptr_factory_.GetWeakPtr()));
  }

 private:
  // Makes a method call to the session manager with no arguments and no
  // response.
  void SimpleMethodCallToSessionManager(const std::string& method_name) {
    dbus::MethodCall method_call(login_manager::kSessionManagerInterface,
                                 method_name);
    session_manager_proxy_->CallMethod(&method_call,
                                       dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
                                       base::DoNothing());
  }

  // Called when the method call without result is completed.
  void OnVoidMethod(VoidDBusMethodCallback callback, dbus::Response* response) {
    std::move(callback).Run(response);
  }

  // Non-blocking call to Session Manager to retrieve policy.
  void CallRetrievePolicy(const login_manager::PolicyDescriptor& descriptor,
                          RetrievePolicyCallback callback) {
    dbus::MethodCall method_call(
        login_manager::kSessionManagerInterface,
        login_manager::kSessionManagerRetrievePolicyEx);
    dbus::MessageWriter writer(&method_call);
    const std::string descriptor_blob = descriptor.SerializeAsString();
    // static_cast does not work due to signedness.
    writer.AppendArrayOfBytes(
        reinterpret_cast<const uint8_t*>(descriptor_blob.data()),
        descriptor_blob.size());
    session_manager_proxy_->CallMethodWithErrorResponse(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SessionManagerClientImpl::OnRetrievePolicy,
                       weak_ptr_factory_.GetWeakPtr(),
                       descriptor.account_type(), std::move(callback)));
  }

  // Blocking call to Session Manager to retrieve policy.
  RetrievePolicyResponseType CallBlockingRetrievePolicy(
      const login_manager::PolicyDescriptor& descriptor,
      std::string* policy_out) {
    dbus::MethodCall method_call(
        login_manager::kSessionManagerInterface,
        login_manager::kSessionManagerRetrievePolicyEx);
    dbus::MessageWriter writer(&method_call);
    const std::string descriptor_blob = descriptor.SerializeAsString();
    // static_cast does not work due to signedness.
    writer.AppendArrayOfBytes(
        reinterpret_cast<const uint8_t*>(descriptor_blob.data()),
        descriptor_blob.size());
    dbus::ScopedDBusError error;
    std::unique_ptr<dbus::Response> response =
        blocking_method_caller_->CallMethodAndBlockWithError(&method_call,
                                                             &error);
    RetrievePolicyResponseType result = RetrievePolicyResponseType::SUCCESS;
    if (error.is_set() && error.name()) {
      result = GetPolicyResponseTypeByError(error.name());
    }
    if (result == RetrievePolicyResponseType::SUCCESS) {
      ExtractPolicyResponseString(descriptor.account_type(), response.get(),
                                  policy_out);
    } else {
      policy_out->clear();
    }
    LogPolicyResponseUma(descriptor.account_type(), result);
    return result;
  }

  void CallStorePolicy(const login_manager::PolicyDescriptor& descriptor,
                       const std::string& policy_blob,
                       VoidDBusMethodCallback callback) {
    dbus::MethodCall method_call(login_manager::kSessionManagerInterface,
                                 login_manager::kSessionManagerStorePolicyEx);
    dbus::MessageWriter writer(&method_call);
    const std::string descriptor_blob = descriptor.SerializeAsString();
    // static_cast does not work due to signedness.
    writer.AppendArrayOfBytes(
        reinterpret_cast<const uint8_t*>(descriptor_blob.data()),
        descriptor_blob.size());
    writer.AppendArrayOfBytes(
        reinterpret_cast<const uint8_t*>(policy_blob.data()),
        policy_blob.size());
    session_manager_proxy_->CallMethod(
        &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::BindOnce(&SessionManagerClientImpl::OnVoidMethod,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }

  // Called when kSessionManagerRetrieveActiveSessions method is complete.
  void OnRetrieveActiveSessions(const std::string& method_name,
                                ActiveSessionsCallback callback,
                                dbus::Response* response) {
    if (!response) {
      std::move(callback).Run(base::nullopt);
      return;
    }

    dbus::MessageReader reader(response);
    dbus::MessageReader array_reader(nullptr);
    if (!reader.PopArray(&array_reader)) {
      LOG(ERROR) << method_name
                 << " response is incorrect: " << response->ToString();
      std::move(callback).Run(base::nullopt);
      return;
    }

    ActiveSessionsMap sessions;
    while (array_reader.HasMoreData()) {
      dbus::MessageReader dict_entry_reader(nullptr);
      std::string key;
      std::string value;
      if (!array_reader.PopDictEntry(&dict_entry_reader) ||
          !dict_entry_reader.PopString(&key) ||
          !dict_entry_reader.PopString(&value)) {
        LOG(ERROR) << method_name
                   << " response is incorrect: " << response->ToString();
      } else {
        sessions[cryptohome::Identification::FromString(key)] = value;
      }
    }
    std::move(callback).Run(std::move(sessions));
  }

  // Reads an array of policy data bytes data as std::string.
  void ExtractPolicyResponseString(
      login_manager::PolicyAccountType account_type,
      dbus::Response* response,
      std::string* extracted) {
    if (!response) {
      LOG(ERROR) << "Failed to call RetrievePolicyEx for account type "
                 << account_type;
      return;
    }
    dbus::MessageReader reader(response);
    const uint8_t* values = nullptr;
    size_t length = 0;
    if (!reader.PopArrayOfBytes(&values, &length)) {
      LOG(ERROR) << "Invalid response: " << response->ToString();
      return;
    }
    // static_cast does not work due to signedness.
    extracted->assign(reinterpret_cast<const char*>(values), length);
  }

  // Called when kSessionManagerRetrievePolicy or
  // kSessionManagerRetrievePolicyForUser method is complete.
  void OnRetrievePolicy(login_manager::PolicyAccountType account_type,
                        RetrievePolicyCallback callback,
                        dbus::Response* response,
                        dbus::ErrorResponse* error) {
    if (!response) {
      RetrievePolicyResponseType response_type =
          GetPolicyResponseTypeByError(error ? error->GetErrorName() : "");
      LogPolicyResponseUma(account_type, response_type);
      std::move(callback).Run(response_type, std::string());
      return;
    }

    dbus::MessageReader reader(response);
    std::string proto_blob;
    ExtractPolicyResponseString(account_type, response, &proto_blob);
    LogPolicyResponseUma(account_type, RetrievePolicyResponseType::SUCCESS);
    std::move(callback).Run(RetrievePolicyResponseType::SUCCESS, proto_blob);
  }

  // Called when the owner key set signal is received.
  void OwnerKeySetReceived(dbus::Signal* signal) {
    dbus::MessageReader reader(signal);
    std::string result_string;
    if (!reader.PopString(&result_string)) {
      LOG(ERROR) << "Invalid signal: " << signal->ToString();
      return;
    }
    const bool success = base::StartsWith(result_string, "success",
                                          base::CompareCase::INSENSITIVE_ASCII);
    for (auto& observer : observers_)
      observer.OwnerKeySet(success);
  }

  // Called when the property change complete signal is received.
  void PropertyChangeCompleteReceived(dbus::Signal* signal) {
    dbus::MessageReader reader(signal);
    std::string result_string;
    if (!reader.PopString(&result_string)) {
      LOG(ERROR) << "Invalid signal: " << signal->ToString();
      return;
    }
    const bool success = base::StartsWith(result_string, "success",
                                          base::CompareCase::INSENSITIVE_ASCII);
    for (auto& observer : observers_)
      observer.PropertyChangeComplete(success);
  }

  void ScreenIsLockedReceived(dbus::Signal* signal) {
    screen_is_locked_ = true;
  }

  void ScreenIsUnlockedReceived(dbus::Signal* signal) {
    screen_is_locked_ = false;
  }

  void ArcInstanceStoppedReceived(dbus::Signal* signal) {
    dbus::MessageReader reader(signal);

    auto reason = login_manager::ArcContainerStopReason::CRASH;
    uint32_t value = 0;
    bool clean = false;
    if (reader.PopUint32(&value)) {
      reason = static_cast<login_manager::ArcContainerStopReason>(value);
    } else if (reader.PopBool(&clean)) {
      // This is for the transition period.
      // We can think the change is virtually split into two;
      // - bool becomes enum ArcContainerStopReason. true is mapped to
      //   USER_REQUEST, false is to CRASH. Then,
      // - USER_REQUEST cases are split into more precise categories.
      // The only client of this signal, which is ArcSessionImpl, can handle
      // this approach.
      // TODO(b/76152951): Remove this.
      reason = clean ? login_manager::ArcContainerStopReason::USER_REQUEST
                     : login_manager::ArcContainerStopReason::CRASH;
    } else {
      LOG(ERROR) << "Invalid signal: " << signal->ToString();
      return;
    }

    std::string container_instance_id;
    if (!reader.PopString(&container_instance_id)) {
      LOG(ERROR) << "Invalid signal: " << signal->ToString();
      return;
    }
    for (auto& observer : observers_)
      observer.ArcInstanceStopped(reason, container_instance_id);
  }

  // Called when the object is connected to the signal.
  void SignalConnected(const std::string& interface_name,
                       const std::string& signal_name,
                       bool success) {
    LOG_IF(ERROR, !success) << "Failed to connect to " << signal_name;
  }

  // Called when kSessionManagerGetServerBackedStateKeys method is complete.
  void OnGetServerBackedStateKeys(StateKeysCallback callback,
                                  dbus::Response* response) {
    std::vector<std::string> state_keys;
    if (response) {
      dbus::MessageReader reader(response);
      dbus::MessageReader array_reader(nullptr);

      if (!reader.PopArray(&array_reader)) {
        LOG(ERROR) << "Bad response: " << response->ToString();
      } else {
        while (array_reader.HasMoreData()) {
          const uint8_t* data = nullptr;
          size_t size = 0;
          if (!array_reader.PopArrayOfBytes(&data, &size)) {
            LOG(ERROR) << "Bad response: " << response->ToString();
            state_keys.clear();
            break;
          }
          state_keys.emplace_back(reinterpret_cast<const char*>(data), size);
        }
      }
    }

    std::move(callback).Run(state_keys);
  }

  void OnGetArcStartTime(DBusMethodCallback<base::TimeTicks> callback,
                         dbus::Response* response) {
    if (!response) {
      std::move(callback).Run(base::nullopt);
      return;
    }

    dbus::MessageReader reader(response);
    int64_t ticks = 0;
    if (!reader.PopInt64(&ticks)) {
      LOG(ERROR) << "Invalid response: " << response->ToString();
      std::move(callback).Run(base::nullopt);
      return;
    }

    std::move(callback).Run(base::TimeTicks::FromInternalValue(ticks));
  }

  void OnStartArcMiniContainer(StartArcMiniContainerCallback callback,
                               dbus::Response* response) {
    if (!response) {
      std::move(callback).Run(base::nullopt);
      return;
    }

    dbus::MessageReader reader(response);
    std::string container_instance_id;
    if (!reader.PopString(&container_instance_id)) {
      LOG(ERROR) << "Invalid response: " << response->ToString();
      std::move(callback).Run(base::nullopt);
      return;
    }
    std::move(callback).Run(std::move(container_instance_id));
  }

  void OnUpgradeArcContainer(UpgradeArcContainerCallback success_callback,
                             UpgradeErrorCallback error_callback,
                             dbus::Response* response,
                             dbus::ErrorResponse* error) {
    if (!response) {
      LOG(ERROR) << "Failed to call UpgradeArcContainer: "
                 << (error ? error->ToString() : "(null)");
      std::move(error_callback)
          .Run(error && error->GetErrorName() ==
                            login_manager::dbus_error::kLowFreeDisk);
      return;
    }

    dbus::MessageReader reader(response);
    base::ScopedFD server_socket;
    if (!reader.PopFileDescriptor(&server_socket)) {
      LOG(ERROR) << "Invalid response: " << response->ToString();
      std::move(error_callback).Run(false);
      return;
    }
    std::move(success_callback).Run(std::move(server_socket));
  }

  dbus::ObjectProxy* session_manager_proxy_ = nullptr;
  std::unique_ptr<BlockingMethodCaller> blocking_method_caller_;
  base::ObserverList<Observer> observers_;

  // Most recent screen-lock state received from session_manager.
  bool screen_is_locked_ = false;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<SessionManagerClientImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SessionManagerClientImpl);
};

SessionManagerClient::SessionManagerClient() = default;

SessionManagerClient::~SessionManagerClient() = default;

SessionManagerClient* SessionManagerClient::Create(
    DBusClientImplementationType type) {
  if (type == REAL_DBUS_CLIENT_IMPLEMENTATION)
    return new SessionManagerClientImpl();
  DCHECK_EQ(FAKE_DBUS_CLIENT_IMPLEMENTATION, type);
  return new FakeSessionManagerClient(
      FakeSessionManagerClient::PolicyStorageType::kOnDisk);
}

}  // namespace chromeos
