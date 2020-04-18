// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDERER_AUDIO_OUTPUT_STREAM_FACTORY_CONTEXT_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDERER_AUDIO_OUTPUT_STREAM_FACTORY_CONTEXT_H_

#include <memory>
#include <string>

#include "content/browser/renderer_host/media/audio_output_authorization_handler.h"
#include "content/common/content_export.h"
#include "media/audio/audio_output_delegate.h"
#include "media/mojo/interfaces/audio_output_stream.mojom.h"

namespace media {
class AudioParameters;
}

namespace content {

// RendererAudioOutputStreamFactoryContext provides functions common to all
// AudioOutputFactory instances for a single renderer process.
// Not needed after switching to serving audio streams with the audio service
// (https://crbug.com/830493).
class CONTENT_EXPORT RendererAudioOutputStreamFactoryContext {
 public:
  virtual ~RendererAudioOutputStreamFactoryContext() {}

  using AuthorizationCompletedCallback =
      AudioOutputAuthorizationHandler::AuthorizationCompletedCallback;

  virtual int GetRenderProcessId() const = 0;

  // Called to request access to a device on behalf of the renderer.
  virtual void RequestDeviceAuthorization(
      int render_frame_id,
      int session_id,
      const std::string& device_id,
      AuthorizationCompletedCallback cb) const = 0;

  virtual std::unique_ptr<media::AudioOutputDelegate> CreateDelegate(
      const std::string& unique_device_id,
      int render_frame_id,
      int stream_id,
      const media::AudioParameters& params,
      media::mojom::AudioOutputStreamObserverPtr stream_observer,
      media::AudioOutputDelegate::EventHandler* handler) = 0;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDERER_AUDIO_OUTPUT_STREAM_FACTORY_CONTEXT_H_
