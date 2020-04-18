// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_BLOB_LOCAL_FILE_STREAM_READER_H_
#define STORAGE_BROWSER_BLOB_LOCAL_FILE_STREAM_READER_H_

#include <stdint.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "storage/browser/fileapi/file_stream_reader.h"
#include "storage/browser/storage_browser_export.h"

namespace base {
class TaskRunner;
}

namespace content {
class LocalFileStreamReaderTest;
}

namespace net {
class FileStream;
}

namespace storage {

// A thin wrapper of net::FileStream with range support for sliced file
// handling.
class STORAGE_EXPORT LocalFileStreamReader : public FileStreamReader {
 public:
  ~LocalFileStreamReader() override;

  // FileStreamReader overrides.
  int Read(net::IOBuffer* buf,
           int buf_len,
           const net::CompletionCallback& callback) override;
  int64_t GetLength(const net::Int64CompletionCallback& callback) override;

 private:
  friend class FileStreamReader;
  friend class content::LocalFileStreamReaderTest;

  LocalFileStreamReader(base::TaskRunner* task_runner,
                        const base::FilePath& file_path,
                        int64_t initial_offset,
                        const base::Time& expected_modification_time);
  int Open(const net::CompletionCallback& callback);

  // Callbacks that are chained from Open for Read.
  void DidVerifyForOpen(const net::CompletionCallback& callback,
                        int64_t get_length_result);
  void DidOpenFileStream(const net::CompletionCallback& callback,
                         int result);
  void DidSeekFileStream(const net::CompletionCallback& callback,
                         int64_t seek_result);
  void DidOpenForRead(net::IOBuffer* buf,
                      int buf_len,
                      const net::CompletionCallback& callback,
                      int open_result);

  void DidGetFileInfoForGetLength(const net::Int64CompletionCallback& callback,
                                  base::File::Error error,
                                  const base::File::Info& file_info);

  scoped_refptr<base::TaskRunner> task_runner_;
  std::unique_ptr<net::FileStream> stream_impl_;
  const base::FilePath file_path_;
  const int64_t initial_offset_;
  const base::Time expected_modification_time_;
  bool has_pending_open_;
  base::WeakPtrFactory<LocalFileStreamReader> weak_factory_;
};

}  // namespace storage

#endif  // STORAGE_BROWSER_BLOB_LOCAL_FILE_STREAM_READER_H_
