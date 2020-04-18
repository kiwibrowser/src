// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_PUBLIC_CPP_INPUT_IPC_H_
#define SERVICES_AUDIO_PUBLIC_CPP_INPUT_IPC_H_

#include <string>

#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/time/time.h"
#include "media/audio/audio_input_ipc.h"
#include "media/mojo/interfaces/audio_input_stream.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/audio/public/mojom/stream_factory.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace audio {

// InputIPC is a client-side class for handling creation,
// initialization and control of an input stream. May only be used on a single
// thread.
class InputIPC : public media::AudioInputIPC,
                 public media::mojom::AudioInputStreamClient {
 public:
  explicit InputIPC(std::unique_ptr<service_manager::Connector> connector,
                    const std::string& device_id,
                    media::mojom::AudioLogPtr log);
  ~InputIPC() override;

  // AudioInputIPC implementation
  void CreateStream(media::AudioInputIPCDelegate* delegate,
                    const media::AudioParameters& params,
                    bool automatic_gain_control,
                    uint32_t total_segments) override;
  void RecordStream() override;
  void SetVolume(double volume) override;
  void SetOutputDeviceForAec(const std::string& output_device_id) override;
  void CloseStream() override;

 private:
  // AudioInputStreamClient implementation.
  void OnError() override;
  void OnMutedStateChanged(bool is_muted) override;

  void StreamCreated(media::mojom::AudioDataPipePtr data_pipe,
                     bool is_muted,
                     const base::Optional<base::UnguessableToken>& stream_id);

  SEQUENCE_CHECKER(sequence_checker_);

  media::mojom::AudioInputStreamPtr stream_;
  mojo::Binding<AudioInputStreamClient> stream_client_binding_;
  media::AudioInputIPCDelegate* delegate_ = nullptr;

  const std::string& device_id_;
  base::Optional<base::UnguessableToken> stream_id_;

  // stream_factory_info_ is bound in the constructor, and later used to
  // bind stream_factory_. This is done because the constructor may be called
  // from a different thread than the other functions.
  audio::mojom::StreamFactoryPtr stream_factory_;
  audio::mojom::StreamFactoryPtrInfo stream_factory_info_;

  media::mojom::AudioLogPtr log_;

  base::WeakPtrFactory<InputIPC> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(InputIPC);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_PUBLIC_CPP_INPUT_IPC_H_
