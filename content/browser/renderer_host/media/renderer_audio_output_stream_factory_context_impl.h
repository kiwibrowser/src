// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDERER_AUDIO_OUTPUT_STREAM_FACTORY_CONTEXT_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDERER_AUDIO_OUTPUT_STREAM_FACTORY_CONTEXT_IMPL_H_

#include <memory>
#include <string>

#include "content/browser/renderer_host/media/audio_output_authorization_handler.h"
#include "content/browser/renderer_host/media/old_render_frame_audio_output_stream_factory.h"
#include "content/browser/renderer_host/media/renderer_audio_output_stream_factory_context.h"
#include "content/public/browser/browser_thread.h"

namespace media {
class AudioManager;
class AudioSystem;
}  // namespace media

namespace content {

class MediaStreamManager;

// In addition to being a RendererAudioOutputStreamFactoryContext, this class
// also handles requests for mojom::RendererAudioOutputStreamFactory instances.
//
// Ownership diagram for stream IPC classes (excluding interfaces):
// RendererAudioOutputStreamFactoryContext
//                 ^
//                 | owns (at most one per render frame in the process).
//                 |
// RenderFrameAudioOutputStreamFactory
//                 ^
//                 | owns (one per stream for the frame).
//                 |
// media::MojoAudioOutputStreamProvider
//                 ^
//                 | owns (one).
//                 |
// media::MojoAudioOutputStream
//
// Not needed after switching to serving audio streams with the audio service
// (https://crbug.com/830493).
class CONTENT_EXPORT RendererAudioOutputStreamFactoryContextImpl
    : public RendererAudioOutputStreamFactoryContext {
 public:
  RendererAudioOutputStreamFactoryContextImpl(
      int render_process_id,
      media::AudioSystem* audio_system,
      media::AudioManager* audio_manager,
      MediaStreamManager* media_stream_manager);

  ~RendererAudioOutputStreamFactoryContextImpl() override;

  // RendererAudioOutputStreamFactoryContext implementation. To be called on
  // the IO thread.
  int GetRenderProcessId() const override;

  void RequestDeviceAuthorization(
      int render_frame_id,
      int session_id,
      const std::string& device_id,
      AuthorizationCompletedCallback cb) const override;

  std::unique_ptr<media::AudioOutputDelegate> CreateDelegate(
      const std::string& unique_device_id,
      int render_frame_id,
      int stream_id,
      const media::AudioParameters& params,
      media::mojom::AudioOutputStreamObserverPtr stream_observer,
      media::AudioOutputDelegate::EventHandler* handler) override;

 private:
  // Used for hashing the device_id.
  media::AudioSystem* const audio_system_;
  media::AudioManager* const audio_manager_;
  MediaStreamManager* const media_stream_manager_;
  const AudioOutputAuthorizationHandler authorization_handler_;
  const int render_process_id_;

  DISALLOW_COPY_AND_ASSIGN(RendererAudioOutputStreamFactoryContextImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDERER_AUDIO_OUTPUT_STREAM_FACTORY_CONTEXT_IMPL_H_
