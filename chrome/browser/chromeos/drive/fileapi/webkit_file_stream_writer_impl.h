// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DRIVE_FILEAPI_WEBKIT_FILE_STREAM_WRITER_IMPL_H_
#define CHROME_BROWSER_CHROMEOS_DRIVE_FILEAPI_WEBKIT_FILE_STREAM_WRITER_IMPL_H_

#include <stdint.h>

#include <memory>

#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "storage/browser/fileapi/file_stream_writer.h"

namespace base {
class TaskRunner;
}  // namespace base

namespace net {
class IOBuffer;
}  // namespace net

namespace drive {

class FileSystemInterface;

namespace internal {

// The implementation of storage::FileStreamWriter for the Drive File System.
class WebkitFileStreamWriterImpl : public storage::FileStreamWriter {
 public:
  // Callback to return the FileSystemInterface instance. This is an
  // injecting point for testing.
  // Note that the callback will be copied between threads (IO and UI), and
  // will be called on UI thread.
  typedef base::Callback<FileSystemInterface*()> FileSystemGetter;

  // Creates a writer for a file at |file_path| on FileSystem returned by
  // |file_system_getter| that starts writing from |offset|.
  // When invalid parameters are set, the first call to Write() method fails.
  // Uses |file_task_runner| for local file operations.
  WebkitFileStreamWriterImpl(const FileSystemGetter& file_system_getter,
                             base::TaskRunner* file_task_runner,
                             const base::FilePath& file_path,
                             int64_t offset);
  ~WebkitFileStreamWriterImpl() override;

  // FileWriter override.
  int Write(net::IOBuffer* buf,
            int buf_len,
            const net::CompletionCallback& callback) override;
  int Cancel(const net::CompletionCallback& callback) override;
  int Flush(const net::CompletionCallback& callback) override;

 private:
  // Part of Write(). Called after CreateWritableSnapshotFile is completed.
  void WriteAfterCreateWritableSnapshotFile(
      net::IOBuffer* buf,
      int buf_len,
      base::File::Error open_result,
      const base::FilePath& local_path,
      const base::Closure& close_callback_on_ui_thread);

  FileSystemGetter file_system_getter_;
  scoped_refptr<base::TaskRunner> file_task_runner_;
  const base::FilePath file_path_;
  const int64_t offset_;

  std::unique_ptr<storage::FileStreamWriter> local_file_writer_;
  base::Closure close_callback_on_ui_thread_;
  net::CompletionCallback pending_write_callback_;
  net::CompletionCallback pending_cancel_callback_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate the weak pointers before any other members are destroyed.
  base::WeakPtrFactory<WebkitFileStreamWriterImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebkitFileStreamWriterImpl);
};

}  // namespace internal
}  // namespace drive

#endif  // CHROME_BROWSER_CHROMEOS_DRIVE_FILEAPI_WEBKIT_FILE_STREAM_WRITER_IMPL_H_
