// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_SERIAL_BUFFER_H_
#define DEVICE_SERIAL_BUFFER_H_

#include <stdint.h>

#include "base/callback.h"
#include "net/base/io_buffer.h"
#include "services/device/public/mojom/serial.mojom.h"

namespace device {

// A fixed-size read-only buffer. The data-reader should call Done() when it is
// finished reading bytes from the buffer. Alternatively, the reader can report
// an error by calling DoneWithError() with the number of bytes read and the
// error it wishes to report.
class ReadOnlyBuffer {
 public:
  virtual ~ReadOnlyBuffer();
  virtual const uint8_t* GetData() = 0;
  virtual uint32_t GetSize() = 0;
  virtual void Done(uint32_t bytes_read) = 0;
  virtual void DoneWithError(uint32_t bytes_read, int32_t error) = 0;
};

// A fixed-size writable buffer. The data-writer should call Done() when it is
// finished writing bytes to the buffer. Alternatively, the writer can report
// an error by calling DoneWithError() with the number of bytes written and the
// error it wishes to report.
class WritableBuffer {
 public:
  virtual ~WritableBuffer();
  virtual char* GetData() = 0;
  virtual uint32_t GetSize() = 0;
  virtual void Done(uint32_t bytes_written) = 0;
  virtual void DoneWithError(uint32_t bytes_written, int32_t error) = 0;
};

// A useful basic implementation of a ReadOnlyBuffer in which the data is
// initialized via a character vector.
class SendBuffer : public device::ReadOnlyBuffer {
 public:
  using SendCompleteCallback =
      base::OnceCallback<void(int, device::mojom::SerialSendError)>;
  SendBuffer(const std::vector<uint8_t>& data, SendCompleteCallback callback);
  ~SendBuffer() override;

  const uint8_t* GetData() override;
  uint32_t GetSize() override;
  void Done(uint32_t bytes_read) override;
  void DoneWithError(uint32_t bytes_read, int32_t error) override;

 private:
  const std::vector<uint8_t> data_;
  SendCompleteCallback callback_;
};

// A useful basic implementation of a WritableBuffer in which the data is
// stored in a net::IOBuffer.
class ReceiveBuffer : public device::WritableBuffer {
 public:
  using ReceiveCompleteCallback =
      base::OnceCallback<void(int, device::mojom::SerialReceiveError)>;
  ReceiveBuffer(scoped_refptr<net::IOBuffer> buffer,
                uint32_t size,
                ReceiveCompleteCallback callback);
  ~ReceiveBuffer() override;

  char* GetData() override;
  uint32_t GetSize() override;
  void Done(uint32_t bytes_written) override;
  void DoneWithError(uint32_t bytes_written, int32_t error) override;

 private:
  scoped_refptr<net::IOBuffer> buffer_;
  const uint32_t size_;
  ReceiveCompleteCallback callback_;
};

}  // namespace device

#endif  // DEVICE_SERIAL_BUFFER_H_
