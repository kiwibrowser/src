// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/local_file_stream_reader.h"

#include <stdint.h>

#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/task_runner.h"
#include "base/task_runner_util.h"
#include "net/base/file_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"

namespace storage {

namespace {

const int kOpenFlagsForRead = base::File::FLAG_OPEN |
                              base::File::FLAG_READ |
                              base::File::FLAG_ASYNC;

struct GetFileInfoResults {
  base::File::Error error;
  base::File::Info info;
};

using GetFileInfoCallback =
    base::OnceCallback<void(base::File::Error, const base::File::Info&)>;

GetFileInfoResults DoGetFileInfo(const base::FilePath& path) {
  GetFileInfoResults results;
  if (!base::PathExists(path)) {
    results.error = base::File::FILE_ERROR_NOT_FOUND;
    return results;
  }
  results.error = base::GetFileInfo(path, &results.info)
                      ? base::File::FILE_OK
                      : base::File::FILE_ERROR_FAILED;
  return results;
}

void SendGetFileInfoResults(GetFileInfoCallback callback,
                            const GetFileInfoResults& results) {
  std::move(callback).Run(results.error, results.info);
}

}  // namespace

FileStreamReader* FileStreamReader::CreateForLocalFile(
    base::TaskRunner* task_runner,
    const base::FilePath& file_path,
    int64_t initial_offset,
    const base::Time& expected_modification_time) {
  return new LocalFileStreamReader(task_runner, file_path, initial_offset,
                                   expected_modification_time);
}

LocalFileStreamReader::~LocalFileStreamReader() = default;

int LocalFileStreamReader::Read(net::IOBuffer* buf, int buf_len,
                          const net::CompletionCallback& callback) {
  DCHECK(!has_pending_open_);
  if (stream_impl_)
    return stream_impl_->Read(buf, buf_len, callback);
  return Open(base::Bind(&LocalFileStreamReader::DidOpenForRead,
                         weak_factory_.GetWeakPtr(), base::RetainedRef(buf),
                         buf_len, callback));
}

int64_t LocalFileStreamReader::GetLength(
    const net::Int64CompletionCallback& callback) {
  bool posted = base::PostTaskAndReplyWithResult(
      task_runner_.get(), FROM_HERE, base::BindOnce(&DoGetFileInfo, file_path_),
      base::BindOnce(
          &SendGetFileInfoResults,
          base::BindOnce(&LocalFileStreamReader::DidGetFileInfoForGetLength,
                         weak_factory_.GetWeakPtr(), callback)));
  DCHECK(posted);
  return net::ERR_IO_PENDING;
}

LocalFileStreamReader::LocalFileStreamReader(
    base::TaskRunner* task_runner,
    const base::FilePath& file_path,
    int64_t initial_offset,
    const base::Time& expected_modification_time)
    : task_runner_(task_runner),
      file_path_(file_path),
      initial_offset_(initial_offset),
      expected_modification_time_(expected_modification_time),
      has_pending_open_(false),
      weak_factory_(this) {}

int LocalFileStreamReader::Open(const net::CompletionCallback& callback) {
  DCHECK(!has_pending_open_);
  DCHECK(!stream_impl_.get());
  has_pending_open_ = true;

  // Call GetLength first to make it perform last-modified-time verification,
  // and then call DidVerifyForOpen for do the rest.
  return GetLength(base::Bind(&LocalFileStreamReader::DidVerifyForOpen,
                              weak_factory_.GetWeakPtr(), callback));
}

void LocalFileStreamReader::DidVerifyForOpen(
    const net::CompletionCallback& callback,
    int64_t get_length_result) {
  if (get_length_result < 0) {
    callback.Run(static_cast<int>(get_length_result));
    return;
  }

  stream_impl_.reset(new net::FileStream(task_runner_));
  const int result = stream_impl_->Open(
      file_path_, kOpenFlagsForRead,
      base::BindOnce(&LocalFileStreamReader::DidOpenFileStream,
                     weak_factory_.GetWeakPtr(), callback));
  if (result != net::ERR_IO_PENDING)
    callback.Run(result);
}

void LocalFileStreamReader::DidOpenFileStream(
    const net::CompletionCallback& callback,
    int result) {
  if (result != net::OK) {
    callback.Run(result);
    return;
  }
  result = stream_impl_->Seek(
      initial_offset_, base::BindOnce(&LocalFileStreamReader::DidSeekFileStream,
                                      weak_factory_.GetWeakPtr(), callback));
  if (result != net::ERR_IO_PENDING) {
    callback.Run(result);
  }
}

void LocalFileStreamReader::DidSeekFileStream(
    const net::CompletionCallback& callback,
    int64_t seek_result) {
  if (seek_result < 0) {
    callback.Run(static_cast<int>(seek_result));
    return;
  }
  if (seek_result != initial_offset_) {
    callback.Run(net::ERR_REQUEST_RANGE_NOT_SATISFIABLE);
    return;
  }
  callback.Run(net::OK);
}

void LocalFileStreamReader::DidOpenForRead(
    net::IOBuffer* buf,
    int buf_len,
    const net::CompletionCallback& callback,
    int open_result) {
  DCHECK(has_pending_open_);
  has_pending_open_ = false;
  if (open_result != net::OK) {
    stream_impl_.reset();
    callback.Run(open_result);
    return;
  }
  DCHECK(stream_impl_.get());
  const int read_result = stream_impl_->Read(buf, buf_len, callback);
  if (read_result != net::ERR_IO_PENDING)
    callback.Run(read_result);
}

void LocalFileStreamReader::DidGetFileInfoForGetLength(
    const net::Int64CompletionCallback& callback,
    base::File::Error error,
    const base::File::Info& file_info) {
  if (file_info.is_directory) {
    callback.Run(net::ERR_FILE_NOT_FOUND);
    return;
  }
  if (error != base::File::FILE_OK) {
    callback.Run(net::FileErrorToNetError(error));
    return;
  }
  if (!VerifySnapshotTime(expected_modification_time_, file_info)) {
    callback.Run(net::ERR_UPLOAD_FILE_CHANGED);
    return;
  }
  callback.Run(file_info.size);
}

}  // namespace storage
