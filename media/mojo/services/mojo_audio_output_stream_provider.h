// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_AUDIO_OUTPUT_STREAM_PROVIDER_H_
#define MEDIA_MOJO_SERVICES_MOJO_AUDIO_OUTPUT_STREAM_PROVIDER_H_

#include <memory>
#include <string>

#include "base/sequence_checker.h"
#include "media/audio/audio_output_delegate.h"
#include "media/mojo/interfaces/audio_output_stream.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "media/mojo/services/mojo_audio_output_stream.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {

// Provides a single AudioOutput, given the audio parameters to use.
class MEDIA_MOJO_EXPORT MojoAudioOutputStreamProvider
    : public mojom::AudioOutputStreamProvider {
 public:
  using CreateDelegateCallback =
      base::OnceCallback<std::unique_ptr<AudioOutputDelegate>(
          const AudioParameters& params,
          mojom::AudioOutputStreamObserverPtr observer,
          AudioOutputDelegate::EventHandler*)>;
  using DeleterCallback = base::OnceCallback<void(AudioOutputStreamProvider*)>;

  // |create_delegate_callback| is used to obtain an AudioOutputDelegate for the
  // AudioOutput when it's initialized and |deleter_callback| is called when
  // this class should be removed (stream ended/error). |deleter_callback| is
  // required to destroy |this| synchronously.
  MojoAudioOutputStreamProvider(
      mojom::AudioOutputStreamProviderRequest request,
      CreateDelegateCallback create_delegate_callback,
      DeleterCallback deleter_callback,
      std::unique_ptr<mojom::AudioOutputStreamObserver> observer);

  ~MojoAudioOutputStreamProvider() override;

 private:
  // mojom::AudioOutputStreamProvider implementation.
  void Acquire(
      const AudioParameters& params,
      mojom::AudioOutputStreamProviderClientPtr provider_client) override;

  // Called when |audio_output_| had an error.
  void CleanUp(bool had_error);

  // Closes mojo connections, reports a bad message, and self-destructs.
  void BadMessage(const std::string& error);

  SEQUENCE_CHECKER(sequence_checker_);

  mojo::Binding<AudioOutputStreamProvider> binding_;
  CreateDelegateCallback create_delegate_callback_;
  DeleterCallback deleter_callback_;
  std::unique_ptr<mojom::AudioOutputStreamObserver> observer_;
  mojo::Binding<mojom::AudioOutputStreamObserver> observer_binding_;
  base::Optional<MojoAudioOutputStream> audio_output_;
  mojom::AudioOutputStreamProviderClientPtr provider_client_;

  DISALLOW_COPY_AND_ASSIGN(MojoAudioOutputStreamProvider);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_AUDIO_OUTPUT_STREAM_PROVIDER_H_
