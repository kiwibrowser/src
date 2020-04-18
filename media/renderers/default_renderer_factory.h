// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_RENDERERS_DEFAULT_RENDERER_FACTORY_H_
#define MEDIA_RENDERERS_DEFAULT_RENDERER_FACTORY_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "media/base/media_export.h"
#include "media/base/renderer_factory.h"

namespace media {

class AudioDecoder;
class AudioRendererSink;
class DecoderFactory;
class GpuVideoAcceleratorFactories;
class MediaLog;
class VideoDecoder;
class VideoRendererSink;

using CreateAudioDecodersCB =
    base::RepeatingCallback<std::vector<std::unique_ptr<AudioDecoder>>()>;
using CreateVideoDecodersCB =
    base::RepeatingCallback<std::vector<std::unique_ptr<VideoDecoder>>()>;

// The default factory class for creating RendererImpl.
class MEDIA_EXPORT DefaultRendererFactory : public RendererFactory {
 public:
  using GetGpuFactoriesCB = base::Callback<GpuVideoAcceleratorFactories*()>;

  DefaultRendererFactory(MediaLog* media_log,
                         DecoderFactory* decoder_factory,
                         const GetGpuFactoriesCB& get_gpu_factories_cb);
  ~DefaultRendererFactory() final;

  std::unique_ptr<Renderer> CreateRenderer(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const scoped_refptr<base::TaskRunner>& worker_task_runner,
      AudioRendererSink* audio_renderer_sink,
      VideoRendererSink* video_renderer_sink,
      const RequestOverlayInfoCB& request_overlay_info_cb,
      const gfx::ColorSpace& target_color_space) final;

 private:
  std::vector<std::unique_ptr<AudioDecoder>> CreateAudioDecoders(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner);
  std::vector<std::unique_ptr<VideoDecoder>> CreateVideoDecoders(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const RequestOverlayInfoCB& request_overlay_info_cb,
      const gfx::ColorSpace& target_color_space,
      GpuVideoAcceleratorFactories* gpu_factories);

  MediaLog* media_log_;

  // Factory to create extra audio and video decoders.
  // Could be nullptr if not extra decoders are available.
  DecoderFactory* decoder_factory_;

  // Creates factories for supporting video accelerators. May be null.
  GetGpuFactoriesCB get_gpu_factories_cb_;

  DISALLOW_COPY_AND_ASSIGN(DefaultRendererFactory);
};

}  // namespace media

#endif  // MEDIA_RENDERERS_DEFAULT_RENDERER_FACTORY_H_
