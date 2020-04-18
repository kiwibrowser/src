// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_RENDERERS_DEFAULT_DECODER_FACTORY_H_
#define MEDIA_RENDERERS_DEFAULT_DECODER_FACTORY_H_

#include <memory>

#include "media/base/decoder_factory.h"
#include "media/base/media_export.h"

namespace media {

class MEDIA_EXPORT DefaultDecoderFactory : public DecoderFactory {
 public:
  // |external_decoder_factory| is optional decoder factory that provides
  // additional decoders.
  explicit DefaultDecoderFactory(
      std::unique_ptr<DecoderFactory> external_decoder_factory);
  ~DefaultDecoderFactory() final;

  void CreateAudioDecoders(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      MediaLog* media_log,
      std::vector<std::unique_ptr<AudioDecoder>>* audio_decoders) final;

  void CreateVideoDecoders(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      GpuVideoAcceleratorFactories* gpu_factories,
      MediaLog* media_log,
      const RequestOverlayInfoCB& request_overlay_info_cb,
      const gfx::ColorSpace& target_color_space,
      std::vector<std::unique_ptr<VideoDecoder>>* video_decoders) final;

 private:
  std::unique_ptr<DecoderFactory> external_decoder_factory_;

  DISALLOW_COPY_AND_ASSIGN(DefaultDecoderFactory);
};

}  // namespace media

#endif  // MEDIA_RENDERERS_DEFAULT_DECODER_FACTORY_H_
