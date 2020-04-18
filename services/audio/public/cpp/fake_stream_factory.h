// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_PUBLIC_CPP_FAKE_STREAM_FACTORY_H_
#define SERVICES_AUDIO_PUBLIC_CPP_FAKE_STREAM_FACTORY_H_

#include <string>

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/audio/public/mojom/stream_factory.mojom.h"

namespace audio {

class FakeStreamFactory : public mojom::StreamFactory {
 public:
  FakeStreamFactory();
  ~FakeStreamFactory() override;

  mojom::StreamFactoryPtr MakePtr() {
    mojom::StreamFactoryPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  void CloseBinding() { binding_.Close(); }

  void CreateInputStream(media::mojom::AudioInputStreamRequest stream_request,
                         media::mojom::AudioInputStreamClientPtr client,
                         media::mojom::AudioInputStreamObserverPtr observer,
                         media::mojom::AudioLogPtr log,
                         const std::string& device_id,
                         const media::AudioParameters& params,
                         uint32_t shared_memory_count,
                         bool enable_agc,
                         mojo::ScopedSharedBufferHandle key_press_count_buffer,
                         CreateInputStreamCallback created_callback) override {}

  void AssociateInputAndOutputForAec(
      const base::UnguessableToken& input_stream_id,
      const std::string& output_device_id) override {}

  void CreateOutputStream(
      media::mojom::AudioOutputStreamRequest stream_request,
      media::mojom::AudioOutputStreamObserverAssociatedPtrInfo observer_info,
      media::mojom::AudioLogPtr log,
      const std::string& output_device_id,
      const media::AudioParameters& params,
      const base::UnguessableToken& group_id,
      CreateOutputStreamCallback created_callback) override {}
  void BindMuter(mojom::LocalMuterAssociatedRequest request,
                 const base::UnguessableToken& group_id) override {}
  void CreateLoopbackStream(
      media::mojom::AudioInputStreamRequest stream_request,
      media::mojom::AudioInputStreamClientPtr client,
      media::mojom::AudioInputStreamObserverPtr observer,
      const media::AudioParameters& params,
      uint32_t shared_memory_count,
      const base::UnguessableToken& group_id,
      CreateLoopbackStreamCallback created_callback) override {}

  mojo::Binding<mojom::StreamFactory> binding_;
};

}  // namespace audio

#endif  // SERVICES_AUDIO_PUBLIC_CPP_FAKE_STREAM_FACTORY_H_
