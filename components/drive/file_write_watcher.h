// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_FILE_WRITE_WATCHER_H_
#define COMPONENTS_DRIVE_FILE_WRITE_WATCHER_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/drive/file_system_core_util.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace drive {

namespace internal {

typedef base::Callback<void(bool)> StartWatchCallback;

// The class watches modification to Drive files in the cache directory.
// This is used for returning a local writable snapshot of Drive files from the
// Save-As file dialog, so that the callers of the dialog can save to Drive
// without any special handling about Drive.
class FileWriteWatcher {
 public:
  explicit FileWriteWatcher(base::SequencedTaskRunner* blocking_task_runner);
  ~FileWriteWatcher();

  // Starts watching the modification to |path|. When it successfully started
  // watching, it runs |on_start_callback| by passing true as the argument.
  // Or if it failed, the callback is run with false.
  // Detected modification is notified by calling |on_write_callback|.
  //
  // Currently, the modification is watched in "one-shot" manner. That is, once
  // a modification is notified, the watch is deactivated for freeing system
  // resources. As a heuristic to capture the real end of write operations that
  // might be done by several chunked writes, the notification is fired after
  // 5 seconds has passed after the last write operation is detected.
  //
  // TODO(kinaba): investigate the possibility to continuously watch the whole
  // cache directory. http://crbug.com/269424
  void StartWatch(const base::FilePath& path,
                  const StartWatchCallback& on_start_callback,
                  const base::Closure& on_write_callback);

  // For testing purpose, stops inserting delay between the write detection and
  // notification to the |on_write_callback|.
  void DisableDelayForTesting();

 private:
  class FileWriteWatcherImpl;
  std::unique_ptr<FileWriteWatcherImpl, util::DestroyHelper> watcher_impl_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(FileWriteWatcher);
};

}  // namespace internal
}  // namespace drive

#endif  // COMPONENTS_DRIVE_FILE_WRITE_WATCHER_H_
