// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file provides Drive specific API functions.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_PRIVATE_API_DRIVE_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_PRIVATE_API_DRIVE_H_

#include <stddef.h>

#include <memory>
#include <string>
#include <vector>

#include "base/files/file.h"
#include "chrome/browser/chromeos/extensions/file_manager/private_api_base.h"
#include "chrome/browser/chromeos/file_manager/fileapi_util.h"
#include "components/drive/chromeos/file_system_interface.h"
#include "components/drive/file_errors.h"

namespace drive {
class ResourceEntry;
struct SearchResultInfo;
}

namespace google_apis {
class AuthService;
}

namespace extensions {

namespace api {
namespace file_manager_private {
struct EntryProperties;
}  // namespace file_manager_private
}  // namespace api

// Implements the chrome.fileManagerPrivate.ensureFileDownloaded method.
class FileManagerPrivateInternalEnsureFileDownloadedFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.ensureFileDownloaded",
                             FILEMANAGERPRIVATE_ENSUREFILEDOWNLOADED)

 protected:
  ~FileManagerPrivateInternalEnsureFileDownloadedFunction() override {}

  // AsyncExtensionFunction overrides.
  bool RunAsync() override;

 private:
  // Callback for RunAsync().
  void OnDownloadFinished(drive::FileError error,
                          const base::FilePath& file_path,
                          std::unique_ptr<drive::ResourceEntry> entry);
};

// Retrieves property information for an entry and returns it as a dictionary.
// On error, returns a dictionary with the key "error" set to the error number
// (base::File::Error).
class FileManagerPrivateInternalGetEntryPropertiesFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.getEntryProperties",
                             FILEMANAGERPRIVATEINTERNAL_GETENTRYPROPERTIES)

  FileManagerPrivateInternalGetEntryPropertiesFunction();

 protected:
  ~FileManagerPrivateInternalGetEntryPropertiesFunction() override;

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;

 private:
  void CompleteGetEntryProperties(
      size_t index,
      const storage::FileSystemURL& url,
      std::unique_ptr<api::file_manager_private::EntryProperties> properties,
      base::File::Error error);

  size_t processed_count_;
  std::vector<api::file_manager_private::EntryProperties> properties_list_;
};

// Implements the chrome.fileManagerPrivate.pinDriveFile method.
class FileManagerPrivateInternalPinDriveFileFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.pinDriveFile",
                             FILEMANAGERPRIVATEINTERNAL_PINDRIVEFILE)

 protected:
  ~FileManagerPrivateInternalPinDriveFileFunction() override {}

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;

 private:
  // Callback for RunAsync().
  void OnPinStateSet(drive::FileError error);
};

// Implements the chrome.fileManagerPrivate.cancelFileTransfers method.
class FileManagerPrivateInternalCancelFileTransfersFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.cancelFileTransfers",
                             FILEMANAGERPRIVATEINTERNAL_CANCELFILETRANSFERS)

 protected:
  ~FileManagerPrivateInternalCancelFileTransfersFunction() override {}

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;
};

// Implements the chrome.fileManagerPrivate.cancelAllFileTransfers method.
class FileManagerPrivateCancelAllFileTransfersFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.cancelAllFileTransfers",
                             FILEMANAGERPRIVATE_CANCELALLFILETRANSFERS)

 protected:
  ~FileManagerPrivateCancelAllFileTransfersFunction() override {}

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;
};

class FileManagerPrivateSearchDriveFunction
    : public LoggedAsyncExtensionFunction {
 public:
  typedef std::vector<drive::SearchResultInfo> SearchResultInfoList;

  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.searchDrive",
                             FILEMANAGERPRIVATE_SEARCHDRIVE)

 protected:
  ~FileManagerPrivateSearchDriveFunction() override {}

  bool RunAsync() override;

 private:
  // Callback for Search().
  void OnSearch(
      drive::FileError error,
      const GURL& next_link,
      std::unique_ptr<std::vector<drive::SearchResultInfo>> result_paths);

  // Called when |result_paths| in OnSearch() are converted to a list of
  // entry definitions.
  void OnEntryDefinitionList(
      const GURL& next_link,
      std::unique_ptr<SearchResultInfoList> search_result_info_list,
      std::unique_ptr<file_manager::util::EntryDefinitionList>
          entry_definition_list);
};

// Similar to FileManagerPrivateSearchDriveFunction but this one is used for
// searching drive metadata which is stored locally.
class FileManagerPrivateSearchDriveMetadataFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.searchDriveMetadata",
                             FILEMANAGERPRIVATE_SEARCHDRIVEMETADATA)

 protected:
  ~FileManagerPrivateSearchDriveMetadataFunction() override {}

  bool RunAsync() override;

 private:
  // Callback for SearchMetadata();
  void OnSearchMetadata(
      drive::FileError error,
      std::unique_ptr<drive::MetadataSearchResultVector> results);

  // Called when |results| in OnSearchMetadata() are converted to a list of
  // entry definitions.
  void OnEntryDefinitionList(
      std::unique_ptr<drive::MetadataSearchResultVector>
          search_result_info_list,
      std::unique_ptr<file_manager::util::EntryDefinitionList>
          entry_definition_list);
};

// Implements the chrome.fileManagerPrivate.getDriveConnectionState method.
class FileManagerPrivateGetDriveConnectionStateFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION(
      "fileManagerPrivate.getDriveConnectionState",
      FILEMANAGERPRIVATE_GETDRIVECONNECTIONSTATE);

 protected:
  ~FileManagerPrivateGetDriveConnectionStateFunction() override {}

  ResponseAction Run() override;
};

// Implements the chrome.fileManagerPrivate.requestAccessToken method.
class FileManagerPrivateRequestAccessTokenFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivate.requestAccessToken",
                             FILEMANAGERPRIVATE_REQUESTACCESSTOKEN)

 protected:
  ~FileManagerPrivateRequestAccessTokenFunction() override {}

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;

  // Callback with a cached auth token (if available) or a fetched one.
  void OnAccessTokenFetched(google_apis::DriveApiErrorCode code,
                            const std::string& access_token);
};

// Implements the chrome.fileManagerPrivate.getShareUrl method.
class FileManagerPrivateInternalGetShareUrlFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.getShareUrl",
                             FILEMANAGERPRIVATEINTERNAL_GETSHAREURL)

 protected:
  ~FileManagerPrivateInternalGetShareUrlFunction() override {}

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;

  // Callback with an url to the sharing dialog as |share_url|, called by
  // FileSystem::GetShareUrl.
  void OnGetShareUrl(drive::FileError error, const GURL& share_url);
};

// Implements the chrome.fileManagerPrivate.requestDriveShare method.
class FileManagerPrivateInternalRequestDriveShareFunction
    : public LoggedAsyncExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.requestDriveShare",
                             FILEMANAGERPRIVATEINTERNAL_REQUESTDRIVESHARE);

 protected:
  ~FileManagerPrivateInternalRequestDriveShareFunction() override {}
  bool RunAsync() override;

 private:
  // Called back after the drive file system operation is finished.
  void OnAddPermission(drive::FileError error);
};

// Implements the chrome.fileManagerPrivate.getDownloadUrl method.
class FileManagerPrivateInternalGetDownloadUrlFunction
    : public LoggedAsyncExtensionFunction {
 public:
  FileManagerPrivateInternalGetDownloadUrlFunction();

  DECLARE_EXTENSION_FUNCTION("fileManagerPrivateInternal.getDownloadUrl",
                             FILEMANAGERPRIVATEINTERNAL_GETDOWNLOADURL)

 protected:
  ~FileManagerPrivateInternalGetDownloadUrlFunction() override;

  // ChromeAsyncExtensionFunction overrides.
  bool RunAsync() override;

  void OnGetResourceEntry(drive::FileError error,
                          std::unique_ptr<drive::ResourceEntry> entry);

  // Callback with an |access_token|, called by
  // drive::DriveReadonlyTokenFetcher.
  void OnTokenFetched(google_apis::DriveApiErrorCode code,
                      const std::string& access_token);

 private:
  GURL download_url_;
  std::unique_ptr<google_apis::AuthService> auth_service_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_FILE_MANAGER_PRIVATE_API_DRIVE_H_
