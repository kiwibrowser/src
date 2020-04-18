// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_JPEG_DECODE_ACCELERATOR_SERVICE_H_
#define MEDIA_MOJO_SERVICES_MOJO_JPEG_DECODE_ACCELERATOR_SERVICE_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "media/gpu/gpu_jpeg_decode_accelerator_factory.h"
#include "media/mojo/interfaces/jpeg_decode_accelerator.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "media/video/jpeg_decode_accelerator.h"

namespace media {

// Implementation of a mojom::JpegDecodeAccelerator which runs in the GPU
// process, and wraps a JpegDecodeAccelerator.
class MEDIA_MOJO_EXPORT MojoJpegDecodeAcceleratorService
    : public mojom::JpegDecodeAccelerator,
      public JpegDecodeAccelerator::Client {
 public:
  static void Create(mojom::JpegDecodeAcceleratorRequest request);

  ~MojoJpegDecodeAcceleratorService() override;

  // JpegDecodeAccelerator::Client implementation.
  void VideoFrameReady(int32_t buffer_id) override;
  void NotifyError(int32_t buffer_id,
                   ::media::JpegDecodeAccelerator::Error error) override;

 private:
  using DecodeCallbackMap = std::unordered_map<int32_t, DecodeCallback>;

  // This constructor internally calls
  // GpuJpegDecodeAcceleratorFactory::GetAcceleratorFactories() to
  // fill |accelerator_factory_functions_|.
  MojoJpegDecodeAcceleratorService();

  // mojom::JpegDecodeAccelerator implementation.
  void Initialize(InitializeCallback callback) override;
  void Decode(const BitstreamBuffer& input_buffer,
              const gfx::Size& coded_size,
              mojo::ScopedSharedBufferHandle output_handle,
              uint32_t output_buffer_size,
              DecodeCallback callback) override;
  void DecodeWithFD(int32_t buffer_id,
                    mojo::ScopedHandle input_fd,
                    uint32_t input_buffer_size,
                    int32_t coded_size_width,
                    int32_t coded_size_height,
                    mojo::ScopedHandle output_fd,
                    uint32_t output_buffer_size,
                    DecodeWithFDCallback callback) override;
  void Uninitialize() override;

  void NotifyDecodeStatus(int32_t bitstream_buffer_id,
                          ::media::JpegDecodeAccelerator::Error error);

  const std::vector<GpuJpegDecodeAcceleratorFactory::CreateAcceleratorCB>
      accelerator_factory_functions_;

  // A map from bitstream_buffer_id to DecodeCallback.
  DecodeCallbackMap decode_cb_map_;

  std::unique_ptr<::media::JpegDecodeAccelerator> accelerator_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(MojoJpegDecodeAcceleratorService);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_JPEG_DECODE_ACCELERATOR_SERVICE_H_
