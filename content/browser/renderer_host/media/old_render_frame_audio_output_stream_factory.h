// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_OLD_RENDER_FRAME_AUDIO_OUTPUT_STREAM_FACTORY_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_OLD_RENDER_FRAME_AUDIO_OUTPUT_STREAM_FACTORY_H_

#include <memory>
#include <string>

#include "base/containers/flat_set.h"
#include "base/containers/unique_ptr_adapters.h"
#include "content/common/content_export.h"
#include "content/common/media/renderer_audio_output_stream_factory.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

class RendererAudioOutputStreamFactoryContext;

// Handles a RendererAudioOutputStreamFactory request for a render frame host,
// using the provided RendererAudioOutputStreamFactoryContext. This class may
// be constructed on any thread, but must be used on the IO thread after that.
// This class is used for creating streams hosted by the browser. It is being
// replaced by RenderFrameAudioOutputStreamFactory, which forwards stream
// requests to the audio service (https://crbug.com/830493).
class CONTENT_EXPORT OldRenderFrameAudioOutputStreamFactory
    : public mojom::RendererAudioOutputStreamFactory {
 public:
  OldRenderFrameAudioOutputStreamFactory(
      int render_frame_id,
      RendererAudioOutputStreamFactoryContext* context);

  ~OldRenderFrameAudioOutputStreamFactory() override;

 private:
  using OutputStreamProviderSet =
      base::flat_set<std::unique_ptr<media::mojom::AudioOutputStreamProvider>,
                     base::UniquePtrComparator>;

  // mojom::RendererAudioOutputStreamFactory implementation.
  void RequestDeviceAuthorization(
      media::mojom::AudioOutputStreamProviderRequest stream_provider,
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

  void RemoveStream(media::mojom::AudioOutputStreamProvider* stream_provider);

  const int render_frame_id_;
  RendererAudioOutputStreamFactoryContext* const context_;

  // The stream providers will contain the corresponding streams.
  OutputStreamProviderSet stream_providers_;

  // All streams require IDs. Use a counter to generate them.
  int next_stream_id_ = 0;

  base::WeakPtrFactory<OldRenderFrameAudioOutputStreamFactory>
      weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(OldRenderFrameAudioOutputStreamFactory);
};

// This class is a convenient bundle of factory and binding.
class CONTENT_EXPORT RenderFrameAudioOutputStreamFactoryHandle {
 public:
  static std::unique_ptr<RenderFrameAudioOutputStreamFactoryHandle,
                         BrowserThread::DeleteOnIOThread>
  CreateFactory(RendererAudioOutputStreamFactoryContext* context,
                int render_frame_id,
                mojom::RendererAudioOutputStreamFactoryRequest request);

  ~RenderFrameAudioOutputStreamFactoryHandle();

 private:
  RenderFrameAudioOutputStreamFactoryHandle(
      RendererAudioOutputStreamFactoryContext* context,
      int render_frame_id);

  void Init(mojom::RendererAudioOutputStreamFactoryRequest request);

  OldRenderFrameAudioOutputStreamFactory impl_;
  mojo::Binding<mojom::RendererAudioOutputStreamFactory> binding_;

  DISALLOW_COPY_AND_ASSIGN(RenderFrameAudioOutputStreamFactoryHandle);
};

using UniqueAudioOutputStreamFactoryPtr =
    std::unique_ptr<RenderFrameAudioOutputStreamFactoryHandle,
                    BrowserThread::DeleteOnIOThread>;

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_OLD_RENDER_FRAME_AUDIO_OUTPUT_STREAM_FACTORY_H_
