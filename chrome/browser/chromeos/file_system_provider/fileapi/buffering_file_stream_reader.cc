// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_system_provider/fileapi/buffering_file_stream_reader.h"

#include <algorithm>
#include <utility>

#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "storage/browser/fileapi/file_system_backend.h"

namespace chromeos {
namespace file_system_provider {

BufferingFileStreamReader::BufferingFileStreamReader(
    std::unique_ptr<storage::FileStreamReader> file_stream_reader,
    int preloading_buffer_length,
    int64_t max_bytes_to_read)
    : file_stream_reader_(std::move(file_stream_reader)),
      preloading_buffer_length_(preloading_buffer_length),
      max_bytes_to_read_(max_bytes_to_read),
      bytes_read_(0),
      preloading_buffer_(new net::IOBuffer(preloading_buffer_length)),
      preloading_buffer_offset_(0),
      preloaded_bytes_(0),
      weak_ptr_factory_(this) {}

BufferingFileStreamReader::~BufferingFileStreamReader() {
}

int BufferingFileStreamReader::Read(net::IOBuffer* buffer,
                                    int buffer_length,
                                    const net::CompletionCallback& callback) {
  // Return as much as available in the internal buffer. It may be less than
  // |buffer_length|, what is valid.
  const int bytes_read =
      CopyFromPreloadingBuffer(base::WrapRefCounted(buffer), buffer_length);
  if (bytes_read)
    return bytes_read;

  // If the internal buffer is empty, and more bytes than the internal buffer
  // size is requested, then call the internal file stream reader directly.
  if (buffer_length >= preloading_buffer_length_) {
    const int result = file_stream_reader_->Read(
        buffer,
        buffer_length,
        base::Bind(&BufferingFileStreamReader::OnReadCompleted,
                   weak_ptr_factory_.GetWeakPtr(),
                   callback));
    DCHECK_EQ(result, net::ERR_IO_PENDING);
    return result;
  }

  // Nothing copied, so contents have to be preloaded.
  Preload(base::Bind(
      &BufferingFileStreamReader::OnReadCompleted,
      weak_ptr_factory_.GetWeakPtr(),
      base::Bind(&BufferingFileStreamReader::OnPreloadCompleted,
                 weak_ptr_factory_.GetWeakPtr(), base::WrapRefCounted(buffer),
                 buffer_length, callback)));

  return net::ERR_IO_PENDING;
}

int64_t BufferingFileStreamReader::GetLength(
    const net::Int64CompletionCallback& callback) {
  const int64_t result = file_stream_reader_->GetLength(callback);
  DCHECK_EQ(net::ERR_IO_PENDING, result);

  return result;
}

int BufferingFileStreamReader::CopyFromPreloadingBuffer(
    scoped_refptr<net::IOBuffer> buffer,
    int buffer_length) {
  const int read_bytes = std::min(buffer_length, preloaded_bytes_);

  memcpy(buffer->data(),
         preloading_buffer_->data() + preloading_buffer_offset_,
         read_bytes);
  preloading_buffer_offset_ += read_bytes;
  preloaded_bytes_ -= read_bytes;

  return read_bytes;
}

void BufferingFileStreamReader::Preload(
    const net::CompletionCallback& callback) {
  const int preload_bytes =
      std::min(static_cast<int64_t>(preloading_buffer_length_),
               max_bytes_to_read_ - bytes_read_);

  const int result = file_stream_reader_->Read(
      preloading_buffer_.get(), preload_bytes, callback);
  DCHECK_EQ(result, net::ERR_IO_PENDING);
}

void BufferingFileStreamReader::OnPreloadCompleted(
    scoped_refptr<net::IOBuffer> buffer,
    int buffer_length,
    const net::CompletionCallback& callback,
    int result) {
  if (result < 0) {
    callback.Run(result);
    return;
  }

  preloading_buffer_offset_ = 0;
  preloaded_bytes_ = result;

  callback.Run(CopyFromPreloadingBuffer(buffer, buffer_length));
}

void BufferingFileStreamReader::OnReadCompleted(
    const net::CompletionCallback& callback,
    int result) {
  if (result < 0) {
    callback.Run(result);
    return;
  }

  // If more bytes than declared in |max_bytes_to_read_| was read in total, then
  // emit an
  // error.
  if (result > max_bytes_to_read_ - bytes_read_) {
    callback.Run(net::ERR_FAILED);
    return;
  }

  bytes_read_ += result;
  DCHECK_LE(bytes_read_, max_bytes_to_read_);

  callback.Run(result);
}

}  // namespace file_system_provider
}  // namespace chromeos
