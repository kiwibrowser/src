// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/numerics/safe_conversions.h"
#include "device/serial/buffer.h"
#include "net/base/io_buffer.h"

namespace device {

ReadOnlyBuffer::~ReadOnlyBuffer() = default;

WritableBuffer::~WritableBuffer() = default;

SendBuffer::SendBuffer(const std::vector<uint8_t>& data,
                       SendCompleteCallback callback)
    : data_(data), callback_(std::move(callback)) {}

SendBuffer::~SendBuffer() = default;

const uint8_t* SendBuffer::GetData() {
  return data_.data();
}

uint32_t SendBuffer::GetSize() {
  return base::checked_cast<uint32_t>(data_.size());
}

void SendBuffer::Done(uint32_t bytes_read) {
  std::move(callback_).Run(bytes_read, device::mojom::SerialSendError::NONE);
}

void SendBuffer::DoneWithError(uint32_t bytes_read, int32_t error) {
  std::move(callback_).Run(bytes_read,
                           static_cast<device::mojom::SerialSendError>(error));
}

ReceiveBuffer::ReceiveBuffer(scoped_refptr<net::IOBuffer> buffer,
                             uint32_t size,
                             ReceiveCompleteCallback callback)
    : buffer_(buffer), size_(size), callback_(std::move(callback)) {}

ReceiveBuffer::~ReceiveBuffer() = default;

char* ReceiveBuffer::GetData() {
  return buffer_->data();
}

uint32_t ReceiveBuffer::GetSize() {
  return size_;
}

void ReceiveBuffer::Done(uint32_t bytes_written) {
  std::move(callback_).Run(bytes_written,
                           device::mojom::SerialReceiveError::NONE);
}

void ReceiveBuffer::DoneWithError(uint32_t bytes_written, int32_t error) {
  std::move(callback_).Run(
      bytes_written, static_cast<device::mojom::SerialReceiveError>(error));
}

}  // namespace device
