// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/gcm/base/socket_stream.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/callback.h"
#include "net/base/io_buffer.h"
#include "net/socket/stream_socket.h"

namespace gcm {

namespace {

// TODO(zea): consider having dynamically-sized buffers if this becomes too
// expensive.
const size_t kDefaultBufferSize = 8*1024;

}  // namespace

SocketInputStream::SocketInputStream(net::StreamSocket* socket)
    : socket_(socket),
      io_buffer_(new net::IOBuffer(kDefaultBufferSize)),
      read_buffer_(new net::DrainableIOBuffer(io_buffer_.get(),
                                              kDefaultBufferSize)),
      next_pos_(0),
      last_error_(net::OK),
      weak_ptr_factory_(this) {
  DCHECK(socket->IsConnected());
}

SocketInputStream::~SocketInputStream() {
}

bool SocketInputStream::Next(const void** data, int* size) {
  if (GetState() != EMPTY && GetState() != READY) {
    NOTREACHED() << "Invalid input stream read attempt.";
    return false;
  }

  if (GetState() == EMPTY) {
    DVLOG(1) << "No unread data remaining, ending read.";
    return false;
  }

  DCHECK_EQ(GetState(), READY)
      << " Input stream must have pending data before reading.";
  DCHECK_LT(next_pos_, read_buffer_->BytesConsumed());
  *data = io_buffer_->data() + next_pos_;
  *size = UnreadByteCount();
  next_pos_ = read_buffer_->BytesConsumed();
  DVLOG(1) << "Consuming " << *size << " bytes in input buffer.";
  return true;
}

void SocketInputStream::BackUp(int count) {
  DCHECK(GetState() == READY || GetState() == EMPTY);
  // TODO(zea): investigating crbug.com/409985
  CHECK_GT(count, 0);
  CHECK_LE(count, next_pos_);

  next_pos_ -= count;
  DVLOG(1) << "Backing up " << count << " bytes in input buffer. "
           << "Current position now at " << next_pos_
           << " of " << read_buffer_->BytesConsumed();
}

bool SocketInputStream::Skip(int count) {
  NOTIMPLEMENTED();
  return false;
}

int64_t SocketInputStream::ByteCount() const {
  DCHECK_NE(GetState(), CLOSED);
  DCHECK_NE(GetState(), READING);
  return next_pos_;
}

int SocketInputStream::UnreadByteCount() const {
  DCHECK_NE(GetState(), CLOSED);
  DCHECK_NE(GetState(), READING);
  return read_buffer_->BytesConsumed() - next_pos_;
}

net::Error SocketInputStream::Refresh(const base::Closure& callback,
                                      int byte_limit) {
  DCHECK_NE(GetState(), CLOSED);
  DCHECK_NE(GetState(), READING);
  DCHECK_GT(byte_limit, 0);

  if (byte_limit > read_buffer_->BytesRemaining()) {
    LOG(ERROR) << "Out of buffer space, closing input stream.";
    CloseStream(net::ERR_FILE_TOO_BIG, base::Closure());
    return net::OK;
  }

  if (!socket_->IsConnected()) {
    LOG(ERROR) << "Socket was disconnected, closing input stream";
    CloseStream(net::ERR_CONNECTION_CLOSED, base::Closure());
    return net::OK;
  }

  DVLOG(1) << "Refreshing input stream, limit of " << byte_limit << " bytes.";
  int result =
      socket_->Read(read_buffer_.get(),
                    byte_limit,
                    base::Bind(&SocketInputStream::RefreshCompletionCallback,
                               weak_ptr_factory_.GetWeakPtr(),
                               callback));
  DVLOG(1) << "Read returned " << result;
  if (result == net::ERR_IO_PENDING) {
    last_error_ = net::ERR_IO_PENDING;
    return net::ERR_IO_PENDING;
  }

  RefreshCompletionCallback(base::Closure(), result);
  return net::OK;
}

void SocketInputStream::RebuildBuffer() {
  DVLOG(1) << "Rebuilding input stream, consumed "
           << next_pos_ << " bytes.";
  DCHECK_NE(GetState(), READING);
  DCHECK_NE(GetState(), CLOSED);

  int unread_data_size = 0;
  const void* unread_data_ptr = NULL;
  Next(&unread_data_ptr, &unread_data_size);
  ResetInternal();

  if (unread_data_ptr != io_buffer_->data()) {
    DVLOG(1) << "Have " << unread_data_size
             << " unread bytes remaining, shifting.";
    // Move any remaining unread data to the start of the buffer;
    std::memmove(io_buffer_->data(), unread_data_ptr, unread_data_size);
  } else {
    DVLOG(1) << "Have " << unread_data_size << " unread bytes remaining.";
  }
  read_buffer_->DidConsume(unread_data_size);
  // TODO(zea): investigating crbug.com/409985
  CHECK_GE(UnreadByteCount(), 0);
}

net::Error SocketInputStream::last_error() const {
  return last_error_;
}

SocketInputStream::State SocketInputStream::GetState() const {
  if (last_error_ < net::ERR_IO_PENDING)
    return CLOSED;

  if (last_error_ == net::ERR_IO_PENDING)
    return READING;

  DCHECK_EQ(last_error_, net::OK);
  if (read_buffer_->BytesConsumed() == next_pos_)
    return EMPTY;

  return READY;
}

void SocketInputStream::RefreshCompletionCallback(
    const base::Closure& callback, int result) {
  // If an error occurred before the completion callback could complete, ignore
  // the result.
  if (GetState() == CLOSED)
    return;

  // Result == 0 implies EOF, which is treated as an error.
  if (result == 0)
    result = net::ERR_CONNECTION_CLOSED;

  DCHECK_NE(result, net::ERR_IO_PENDING);

  if (result < net::OK) {
    DVLOG(1) << "Failed to refresh socket: " << result;
    CloseStream(static_cast<net::Error>(result), callback);
    return;
  }

  DCHECK_GT(result, 0);
  last_error_ = net::OK;
  read_buffer_->DidConsume(result);
  // TODO(zea): investigating crbug.com/409985
  CHECK_GT(UnreadByteCount(), 0);

  DVLOG(1) << "Refresh complete with " << result << " new bytes. "
           << "Current position " << next_pos_
           << " of " << read_buffer_->BytesConsumed() << ".";

  if (!callback.is_null())
    callback.Run();
}

void SocketInputStream::ResetInternal() {
  read_buffer_->SetOffset(0);
  next_pos_ = 0;
  last_error_ = net::OK;
  weak_ptr_factory_.InvalidateWeakPtrs();  // Invalidate any callbacks.
}

void SocketInputStream::CloseStream(net::Error error,
                                    const base::Closure& callback) {
  DCHECK_LT(error, net::ERR_IO_PENDING);
  ResetInternal();
  last_error_ = error;
  LOG(ERROR) << "Closing stream with result " << error;
  if (!callback.is_null())
    callback.Run();
}

SocketOutputStream::SocketOutputStream(
    net::StreamSocket* socket,
    const net::NetworkTrafficAnnotationTag& traffic_annotation)
    : socket_(socket),
      io_buffer_(new net::IOBuffer(kDefaultBufferSize)),
      write_buffer_(
          new net::DrainableIOBuffer(io_buffer_.get(), kDefaultBufferSize)),
      next_pos_(0),
      last_error_(net::OK),
      traffic_annotation_(traffic_annotation),
      weak_ptr_factory_(this) {
  DCHECK(socket->IsConnected());
}

SocketOutputStream::~SocketOutputStream() {
}

bool SocketOutputStream::Next(void** data, int* size) {
  DCHECK_NE(GetState(), CLOSED);
  DCHECK_NE(GetState(), FLUSHING);
  if (next_pos_ == write_buffer_->size())
    return false;

  *data = write_buffer_->data() + next_pos_;
  *size = write_buffer_->size() - next_pos_;
  next_pos_ = write_buffer_->size();
  return true;
}

void SocketOutputStream::BackUp(int count) {
  DCHECK_GE(count, 0);
  if (count > next_pos_)
    next_pos_ = 0;
  next_pos_ -= count;
  DVLOG(1) << "Backing up " << count << " bytes in output buffer. "
           << next_pos_ << " bytes used.";
}

int64_t SocketOutputStream::ByteCount() const {
  DCHECK_NE(GetState(), CLOSED);
  DCHECK_NE(GetState(), FLUSHING);
  return next_pos_;
}

net::Error SocketOutputStream::Flush(const base::Closure& callback) {
  DCHECK_EQ(GetState(), READY);

  if (!socket_->IsConnected()) {
    LOG(ERROR) << "Socket was disconnected, closing output stream";
    last_error_ = net::ERR_CONNECTION_CLOSED;
    return net::OK;
  }

  DVLOG(1) << "Flushing " << next_pos_ << " bytes into socket.";
  int result =
      socket_->Write(write_buffer_.get(), next_pos_,
                     base::Bind(&SocketOutputStream::FlushCompletionCallback,
                                weak_ptr_factory_.GetWeakPtr(), callback),
                     traffic_annotation_);
  DVLOG(1) << "Write returned " << result;
  if (result == net::ERR_IO_PENDING) {
    last_error_ = net::ERR_IO_PENDING;
    return net::ERR_IO_PENDING;
  }

  FlushCompletionCallback(base::Closure(), result);
  return net::OK;
}

SocketOutputStream::State SocketOutputStream::GetState() const{
  if (last_error_ < net::ERR_IO_PENDING)
    return CLOSED;

  if (last_error_ == net::ERR_IO_PENDING)
    return FLUSHING;

  DCHECK_EQ(last_error_, net::OK);
  if (next_pos_ == 0)
    return EMPTY;

  return READY;
}

net::Error SocketOutputStream::last_error() const {
  return last_error_;
}

void SocketOutputStream::FlushCompletionCallback(
    const base::Closure& callback, int result) {
  // If an error occurred before the completion callback could complete, ignore
  // the result.
  if (GetState() == CLOSED)
    return;

  // Result == 0 implies EOF, which is treated as an error.
  if (result == 0)
    result = net::ERR_CONNECTION_CLOSED;

  DCHECK_NE(result, net::ERR_IO_PENDING);

  if (result < net::OK) {
    LOG(ERROR) << "Failed to flush socket.";
    last_error_ = static_cast<net::Error>(result);
    if (!callback.is_null())
      callback.Run();
    return;
  }

  DCHECK_GT(result, net::OK);
  last_error_ = net::OK;

  if (write_buffer_->BytesConsumed() + result < next_pos_) {
    DVLOG(1) << "Partial flush complete. Retrying.";
     // Only a partial write was completed. Flush again to finish the write.
    write_buffer_->DidConsume(result);
    Flush(callback);
    return;
  }

  DVLOG(1) << "Socket flush complete.";
  write_buffer_->SetOffset(0);
  next_pos_ = 0;
  if (!callback.is_null())
    callback.Run();
}

}  // namespace gcm
