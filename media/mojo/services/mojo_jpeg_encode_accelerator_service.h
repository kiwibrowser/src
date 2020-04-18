// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_JPEG_ENCODE_ACCELERATOR_SERVICE_H_
#define MEDIA_MOJO_SERVICES_MOJO_JPEG_ENCODE_ACCELERATOR_SERVICE_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "media/gpu/gpu_jpeg_encode_accelerator_factory.h"
#include "media/mojo/interfaces/jpeg_encode_accelerator.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "media/video/jpeg_encode_accelerator.h"

namespace media {

// Implementation of a mojom::JpegEncodeAccelerator which runs in the GPU
// process, and wraps a JpegEncodeAccelerator.
class MEDIA_MOJO_EXPORT MojoJpegEncodeAcceleratorService
    : public mojom::JpegEncodeAccelerator,
      public JpegEncodeAccelerator::Client {
 public:
  static void Create(mojom::JpegEncodeAcceleratorRequest request);

  ~MojoJpegEncodeAcceleratorService() override;

  // JpegEncodeAccelerator::Client implementation.
  void VideoFrameReady(int32_t buffer_id, size_t encoded_picture_size) override;
  void NotifyError(int32_t buffer_id,
                   ::media::JpegEncodeAccelerator::Status status) override;

 private:
  using EncodeCallbackMap = std::unordered_map<int32_t, EncodeWithFDCallback>;

  // This constructor internally calls
  // GpuJpegEncodeAcceleratorFactory::GetAcceleratorFactories() to
  // fill |accelerator_factory_functions_|.
  MojoJpegEncodeAcceleratorService();

  // mojom::JpegEncodeAccelerator implementation.
  void Initialize(InitializeCallback callback) override;
  void EncodeWithFD(int32_t buffer_id,
                    mojo::ScopedHandle input_fd,
                    uint32_t input_buffer_size,
                    int32_t coded_size_width,
                    int32_t coded_size_height,
                    mojo::ScopedHandle exif_fd,
                    uint32_t exif_buffer_size,
                    mojo::ScopedHandle output_fd,
                    uint32_t output_buffer_size,
                    EncodeWithFDCallback callback) override;

  void NotifyEncodeStatus(int32_t bitstream_buffer_id,
                          size_t encoded_picture_size,
                          ::media::JpegEncodeAccelerator::Status status);

  const std::vector<GpuJpegEncodeAcceleratorFactory::CreateAcceleratorCB>
      accelerator_factory_functions_;

  // A map from bitstream_buffer_id to EncodeCallback.
  EncodeCallbackMap encode_cb_map_;

  std::unique_ptr<::media::JpegEncodeAccelerator> accelerator_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(MojoJpegEncodeAcceleratorService);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_JPEG_ENCODE_ACCELERATOR_SERVICE_H_
