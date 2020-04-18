// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_FILE_TASK_EXECUTOR_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_FILE_TASK_EXECUTOR_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/file_manager/file_tasks.h"
#include "components/drive/file_errors.h"
#include "google_apis/drive/drive_api_error_codes.h"

namespace drive {

class DriveServiceInterface;
class FileSystemInterface;
class ResourceEntry;

// Delegate class for hooking fake instances and behaviors for testing.
class FileTaskExecutorDelegate {
 public:
  virtual ~FileTaskExecutorDelegate() {}

  virtual FileSystemInterface* GetFileSystem() = 0;
  virtual DriveServiceInterface* GetDriveService() = 0;
  virtual void OpenBrowserWindow(const GURL& open_link) = 0;
};

// This class implements an "executor" class that will execute tasks for
// third party Drive apps that store data in Drive itself.  To do that, it
// needs to find the file resource IDs and pass them to a server-side function
// that will authorize the app to open the given document and return a URL
// for opening the document in that app directly.
class FileTaskExecutor {
 public:
  // Creates FileTaskExecutor with delegate derived from |profile|. Used in
  // product environment.
  FileTaskExecutor(Profile* profile, const std::string& app_id);

  // Creates FileTaskExecutor with a specific delegate.
  FileTaskExecutor(std::unique_ptr<FileTaskExecutorDelegate> delegate,
                   const std::string& app_id);

  // Executes file tasks, runs |done| and deletes |this|.
  void Execute(const std::vector<storage::FileSystemURL>& file_urls,
               const file_manager::file_tasks::FileTaskFinishedCallback& done);

 private:
  ~FileTaskExecutor();

  void OnFileEntryFetched(FileError error,
                          std::unique_ptr<ResourceEntry> entry);
  void OnAppAuthorized(const std::string& resource_id,
                       google_apis::DriveApiErrorCode error,
                       const GURL& open_link);

  // Calls |done_| with |success| status and deletes |this|.
  void Done(bool success);

  std::unique_ptr<FileTaskExecutorDelegate> delegate_;
  std::string app_id_;
  int current_index_;
  file_manager::file_tasks::FileTaskFinishedCallback done_;

  base::WeakPtrFactory<FileTaskExecutor> weak_ptr_factory_;
};

}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_FILE_TASK_EXECUTOR_H_
