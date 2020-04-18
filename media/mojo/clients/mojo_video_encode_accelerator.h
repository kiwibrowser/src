// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_CLIENTS_MOJO_VIDEO_ENCODE_ACCELERATOR_H_
#define MEDIA_MOJO_CLIENTS_MOJO_VIDEO_ENCODE_ACCELERATOR_H_

#include <stdint.h>

#include <vector>

#include "base/sequence_checker.h"
#include "gpu/config/gpu_info.h"
#include "media/mojo/interfaces/video_encode_accelerator.mojom.h"
#include "media/video/video_encode_accelerator.h"

namespace gfx {
class Size;
}  // namespace gfx

namespace media {
class VideoFrame;
}  // namespace media

namespace media {

// This class is a renderer-side host bridge from VideoEncodeAccelerator to a
// remote media::mojom::VideoEncodeAccelerator passed on ctor and held as
// |vea_|. An internal mojo::VEA::Client acts as the remote's mojo VEA client,
// trampolining methods to the media::VEA::Client passed on Initialize(). For
// proper operation this class should be managed in a std::unique_ptr that calls
// Destroy() upon destruction.
class MojoVideoEncodeAccelerator : public VideoEncodeAccelerator {
 public:
  MojoVideoEncodeAccelerator(
      mojom::VideoEncodeAcceleratorPtr vea,
      const gpu::VideoEncodeAcceleratorSupportedProfiles& supported_profiles);

  // VideoEncodeAccelerator implementation.
  SupportedProfiles GetSupportedProfiles() override;
  bool Initialize(VideoPixelFormat input_format,
                  const gfx::Size& input_visible_size,
                  VideoCodecProfile output_profile,
                  uint32_t initial_bitrate,
                  Client* client) override;
  void Encode(const scoped_refptr<VideoFrame>& frame,
              bool force_keyframe) override;
  void UseOutputBitstreamBuffer(const BitstreamBuffer& buffer) override;
  void RequestEncodingParametersChange(uint32_t bitrate,
                                       uint32_t framerate_num) override;
  void Destroy() override;

 private:
  // Only Destroy() should be deleting |this|.
  ~MojoVideoEncodeAccelerator() override;

  mojom::VideoEncodeAcceleratorPtr vea_;

  // Constructed during Initialize().
  std::unique_ptr<mojom::VideoEncodeAcceleratorClient> vea_client_;

  const gpu::VideoEncodeAcceleratorSupportedProfiles supported_profiles_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(MojoVideoEncodeAccelerator);
};

}  // namespace media

#endif  // MEDIA_MOJO_CLIENTS_MOJO_VIDEO_ENCODE_ACCELERATOR_H_
