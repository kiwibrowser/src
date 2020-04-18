// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_CHANGE_LIST_LOADER_OBSERVER_H_
#define COMPONENTS_DRIVE_CHROMEOS_CHANGE_LIST_LOADER_OBSERVER_H_

namespace base {
class FilePath;
}  // namespace base

namespace drive {
class FileChange;
namespace internal {

// Interface for classes that need to observe events from ChangeListLoader.
// All events are notified on UI thread.
class ChangeListLoaderObserver {
 public:
  // Triggered when the content of a directory is reloaded. The content may
  // changed.  |directory_path| is a virtual directory path representing the
  // reloaded directory.
  virtual void OnDirectoryReloaded(const base::FilePath& directory_path) {}

  // Triggered when content(s) in drive has been changed.
  virtual void OnFileChanged(const FileChange& changed_files) {}

  // Triggered when loading from the server is complete.
  virtual void OnLoadFromServerComplete() {}

  // Triggered when loading is complete for the first time, either from the
  // the server or the cache. To be precise, for the latter case, we do not
  // load anything from the cache. We just check that the resource metadata
  // is stored locally thus can be used.
  virtual void OnInitialLoadComplete() {}

 protected:
  virtual ~ChangeListLoaderObserver() = default;
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_CHANGE_LIST_LOADER_OBSERVER_H_
