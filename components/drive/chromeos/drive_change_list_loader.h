// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_DRIVE_CHANGE_LIST_LOADER_H_
#define COMPONENTS_DRIVE_CHROMEOS_DRIVE_CHANGE_LIST_LOADER_H_

#include "components/drive/chromeos/file_system_interface.h"
#include "components/drive/file_errors.h"

namespace drive {
namespace internal {

class ChangeListLoaderObserver;

// DriveChangeListLoader provides an abstraction for loading change lists, the
// full resource list amd directory contents, from Google Drive API.
// Implementations of the interface can, for example, include:
// - An implementation to retrieve from the users default corpus.
// - An implementation to retrieve from a specific team drive.
class DriveChangeListLoader {
 public:
  virtual ~DriveChangeListLoader() = default;

  // Adds and removes an observer for change list loading events.
  virtual void AddObserver(ChangeListLoaderObserver* observer) = 0;
  virtual void RemoveObserver(ChangeListLoaderObserver* observer) = 0;

  // Indicates whether there is a request for full resource list or change
  // list fetching is in flight (i.e. directory contents fetching does not
  // count).
  virtual bool IsRefreshing() = 0;

  // Starts the change list loading if needed.
  // |callback| must not be null.
  virtual void LoadIfNeeded(const FileOperationCallback& callback) = 0;

  // Check for updates on the server. May do nothing if a change list is
  // already in the process of being loaded.
  // |callback| must not be null.
  virtual void CheckForUpdates(const FileOperationCallback& callback) = 0;

  // Reads the given directory contents.
  // |entries_callback| can be null.
  // |completion_callback| must not be null.
  virtual void ReadDirectory(
      const base::FilePath& directory_path,
      const ReadDirectoryEntriesCallback& entries_callback,
      const FileOperationCallback& completion_callback) = 0;
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_DRIVE_CHANGE_LIST_LOADER_H_
