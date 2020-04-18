// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/audio_output_stream_broker.h"

#include <utility>

#include "base/metrics/histogram_macros.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/media/media_internals.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/media_observer.h"
#include "content/public/common/content_client.h"
#include "media/audio/audio_logging.h"

namespace content {

AudioOutputStreamBroker::AudioOutputStreamBroker(
    int render_process_id,
    int render_frame_id,
    int stream_id,
    const std::string& output_device_id,
    const media::AudioParameters& params,
    const base::UnguessableToken& group_id,
    DeleterCallback deleter,
    media::mojom::AudioOutputStreamProviderClientPtr client)
    : AudioStreamBroker(render_process_id, render_frame_id),
      output_device_id_(output_device_id),
      params_(params),
      group_id_(group_id),
      deleter_(std::move(deleter)),
      client_(std::move(client)),
      observer_(render_process_id, render_frame_id, stream_id),
      observer_binding_(&observer_),
      weak_ptr_factory_(this) {
  DCHECK(client_);
  DCHECK(deleter_);
  DCHECK(group_id_);
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0("audio", "AudioOutputStreamBroker", this);

  MediaObserver* media_observer =
      GetContentClient()->browser()->GetMediaObserver();

  // May be null in unit tests.
  if (media_observer)
    media_observer->OnCreatingAudioStream(render_process_id, render_frame_id);

  // Unretained is safe because |this| owns |client_|
  client_.set_connection_error_handler(base::BindOnce(
      &AudioOutputStreamBroker::Cleanup, base::Unretained(this)));
}

AudioOutputStreamBroker::~AudioOutputStreamBroker() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);

  if (awaiting_created_) {
    TRACE_EVENT_NESTABLE_ASYNC_END1("audio", "CreateStream", this, "success",
                                    "failed or cancelled");
  }
  TRACE_EVENT_NESTABLE_ASYNC_END1("audio", "AudioOutputStreamBroker", this,
                                  "disconnect reason",
                                  static_cast<uint32_t>(disconnect_reason_));

  UMA_HISTOGRAM_ENUMERATION("Media.Audio.Render.StreamBrokerDisconnectReason",
                            disconnect_reason_);
}

void AudioOutputStreamBroker::CreateStream(
    audio::mojom::StreamFactory* factory) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  DCHECK(!observer_binding_.is_bound());
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN1("audio", "CreateStream", this, "device id",
                                    output_device_id_);
  awaiting_created_ = true;

  // Set up observer ptr. Unretained is safe because |this| owns
  // |observer_binding_|.
  media::mojom::AudioOutputStreamObserverAssociatedPtrInfo ptr_info;
  observer_binding_.Bind(mojo::MakeRequest(&ptr_info));
  observer_binding_.set_connection_error_with_reason_handler(base::BindOnce(
      &AudioOutputStreamBroker::ObserverBindingLost, base::Unretained(this)));

  media::mojom::AudioOutputStreamPtr stream;
  media::mojom::AudioOutputStreamRequest stream_request =
      mojo::MakeRequest(&stream);

  // Note that the component id for AudioLog is used to differentiate between
  // several users of the same audio log. Since this audio log is for a single
  // stream, the component id used doesn't matter.
  constexpr int log_component_id = 0;
  factory->CreateOutputStream(
      std::move(stream_request), std::move(ptr_info),
      MediaInternals::GetInstance()->CreateMojoAudioLog(
          media::AudioLogFactory::AudioComponent::AUDIO_OUTPUT_CONTROLLER,
          log_component_id, render_process_id(), render_frame_id()),
      output_device_id_, params_, group_id_,
      base::BindOnce(&AudioOutputStreamBroker::StreamCreated,
                     weak_ptr_factory_.GetWeakPtr(), std::move(stream)));
}

void AudioOutputStreamBroker::StreamCreated(
    media::mojom::AudioOutputStreamPtr stream,
    media::mojom::AudioDataPipePtr data_pipe) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  TRACE_EVENT_NESTABLE_ASYNC_END1("audio", "CreateStream", this, "success",
                                  !!data_pipe);
  awaiting_created_ = false;
  if (!data_pipe) {
    // Stream creation failed. Signal error.
    client_.ResetWithReason(
        static_cast<uint32_t>(media::mojom::AudioOutputStreamObserver::
                                  DisconnectReason::kPlatformError),
        std::string());
    disconnect_reason_ = media::mojom::AudioOutputStreamObserver::
        DisconnectReason::kStreamCreationFailed;
    Cleanup();
    return;
  }

  client_->Created(std::move(stream), std::move(data_pipe));
}

void AudioOutputStreamBroker::ObserverBindingLost(
    uint32_t reason,
    const std::string& description) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);

  TRACE_EVENT_NESTABLE_ASYNC_INSTANT1("audio", "ObserverBindingLost", this,
                                      "reset reason", reason);
  const uint32_t maxValidReason = static_cast<uint32_t>(
      media::mojom::AudioOutputStreamObserver::DisconnectReason::kMaxValue);
  if (reason > maxValidReason) {
    NOTREACHED() << "Invalid reason: " << reason;
  } else if (disconnect_reason_ == media::mojom::AudioOutputStreamObserver::
                                       DisconnectReason::kDocumentDestroyed) {
    disconnect_reason_ =
        static_cast<media::mojom::AudioOutputStreamObserver::DisconnectReason>(
            reason);
  }

  // TODO(https://crbug.com/787806): Don't propagate errors if we can retry
  // instead.
  client_.ResetWithReason(
      static_cast<uint32_t>(media::mojom::AudioOutputStreamObserver::
                                DisconnectReason::kPlatformError),
      std::string());

  Cleanup();
}

void AudioOutputStreamBroker::Cleanup() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  std::move(deleter_).Run(this);
}

}  // namespace content
