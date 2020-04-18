// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_IO_BUFFER_WITH_CRONET_BUFFER_H_
#define COMPONENTS_CRONET_NATIVE_IO_BUFFER_WITH_CRONET_BUFFER_H_

#include <memory>

#include "components/cronet/native/generated/cronet.idl_c.h"
#include "net/base/io_buffer.h"

namespace cronet {

// net::WrappedIOBuffer subclass for a buffer owned by a Cronet_Buffer.
// Keeps the Cronet_Buffer alive until destroyed or released.
// Uses WrappedIOBuffer because data() is owned by the Cronet_Buffer.
class IOBufferWithCronet_Buffer : public net::WrappedIOBuffer {
 public:
  // Creates a buffer that takes ownership of the Cronet_Buffer.
  explicit IOBufferWithCronet_Buffer(Cronet_BufferPtr cronet_buffer);

  // Releases ownership of |cronet_buffer_| and returns it to caller.
  Cronet_BufferPtr Release();

 private:
  ~IOBufferWithCronet_Buffer() override;

  // Cronet buffer owned by |this|.
  std::unique_ptr<Cronet_Buffer> cronet_buffer_;

  DISALLOW_COPY_AND_ASSIGN(IOBufferWithCronet_Buffer);
};

}  // namespace cronet

#endif  // COMPONENTS_CRONET_NATIVE_IO_BUFFER_WITH_CRONET_BUFFER_H_
