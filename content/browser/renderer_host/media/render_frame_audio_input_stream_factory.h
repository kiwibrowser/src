// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDER_FRAME_AUDIO_INPUT_STREAM_FACTORY_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDER_FRAME_AUDIO_INPUT_STREAM_FACTORY_H_

#include <cstdint>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "content/browser/renderer_host/media/audio_input_device_manager.h"
#include "content/common/content_export.h"
#include "content/common/media/renderer_audio_input_stream_factory.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/media_stream_request.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {
class AudioParameters;
}  // namespace media

namespace content {

class AudioInputDeviceManager;
class RenderFrameHost;

// Handles a RendererAudioInputStreamFactory request for a render frame host,
// using the provided RendererAudioInputStreamFactoryContext. This class may
// be constructed on any thread, but must be used on the IO thread after that,
// and also destructed on the IO thread.
class CONTENT_EXPORT RenderFrameAudioInputStreamFactory
    : public mojom::RendererAudioInputStreamFactory {
 public:
  RenderFrameAudioInputStreamFactory(
      mojom::RendererAudioInputStreamFactoryRequest request,
      scoped_refptr<AudioInputDeviceManager> audio_input_device_manager,
      RenderFrameHost* render_frame_host);

  ~RenderFrameAudioInputStreamFactory() override;

 private:
  // mojom::RendererAudioInputStreamFactory implementation.
  void CreateStream(mojom::RendererAudioInputStreamFactoryClientPtr client,
                    int32_t session_id,
                    const media::AudioParameters& audio_params,
                    bool automatic_gain_control,
                    uint32_t shared_memory_count) override;

  void CreateStreamAfterLookingUpDevice(
      mojom::RendererAudioInputStreamFactoryClientPtr client,
      const media::AudioParameters& audio_params,
      bool automatic_gain_control,
      uint32_t shared_memory_count,
      const MediaStreamDevice& device);

  void AssociateInputAndOutputForAec(
      const base::UnguessableToken& input_stream_id,
      const std::string& output_device_id) override;

  void AssociateTranslatedOutputDeviceForAec(
      const base::UnguessableToken& input_stream_id,
      const std::string& raw_output_device_id);

  const mojo::Binding<RendererAudioInputStreamFactory> binding_;
  const scoped_refptr<AudioInputDeviceManager> audio_input_device_manager_;
  RenderFrameHost* const render_frame_host_;

  base::WeakPtrFactory<RenderFrameAudioInputStreamFactory> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RenderFrameAudioInputStreamFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDER_FRAME_AUDIO_INPUT_STREAM_FACTORY_H_
