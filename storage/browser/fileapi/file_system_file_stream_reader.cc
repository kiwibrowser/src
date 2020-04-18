// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/file_system_file_stream_reader.h"

#include <stdint.h>

#include "base/single_thread_task_runner.h"
#include "net/base/file_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_runner.h"

using storage::FileStreamReader;

// TODO(kinuko): Remove this temporary namespace hack after we move both
// blob and fileapi into content namespace.
namespace storage {

FileStreamReader* FileStreamReader::CreateForFileSystemFile(
    storage::FileSystemContext* file_system_context,
    const storage::FileSystemURL& url,
    int64_t initial_offset,
    const base::Time& expected_modification_time) {
  return new storage::FileSystemFileStreamReader(
      file_system_context, url, initial_offset, expected_modification_time);
}

}  // namespace storage

namespace storage {

namespace {

void ReadAdapter(base::WeakPtr<FileSystemFileStreamReader> reader,
                 net::IOBuffer* buf, int buf_len,
                 const net::CompletionCallback& callback) {
  if (!reader.get())
    return;
  int rv = reader->Read(buf, buf_len, callback);
  if (rv != net::ERR_IO_PENDING)
    callback.Run(rv);
}

void GetLengthAdapter(base::WeakPtr<FileSystemFileStreamReader> reader,
                      const net::Int64CompletionCallback& callback) {
  if (!reader.get())
    return;
  int rv = reader->GetLength(callback);
  if (rv != net::ERR_IO_PENDING)
    callback.Run(rv);
}

void Int64CallbackAdapter(const net::Int64CompletionCallback& callback,
                          int value) {
  callback.Run(value);
}

}  // namespace

FileSystemFileStreamReader::~FileSystemFileStreamReader() = default;

int FileSystemFileStreamReader::Read(
    net::IOBuffer* buf, int buf_len,
    const net::CompletionCallback& callback) {
  if (local_file_reader_)
    return local_file_reader_->Read(buf, buf_len, callback);
  return CreateSnapshot(base::Bind(&ReadAdapter, weak_factory_.GetWeakPtr(),
                                   base::RetainedRef(buf), buf_len, callback),
                        callback);
}

int64_t FileSystemFileStreamReader::GetLength(
    const net::Int64CompletionCallback& callback) {
  if (local_file_reader_)
    return local_file_reader_->GetLength(callback);
  return CreateSnapshot(
      base::Bind(&GetLengthAdapter, weak_factory_.GetWeakPtr(), callback),
      base::Bind(&Int64CallbackAdapter, callback));
}

FileSystemFileStreamReader::FileSystemFileStreamReader(
    FileSystemContext* file_system_context,
    const FileSystemURL& url,
    int64_t initial_offset,
    const base::Time& expected_modification_time)
    : file_system_context_(file_system_context),
      url_(url),
      initial_offset_(initial_offset),
      expected_modification_time_(expected_modification_time),
      has_pending_create_snapshot_(false),
      weak_factory_(this) {}

int FileSystemFileStreamReader::CreateSnapshot(
    const base::Closure& callback,
    const net::CompletionCallback& error_callback) {
  DCHECK(!has_pending_create_snapshot_);
  has_pending_create_snapshot_ = true;
  file_system_context_->operation_runner()->CreateSnapshotFile(
      url_,
      base::Bind(&FileSystemFileStreamReader::DidCreateSnapshot,
                 weak_factory_.GetWeakPtr(),
                 callback,
                 error_callback));
  return net::ERR_IO_PENDING;
}

void FileSystemFileStreamReader::DidCreateSnapshot(
    const base::Closure& callback,
    const net::CompletionCallback& error_callback,
    base::File::Error file_error,
    const base::File::Info& file_info,
    const base::FilePath& platform_path,
    scoped_refptr<storage::ShareableFileReference> file_ref) {
  DCHECK(has_pending_create_snapshot_);
  DCHECK(!local_file_reader_.get());
  has_pending_create_snapshot_ = false;

  if (file_error != base::File::FILE_OK) {
    error_callback.Run(net::FileErrorToNetError(file_error));
    return;
  }

  // Keep the reference (if it's non-null) so that the file won't go away.
  snapshot_ref_ = std::move(file_ref);

  local_file_reader_.reset(
      FileStreamReader::CreateForLocalFile(
          file_system_context_->default_file_task_runner(),
          platform_path, initial_offset_, expected_modification_time_));

  callback.Run();
}

}  // namespace storage
