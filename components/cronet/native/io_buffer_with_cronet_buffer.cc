// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/io_buffer_with_cronet_buffer.h"

#include "components/cronet/native/generated/cronet.idl_impl_interface.h"

namespace cronet {

IOBufferWithCronet_Buffer::IOBufferWithCronet_Buffer(
    Cronet_BufferPtr cronet_buffer)
    : net::WrappedIOBuffer(
          reinterpret_cast<const char*>(cronet_buffer->GetData())),
      cronet_buffer_(cronet_buffer) {}

IOBufferWithCronet_Buffer::~IOBufferWithCronet_Buffer() {
  if (cronet_buffer_) {
    Cronet_Buffer_Destroy(cronet_buffer_.release());
  }
}

Cronet_BufferPtr IOBufferWithCronet_Buffer::Release() {
  data_ = nullptr;
  return cronet_buffer_.release();
}

}  // namespace cronet
