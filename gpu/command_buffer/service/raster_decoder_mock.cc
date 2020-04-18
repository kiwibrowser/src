// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/raster_decoder_mock.h"

#include "gpu/command_buffer/service/gles2_cmd_decoder.h"

namespace gpu {
namespace raster {

MockRasterDecoder::MockRasterDecoder(
    CommandBufferServiceBase* command_buffer_service)
    : RasterDecoder(command_buffer_service, /*outputter=*/nullptr),
      weak_ptr_factory_(this) {
  ON_CALL(*this, MakeCurrent()).WillByDefault(testing::Return(true));
}

MockRasterDecoder::~MockRasterDecoder() = default;

base::WeakPtr<DecoderContext> MockRasterDecoder::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace raster
}  // namespace gpu
