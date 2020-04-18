// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_FILE_SYSTEM_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_FILE_SYSTEM_UTIL_H_

#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "components/drive/file_errors.h"
#include "url/gurl.h"

class Profile;

namespace storage {
class FileSystemURL;
}

namespace drive {

class DriveAppRegistry;
class DriveServiceInterface;
class FileSystemInterface;

namespace util {

// Returns the Drive mount point path, which looks like "/special/drive-<hash>".
base::FilePath GetDriveMountPointPath(Profile* profile);

// Returns the Drive mount point path, which looks like
// "/special/drive-<username_hash>", when provided with the |user_id_hash|.
base::FilePath GetDriveMountPointPathForUserIdHash(
    const std::string& user_id_hash);

// Returns true if the given path is under the Drive mount point.
bool IsUnderDriveMountPoint(const base::FilePath& path);

// Extracts the Drive path from the given path located under the Drive mount
// point. Returns an empty path if |path| is not under the Drive mount point.
// Examples: ExtractDrivePath("/special/drive-xxx/foo.txt") => "drive/foo.txt"
base::FilePath ExtractDrivePath(const base::FilePath& path);

// Returns the FileSystem for the |profile|. If not available (not mounted
// or disabled), returns NULL.
FileSystemInterface* GetFileSystemByProfile(Profile* profile);

// Returns a FileSystemInterface instance for the |profile_id|, or NULL
// if the Profile for |profile_id| is destructed or Drive File System is
// disabled for the profile.
// Note: |profile_id| should be the pointer of the Profile instance if it is
// alive. Considering timing issues due to task posting across threads,
// this function can accept a dangling pointer as |profile_id| (and will return
// NULL for such a case).
// This function must be called on UI thread.
FileSystemInterface* GetFileSystemByProfileId(void* profile_id);

// Returns the DriveAppRegistry for the |profile|. If not available (not
// mounted or disabled), returns NULL.
DriveAppRegistry* GetDriveAppRegistryByProfile(Profile* profile);

// Returns the DriveService for the |profile|. If not available (not mounted
// or disabled), returns NULL.
DriveServiceInterface* GetDriveServiceByProfile(Profile* profile);

// Extracts |profile| from the given paths located under
// GetDriveMountPointPath(profile). Returns NULL if it does not correspond to
// a valid mount point path. Must be called from UI thread.
Profile* ExtractProfileFromPath(const base::FilePath& path);

// Extracts the Drive path (e.g., "drive/foo.txt") from the filesystem URL.
// Returns an empty path if |url| does not point under Drive mount point.
base::FilePath ExtractDrivePathFromFileSystemUrl(
    const storage::FileSystemURL& url);

// Gets the cache root path (i.e. <user_profile_dir>/GCache/v1) from the
// profile.
base::FilePath GetCacheRootPath(Profile* profile);

// Callback type for PrepareWritableFileAndRun.
typedef base::Callback<void(FileError, const base::FilePath& path)>
    PrepareWritableFileCallback;

// Invokes |callback| on blocking thread pool, after converting virtual |path|
// string like "/special/drive/foo.txt" to the concrete local cache file path.
// After |callback| returns, the written content is synchronized to the server.
//
// The |path| must be a path under Drive. Must be called from UI thread.
void PrepareWritableFileAndRun(Profile* profile,
                               const base::FilePath& path,
                               const PrepareWritableFileCallback& callback);

// Ensures the existence of |directory| of '/special/drive/foo'.  This will
// create |directory| and its ancestors if they don't exist.  |callback| is
// invoked after making sure that |directory| exists.  |callback| should
// interpret error codes of either FILE_ERROR_OK or FILE_ERROR_EXISTS as
// indicating that |directory| now exists.
//
// If |directory| is not a Drive path, it won't check the existence and just
// runs |callback|.
//
// Must be called from UI thread.
void EnsureDirectoryExists(Profile* profile,
                           const base::FilePath& directory,
                           const FileOperationCallback& callback);

// Returns true if Drive is enabled for the given Profile.
bool IsDriveEnabledForProfile(Profile* profile);

// Enum type for describing the current connection status to Drive.
enum ConnectionStatusType {
  // Disconnected because Drive service is unavailable for this account (either
  // disabled by a flag or the account has no Google account (e.g., guests)).
  DRIVE_DISCONNECTED_NOSERVICE,
  // Disconnected because no network is available.
  DRIVE_DISCONNECTED_NONETWORK,
  // Disconnected because authentication is not ready.
  DRIVE_DISCONNECTED_NOTREADY,
  // Connected by cellular network. Background sync is disabled.
  DRIVE_CONNECTED_METERED,
  // Connected without condition (WiFi, Ethernet, or cellular with the
  // disable-sync preference turned off.)
  DRIVE_CONNECTED,
};

// Returns the Drive connection status for the |profile|.
ConnectionStatusType GetDriveConnectionStatus(Profile* profile);

}  // namespace util
}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_FILE_SYSTEM_UTIL_H_
