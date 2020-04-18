// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/smb_client/smb_service.h"

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/smb_client/smb_errors.h"
#include "chrome/browser/chromeos/smb_client/smb_file_system.h"
#include "chrome/browser/chromeos/smb_client/smb_file_system_id.h"
#include "chrome/browser/chromeos/smb_client/smb_provider.h"
#include "chrome/browser/chromeos/smb_client/smb_service_factory.h"
#include "chrome/browser/chromeos/smb_client/smb_service_helper.h"
#include "chrome/common/chrome_features.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/smb_provider_client.h"

using chromeos::file_system_provider::Service;

namespace chromeos {
namespace smb_client {

namespace {

bool ContainsAt(const std::string& username) {
  return username.find('@') != std::string::npos;
}

}  // namespace

SmbService::SmbService(Profile* profile)
    : provider_id_(ProviderId::CreateFromNativeId("smb")), profile_(profile) {
  if (base::FeatureList::IsEnabled(features::kNativeSmb)) {
    StartSetup();
  }
}

SmbService::~SmbService() {}

// static
SmbService* SmbService::Get(content::BrowserContext* context) {
  return SmbServiceFactory::Get(context);
}

void SmbService::Mount(const file_system_provider::MountOptions& options,
                       const base::FilePath& share_path,
                       const std::string& username,
                       const std::string& password,
                       MountResponse callback) {
  if (!temp_file_manager_) {
    InitTempFileManagerAndMount(options, share_path, username, password,
                                std::move(callback));
    return;
  }

  CallMount(options, share_path, username, password, std::move(callback));
}

void SmbService::InitTempFileManagerAndMount(
    const file_system_provider::MountOptions& options,
    const base::FilePath& share_path,
    const std::string& username,
    const std::string& password,
    MountResponse callback) {
  // InitTempFileManager() has to be called on a separate thread since it
  // contains a call that requires a blockable thread.
  base::TaskTraits task_traits = {base::MayBlock(),
                                  base::TaskPriority::USER_BLOCKING,
                                  base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN};
  base::OnceClosure task =
      base::BindOnce(&SmbService::InitTempFileManager, base::Unretained(this));
  base::OnceClosure reply =
      base::BindOnce(&SmbService::CallMount, base::Unretained(this), options,
                     share_path, username, password, base::Passed(&callback));

  base::PostTaskWithTraitsAndReply(FROM_HERE, task_traits, std::move(task),
                                   std::move(reply));
}

void SmbService::CallMount(const file_system_provider::MountOptions& options,
                           const base::FilePath& share_path,
                           const std::string& username_input,
                           const std::string& password_input,
                           MountResponse callback) {
  std::string username;
  std::string password;
  std::string workgroup;

  if (username_input.empty()) {
    // If no credentials were provided and the user is ChromAD, pass the users
    // username and workgroup for their email address to be used for Kerberos
    // authentication.
    user_manager::User* user =
        chromeos::ProfileHelper::Get()->GetUserByProfile(profile_);
    if (user && user->IsActiveDirectoryUser()) {
      ParseUserPrincipalName(user->GetDisplayEmail(), &username, &workgroup);
    }
  } else {
    // Credentials were provided so use them and parse the username into
    // username and workgroup if neccessary.
    username = username_input;
    password = password_input;
    if (ContainsAt(username)) {
      ParseUserPrincipalName(username_input, &username, &workgroup);
    }
  }

  // TODO(allenvic): Add URL parsing here once SmbUrl can be used without adding
  // a standard scheme.

  // TODO(allenvic): Resolve parsed_url here using NetworkScanner once name
  // resolution is wired up.
  GetSmbProviderClient()->Mount(
      share_path, workgroup, username,
      temp_file_manager_->WritePasswordToFile(password),
      base::BindOnce(&SmbService::OnMountResponse, AsWeakPtr(),
                     base::Passed(&callback), options, share_path));
}

void SmbService::OnMountResponse(
    MountResponse callback,
    const file_system_provider::MountOptions& options,
    const base::FilePath& share_path,
    smbprovider::ErrorType error,
    int32_t mount_id) {
  if (error != smbprovider::ERROR_OK) {
    std::move(callback).Run(TranslateErrorToMountResult(error));
    return;
  }

  DCHECK_GE(mount_id, 0);

  file_system_provider::MountOptions mount_options(options);
  mount_options.file_system_id = CreateFileSystemId(mount_id, share_path);

  base::File::Error result =
      GetProviderService()->MountFileSystem(provider_id_, mount_options);

  std::move(callback).Run(TranslateErrorToMountResult(result));
}

base::File::Error SmbService::Unmount(
    const std::string& file_system_id,
    file_system_provider::Service::UnmountReason reason) const {
  return GetProviderService()->UnmountFileSystem(provider_id_, file_system_id,
                                                 reason);
}

Service* SmbService::GetProviderService() const {
  return file_system_provider::Service::Get(profile_);
}

SmbProviderClient* SmbService::GetSmbProviderClient() const {
  return chromeos::DBusThreadManager::Get()->GetSmbProviderClient();
}

void SmbService::RestoreMounts() {
  const std::vector<ProvidedFileSystemInfo> file_systems =
      GetProviderService()->GetProvidedFileSystemInfoList(provider_id_);

  for (const auto& file_system : file_systems) {
    Remount(file_system);
  }
}

void SmbService::Remount(const ProvidedFileSystemInfo& file_system_info) {
  const base::FilePath share_path =
      GetSharePathFromFileSystemId(file_system_info.file_system_id());
  const int32_t mount_id =
      GetMountIdFromFileSystemId(file_system_info.file_system_id());
  GetSmbProviderClient()->Remount(
      share_path, mount_id,
      base::BindOnce(&SmbService::OnRemountResponse, AsWeakPtr(),
                     file_system_info.file_system_id()));
}

void SmbService::OnRemountResponse(const std::string& file_system_id,
                                   smbprovider::ErrorType error) {
  if (error != smbprovider::ERROR_OK) {
    LOG(ERROR) << "SmbService: failed to restore filesystem: "
               << file_system_id;
    Unmount(file_system_id, file_system_provider::Service::UNMOUNT_REASON_USER);
  }
}

void SmbService::InitTempFileManager() {
  temp_file_manager_ = std::make_unique<TempFileManager>();
}

void SmbService::StartSetup() {
  user_manager::User* user =
      chromeos::ProfileHelper::Get()->GetUserByProfile(profile_);

  if (!user) {
    // An instance of SmbService is created on the lockscreen. When this
    // instance is created, no setup will run.
    return;
  }

  if (user->IsActiveDirectoryUser()) {
    auto account_id = user->GetAccountId();
    const std::string account_id_guid = account_id.GetObjGuid();

    GetSmbProviderClient()->SetupKerberos(
        account_id_guid,
        base::BindOnce(&SmbService::OnSetupKerberosResponse, AsWeakPtr()));
    return;
  }

  CompleteSetup();
}

void SmbService::OnSetupKerberosResponse(bool success) {
  if (!success) {
    LOG(ERROR) << "SmbService: Kerberos setup failed.";
  }

  CompleteSetup();
}

void SmbService::CompleteSetup() {
  GetProviderService()->RegisterProvider(std::make_unique<SmbProvider>(
      base::BindRepeating(&SmbService::Unmount, base::Unretained(this))));
  RestoreMounts();
}

SmbMountResult SmbService::TranslateErrorToMountResult(
    smbprovider::ErrorType error) const {
  DCHECK_NE(smbprovider::ERROR_NONE, error);

  switch (error) {
    case smbprovider::ERROR_OK:
      return SmbMountResult::SUCCESS;
    case smbprovider::ERROR_EXISTS:
    case smbprovider::ERROR_IN_USE:
      return SmbMountResult::MOUNT_EXISTS;
    case smbprovider::ERROR_NOT_FOUND:
    case smbprovider::ERROR_NOT_A_DIRECTORY:
    case smbprovider::ERROR_INVALID_URL:
      return SmbMountResult::NOT_FOUND;
    case smbprovider::ERROR_ACCESS_DENIED:
    case smbprovider::ERROR_SECURITY:
      return SmbMountResult::AUTHENTICATION_FAILED;
    case smbprovider::ERROR_FAILED:
    case smbprovider::ERROR_TOO_MANY_OPENED:
    case smbprovider::ERROR_NO_MEMORY:
    case smbprovider::ERROR_NO_SPACE:
    case smbprovider::ERROR_INVALID_OPERATION:
    case smbprovider::ERROR_ABORT:
    case smbprovider::ERROR_NOT_A_FILE:
    case smbprovider::ERROR_NOT_EMPTY:
    case smbprovider::ERROR_IO:
    case smbprovider::ERROR_DBUS_PARSE_FAILED:
      return SmbMountResult::UNKNOWN_FAILURE;
    default:
      break;
  }

  NOTREACHED();
  return SmbMountResult::UNKNOWN_FAILURE;
}

SmbMountResult SmbService::TranslateErrorToMountResult(
    base::File::Error error) const {
  return TranslateErrorToMountResult(TranslateToErrorType(error));
}

}  // namespace smb_client
}  // namespace chromeos
