// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_FILE_STREAM_READER_H_
#define STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_FILE_STREAM_READER_H_

#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "storage/browser/blob/shareable_file_reference.h"
#include "storage/browser/fileapi/file_stream_reader.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/browser/storage_browser_export.h"

namespace base {
class FilePath;
}

namespace content {
class FileSystemFileStreamReaderTest;
}

namespace storage {

class FileSystemContext;

// Generic FileStreamReader implementation for FileSystem files.
// Note: This generic implementation would work for any filesystems but
// remote filesystem should implement its own reader rather than relying
// on FileSystemOperation::GetSnapshotFile() which may force downloading
// the entire contents for remote files.
class STORAGE_EXPORT FileSystemFileStreamReader
    : public storage::FileStreamReader {
 public:
  ~FileSystemFileStreamReader() override;

  // FileStreamReader overrides.
  int Read(net::IOBuffer* buf,
           int buf_len,
           const net::CompletionCallback& callback) override;
  int64_t GetLength(const net::Int64CompletionCallback& callback) override;

 private:
  friend class storage::FileStreamReader;
  friend class content::FileSystemFileStreamReaderTest;

  FileSystemFileStreamReader(FileSystemContext* file_system_context,
                             const FileSystemURL& url,
                             int64_t initial_offset,
                             const base::Time& expected_modification_time);

  int CreateSnapshot(const base::Closure& callback,
                     const net::CompletionCallback& error_callback);
  void DidCreateSnapshot(
      const base::Closure& callback,
      const net::CompletionCallback& error_callback,
      base::File::Error file_error,
      const base::File::Info& file_info,
      const base::FilePath& platform_path,
      scoped_refptr<storage::ShareableFileReference> file_ref);

  scoped_refptr<FileSystemContext> file_system_context_;
  FileSystemURL url_;
  const int64_t initial_offset_;
  const base::Time expected_modification_time_;
  std::unique_ptr<storage::FileStreamReader> local_file_reader_;
  scoped_refptr<storage::ShareableFileReference> snapshot_ref_;
  bool has_pending_create_snapshot_;
  base::WeakPtrFactory<FileSystemFileStreamReader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FileSystemFileStreamReader);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_FILEAPI_FILE_SYSTEM_FILE_STREAM_READER_H_
