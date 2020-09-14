// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/audio_stream_broker.h"

#include <utility>

#include "content/browser/media/audio_input_stream_broker.h"
#include "content/browser/media/audio_loopback_stream_broker.h"
#include "content/browser/media/audio_output_stream_broker.h"

namespace content {

namespace {

class AudioStreamBrokerFactoryImpl final : public AudioStreamBrokerFactory {
 public:
  AudioStreamBrokerFactoryImpl() = default;
  ~AudioStreamBrokerFactoryImpl() final = default;

  std::unique_ptr<AudioStreamBroker> CreateAudioInputStreamBroker(
      int render_process_id,
      int render_frame_id,
      const std::string& device_id,
      const media::AudioParameters& params,
      uint32_t shared_memory_count,
      bool enable_agc,
      AudioStreamBroker::DeleterCallback deleter,
      mojom::RendererAudioInputStreamFactoryClientPtr renderer_factory_client)
      final {
    return std::make_unique<AudioInputStreamBroker>(
        render_process_id, render_frame_id, device_id, params,
        shared_memory_count, enable_agc, std::move(deleter),
        std::move(renderer_factory_client));
  }

  std::unique_ptr<AudioStreamBroker> CreateAudioLoopbackStreamBroker(
      int render_process_id,
      int render_frame_id,
      std::unique_ptr<LoopbackSource> source,
      const media::AudioParameters& params,
      uint32_t shared_memory_count,
      bool mute_source,
      AudioStreamBroker::DeleterCallback deleter,
      mojom::RendererAudioInputStreamFactoryClientPtr renderer_factory_client)
      final {
    return std::make_unique<AudioLoopbackStreamBroker>(
        render_process_id, render_frame_id, std::move(source), params,
        shared_memory_count, mute_source, std::move(deleter),
        std::move(renderer_factory_client));
  }

  std::unique_ptr<AudioStreamBroker> CreateAudioOutputStreamBroker(
      int render_process_id,
      int render_frame_id,
      int stream_id,
      const std::string& output_device_id,
      const media::AudioParameters& params,
      const base::UnguessableToken& group_id,
      AudioStreamBroker::DeleterCallback deleter,
      media::mojom::AudioOutputStreamProviderClientPtr client) final {
    return std::make_unique<AudioOutputStreamBroker>(
        render_process_id, render_frame_id, stream_id, output_device_id, params,
        group_id, std::move(deleter), std::move(client));
  }
};

}  // namespace

AudioStreamBroker::AudioStreamBroker(int render_process_id, int render_frame_id)
    : render_process_id_(render_process_id),
      render_frame_id_(render_frame_id) {}
AudioStreamBroker::~AudioStreamBroker() {}

AudioStreamBrokerFactory::AudioStreamBrokerFactory() {}
AudioStreamBrokerFactory::~AudioStreamBrokerFactory() {}

// static
std::unique_ptr<AudioStreamBrokerFactory>
AudioStreamBrokerFactory::CreateImpl() {
  return std::make_unique<AudioStreamBrokerFactoryImpl>();
}

}  // namespace content
