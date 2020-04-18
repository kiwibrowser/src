// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_AUDIO_OUTPUT_STREAM_BROKER_H_
#define CONTENT_BROWSER_MEDIA_AUDIO_OUTPUT_STREAM_BROKER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/unguessable_token.h"
#include "content/browser/media/audio_stream_broker.h"
#include "content/browser/renderer_host/media/audio_output_stream_observer_impl.h"
#include "content/common/content_export.h"
#include "media/base/audio_parameters.h"
#include "media/mojo/interfaces/audio_output_stream.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "services/audio/public/mojom/stream_factory.mojom.h"

namespace content {

// AudioOutputStreamBroker is used to broker a connection between a client
// (typically renderer) and the audio service. It also sets up all objects
// used for monitoring the stream.
class CONTENT_EXPORT AudioOutputStreamBroker final : public AudioStreamBroker {
 public:
  AudioOutputStreamBroker(
      int render_process_id,
      int render_frame_id,
      int stream_id,
      const std::string& output_device_id,
      const media::AudioParameters& params,
      const base::UnguessableToken& group_id,
      DeleterCallback deleter,
      media::mojom::AudioOutputStreamProviderClientPtr client);

  ~AudioOutputStreamBroker() final;

  // Creates the stream.
  void CreateStream(audio::mojom::StreamFactory* factory) final;

 private:
  void StreamCreated(media::mojom::AudioOutputStreamPtr stream,
                     media::mojom::AudioDataPipePtr data_pipe);
  void ObserverBindingLost(uint32_t reason, const std::string& description);
  void Cleanup();

  SEQUENCE_CHECKER(owning_sequence_);

  const std::string output_device_id_;
  const media::AudioParameters params_;
  const base::UnguessableToken group_id_;

  // Indicates that CreateStream has been called, but not StreamCreated.
  bool awaiting_created_ = false;

  DeleterCallback deleter_;

  media::mojom::AudioOutputStreamProviderClientPtr client_;

  AudioOutputStreamObserverImpl observer_;
  mojo::AssociatedBinding<media::mojom::AudioOutputStreamObserver>
      observer_binding_;

  media::mojom::AudioOutputStreamObserver::DisconnectReason disconnect_reason_ =
      media::mojom::AudioOutputStreamObserver::DisconnectReason::
          kDocumentDestroyed;

  base::WeakPtrFactory<AudioOutputStreamBroker> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(AudioOutputStreamBroker);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_AUDIO_OUTPUT_STREAM_BROKER_H_
