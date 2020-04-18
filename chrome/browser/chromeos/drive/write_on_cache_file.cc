// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/write_on_cache_file.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/task_scheduler/post_task.h"
#include "components/drive/chromeos/file_system_interface.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace drive {

namespace {

// Runs |close_callback| and |reply|.
void RunCloseCallbackAndReplyTask(const base::Closure& close_callback,
                                  const FileOperationCallback& reply,
                                  FileError error) {
  if (!close_callback.is_null())
    close_callback.Run();
  DCHECK(!reply.is_null());
  reply.Run(error);
}

// Runs |file_io_task_callback| in blocking pool and runs |close_callback|
// in the UI thread after that.
void WriteOnCacheFileAfterOpenFile(
    const base::FilePath& drive_path,
    const WriteOnCacheFileCallback& file_io_task_callback,
    const FileOperationCallback& reply,
    FileError error,
    const base::FilePath& local_cache_path,
    const base::Closure& close_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_BLOCKING},
      base::BindOnce(file_io_task_callback, error, local_cache_path),
      base::BindOnce(&RunCloseCallbackAndReplyTask, close_callback, reply,
                     error));
}

}  // namespace

void WriteOnCacheFile(FileSystemInterface* file_system,
                      const base::FilePath& path,
                      const std::string& mime_type,
                      const WriteOnCacheFileCallback& callback) {
  WriteOnCacheFileAndReply(file_system, path, mime_type, callback,
                           base::DoNothing());
}

void WriteOnCacheFileAndReply(FileSystemInterface* file_system,
                              const base::FilePath& path,
                              const std::string& mime_type,
                              const WriteOnCacheFileCallback& callback,
                              const FileOperationCallback& reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(file_system);
  DCHECK(!callback.is_null());
  DCHECK(!reply.is_null());

  file_system->OpenFile(
      path,
      OPEN_OR_CREATE_FILE,
      mime_type,
      base::Bind(&WriteOnCacheFileAfterOpenFile, path, callback, reply));
}

}  // namespace drive
