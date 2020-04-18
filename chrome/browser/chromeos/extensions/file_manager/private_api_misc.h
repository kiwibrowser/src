// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file provides miscellaneous API functions, which don't belong to
// other files.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_PRIVATE_API_MISC_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_PRIVATE_API_MISC_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file.h"
#include "base/macros.h"
#include "chrome/browser/chromeos/extensions/file_manager/private_api_base.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_interface.h"
#include "chrome/browser/extensions/chrome_extension_function.h"
#include "chrome/browser/extensions/chrome_extension_function_details.h"
#include "chrome/common/extensions/api/file_manager_private.h"
#include "chromeos/disks/disk_mount_manager.h"
#include "google_apis/drive/drive_api_error_codes.h"
#include "storage/browser/fileapi/file_system_url.h"

namespace chromeos {
class RecentFile;
}  // namespace chromeos

namespace crostini {
enum class ConciergeClientResult;
}

namespace file_manager {
namespace util {
struct EntryDefinition;
typedef std::vector<EntryDefinition> EntryDefinitionList;
}  // namespace util
}  // namespace file_manager

namespace google_apis {
class AuthServiceInterface;
}  // namespace google_apis

namespace extensions {

// Implements the chrome.fileManagerPrivate.logoutUserForReauthentication
// method.
class FileManagerPrivateLogoutUserForReauthenticationFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.logoutUserForReauthentication",
                             FILEMANAGERPRIVATE_LOGOUTUSERFORREAUTHENTICATION)

 protected:
  ~FileManagerPrivateLogoutUserForReauthenticationFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

// Implements the chrome.fileManagerPrivate.getPreferences method.
// Gets settings for the Files app.
class FileManagerPrivateGetPreferencesFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.getPreferences",
                             FILEMANAGERPRIVATE_GETPREFERENCES)

 protected:
  ~FileManagerPrivateGetPreferencesFunction() override {}

  ResponseAction Run() override;
};

// Implements the chrome.fileManagerPrivate.setPreferences method.
// Sets settings for the Files app.
class FileManagerPrivateSetPreferencesFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.setPreferences",
                             FILEMANAGERPRIVATE_SETPREFERENCES)

 protected:
  ~FileManagerPrivateSetPreferencesFunction() override {}

  ResponseAction Run() override;
};

// Implements the chrome.fileManagerPrivate.zipSelection method.
// Creates a zip file for the selected files.
class FileManagerPrivateInternalZipSelectionFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.zipSelection",
                             FILEMANAGERPRIVATEINTERNAL_ZIPSELECTION)

  FileManagerPrivateInternalZipSelectionFunction();

 protected:
  ~FileManagerPrivateInternalZipSelectionFunction() override;

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;

  // Receives the result from ZipFileCreator.
  void OnZipDone(bool success);
};

// Implements the chrome.fileManagerPrivate.zoom method.
// Changes the zoom level of the file manager by modifying the zoom level of the
// WebContents.
// TODO(hirono): Remove this function once the zoom level change is supported
// for all apps. crbug.com/227175.
class FileManagerPrivateZoomFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.zoom",
                             FILEMANAGERPRIVATE_ZOOM);

 protected:
  ~FileManagerPrivateZoomFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

class FileManagerPrivateRequestWebStoreAccessTokenFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.requestWebStoreAccessToken",
                             FILEMANAGERPRIVATE_REQUESTWEBSTOREACCESSTOKEN);

  FileManagerPrivateRequestWebStoreAccessTokenFunction();

 protected:
  ~FileManagerPrivateRequestWebStoreAccessTokenFunction() override;

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;

 private:
  std::unique_ptr<google_apis::AuthServiceInterface> auth_service_;

  void OnAccessTokenFetched(google_apis::DriveApiErrorCode code,
                            const std::string& access_token);
};

class FileManagerPrivateGetProfilesFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.getProfiles",
                             FILEMANAGERPRIVATE_GETPROFILES);

 protected:
  ~FileManagerPrivateGetProfilesFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

// Implements the chrome.fileManagerPrivate.openInspector method.
class FileManagerPrivateOpenInspectorFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.openInspector",
                             FILEMANAGERPRIVATE_OPENINSPECTOR);

 protected:
  ~FileManagerPrivateOpenInspectorFunction() override {}

  ResponseAction Run() override;
};

// Implements the chrome.fileManagerPrivate.openSettingsSubpage method.
class FileManagerPrivateOpenSettingsSubpageFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.openSettingsSubpage",
                             FILEMANAGERPRIVATE_OPENSETTINGSSUBPAGE);

 protected:
  ~FileManagerPrivateOpenSettingsSubpageFunction() override {}

  ResponseAction Run() override;
};

// Implements the chrome.fileManagerPrivate.getMimeType method.
class FileManagerPrivateInternalGetMimeTypeFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.getMimeType",
                             FILEMANAGERPRIVATEINTERNAL_GETMIMETYPE)

  FileManagerPrivateInternalGetMimeTypeFunction();

 protected:
  ~FileManagerPrivateInternalGetMimeTypeFunction() override;

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;

  void OnGetMimeType(const std::string& mimeType);
};

// Implements the chrome.fileManagerPrivate.isPiexLoaderEnabled method.
class FileManagerPrivateIsPiexLoaderEnabledFunction
    : public UIThreadExtensionFunction {
 public:
  FileManagerPrivateIsPiexLoaderEnabledFunction() {}
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.isPiexLoaderEnabled",
                             FILEMANAGERPRIVATE_ISPIEXLOADERENABLED)
 protected:
  ~FileManagerPrivateIsPiexLoaderEnabledFunction() override {}

 private:
  ResponseAction Run() override;
  DISALLOW_COPY_AND_ASSIGN(FileManagerPrivateIsPiexLoaderEnabledFunction);
};

// Implements the chrome.fileManagerPrivate.getProviders method.
class FileManagerPrivateGetProvidersFunction
    : public UIThreadExtensionFunction {
 public:
  FileManagerPrivateGetProvidersFunction();
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.getProviders",
                             FILEMANAGERPRIVATE_GETPROVIDERS)
 protected:
  ~FileManagerPrivateGetProvidersFunction() override {}

 private:
  ResponseAction Run() override;
  const ChromeExtensionFunctionDetails chrome_details_;
  DISALLOW_COPY_AND_ASSIGN(FileManagerPrivateGetProvidersFunction);
};

// Implements the chrome.fileManagerPrivate.addProvidedFileSystem method.
class FileManagerPrivateAddProvidedFileSystemFunction
    : public UIThreadExtensionFunction {
 public:
  FileManagerPrivateAddProvidedFileSystemFunction();
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.addProvidedFileSystem",
                             FILEMANAGERPRIVATE_ADDPROVIDEDFILESYSTEM)
 protected:
  ~FileManagerPrivateAddProvidedFileSystemFunction() override {}

 private:
  ResponseAction Run() override;
  const ChromeExtensionFunctionDetails chrome_details_;
  DISALLOW_COPY_AND_ASSIGN(FileManagerPrivateAddProvidedFileSystemFunction);
};

// Implements the chrome.fileManagerPrivate.configureVolume method.
class FileManagerPrivateConfigureVolumeFunction
    : public UIThreadExtensionFunction {
 public:
  FileManagerPrivateConfigureVolumeFunction();
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.configureVolume",
                             FILEMANAGERPRIVATE_CONFIGUREVOLUME)
 protected:
  ~FileManagerPrivateConfigureVolumeFunction() override {}

 private:
  ResponseAction Run() override;
  void OnCompleted(base::File::Error result);

  const ChromeExtensionFunctionDetails chrome_details_;
  DISALLOW_COPY_AND_ASSIGN(FileManagerPrivateConfigureVolumeFunction);
};

// Implements the chrome.fileManagerPrivate.isCrostiniEnabled method.
// Gets crostini sftp mount params.
class FileManagerPrivateIsCrostiniEnabledFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.isCrostiniEnabled",
                             FILEMANAGERPRIVATE_ISCROSTINIENABLED)

 protected:
  ~FileManagerPrivateIsCrostiniEnabledFunction() override {}

  ResponseAction Run() override;
};

// Implements the chrome.fileManagerPrivate.mountCrostiniContainer method.
// Starts and mounts crostini container.
class FileManagerPrivateMountCrostiniContainerFunction
    : public LoggedAsyncExtensionFunction,
      public chromeos::disks::DiskMountManager::Observer {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.mountCrostiniContainer",
                             FILEMANAGERPRIVATE_MOUNTCROSTINICONTAINER)
  FileManagerPrivateMountCrostiniContainerFunction();

 protected:
  ~FileManagerPrivateMountCrostiniContainerFunction() override;

  bool RunAsync() override;
  void RestartCallback(crostini::ConciergeClientResult);
  void SshKeysCallback(crostini::ConciergeClientResult,
                       const std::string& container_public_key,
                       const std::string& host_private_key);

  // DiskMountManager::Observer
  void OnMountEvent(chromeos::disks::DiskMountManager::MountEvent event,
                    chromeos::MountError error_code,
                    const chromeos::disks::DiskMountManager::MountPointInfo&
                        mount_info) override;

 private:
  std::string source_path_;
  std::string mount_label_;
  // Self reference used to keep object alive to receive OnMountEvent.
  scoped_refptr<FileManagerPrivateMountCrostiniContainerFunction> self_;
};

// Implements the chrome.fileManagerPrivate.getCustomActions method.
class FileManagerPrivateInternalGetCustomActionsFunction
    : public UIThreadExtensionFunction {
 public:
  FileManagerPrivateInternalGetCustomActionsFunction();
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.getCustomActions",
                             FILEMANAGERPRIVATEINTERNAL_GETCUSTOMACTIONS)
 protected:
  ~FileManagerPrivateInternalGetCustomActionsFunction() override {}

 private:
  ResponseAction Run() override;
  void OnCompleted(const chromeos::file_system_provider::Actions& actions,
                   base::File::Error result);

  const ChromeExtensionFunctionDetails chrome_details_;
  DISALLOW_COPY_AND_ASSIGN(FileManagerPrivateInternalGetCustomActionsFunction);
};

// Implements the chrome.fileManagerPrivate.executeCustomAction method.
class FileManagerPrivateInternalExecuteCustomActionFunction
    : public UIThreadExtensionFunction {
 public:
  FileManagerPrivateInternalExecuteCustomActionFunction();
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.executeCustomAction",
                             FILEMANAGERPRIVATEINTERNAL_EXECUTECUSTOMACTION)
 protected:
  ~FileManagerPrivateInternalExecuteCustomActionFunction() override {}

 private:
  ResponseAction Run() override;
  void OnCompleted(base::File::Error result);

  const ChromeExtensionFunctionDetails chrome_details_;
  DISALLOW_COPY_AND_ASSIGN(
      FileManagerPrivateInternalExecuteCustomActionFunction);
};

// Implements the chrome.fileManagerPrivateInternal.getRecentFiles method.
class FileManagerPrivateInternalGetRecentFilesFunction
    : public UIThreadExtensionFunction {
 public:
  FileManagerPrivateInternalGetRecentFilesFunction();
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.getRecentFiles",
                             FILEMANAGERPRIVATE_GETRECENTFILES)
 protected:
  ~FileManagerPrivateInternalGetRecentFilesFunction() override {}

 private:
  ResponseAction Run() override;
  void OnGetRecentFiles(
      api::file_manager_private::SourceRestriction restriction,
      const std::vector<chromeos::RecentFile>& files);
  void OnConvertFileDefinitionListToEntryDefinitionList(
      std::unique_ptr<file_manager::util::EntryDefinitionList>
          entry_definition_list);

  const ChromeExtensionFunctionDetails chrome_details_;
  DISALLOW_COPY_AND_ASSIGN(FileManagerPrivateInternalGetRecentFilesFunction);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_PRIVATE_API_MISC_H_
