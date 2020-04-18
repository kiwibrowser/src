// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/pairing/message_buffer.h"

#include "net/base/io_buffer.h"

namespace pairing_chromeos {

MessageBuffer::MessageBuffer()
    : buffer_offset_(0),
      total_buffer_size_(0) {
}

MessageBuffer::~MessageBuffer() {}

int MessageBuffer::AvailableBytes() {
  return total_buffer_size_ - buffer_offset_;
}

void MessageBuffer::ReadBytes(char* buffer, int size) {
  CHECK_LE(size, AvailableBytes());

  int offset = 0;
  while (offset < size) {
    scoped_refptr<net::IOBuffer> io_buffer = pending_data_.front().first;
    int io_buffer_size = pending_data_.front().second;

    CHECK_GT(io_buffer_size, buffer_offset_);
    int copy_size = std::min(size - offset, io_buffer_size - buffer_offset_);
    memcpy(&buffer[offset], &io_buffer->data()[buffer_offset_], copy_size);

    offset += copy_size;
    buffer_offset_ += copy_size;

    CHECK_LE(buffer_offset_, io_buffer_size);
    if (buffer_offset_ == io_buffer_size) {
      CHECK_GE(total_buffer_size_, io_buffer_size);
      total_buffer_size_ -= io_buffer_size;
      pending_data_.pop_front();
      buffer_offset_ = 0;
    }
  }
  CHECK_EQ(offset, size);
}

void MessageBuffer::AddIOBuffer(scoped_refptr<net::IOBuffer> io_buffer,
                                int size) {
  if (size == 0)
    return;
  pending_data_.push_back(std::make_pair(io_buffer, size));
  total_buffer_size_ += size;
}

}  // namespace pairing_chromeos
