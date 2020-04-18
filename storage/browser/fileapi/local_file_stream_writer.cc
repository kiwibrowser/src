// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/local_file_stream_writer.h"

#include <stdint.h>

#include "net/base/file_stream.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"

namespace storage {

namespace {

const int kOpenFlagsForWrite = base::File::FLAG_OPEN |
                               base::File::FLAG_WRITE |
                               base::File::FLAG_ASYNC;
const int kCreateFlagsForWrite = base::File::FLAG_CREATE |
                                 base::File::FLAG_WRITE |
                                 base::File::FLAG_ASYNC;

}  // namespace

FileStreamWriter* FileStreamWriter::CreateForLocalFile(
    base::TaskRunner* task_runner,
    const base::FilePath& file_path,
    int64_t initial_offset,
    OpenOrCreate open_or_create) {
  return new LocalFileStreamWriter(
      task_runner, file_path, initial_offset, open_or_create);
}

LocalFileStreamWriter::~LocalFileStreamWriter() {
  // Invalidate weak pointers so that we won't receive any callbacks from
  // in-flight stream operations, which might be triggered during the file close
  // in the FileStream destructor.
  weak_factory_.InvalidateWeakPtrs();

  // FileStream's destructor closes the file safely, since we opened the file
  // by its Open() method.
}

int LocalFileStreamWriter::Write(net::IOBuffer* buf, int buf_len,
                                 const net::CompletionCallback& callback) {
  DCHECK(!has_pending_operation_);
  DCHECK(cancel_callback_.is_null());

  has_pending_operation_ = true;
  if (stream_impl_) {
    int result = InitiateWrite(buf, buf_len, callback);
    if (result != net::ERR_IO_PENDING)
      has_pending_operation_ = false;
    return result;
  }
  return InitiateOpen(
      callback, base::Bind(&LocalFileStreamWriter::ReadyToWrite,
                           weak_factory_.GetWeakPtr(), base::RetainedRef(buf),
                           buf_len, callback));
}

int LocalFileStreamWriter::Cancel(const net::CompletionCallback& callback) {
  if (!has_pending_operation_)
    return net::ERR_UNEXPECTED;

  DCHECK(!callback.is_null());
  cancel_callback_ = callback;
  return net::ERR_IO_PENDING;
}

int LocalFileStreamWriter::Flush(const net::CompletionCallback& callback) {
  DCHECK(!has_pending_operation_);
  DCHECK(cancel_callback_.is_null());

  // Write() is not called yet, so there's nothing to flush.
  if (!stream_impl_)
    return net::OK;

  has_pending_operation_ = true;
  int result = InitiateFlush(callback);
  if (result != net::ERR_IO_PENDING)
    has_pending_operation_ = false;
  return result;
}

LocalFileStreamWriter::LocalFileStreamWriter(base::TaskRunner* task_runner,
                                             const base::FilePath& file_path,
                                             int64_t initial_offset,
                                             OpenOrCreate open_or_create)
    : file_path_(file_path),
      open_or_create_(open_or_create),
      initial_offset_(initial_offset),
      task_runner_(task_runner),
      has_pending_operation_(false),
      weak_factory_(this) {}

int LocalFileStreamWriter::InitiateOpen(
    const net::CompletionCallback& error_callback,
    const base::Closure& main_operation) {
  DCHECK(has_pending_operation_);
  DCHECK(!stream_impl_.get());

  stream_impl_.reset(new net::FileStream(task_runner_));

  int open_flags = 0;
  switch (open_or_create_) {
  case OPEN_EXISTING_FILE:
    open_flags = kOpenFlagsForWrite;
    break;
  case CREATE_NEW_FILE:
    open_flags = kCreateFlagsForWrite;
    break;
  }

  return stream_impl_->Open(file_path_, open_flags,
                            base::BindOnce(&LocalFileStreamWriter::DidOpen,
                                           weak_factory_.GetWeakPtr(),
                                           error_callback, main_operation));
}

void LocalFileStreamWriter::DidOpen(
    const net::CompletionCallback& error_callback,
    const base::Closure& main_operation,
    int result) {
  DCHECK(has_pending_operation_);
  DCHECK(stream_impl_.get());

  if (CancelIfRequested())
    return;

  if (result != net::OK) {
    has_pending_operation_ = false;
    stream_impl_.reset(NULL);
    error_callback.Run(result);
    return;
  }

  InitiateSeek(error_callback, main_operation);
}

void LocalFileStreamWriter::InitiateSeek(
    const net::CompletionCallback& error_callback,
    const base::Closure& main_operation) {
  DCHECK(has_pending_operation_);
  DCHECK(stream_impl_.get());

  if (initial_offset_ == 0) {
    // No need to seek.
    main_operation.Run();
    return;
  }

  int result = stream_impl_->Seek(
      initial_offset_, base::BindOnce(&LocalFileStreamWriter::DidSeek,
                                      weak_factory_.GetWeakPtr(),
                                      error_callback, main_operation));
  if (result != net::ERR_IO_PENDING) {
    has_pending_operation_ = false;
    error_callback.Run(result);
  }
}

void LocalFileStreamWriter::DidSeek(
    const net::CompletionCallback& error_callback,
    const base::Closure& main_operation,
    int64_t result) {
  DCHECK(has_pending_operation_);

  if (CancelIfRequested())
    return;

  if (result != initial_offset_) {
    // TODO(kinaba) add a more specific error code.
    result = net::ERR_FAILED;
  }

  if (result < 0) {
    has_pending_operation_ = false;
    error_callback.Run(static_cast<int>(result));
    return;
  }

  main_operation.Run();
}

void LocalFileStreamWriter::ReadyToWrite(
    net::IOBuffer* buf, int buf_len,
    const net::CompletionCallback& callback) {
  DCHECK(has_pending_operation_);

  int result = InitiateWrite(buf, buf_len, callback);
  if (result != net::ERR_IO_PENDING) {
    has_pending_operation_ = false;
    callback.Run(result);
  }
}

int LocalFileStreamWriter::InitiateWrite(
    net::IOBuffer* buf, int buf_len,
    const net::CompletionCallback& callback) {
  DCHECK(has_pending_operation_);
  DCHECK(stream_impl_.get());

  return stream_impl_->Write(
      buf, buf_len,
      base::BindOnce(&LocalFileStreamWriter::DidWrite,
                     weak_factory_.GetWeakPtr(), callback));
}

void LocalFileStreamWriter::DidWrite(const net::CompletionCallback& callback,
                                     int result) {
  DCHECK(has_pending_operation_);

  if (CancelIfRequested())
    return;
  has_pending_operation_ = false;
  callback.Run(result);
}

int LocalFileStreamWriter::InitiateFlush(
    const net::CompletionCallback& callback) {
  DCHECK(has_pending_operation_);
  DCHECK(stream_impl_.get());

  return stream_impl_->Flush(base::BindOnce(
      &LocalFileStreamWriter::DidFlush, weak_factory_.GetWeakPtr(), callback));
}

void LocalFileStreamWriter::DidFlush(const net::CompletionCallback& callback,
                                     int result) {
  DCHECK(has_pending_operation_);

  if (CancelIfRequested())
    return;
  has_pending_operation_ = false;
  callback.Run(result);
}

bool LocalFileStreamWriter::CancelIfRequested() {
  DCHECK(has_pending_operation_);

  if (cancel_callback_.is_null())
    return false;

  net::CompletionCallback pending_cancel = cancel_callback_;
  has_pending_operation_ = false;
  cancel_callback_.Reset();
  pending_cancel.Run(net::OK);
  return true;
}

}  // namespace storage
