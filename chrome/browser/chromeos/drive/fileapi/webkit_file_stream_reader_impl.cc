// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/drive/fileapi/webkit_file_stream_reader_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/logging.h"
#include "chrome/browser/chromeos/drive/drive_file_stream_reader.h"
#include "components/drive/drive.pb.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/http/http_byte_range.h"

using content::BrowserThread;

namespace drive {
namespace internal {

WebkitFileStreamReaderImpl::WebkitFileStreamReaderImpl(
    const DriveFileStreamReader::FileSystemGetter& file_system_getter,
    base::SequencedTaskRunner* file_task_runner,
    const base::FilePath& drive_file_path,
    int64_t offset,
    const base::Time& expected_modification_time)
    : stream_reader_(
          new DriveFileStreamReader(file_system_getter, file_task_runner)),
      drive_file_path_(drive_file_path),
      offset_(offset),
      expected_modification_time_(expected_modification_time),
      file_size_(-1),
      weak_ptr_factory_(this) {
  DCHECK_GE(offset, 0);
}

WebkitFileStreamReaderImpl::~WebkitFileStreamReaderImpl() {
}

int WebkitFileStreamReaderImpl::Read(net::IOBuffer* buffer,
                                     int buffer_length,
                                     const net::CompletionCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(stream_reader_);
  DCHECK(buffer);
  DCHECK(!callback.is_null());

  if (stream_reader_->IsInitialized())
    return stream_reader_->Read(buffer, buffer_length, callback);

  net::HttpByteRange byte_range;
  byte_range.set_first_byte_position(offset_);
  stream_reader_->Initialize(
      drive_file_path_, byte_range,
      base::Bind(
          &WebkitFileStreamReaderImpl::OnStreamReaderInitialized,
          weak_ptr_factory_.GetWeakPtr(),
          base::Bind(
              &WebkitFileStreamReaderImpl ::ReadAfterStreamReaderInitialized,
              weak_ptr_factory_.GetWeakPtr(), base::WrapRefCounted(buffer),
              buffer_length, callback)));
  return net::ERR_IO_PENDING;
}

int64_t WebkitFileStreamReaderImpl::GetLength(
    const net::Int64CompletionCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(stream_reader_);
  DCHECK(!callback.is_null());

  if (stream_reader_->IsInitialized()) {
    // Returns file_size regardless of |offset_|.
    return file_size_;
  }

  net::HttpByteRange byte_range;
  byte_range.set_first_byte_position(offset_);
  stream_reader_->Initialize(
      drive_file_path_,
      byte_range,
      base::Bind(&WebkitFileStreamReaderImpl::OnStreamReaderInitialized,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Bind(&WebkitFileStreamReaderImpl
                                ::GetLengthAfterStreamReaderInitialized,
                            weak_ptr_factory_.GetWeakPtr(),
                            callback)));
  return net::ERR_IO_PENDING;
}

void WebkitFileStreamReaderImpl::OnStreamReaderInitialized(
    const net::CompletionCallback& callback,
    int error,
    std::unique_ptr<ResourceEntry> entry) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(stream_reader_);
  DCHECK(!callback.is_null());

  // TODO(hashimoto): Report ERR_UPLOAD_FILE_CHANGED when modification time
  // doesn't match. crbug.com/346625
  if (error != net::OK) {
    // Found an error. Close the |stream_reader_| and notify it to the caller.
    stream_reader_.reset();
    callback.Run(error);
    return;
  }

  // Remember the size of the file.
  file_size_ = entry->file_info().size();
  callback.Run(net::OK);
}

void WebkitFileStreamReaderImpl::ReadAfterStreamReaderInitialized(
    scoped_refptr<net::IOBuffer> buffer,
    int buffer_length,
    const net::CompletionCallback& callback,
    int initialization_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!callback.is_null());

  if (initialization_result != net::OK) {
    callback.Run(initialization_result);
    return;
  }

  DCHECK(stream_reader_);
  int result = stream_reader_->Read(buffer.get(), buffer_length, callback);
  if (result != net::ERR_IO_PENDING)
    callback.Run(result);
}

void WebkitFileStreamReaderImpl::GetLengthAfterStreamReaderInitialized(
    const net::Int64CompletionCallback& callback,
    int initialization_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(!callback.is_null());

  if (initialization_result != net::OK) {
    callback.Run(initialization_result);
    return;
  }

  DCHECK_GE(file_size_, 0);
  callback.Run(file_size_);
}

}  // namespace internal
}  // namespace drive
