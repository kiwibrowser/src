// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/sync_file_system/sync_file_system_api_helpers.h"

#include "base/logging.h"
#include "base/values.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/common/fileapi/file_system_util.h"

namespace extensions {

api::sync_file_system::ServiceStatus SyncServiceStateToExtensionEnum(
    sync_file_system::SyncServiceState state) {
  switch (state) {
    case sync_file_system::SYNC_SERVICE_RUNNING:
      return api::sync_file_system::SERVICE_STATUS_RUNNING;
    case sync_file_system::SYNC_SERVICE_AUTHENTICATION_REQUIRED:
      return api::sync_file_system::SERVICE_STATUS_AUTHENTICATION_REQUIRED;
    case sync_file_system::SYNC_SERVICE_TEMPORARY_UNAVAILABLE:
      return api::sync_file_system::SERVICE_STATUS_TEMPORARY_UNAVAILABLE;
    case sync_file_system::SYNC_SERVICE_DISABLED:
      return api::sync_file_system::SERVICE_STATUS_DISABLED;
  }
  NOTREACHED() << "Invalid state: " << state;
  return api::sync_file_system::SERVICE_STATUS_NONE;
}

api::sync_file_system::FileStatus SyncFileStatusToExtensionEnum(
    sync_file_system::SyncFileStatus status) {
  switch (status) {
    case sync_file_system::SYNC_FILE_STATUS_SYNCED:
      return api::sync_file_system::FILE_STATUS_SYNCED;
    case sync_file_system::SYNC_FILE_STATUS_HAS_PENDING_CHANGES:
      return api::sync_file_system::FILE_STATUS_PENDING;
    case sync_file_system::SYNC_FILE_STATUS_CONFLICTING:
      return api::sync_file_system::FILE_STATUS_CONFLICTING;
    case sync_file_system::SYNC_FILE_STATUS_UNKNOWN:
      return api::sync_file_system::FILE_STATUS_NONE;
  }
  NOTREACHED() << "Invalid status: " << status;
  return api::sync_file_system::FILE_STATUS_NONE;
}

api::sync_file_system::SyncAction SyncActionToExtensionEnum(
    sync_file_system::SyncAction action) {
  switch (action) {
    case sync_file_system::SYNC_ACTION_ADDED:
      return api::sync_file_system::SYNC_ACTION_ADDED;
    case sync_file_system::SYNC_ACTION_UPDATED:
      return api::sync_file_system::SYNC_ACTION_UPDATED;
    case sync_file_system::SYNC_ACTION_DELETED:
      return api::sync_file_system::SYNC_ACTION_DELETED;
    case sync_file_system::SYNC_ACTION_NONE:
      return api::sync_file_system::SYNC_ACTION_NONE;
  }
  NOTREACHED() << "Invalid action: " << action;
  return api::sync_file_system::SYNC_ACTION_NONE;
}

api::sync_file_system::SyncDirection SyncDirectionToExtensionEnum(
    sync_file_system::SyncDirection direction) {
  switch (direction) {
    case sync_file_system::SYNC_DIRECTION_LOCAL_TO_REMOTE:
      return api::sync_file_system::SYNC_DIRECTION_LOCAL_TO_REMOTE;
    case sync_file_system::SYNC_DIRECTION_REMOTE_TO_LOCAL:
      return api::sync_file_system::SYNC_DIRECTION_REMOTE_TO_LOCAL;
    case sync_file_system::SYNC_DIRECTION_NONE:
      return api::sync_file_system::SYNC_DIRECTION_NONE;
  }
  NOTREACHED() << "Invalid direction: " << direction;
  return api::sync_file_system::SYNC_DIRECTION_NONE;
}

sync_file_system::ConflictResolutionPolicy
ExtensionEnumToConflictResolutionPolicy(
    api::sync_file_system::ConflictResolutionPolicy policy) {
  switch (policy) {
    case api::sync_file_system::CONFLICT_RESOLUTION_POLICY_NONE:
      return sync_file_system::CONFLICT_RESOLUTION_POLICY_UNKNOWN;
    case api::sync_file_system::CONFLICT_RESOLUTION_POLICY_LAST_WRITE_WIN:
      return sync_file_system::CONFLICT_RESOLUTION_POLICY_LAST_WRITE_WIN;
    case api::sync_file_system::CONFLICT_RESOLUTION_POLICY_MANUAL:
      return sync_file_system::CONFLICT_RESOLUTION_POLICY_MANUAL;
  }
  NOTREACHED() << "Invalid conflict resolution policy: " << policy;
  return sync_file_system::CONFLICT_RESOLUTION_POLICY_UNKNOWN;
}

api::sync_file_system::ConflictResolutionPolicy
ConflictResolutionPolicyToExtensionEnum(
    sync_file_system::ConflictResolutionPolicy policy) {
  switch (policy) {
    case sync_file_system::CONFLICT_RESOLUTION_POLICY_UNKNOWN:
      return api::sync_file_system::CONFLICT_RESOLUTION_POLICY_NONE;
    case sync_file_system::CONFLICT_RESOLUTION_POLICY_LAST_WRITE_WIN:
        return api::sync_file_system::CONFLICT_RESOLUTION_POLICY_LAST_WRITE_WIN;
    case sync_file_system::CONFLICT_RESOLUTION_POLICY_MANUAL:
      return api::sync_file_system::CONFLICT_RESOLUTION_POLICY_MANUAL;
    case sync_file_system::CONFLICT_RESOLUTION_POLICY_MAX:
      NOTREACHED();
      return api::sync_file_system::CONFLICT_RESOLUTION_POLICY_NONE;
  }
  NOTREACHED() << "Invalid conflict resolution policy: " << policy;
  return api::sync_file_system::CONFLICT_RESOLUTION_POLICY_NONE;
}

std::unique_ptr<base::DictionaryValue> CreateDictionaryValueForFileSystemEntry(
    const storage::FileSystemURL& url,
    sync_file_system::SyncFileType file_type) {
  if (!url.is_valid() || file_type == sync_file_system::SYNC_FILE_TYPE_UNKNOWN)
    return nullptr;

  std::string file_path =
      base::FilePath(storage::VirtualPath::GetNormalizedFilePath(url.path()))
          .AsUTF8Unsafe();

  std::string root_url =
      storage::GetFileSystemRootURI(url.origin(), url.mount_type()).spec();
  if (!url.filesystem_id().empty()) {
    root_url.append(url.filesystem_id());
    root_url.append("/");
  }

  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetString("fileSystemType",
                  storage::GetFileSystemTypeString(url.mount_type()));
  dict->SetString("fileSystemName",
                  storage::GetFileSystemName(url.origin(), url.type()));
  dict->SetString("rootUrl", root_url);
  dict->SetString("filePath", file_path);
  dict->SetBoolean("isDirectory",
                   (file_type == sync_file_system::SYNC_FILE_TYPE_DIRECTORY));

  return dict;
}

}  // namespace extensions
