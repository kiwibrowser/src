// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/file_writer_delegate.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/net_errors.h"
#include "storage/browser/fileapi/file_stream_writer.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/common/fileapi/file_system_mount_option.h"
#include "storage/common/fileapi/file_system_util.h"

namespace storage {

static const int kReadBufSize = 32768;

FileWriterDelegate::FileWriterDelegate(
    std::unique_ptr<FileStreamWriter> file_stream_writer,
    FlushPolicy flush_policy)
    : file_stream_writer_(std::move(file_stream_writer)),
      writing_started_(false),
      flush_policy_(flush_policy),
      bytes_written_backlog_(0),
      bytes_written_(0),
      bytes_read_(0),
      io_buffer_(new net::IOBufferWithSize(kReadBufSize)),
      weak_factory_(this) {}

FileWriterDelegate::~FileWriterDelegate() = default;

void FileWriterDelegate::Start(std::unique_ptr<net::URLRequest> request,
                               const DelegateWriteCallback& write_callback) {
  write_callback_ = write_callback;
  request_ = std::move(request);
  request_->Start();
}

void FileWriterDelegate::Cancel() {
  // Destroy the request and invalidate weak ptrs to prevent pending callbacks.
  request_.reset();
  weak_factory_.InvalidateWeakPtrs();

  const int status = file_stream_writer_->Cancel(
      base::Bind(&FileWriterDelegate::OnWriteCancelled,
                 weak_factory_.GetWeakPtr()));
  // Return true to finish immediately if we have no pending writes.
  // Otherwise we'll do the final cleanup in the Cancel callback.
  if (status != net::ERR_IO_PENDING) {
    write_callback_.Run(base::File::FILE_ERROR_ABORT, 0,
                        GetCompletionStatusOnError());
  }
}

void FileWriterDelegate::OnReceivedRedirect(
    net::URLRequest* request,
    const net::RedirectInfo& redirect_info,
    bool* defer_redirect) {
  NOTREACHED();
  OnReadError(base::File::FILE_ERROR_SECURITY);
}

void FileWriterDelegate::OnAuthRequired(net::URLRequest* request,
                                        net::AuthChallengeInfo* auth_info) {
  NOTREACHED();
  OnReadError(base::File::FILE_ERROR_SECURITY);
}

void FileWriterDelegate::OnCertificateRequested(
    net::URLRequest* request,
    net::SSLCertRequestInfo* cert_request_info) {
  NOTREACHED();
  OnReadError(base::File::FILE_ERROR_SECURITY);
}

void FileWriterDelegate::OnSSLCertificateError(net::URLRequest* request,
                                               const net::SSLInfo& ssl_info,
                                               bool fatal) {
  NOTREACHED();
  OnReadError(base::File::FILE_ERROR_SECURITY);
}

void FileWriterDelegate::OnResponseStarted(net::URLRequest* request,
                                           int net_error) {
  DCHECK_NE(net::ERR_IO_PENDING, net_error);
  DCHECK_EQ(request_.get(), request);

  if (net_error != net::OK || request->GetResponseCode() != 200) {
    OnReadError(base::File::FILE_ERROR_FAILED);
    return;
  }
  Read();
}

void FileWriterDelegate::OnReadCompleted(net::URLRequest* request,
                                         int bytes_read) {
  DCHECK_NE(net::ERR_IO_PENDING, bytes_read);
  DCHECK_EQ(request_.get(), request);

  if (bytes_read < 0) {
    OnReadError(base::File::FILE_ERROR_FAILED);
    return;
  }
  OnDataReceived(bytes_read);
}

void FileWriterDelegate::Read() {
  bytes_written_ = 0;
  bytes_read_ = request_->Read(io_buffer_.get(), io_buffer_->size());
  if (bytes_read_ == net::ERR_IO_PENDING)
    return;

  if (bytes_read_ >= 0) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&FileWriterDelegate::OnDataReceived,
                                  weak_factory_.GetWeakPtr(), bytes_read_));
  } else {
    OnReadError(base::File::FILE_ERROR_FAILED);
  }
}

void FileWriterDelegate::OnDataReceived(int bytes_read) {
  bytes_read_ = bytes_read;
  if (bytes_read == 0) {  // We're done.
    OnProgress(0, true);
  } else {
    // This could easily be optimized to rotate between a pool of buffers, so
    // that we could read and write at the same time.  It's not yet clear that
    // it's necessary.
    cursor_ = new net::DrainableIOBuffer(io_buffer_.get(), bytes_read_);
    Write();
  }
}

void FileWriterDelegate::Write() {
  writing_started_ = true;
  int64_t bytes_to_write = bytes_read_ - bytes_written_;
  int write_response =
      file_stream_writer_->Write(cursor_.get(),
                                 static_cast<int>(bytes_to_write),
                                 base::Bind(&FileWriterDelegate::OnDataWritten,
                                            weak_factory_.GetWeakPtr()));
  if (write_response > 0) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&FileWriterDelegate::OnDataWritten,
                                  weak_factory_.GetWeakPtr(), write_response));
  } else if (net::ERR_IO_PENDING != write_response) {
    OnWriteError(NetErrorToFileError(write_response));
  } else {
    async_write_in_progress_ = true;
  }
}

void FileWriterDelegate::OnDataWritten(int write_response) {
  async_write_in_progress_ = false;
  if (saved_read_error_ != base::File::FILE_OK) {
    OnReadError(saved_read_error_);
    return;
  }

  if (write_response > 0) {
    OnProgress(write_response, false);
    cursor_->DidConsume(write_response);
    bytes_written_ += write_response;
    if (bytes_written_ == bytes_read_)
      Read();
    else
      Write();
  } else {
    OnWriteError(NetErrorToFileError(write_response));
  }
}

FileWriterDelegate::WriteProgressStatus
FileWriterDelegate::GetCompletionStatusOnError() const {
  return writing_started_ ? ERROR_WRITE_STARTED : ERROR_WRITE_NOT_STARTED;
}

void FileWriterDelegate::OnReadError(base::File::Error error) {
  if (async_write_in_progress_) {
    // Error signaled by the URLRequest while writing. This will be processed
    // when the write completes.
    saved_read_error_ = error;
    return;
  }

  // Destroy the request and invalidate weak ptrs to prevent pending callbacks.
  request_.reset();
  weak_factory_.InvalidateWeakPtrs();

  if (writing_started_)
    MaybeFlushForCompletion(error, 0, ERROR_WRITE_STARTED);
  else
    write_callback_.Run(error, 0, ERROR_WRITE_NOT_STARTED);
}

void FileWriterDelegate::OnWriteError(base::File::Error error) {
  // Destroy the request and invalidate weak ptrs to prevent pending callbacks.
  request_.reset();
  weak_factory_.InvalidateWeakPtrs();

  // Errors when writing are not recoverable, so don't bother flushing.
  write_callback_.Run(
      error, 0,
      writing_started_ ? ERROR_WRITE_STARTED : ERROR_WRITE_NOT_STARTED);
}

void FileWriterDelegate::OnProgress(int bytes_written, bool done) {
  DCHECK(bytes_written + bytes_written_backlog_ >= bytes_written_backlog_);
  static const int kMinProgressDelayMS = 200;
  base::Time currentTime = base::Time::Now();
  if (done || last_progress_event_time_.is_null() ||
      (currentTime - last_progress_event_time_).InMilliseconds() >
          kMinProgressDelayMS) {
    bytes_written += bytes_written_backlog_;
    last_progress_event_time_ = currentTime;
    bytes_written_backlog_ = 0;

    if (done) {
      MaybeFlushForCompletion(base::File::FILE_OK, bytes_written,
                              SUCCESS_COMPLETED);
    } else {
      write_callback_.Run(base::File::FILE_OK, bytes_written,
                          SUCCESS_IO_PENDING);
    }
    return;
  }
  bytes_written_backlog_ += bytes_written;
}

void FileWriterDelegate::OnWriteCancelled(int status) {
  write_callback_.Run(base::File::FILE_ERROR_ABORT, 0,
                      GetCompletionStatusOnError());
}

void FileWriterDelegate::MaybeFlushForCompletion(
    base::File::Error error,
    int bytes_written,
    WriteProgressStatus progress_status) {
  if (flush_policy_ == FlushPolicy::NO_FLUSH_ON_COMPLETION) {
    write_callback_.Run(error, bytes_written, progress_status);
    return;
  }
  // DCHECK_EQ on enum classes is not supported.
  DCHECK(flush_policy_ == FlushPolicy::FLUSH_ON_COMPLETION);

  int flush_error = file_stream_writer_->Flush(
      base::Bind(&FileWriterDelegate::OnFlushed, weak_factory_.GetWeakPtr(),
                 error, bytes_written, progress_status));
  if (flush_error != net::ERR_IO_PENDING)
    OnFlushed(error, bytes_written, progress_status, flush_error);
}

void FileWriterDelegate::OnFlushed(base::File::Error error,
                                   int bytes_written,
                                   WriteProgressStatus progress_status,
                                   int flush_error) {
  if (error == base::File::FILE_OK && flush_error != net::OK) {
    // If the Flush introduced an error, overwrite the status.
    // Otherwise, keep the original error status.
    error = NetErrorToFileError(flush_error);
    progress_status = GetCompletionStatusOnError();
  }
  write_callback_.Run(error, bytes_written, progress_status);
}

}  // namespace storage
