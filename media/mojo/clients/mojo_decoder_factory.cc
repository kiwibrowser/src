// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_decoder_factory.h"

#include <memory>

#include "base/feature_list.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "media/base/media_switches.h"
#include "media/mojo/buildflags.h"
#include "media/mojo/clients/mojo_audio_decoder.h"
#include "media/mojo/clients/mojo_video_decoder.h"
#include "media/mojo/interfaces/audio_decoder.mojom.h"
#include "media/mojo/interfaces/interface_factory.mojom.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace media {

MojoDecoderFactory::MojoDecoderFactory(
    media::mojom::InterfaceFactory* interface_factory)
    : interface_factory_(interface_factory) {
  DCHECK(interface_factory_);
}

MojoDecoderFactory::~MojoDecoderFactory() = default;

void MojoDecoderFactory::CreateAudioDecoders(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    MediaLog* media_log,
    std::vector<std::unique_ptr<AudioDecoder>>* audio_decoders) {
#if BUILDFLAG(ENABLE_MOJO_AUDIO_DECODER)
  mojom::AudioDecoderPtr audio_decoder_ptr;
  interface_factory_->CreateAudioDecoder(mojo::MakeRequest(&audio_decoder_ptr));

  audio_decoders->push_back(std::make_unique<MojoAudioDecoder>(
      task_runner, std::move(audio_decoder_ptr)));
#endif
}

void MojoDecoderFactory::CreateVideoDecoders(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    GpuVideoAcceleratorFactories* gpu_factories,
    MediaLog* media_log,
    const RequestOverlayInfoCB& request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space,
    std::vector<std::unique_ptr<VideoDecoder>>* video_decoders) {
#if BUILDFLAG(ENABLE_MOJO_VIDEO_DECODER)
  // If MojoVideoDecoder is not enabled, then return without adding anything.
  if (!base::FeatureList::IsEnabled(media::kMojoVideoDecoder))
    return;
  mojom::VideoDecoderPtr video_decoder_ptr;
  interface_factory_->CreateVideoDecoder(mojo::MakeRequest(&video_decoder_ptr));

  video_decoders->push_back(std::make_unique<MojoVideoDecoder>(
      task_runner, gpu_factories, media_log, std::move(video_decoder_ptr),
      request_overlay_info_cb, target_color_space));
#endif
}

}  // namespace media
