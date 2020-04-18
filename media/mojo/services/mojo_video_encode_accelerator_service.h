// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_VIDEO_ENCODE_ACCELERATOR_SERVICE_H_
#define MEDIA_MOJO_SERVICES_MOJO_VIDEO_ENCODE_ACCELERATOR_SERVICE_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "media/mojo/interfaces/video_encode_accelerator.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "media/video/video_encode_accelerator.h"

namespace gpu {
struct GpuPreferences;
}  // namespace gpu

namespace media {

// This class implements the interface mojom::VideoEncodeAccelerator.
class MEDIA_MOJO_EXPORT MojoVideoEncodeAcceleratorService
    : public mojom::VideoEncodeAccelerator,
      public VideoEncodeAccelerator::Client {
 public:
  // Create and initialize a VEA. Returns nullptr if either part fails.
  using CreateAndInitializeVideoEncodeAcceleratorCallback =
      base::Callback<std::unique_ptr<::media::VideoEncodeAccelerator>(
          VideoPixelFormat input_format,
          const gfx::Size& input_visible_size,
          VideoCodecProfile output_profile,
          uint32_t initial_bitrate,
          Client* client,
          const gpu::GpuPreferences& gpu_preferences)>;

  static void Create(mojom::VideoEncodeAcceleratorRequest request,
                     const CreateAndInitializeVideoEncodeAcceleratorCallback&
                         create_vea_callback,
                     const gpu::GpuPreferences& gpu_preferences);

  MojoVideoEncodeAcceleratorService(
      const CreateAndInitializeVideoEncodeAcceleratorCallback&
          create_vea_callback,
      const gpu::GpuPreferences& gpu_preferences);
  ~MojoVideoEncodeAcceleratorService() override;

  // mojom::VideoEncodeAccelerator impl.
  void Initialize(VideoPixelFormat input_format,
                  const gfx::Size& input_visible_size,
                  VideoCodecProfile output_profile,
                  uint32_t initial_bitrate,
                  mojom::VideoEncodeAcceleratorClientPtr client,
                  InitializeCallback callback) override;
  void Encode(const scoped_refptr<VideoFrame>& frame,
              bool force_keyframe,
              EncodeCallback callback) override;
  void UseOutputBitstreamBuffer(int32_t bitstream_buffer_id,
                                mojo::ScopedSharedBufferHandle buffer) override;
  void RequestEncodingParametersChange(uint32_t bitrate,
                                       uint32_t framerate) override;

 private:
  friend class MojoVideoEncodeAcceleratorIntegrationTest;
  friend class MojoVideoEncodeAcceleratorServiceTest;

  // VideoEncodeAccelerator::Client implementation.
  void RequireBitstreamBuffers(unsigned int input_count,
                               const gfx::Size& input_coded_size,
                               size_t output_buffer_size) override;
  void BitstreamBufferReady(int32_t bitstream_buffer_id,
                            size_t payload_size,
                            bool key_frame,
                            base::TimeDelta timestamp) override;
  void NotifyError(::media::VideoEncodeAccelerator::Error error) override;

  const CreateAndInitializeVideoEncodeAcceleratorCallback create_vea_callback_;
  const gpu::GpuPreferences& gpu_preferences_;

  // Owned pointer to the underlying VideoEncodeAccelerator.
  std::unique_ptr<::media::VideoEncodeAccelerator> encoder_;
  mojom::VideoEncodeAcceleratorClientPtr vea_client_;

  // Cache of parameters for sanity verification.
  size_t output_buffer_size_;
  gfx::Size input_coded_size_;

  // Note that this class is already thread hostile when bound.
  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<MojoVideoEncodeAcceleratorService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MojoVideoEncodeAcceleratorService);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_VIDEO_ENCODE_ACCELERATOR_SERVICE_H_
