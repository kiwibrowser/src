// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/renderer_audio_output_stream_factory_context_impl.h"

#include <utility>

#include "content/browser/media/media_internals.h"
#include "content/browser/renderer_host/media/audio_output_delegate_impl.h"
#include "content/browser/renderer_host/media/media_stream_manager.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_features.h"
#include "media/audio/audio_system.h"
#include "media/mojo/interfaces/audio_logging.mojom.h"

namespace content {

RendererAudioOutputStreamFactoryContextImpl::
    RendererAudioOutputStreamFactoryContextImpl(
        int render_process_id,
        media::AudioSystem* audio_system,
        media::AudioManager* audio_manager,
        MediaStreamManager* media_stream_manager)
    : audio_system_(audio_system),
      audio_manager_(audio_manager),
      media_stream_manager_(media_stream_manager),
      authorization_handler_(audio_system_,
                             media_stream_manager_,
                             render_process_id),
      render_process_id_(render_process_id) {}

RendererAudioOutputStreamFactoryContextImpl::
    ~RendererAudioOutputStreamFactoryContextImpl() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

int RendererAudioOutputStreamFactoryContextImpl::GetRenderProcessId() const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return render_process_id_;
}

void RendererAudioOutputStreamFactoryContextImpl::RequestDeviceAuthorization(
    int render_frame_id,
    int session_id,
    const std::string& device_id,
    AuthorizationCompletedCallback cb) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  authorization_handler_.RequestDeviceAuthorization(render_frame_id, session_id,
                                                    device_id, std::move(cb));
}

std::unique_ptr<media::AudioOutputDelegate>
RendererAudioOutputStreamFactoryContextImpl::CreateDelegate(
    const std::string& unique_device_id,
    int render_frame_id,
    int stream_id,
    const media::AudioParameters& params,
    media::mojom::AudioOutputStreamObserverPtr stream_observer,
    media::AudioOutputDelegate::EventHandler* handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  MediaObserver* const media_observer =
      GetContentClient()->browser()->GetMediaObserver();

  media::mojom::AudioLogPtr audio_log_ptr =
      MediaInternals::GetInstance()->CreateMojoAudioLog(
          media::AudioLogFactory::AUDIO_OUTPUT_CONTROLLER, stream_id,
          render_process_id_, render_frame_id);
  audio_log_ptr->OnCreated(params, unique_device_id);

  return AudioOutputDelegateImpl::Create(
      handler, audio_manager_, std::move(audio_log_ptr), media_observer,
      stream_id, render_frame_id, render_process_id_, params,
      std::move(stream_observer), unique_device_id);
}

}  // namespace content
