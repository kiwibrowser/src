// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDER_FRAME_AUDIO_OUTPUT_STREAM_FACTORY_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDER_FRAME_AUDIO_OUTPUT_STREAM_FACTORY_H_

#include <memory>
#include <string>

#include "base/containers/flat_set.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/common/media/renderer_audio_output_stream_factory.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "media/base/output_device_info.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {
class AudioSystem;
class AudioParameters;
}  // namespace media

namespace content {

class AudioOutputAuthorizationHandler;
class MediaStreamManager;
class RenderFrameHost;

// This class, which lives on the UI thread, takes care of stream requests from
// a render frame. It verifies that the stream creation is allowed and then
// forwards the request to the appropriate ForwardingAudioStreamFactory.
class CONTENT_EXPORT RenderFrameAudioOutputStreamFactory
    : public mojom::RendererAudioOutputStreamFactory {
 public:
  RenderFrameAudioOutputStreamFactory(
      RenderFrameHost* frame,
      media::AudioSystem* audio_system,
      MediaStreamManager* media_stream_manager,
      mojom::RendererAudioOutputStreamFactoryRequest request);

  ~RenderFrameAudioOutputStreamFactory() override;

  size_t current_number_of_providers_for_testing() {
    return stream_providers_.size();
  }

 private:
  class ProviderImpl;
  friend class ProviderImpl;  // For DeleteProvider.

  using OutputStreamProviderSet =
      base::flat_set<std::unique_ptr<media::mojom::AudioOutputStreamProvider>,
                     base::UniquePtrComparator>;

  // mojom::RendererAudioOutputStreamFactory implementation.
  void RequestDeviceAuthorization(
      media::mojom::AudioOutputStreamProviderRequest provider_request,
      int32_t session_id,
      const std::string& device_id,
      RequestDeviceAuthorizationCallback callback) override;

  // Here, the |raw_device_id| is used to create the stream, and
  // |device_id_for_renderer| is nonempty in the case when the renderer
  // requested a device using a |session_id|, to let it know which device was
  // chosen. This id is hashed.
  void AuthorizationCompleted(
      base::TimeTicks auth_start_time,
      media::mojom::AudioOutputStreamProviderRequest request,
      RequestDeviceAuthorizationCallback callback,
      media::OutputDeviceStatus status,
      const media::AudioParameters& params,
      const std::string& raw_device_id,
      const std::string& device_id_for_renderer);

  void DeleteProvider(media::mojom::AudioOutputStreamProvider* stream_provider);

  const mojo::Binding<mojom::RendererAudioOutputStreamFactory> binding_;
  RenderFrameHost* const frame_;
  const std::unique_ptr<AudioOutputAuthorizationHandler,
                        BrowserThread::DeleteOnIOThread>
      authorization_handler_;

  // The OutputStreamProviders for authorized streams are kept here while
  // waiting for the renderer to finish creating the stream, and destructed
  // afterwards.
  OutputStreamProviderSet stream_providers_;

  // Weak pointers are used to cancel device authorizations that are in flight
  // while |this| is destructed.
  base::WeakPtrFactory<RenderFrameAudioOutputStreamFactory> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RenderFrameAudioOutputStreamFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDER_FRAME_AUDIO_OUTPUT_STREAM_FACTORY_H_
