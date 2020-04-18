// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/fake_session_manager_client.h"

#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/location.h"
#include "base/numerics/safe_conversions.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chromeos/chromeos_paths.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/login_manager/policy_descriptor.pb.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "crypto/sha2.h"

namespace chromeos {

using RetrievePolicyCallback = FakeSessionManagerClient::RetrievePolicyCallback;
using RetrievePolicyResponseType =
    FakeSessionManagerClient::RetrievePolicyResponseType;

namespace {

constexpr char kFakeContainerInstanceId[] = "0123456789ABCDEF";
constexpr char kStubDevicePolicyFileNamePrefix[] = "stub_device_policy";
constexpr char kStubPerAccountPolicyFileNamePrefix[] = "stub_policy";
constexpr char kStubStateKeysFileName[] = "stub_state_keys";
constexpr char kStubExtensionPolicyFileNameFragment[] = "_extension_";
constexpr char kStubSigninExtensionPolicyFileNameFragment[] =
    "_signin_extension_";
constexpr char kStubPerAccountPolicyKeyFileName[] = "policy.pub";
constexpr char kEmptyAccountId[] = "";

// Helper to asynchronously retrieve a file's content.
std::string GetFileContent(const base::FilePath& path) {
  std::string result;
  if (!path.empty())
    base::ReadFileToString(path, &result);
  return result;
}

// Helper to write files in a background thread.
void StoreFiles(std::map<base::FilePath, std::string> paths_and_data) {
  for (const auto& kv : paths_and_data) {
    const base::FilePath& path = kv.first;
    if (path.empty() || !base::CreateDirectory(path.DirName())) {
      LOG(WARNING) << "Failed to write to " << path.value();
      continue;
    }
    const std::string& data = kv.second;
    int result = base::WriteFile(path, data.data(), data.size());
    if (result == -1 || static_cast<size_t>(result) != data.size())
      LOG(WARNING) << "Failed to write to " << path.value();
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

// Returns true if the policy descriptor points to Chrome device policy.
bool IsChromeDevicePolicy(const login_manager::PolicyDescriptor& descriptor) {
  DCHECK(descriptor.has_account_type());
  DCHECK(descriptor.has_domain());
  return descriptor.account_type() == login_manager::ACCOUNT_TYPE_DEVICE &&
         descriptor.domain() == login_manager::POLICY_DOMAIN_CHROME;
}

// Helper to asynchronously read (or if missing create) state key stubs.
std::vector<std::string> ReadCreateStateKeysStub(const base::FilePath& path) {
  std::string contents;
  if (base::PathExists(path)) {
    contents = GetFileContent(path);
  } else {
    // Create stub state keys on the fly.
    for (int i = 0; i < 5; ++i) {
      contents += crypto::SHA256HashString(
          base::IntToString(i) +
          base::Int64ToString(base::Time::Now().ToJavaTime()));
    }
    StoreFiles({{path, contents}});
  }

  std::vector<std::string> state_keys;
  for (size_t i = 0; i < contents.size() / 32; ++i)
    state_keys.push_back(contents.substr(i * 32, 32));
  return state_keys;
}

// Gets the postfix of the stub policy filename, which is based the
// |descriptor|'s domain and component id. Returns an empty string if the domain
// doesn't use a component id (e.g. normal Chrome user/device policy).
std::string GetStubPolicyFilenamePostfix(
    const login_manager::PolicyDescriptor& descriptor) {
  DCHECK(descriptor.has_domain());
  switch (descriptor.domain()) {
    case login_manager::POLICY_DOMAIN_CHROME:
      return std::string();
    case login_manager::POLICY_DOMAIN_EXTENSIONS:
      DCHECK(descriptor.has_component_id());
      return kStubExtensionPolicyFileNameFragment + descriptor.component_id();
    case login_manager::POLICY_DOMAIN_SIGNIN_EXTENSIONS:
      DCHECK(descriptor.has_component_id());
      return kStubSigninExtensionPolicyFileNameFragment +
             descriptor.component_id();
  }
  NOTREACHED();
  return std::string();
}

// Returns the last part of the stub policy file path consisting of the filename
// for device accounts and <cryptohome_id>/filename for user and device local
// accounts, e.g.
//   "stub_device_policy" for Chrome device policy or
//   "<cryptohome_id>/stub_policy_extension_<id>" for extension policy.
// This path is also used as key in the in-memory policy map |policy_|.
base::FilePath GetStubRelativePolicyPath(
    const login_manager::PolicyDescriptor& descriptor) {
  DCHECK(descriptor.has_account_type());
  std::string postfix = GetStubPolicyFilenamePostfix(descriptor);
  switch (descriptor.account_type()) {
    case login_manager::ACCOUNT_TYPE_DEVICE:
      return base::FilePath(kStubDevicePolicyFileNamePrefix + postfix);

    case login_manager::ACCOUNT_TYPE_USER:
    case login_manager::ACCOUNT_TYPE_SESSIONLESS_USER:
    case login_manager::ACCOUNT_TYPE_DEVICE_LOCAL_ACCOUNT: {
      DCHECK(descriptor.has_account_id());
      cryptohome::Identification cryptohome_id =
          cryptohome::Identification::FromString(descriptor.account_id());
      const std::string sanitized_id =
          CryptohomeClient::GetStubSanitizedUsername(cryptohome_id);
      return base::FilePath(sanitized_id)
          .AppendASCII(kStubPerAccountPolicyFileNamePrefix + postfix);
    }
  }
  NOTREACHED();
  return base::FilePath();
}

// Gets the stub file paths of the policy blob and optionally the policy key
// (|key_path|) for the given |descriptor|. |key_path| can be nullptr.
base::FilePath GetStubPolicyFilePath(
    const login_manager::PolicyDescriptor& descriptor,
    base::FilePath* key_path) {
  if (key_path)
    key_path->clear();

  base::FilePath relative_policy_path = GetStubRelativePolicyPath(descriptor);
  DCHECK(descriptor.has_account_type());
  switch (descriptor.account_type()) {
    case login_manager::ACCOUNT_TYPE_DEVICE: {
      base::FilePath owner_key_path;
      CHECK(base::PathService::Get(chromeos::FILE_OWNER_KEY, &owner_key_path));
      if (key_path)
        *key_path = owner_key_path;
      return owner_key_path.DirName().Append(relative_policy_path);
    }

    case login_manager::ACCOUNT_TYPE_USER:
    case login_manager::ACCOUNT_TYPE_SESSIONLESS_USER:
    case login_manager::ACCOUNT_TYPE_DEVICE_LOCAL_ACCOUNT: {
      base::FilePath base_path;
      CHECK(base::PathService::Get(chromeos::DIR_USER_POLICY_KEYS, &base_path));
      if (key_path) {
        *key_path = base_path.Append(relative_policy_path.DirName())
                        .AppendASCII(kStubPerAccountPolicyKeyFileName);
      }
      return base_path.Append(relative_policy_path);
    }
  }
  NOTREACHED();
  return base::FilePath();
}

// Returns a key that's used for storing policy in memory.
std::string GetMemoryStorageKey(
    const login_manager::PolicyDescriptor& descriptor) {
  base::FilePath relative_policy_path = GetStubRelativePolicyPath(descriptor);
  DCHECK(!relative_policy_path.empty());
  return relative_policy_path.value();
}

// Posts a task to call callback(response).
template <typename CallbackType, typename ResponseType>
void PostReply(const base::Location& from_here,
               CallbackType callback,
               ResponseType response) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      from_here, base::BindOnce(std::move(callback), std::move(response)));
}

// Posts a task to call callback(response, policy).
void PostReplyWithPolicy(const base::Location& from_here,
                         RetrievePolicyCallback callback,
                         RetrievePolicyResponseType response,
                         const std::string& policy) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      from_here, base::BindOnce(std::move(callback), response, policy));
}

}  // namespace

FakeSessionManagerClient::FakeSessionManagerClient()
    : FakeSessionManagerClient(PolicyStorageType::kInMemory) {}

FakeSessionManagerClient::FakeSessionManagerClient(
    PolicyStorageType policy_storage)
    : policy_storage_(policy_storage),
      start_device_wipe_call_count_(0),
      request_lock_screen_call_count_(0),
      notify_lock_screen_shown_call_count_(0),
      notify_lock_screen_dismissed_call_count_(0),
      screen_is_locked_(false),
      arc_available_(false),
      delegate_(nullptr),
      weak_ptr_factory_(this) {}

FakeSessionManagerClient::~FakeSessionManagerClient() = default;

void FakeSessionManagerClient::Init(dbus::Bus* bus) {}

void FakeSessionManagerClient::SetStubDelegate(StubDelegate* delegate) {
  delegate_ = delegate;
}

void FakeSessionManagerClient::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void FakeSessionManagerClient::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

bool FakeSessionManagerClient::HasObserver(const Observer* observer) const {
  return observers_.HasObserver(observer);
}

bool FakeSessionManagerClient::IsScreenLocked() const {
  return screen_is_locked_;
}

void FakeSessionManagerClient::EmitLoginPromptVisible() {
  for (auto& observer : observers_)
    observer.EmitLoginPromptVisibleCalled();
}

void FakeSessionManagerClient::EmitAshInitialized() {}

void FakeSessionManagerClient::RestartJob(int socket_fd,
                                          const std::vector<std::string>& argv,
                                          VoidDBusMethodCallback callback) {}

void FakeSessionManagerClient::SaveLoginPassword(const std::string& password) {}

void FakeSessionManagerClient::StartSession(
    const cryptohome::Identification& cryptohome_id) {
  DCHECK_EQ(0UL, user_sessions_.count(cryptohome_id));
  std::string user_id_hash =
      CryptohomeClient::GetStubSanitizedUsername(cryptohome_id);
  user_sessions_[cryptohome_id] = user_id_hash;
}

void FakeSessionManagerClient::StopSession() {}

void FakeSessionManagerClient::NotifySupervisedUserCreationStarted() {}

void FakeSessionManagerClient::NotifySupervisedUserCreationFinished() {}

void FakeSessionManagerClient::StartDeviceWipe() {
  start_device_wipe_call_count_++;
}

void FakeSessionManagerClient::StartTPMFirmwareUpdate(
    const std::string& update_mode) {}

void FakeSessionManagerClient::RequestLockScreen() {
  request_lock_screen_call_count_++;
  if (delegate_)
    delegate_->LockScreenForStub();
}

void FakeSessionManagerClient::NotifyLockScreenShown() {
  notify_lock_screen_shown_call_count_++;
  screen_is_locked_ = true;
}

void FakeSessionManagerClient::NotifyLockScreenDismissed() {
  notify_lock_screen_dismissed_call_count_++;
  screen_is_locked_ = false;
}

void FakeSessionManagerClient::RetrieveActiveSessions(
    ActiveSessionsCallback callback) {
  PostReply(FROM_HERE, std::move(callback), user_sessions_);
}

void FakeSessionManagerClient::RetrieveDevicePolicy(
    RetrievePolicyCallback callback) {
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_DEVICE, kEmptyAccountId);
  RetrievePolicy(descriptor, std::move(callback));
}

RetrievePolicyResponseType
FakeSessionManagerClient::BlockingRetrieveDevicePolicy(
    std::string* policy_out) {
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_DEVICE, kEmptyAccountId);
  return BlockingRetrievePolicy(descriptor, policy_out);
}

void FakeSessionManagerClient::RetrievePolicyForUser(
    const cryptohome::Identification& cryptohome_id,
    RetrievePolicyCallback callback) {
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_USER, cryptohome_id.id());
  RetrievePolicy(descriptor, std::move(callback));
}

RetrievePolicyResponseType
FakeSessionManagerClient::BlockingRetrievePolicyForUser(
    const cryptohome::Identification& cryptohome_id,
    std::string* policy_out) {
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_USER, cryptohome_id.id());
  return BlockingRetrievePolicy(descriptor, policy_out);
}

void FakeSessionManagerClient::RetrievePolicyForUserWithoutSession(
    const cryptohome::Identification& cryptohome_id,
    RetrievePolicyCallback callback) {
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_SESSIONLESS_USER, cryptohome_id.id());
  RetrievePolicy(descriptor, std::move(callback));
}

void FakeSessionManagerClient::RetrieveDeviceLocalAccountPolicy(
    const std::string& account_id,
    RetrievePolicyCallback callback) {
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_DEVICE_LOCAL_ACCOUNT, account_id);
  RetrievePolicy(descriptor, std::move(callback));
}

RetrievePolicyResponseType
FakeSessionManagerClient::BlockingRetrieveDeviceLocalAccountPolicy(
    const std::string& account_id,
    std::string* policy_out) {
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_DEVICE_LOCAL_ACCOUNT, account_id);
  return BlockingRetrievePolicy(descriptor, policy_out);
}

void FakeSessionManagerClient::RetrievePolicy(
    const login_manager::PolicyDescriptor& descriptor,
    RetrievePolicyCallback callback) {
  if (policy_storage_ == PolicyStorageType::kOnDisk) {
    base::FilePath policy_path =
        GetStubPolicyFilePath(descriptor, nullptr /* key_path */);
    DCHECK(!policy_path.empty());

    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE,
        {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
        base::BindOnce(&GetFileContent, policy_path),
        base::BindOnce(std::move(callback),
                       RetrievePolicyResponseType::SUCCESS));
  } else {
    PostReplyWithPolicy(FROM_HERE, std::move(callback),
                        RetrievePolicyResponseType::SUCCESS,
                        policy_[GetMemoryStorageKey(descriptor)]);
  }
}

RetrievePolicyResponseType FakeSessionManagerClient::BlockingRetrievePolicy(
    const login_manager::PolicyDescriptor& descriptor,
    std::string* policy_out) {
  if (policy_storage_ == PolicyStorageType::kOnDisk) {
    base::FilePath policy_path =
        GetStubPolicyFilePath(descriptor, nullptr /* key_path */);
    DCHECK(!policy_path.empty());
    *policy_out = GetFileContent(policy_path);
  } else {
    *policy_out = policy_[GetMemoryStorageKey(descriptor)];
  }
  return RetrievePolicyResponseType::SUCCESS;
}

void FakeSessionManagerClient::StoreDevicePolicy(
    const std::string& policy_blob,
    VoidDBusMethodCallback callback) {
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_DEVICE, kEmptyAccountId);
  StorePolicy(descriptor, policy_blob, std::move(callback));
}

void FakeSessionManagerClient::StorePolicyForUser(
    const cryptohome::Identification& cryptohome_id,
    const std::string& policy_blob,
    VoidDBusMethodCallback callback) {
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_USER, cryptohome_id.id());
  StorePolicy(descriptor, policy_blob, std::move(callback));
}

void FakeSessionManagerClient::StoreDeviceLocalAccountPolicy(
    const std::string& account_id,
    const std::string& policy_blob,
    VoidDBusMethodCallback callback) {
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_DEVICE_LOCAL_ACCOUNT, account_id);
  StorePolicy(descriptor, policy_blob, std::move(callback));
}

void FakeSessionManagerClient::StorePolicy(
    const login_manager::PolicyDescriptor& descriptor,
    const std::string& policy_blob,
    VoidDBusMethodCallback callback) {
  // Decode the blob to get the new key.
  enterprise_management::PolicyFetchResponse response;
  if (!response.ParseFromString(policy_blob)) {
    PostReply(FROM_HERE, std::move(callback), false /* success */);
    return;
  }

  // Simulate failure.
  if (!store_policy_success_) {
    PostReply(FROM_HERE, std::move(callback), false /* success */);
    return;
  }

  if (policy_storage_ == PolicyStorageType::kOnDisk) {
    // Store policy and maybe key in files (background threads) and call
    // callback in main thread.
    base::FilePath key_path;
    base::FilePath policy_path = GetStubPolicyFilePath(descriptor, &key_path);
    DCHECK(!policy_path.empty());
    DCHECK(!key_path.empty());

    std::map<base::FilePath, std::string> files_to_store;
    files_to_store[policy_path] = policy_blob;
    if (response.has_new_public_key())
      files_to_store[key_path] = response.new_public_key();

    base::PostTaskWithTraitsAndReply(
        FROM_HERE,
        {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
        base::BindOnce(StoreFiles, std::move(files_to_store)),
        base::BindOnce(std::move(callback), true /* success */));
  } else {
    policy_[GetMemoryStorageKey(descriptor)] = policy_blob;
    PostReply(FROM_HERE, std::move(callback), true /* success */);

    if (IsChromeDevicePolicy(descriptor)) {
      // TODO(ljusten): For historical reasons, this code path only stores keys
      // for device policy. Should this be extended to other policy?
      if (response.has_new_public_key()) {
        base::FilePath key_path;
        GetStubPolicyFilePath(descriptor, &key_path);
        DCHECK(!key_path.empty());
        StoreFiles({{key_path, response.new_public_key()}});
        for (auto& observer : observers_)
          observer.OwnerKeySet(true /* success */);
      }
      for (auto& observer : observers_)
        observer.PropertyChangeComplete(true /* success */);
    }
  }
}

bool FakeSessionManagerClient::SupportsRestartToApplyUserFlags() const {
  return supports_restart_to_apply_user_flags_;
}

void FakeSessionManagerClient::SetFlagsForUser(
    const cryptohome::Identification& cryptohome_id,
    const std::vector<std::string>& flags) {
  flags_for_user_[cryptohome_id] = flags;
}

void FakeSessionManagerClient::GetServerBackedStateKeys(
    StateKeysCallback callback) {
  if (policy_storage_ == PolicyStorageType::kOnDisk) {
    base::FilePath owner_key_path;
    CHECK(base::PathService::Get(chromeos::FILE_OWNER_KEY, &owner_key_path));
    const base::FilePath state_keys_path =
        owner_key_path.DirName().AppendASCII(kStubStateKeysFileName);
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE,
        {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
        base::BindOnce(&ReadCreateStateKeysStub, state_keys_path),
        std::move(callback));
  } else {
    PostReply(FROM_HERE, std::move(callback), server_backed_state_keys_);
  }
}

void FakeSessionManagerClient::StartArcMiniContainer(
    const login_manager::StartArcMiniContainerRequest& request,
    StartArcMiniContainerCallback callback) {
  if (!arc_available_) {
    PostReply(FROM_HERE, std::move(callback), base::nullopt);
    return;
  }
  // This is starting a new container.
  base::Base64Encode(kFakeContainerInstanceId, &container_instance_id_);
  PostReply(FROM_HERE, std::move(callback), container_instance_id_);
}

void FakeSessionManagerClient::UpgradeArcContainer(
    const login_manager::UpgradeArcContainerRequest& request,
    UpgradeArcContainerCallback success_callback,
    UpgradeErrorCallback error_callback) {
  last_upgrade_arc_request_ = request;

  if (!arc_available_) {
    PostReply(FROM_HERE, std::move(error_callback), false);
    return;
  }
  if (low_disk_) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FakeSessionManagerClient::NotifyArcInstanceStopped,
                       weak_ptr_factory_.GetWeakPtr(),
                       login_manager::ArcContainerStopReason::LOW_DISK_SPACE,
                       std::move(container_instance_id_)));
    PostReply(FROM_HERE, std::move(error_callback), true);
    return;
  }
  PostReply(FROM_HERE, std::move(success_callback), base::ScopedFD());
}

void FakeSessionManagerClient::StopArcInstance(
    VoidDBusMethodCallback callback) {
  if (!arc_available_ || container_instance_id_.empty()) {
    PostReply(FROM_HERE, std::move(callback), false /* result */);
    return;
  }

  PostReply(FROM_HERE, std::move(callback), true /* result */);
  // Emulate ArcInstanceStopped signal propagation.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&FakeSessionManagerClient::NotifyArcInstanceStopped,
                     weak_ptr_factory_.GetWeakPtr(),
                     login_manager::ArcContainerStopReason::USER_REQUEST,
                     std::move(container_instance_id_)));
  container_instance_id_.clear();
}

void FakeSessionManagerClient::SetArcCpuRestriction(
    login_manager::ContainerCpuRestrictionState restriction_state,
    VoidDBusMethodCallback callback) {
  PostReply(FROM_HERE, std::move(callback), arc_available_);
}

void FakeSessionManagerClient::EmitArcBooted(
    const cryptohome::Identification& cryptohome_id,
    VoidDBusMethodCallback callback) {
  PostReply(FROM_HERE, std::move(callback), arc_available_);
}

void FakeSessionManagerClient::GetArcStartTime(
    DBusMethodCallback<base::TimeTicks> callback) {
  PostReply(
      FROM_HERE, std::move(callback),
      arc_available_ ? base::make_optional(arc_start_time_) : base::nullopt);
}

void FakeSessionManagerClient::RemoveArcData(
    const cryptohome::Identification& cryptohome_id,
    VoidDBusMethodCallback callback) {
  if (!callback.is_null())
    PostReply(FROM_HERE, std::move(callback), arc_available_);
}

void FakeSessionManagerClient::NotifyArcInstanceStopped(
    login_manager::ArcContainerStopReason reason,
    const std::string& container_instance_id) {
  for (auto& observer : observers_)
    observer.ArcInstanceStopped(reason, container_instance_id);
}

bool FakeSessionManagerClient::GetFlagsForUser(
    const cryptohome::Identification& cryptohome_id,
    std::vector<std::string>* out_flags_for_user) const {
  auto iter = flags_for_user_.find(cryptohome_id);
  if (iter == flags_for_user_.end())
    return false;

  *out_flags_for_user = iter->second;
  return true;
}

const std::string& FakeSessionManagerClient::device_policy() const {
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_DEVICE, kEmptyAccountId);
  DCHECK(policy_storage_ == PolicyStorageType::kInMemory);
  auto it = policy_.find(GetMemoryStorageKey(descriptor));
  return it != policy_.end() ? it->second : base::EmptyString();
}

void FakeSessionManagerClient::set_device_policy(
    const std::string& policy_blob) {
  DCHECK(policy_storage_ == PolicyStorageType::kInMemory);
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_DEVICE, kEmptyAccountId);
  policy_[GetMemoryStorageKey(descriptor)] = policy_blob;
}

const std::string& FakeSessionManagerClient::user_policy(
    const cryptohome::Identification& cryptohome_id) const {
  DCHECK(policy_storage_ == PolicyStorageType::kInMemory);
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_USER, cryptohome_id.id());
  auto it = policy_.find(GetMemoryStorageKey(descriptor));
  return it != policy_.end() ? it->second : base::EmptyString();
}

void FakeSessionManagerClient::set_user_policy(
    const cryptohome::Identification& cryptohome_id,
    const std::string& policy_blob) {
  DCHECK(policy_storage_ == PolicyStorageType::kInMemory);
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_USER, cryptohome_id.id());
  policy_[GetMemoryStorageKey(descriptor)] = policy_blob;
}

void FakeSessionManagerClient::set_user_policy_without_session(
    const cryptohome::Identification& cryptohome_id,
    const std::string& policy_blob) {
  DCHECK(policy_storage_ == PolicyStorageType::kInMemory);
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_SESSIONLESS_USER, cryptohome_id.id());
  policy_[GetMemoryStorageKey(descriptor)] = policy_blob;
}

const std::string& FakeSessionManagerClient::device_local_account_policy(
    const std::string& account_id) const {
  DCHECK(policy_storage_ == PolicyStorageType::kInMemory);
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_DEVICE_LOCAL_ACCOUNT, account_id);
  auto it = policy_.find(GetMemoryStorageKey(descriptor));
  return it != policy_.end() ? it->second : base::EmptyString();
}

void FakeSessionManagerClient::set_device_local_account_policy(
    const std::string& account_id,
    const std::string& policy_blob) {
  DCHECK(policy_storage_ == PolicyStorageType::kInMemory);
  login_manager::PolicyDescriptor descriptor = MakeChromePolicyDescriptor(
      login_manager::ACCOUNT_TYPE_DEVICE_LOCAL_ACCOUNT, account_id);
  policy_[GetMemoryStorageKey(descriptor)] = policy_blob;
}

void FakeSessionManagerClient::OnPropertyChangeComplete(bool success) {
  for (auto& observer : observers_)
    observer.PropertyChangeComplete(success);
}

}  // namespace chromeos
