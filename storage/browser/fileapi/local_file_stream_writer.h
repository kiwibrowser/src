// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_FILEAPI_LOCAL_FILE_STREAM_WRITER_H_
#define STORAGE_BROWSER_FILEAPI_LOCAL_FILE_STREAM_WRITER_H_

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/task_runner.h"
#include "storage/browser/fileapi/file_stream_writer.h"
#include "storage/browser/storage_browser_export.h"

namespace content {
class LocalFileStreamWriterTest;
}

namespace net {
class FileStream;
}

namespace storage {

// This class is a thin wrapper around net::FileStream for writing local files.
class STORAGE_EXPORT LocalFileStreamWriter : public FileStreamWriter {
 public:
  ~LocalFileStreamWriter() override;

  // FileStreamWriter overrides.
  int Write(net::IOBuffer* buf,
            int buf_len,
            const net::CompletionCallback& callback) override;
  int Cancel(const net::CompletionCallback& callback) override;
  int Flush(const net::CompletionCallback& callback) override;

 private:
  friend class content::LocalFileStreamWriterTest;
  friend class FileStreamWriter;
  LocalFileStreamWriter(base::TaskRunner* task_runner,
                        const base::FilePath& file_path,
                        int64_t initial_offset,
                        OpenOrCreate open_or_create);

  // Opens |file_path_| and if it succeeds, proceeds to InitiateSeek().
  // If failed, the error code is returned by calling |error_callback|.
  int InitiateOpen(const net::CompletionCallback& error_callback,
                   const base::Closure& main_operation);
  void DidOpen(const net::CompletionCallback& error_callback,
               const base::Closure& main_operation,
               int result);

  // Seeks to |initial_offset_| and proceeds to |main_operation| if it succeeds.
  // If failed, the error code is returned by calling |error_callback|.
  void InitiateSeek(const net::CompletionCallback& error_callback,
                    const base::Closure& main_operation);
  void DidSeek(const net::CompletionCallback& error_callback,
               const base::Closure& main_operation,
               int64_t result);

  // Passed as the |main_operation| of InitiateOpen() function.
  void ReadyToWrite(net::IOBuffer* buf, int buf_len,
                    const net::CompletionCallback& callback);

  // Writes asynchronously to the file.
  int InitiateWrite(net::IOBuffer* buf, int buf_len,
                    const net::CompletionCallback& callback);
  void DidWrite(const net::CompletionCallback& callback, int result);

  // Flushes asynchronously to the file.
  int InitiateFlush(const net::CompletionCallback& callback);
  void DidFlush(const net::CompletionCallback& callback, int result);

  // Stops the in-flight operation and calls |cancel_callback_| if it has been
  // set by Cancel() for the current operation.
  bool CancelIfRequested();

  // Initialization parameters.
  const base::FilePath file_path_;
  OpenOrCreate open_or_create_;
  const int64_t initial_offset_;
  scoped_refptr<base::TaskRunner> task_runner_;

  // Current states of the operation.
  bool has_pending_operation_;
  std::unique_ptr<net::FileStream> stream_impl_;
  net::CompletionCallback cancel_callback_;

  base::WeakPtrFactory<LocalFileStreamWriter> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(LocalFileStreamWriter);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_FILEAPI_LOCAL_FILE_STREAM_WRITER_H_
