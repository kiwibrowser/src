// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_SERVICE_H_
#define CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_SERVICE_H_

#include <memory>
#include <string>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/file_system_provider/provider_interface.h"
#include "chrome/browser/chromeos/file_system_provider/service.h"
#include "chrome/browser/chromeos/smb_client/smb_errors.h"
#include "chrome/browser/chromeos/smb_client/temp_file_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/dbus/smb_provider_client.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class FilePath;
}  // namespace base

namespace chromeos {
namespace smb_client {

// These values are written to logs. New enum values may be added, but existing
// enums must never be runumbered or deleted and reused. Must be kept in sync
// with the SmbMountResult enum in
// chrome/browser/resources/settings/downloads_page/smb_browser_proxy.js.
enum SmbMountResult {
  SUCCESS = 0,                // Mount succeeded
  UNKNOWN_FAILURE = 1,        // Mount failed in an unrecognized way
  AUTHENTICATION_FAILED = 2,  // Authentication to the share failed
  NOT_FOUND = 3,              // The specified share was not found
  UNSUPPORTED_DEVICE = 4,     // The specified share is not supported
  MOUNT_EXISTS = 5            // The specified share is already mounted
};

using file_system_provider::Capabilities;
using file_system_provider::ProvidedFileSystemInfo;
using file_system_provider::ProvidedFileSystemInterface;
using file_system_provider::ProviderId;
using file_system_provider::ProviderInterface;
using file_system_provider::Service;

// Creates and manages an smb file system.
class SmbService : public KeyedService,
                   public base::SupportsWeakPtr<SmbService> {
 public:
  using MountResponse = base::OnceCallback<void(SmbMountResult result)>;

  explicit SmbService(Profile* profile);
  ~SmbService() override;

  // Gets the singleton instance for the |context|.
  static SmbService* Get(content::BrowserContext* context);

  // Starts the process of mounting an SMB file system.
  // Calls SmbProviderClient::Mount().
  void Mount(const file_system_provider::MountOptions& options,
             const base::FilePath& share_path,
             const std::string& username,
             const std::string& password,
             MountResponse callback);

  // Completes the mounting of an SMB file system, passing |options| on to
  // file_system_provider::Service::MountFileSystem(). Passes error status to
  // callback.
  void OnMountResponse(MountResponse callback,
                       const file_system_provider::MountOptions& options,
                       const base::FilePath& share_path,
                       smbprovider::ErrorType error,
                       int32_t mount_id);

 private:
  // Initializes temp_file_manager_.
  void InitTempFileManager();

  // Calls InitTempFileManager() and calls Mount.
  void InitTempFileManagerAndMount(
      const file_system_provider::MountOptions& options,
      const base::FilePath& share_path,
      const std::string& username,
      const std::string& password,
      MountResponse callback);

  // Calls SmbProviderClient::Mount(). temp_file_manager_ must be initialized
  // before this is called.
  void CallMount(const file_system_provider::MountOptions& options,
                 const base::FilePath& share_path,
                 const std::string& username,
                 const std::string& password,
                 MountResponse callback);

  // Calls file_system_provider::Service::UnmountFileSystem().
  base::File::Error Unmount(
      const std::string& file_system_id,
      file_system_provider::Service::UnmountReason reason) const;

  Service* GetProviderService() const;

  SmbProviderClient* GetSmbProviderClient() const;

  // Attempts to restore any previously mounted shares remembered by the File
  // System Provider.
  void RestoreMounts();

  // Attempts to remount a share with the information in |file_system_info|.
  void Remount(const ProvidedFileSystemInfo& file_system_info);

  // Handles the response from attempting to remount the file system. If
  // remounting fails, this logs and removes the file_system from the volume
  // manager.
  void OnRemountResponse(const std::string& file_system_id,
                         smbprovider::ErrorType error);

  // Sets up SmbService, including setting up Keberos if the user is ChromAD.
  void StartSetup();

  // Completes SmbService setup. Called by StartSetup().
  void CompleteSetup();

  // Handles the response from attempting to setup Kerberos.
  void OnSetupKerberosResponse(bool success);

  // Translates an error |error| into an SmbMountResult.
  SmbMountResult TranslateErrorToMountResult(
      smbprovider::ErrorType error) const;
  SmbMountResult TranslateErrorToMountResult(base::File::Error error) const;

  const ProviderId provider_id_;
  Profile* profile_;
  std::unique_ptr<TempFileManager> temp_file_manager_;

  DISALLOW_COPY_AND_ASSIGN(SmbService);
};

}  // namespace smb_client
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SMB_CLIENT_SMB_SERVICE_H_
