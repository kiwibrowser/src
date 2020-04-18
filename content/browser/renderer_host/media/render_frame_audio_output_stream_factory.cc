// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/render_frame_audio_output_stream_factory.h"

#include <utility>

#include "base/trace_event/trace_event.h"
#include "content/browser/media/forwarding_audio_stream_factory.h"
#include "content/browser/renderer_host/media/audio_output_authorization_handler.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "media/base/bind_to_current_loop.h"

namespace content {

// This class implements media::mojom::AudioOutputStreamProvider for a single
// streams and cleans itself up (using the |owner| pointer) when done.
class RenderFrameAudioOutputStreamFactory::ProviderImpl final
    : public media::mojom::AudioOutputStreamProvider {
 public:
  ProviderImpl(media::mojom::AudioOutputStreamProviderRequest request,
               RenderFrameAudioOutputStreamFactory* owner,
               const std::string& device_id)
      : owner_(owner),
        device_id_(device_id),
        binding_(this, std::move(request)) {
    DCHECK(owner_);
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    // Unretained is safe since |this| owns |binding_|.
    binding_.set_connection_error_handler(
        base::BindOnce(&ProviderImpl::Done, base::Unretained(this)));
  }

  ~ProviderImpl() final { DCHECK_CURRENTLY_ON(BrowserThread::UI); }

  void Acquire(
      const media::AudioParameters& params,
      media::mojom::AudioOutputStreamProviderClientPtr provider_client) final {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    TRACE_EVENT1("audio",
                 "RenderFrameAudioOutputStreamFactory::ProviderImpl::Acquire",
                 "raw device id", device_id_);

    RenderFrameHost* frame = owner_->frame_;
    ForwardingAudioStreamFactory* factory =
        ForwardingAudioStreamFactory::ForFrame(frame);
    if (factory) {
      // It's possible that |frame| has already been destroyed, in which case we
      // don't need to create a stream. In this case, the renderer will get a
      // connection error since |provider_client| is dropped.
      factory->CreateOutputStream(frame, device_id_, params,
                                  std::move(provider_client));
    }

    // Since the stream creation has been propagated, |this| is no longer
    // needed.
    Done();
  }

  void Done() { owner_->DeleteProvider(this); }

 private:
  RenderFrameAudioOutputStreamFactory* const owner_;
  const std::string device_id_;

  mojo::Binding<media::mojom::AudioOutputStreamProvider> binding_;

  DISALLOW_COPY_AND_ASSIGN(ProviderImpl);
};

RenderFrameAudioOutputStreamFactory::RenderFrameAudioOutputStreamFactory(
    RenderFrameHost* frame,
    media::AudioSystem* audio_system,
    MediaStreamManager* media_stream_manager,
    mojom::RendererAudioOutputStreamFactoryRequest request)
    : binding_(this, std::move(request)),
      frame_(frame),
      authorization_handler_(
          new AudioOutputAuthorizationHandler(audio_system,
                                              media_stream_manager,
                                              frame_->GetProcess()->GetID())),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

RenderFrameAudioOutputStreamFactory::~RenderFrameAudioOutputStreamFactory() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

void RenderFrameAudioOutputStreamFactory::RequestDeviceAuthorization(
    media::mojom::AudioOutputStreamProviderRequest provider_request,
    int32_t session_id,
    const std::string& device_id,
    RequestDeviceAuthorizationCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  TRACE_EVENT2(
      "audio",
      "RenderFrameAudioOutputStreamFactory::RequestDeviceAuthorization",
      "device id", device_id, "session_id", session_id);

  const base::TimeTicks auth_start_time = base::TimeTicks::Now();
  // TODO(https://crbug.com/837625): This thread hopping is suboptimal since
  // AudioOutputAuthorizationHandler was made to be used on the IO thread.
  // Make AudioOutputAuthorizationHandler work on the UI thread instead.
  AudioOutputAuthorizationHandler::AuthorizationCompletedCallback
      completed_callback = media::BindToCurrentLoop(base::BindOnce(
          &RenderFrameAudioOutputStreamFactory::AuthorizationCompleted,
          weak_ptr_factory_.GetWeakPtr(), auth_start_time,
          std::move(provider_request), std::move(callback)));

  // Unretained is safe since |authorization_handler_| is deleted on the IO
  // thread.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &AudioOutputAuthorizationHandler::RequestDeviceAuthorization,
          base::Unretained(authorization_handler_.get()),
          frame_->GetRoutingID(), session_id, device_id,
          std::move(completed_callback)));
}

void RenderFrameAudioOutputStreamFactory::AuthorizationCompleted(
    base::TimeTicks auth_start_time,
    media::mojom::AudioOutputStreamProviderRequest request,
    RequestDeviceAuthorizationCallback callback,
    media::OutputDeviceStatus status,
    const media::AudioParameters& params,
    const std::string& raw_device_id,
    const std::string& device_id_for_renderer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  TRACE_EVENT2("audio",
               "RenderFrameAudioOutputStreamFactory::AuthorizationCompleted",
               "raw device id", raw_device_id, "status", status);

  AudioOutputAuthorizationHandler::UMALogDeviceAuthorizationTime(
      auth_start_time);

  // If |status| is not OK, this call will be considered as an error signal by
  // the renderer.
  std::move(callback).Run(status, params, device_id_for_renderer);

  if (status == media::OUTPUT_DEVICE_STATUS_OK) {
    stream_providers_.insert(std::make_unique<ProviderImpl>(
        std::move(request), this, std::move(raw_device_id)));
  }
}

void RenderFrameAudioOutputStreamFactory::DeleteProvider(
    media::mojom::AudioOutputStreamProvider* stream_provider) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  size_t deleted = stream_providers_.erase(stream_provider);
  DCHECK_EQ(1u, deleted);
}

}  // namespace content
