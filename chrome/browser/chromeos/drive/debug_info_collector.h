// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_DEBUG_INFO_COLLECTOR_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_DEBUG_INFO_COLLECTOR_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/drive/chromeos/file_system_interface.h"

namespace drive {

// This class provides some methods which are useful to show the debug
// info on chrome://drive-internals page.
// All the method should be called on UI thread.
class DebugInfoCollector {
 public:
  // Callback for ReadDirectory().
  typedef base::Callback<void(FileError error,
                              std::unique_ptr<ResourceEntryVector> entries)>
      ReadDirectoryCallback;

  // Callback for IterateFileCache().
  typedef base::Callback<void(const std::string& id,
                              const FileCacheEntry& cache_entry)>
      IterateFileCacheCallback;

  DebugInfoCollector(internal::ResourceMetadata* metadata,
                     FileSystemInterface* file_system,
                     base::SequencedTaskRunner* blocking_task_runner);
  ~DebugInfoCollector();

  // Finds a locally stored entry (a file or a directory) by |file_path|.
  // |callback| must not be null.
  void GetResourceEntry(const base::FilePath& file_path,
                        const GetResourceEntryCallback& callback);

  // Finds and reads a directory by |file_path|.
  // |callback| must not be null.
  void ReadDirectory(const base::FilePath& file_path,
                     const ReadDirectoryCallback& callback);


  // Iterates all files in the file cache and calls |iteration_callback| for
  // each file. |completion_callback| is run upon completion.
  // |iteration_callback| and |completion_callback| must not be null.
  void IterateFileCache(const IterateFileCacheCallback& iteration_callback,
                        const base::Closure& completion_callback);

  // Returns miscellaneous metadata of the file system like the largest
  // timestamp. |callback| must not be null.
  void GetMetadata(const GetFilesystemMetadataCallback& callback);

 private:
  internal::ResourceMetadata* metadata_;  // No owned.
  FileSystemInterface* file_system_;  // Not owned.
  scoped_refptr<base::SequencedTaskRunner> blocking_task_runner_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(DebugInfoCollector);
};

}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_DEBUG_INFO_COLLECTOR_H_
