// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_IOS_AUDIO_AUDIO_PLAYER_BUFFER_H_
#define REMOTING_IOS_AUDIO_AUDIO_PLAYER_BUFFER_H_

#include <cstdint>
#include <list>
#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/threading/thread_task_runner_handle.h"
#include "remoting/client/audio/async_audio_frame_supplier.h"
#include "remoting/client/audio/audio_stream_consumer.h"
#include "remoting/proto/audio.pb.h"

namespace remoting {

// This class consumes the decoded audio stream, buffers it into frames, and
// supplies the player with constant audio frames as they are ready.
// Audio Pipeline Context:
// Stream -> Decode -> [Stream Consumer -> Buffer -> Frame Provider] -> Play
class AudioPlayerBuffer : public AudioStreamConsumer,
                          public AsyncAudioFrameSupplier {
 public:
  // The number of channels in the audio stream (only supporting stereo audio
  // for now).
  static const int kChannels = 2;
  static const int kSampleSizeBytes = 2;

  AudioPlayerBuffer();
  ~AudioPlayerBuffer() override;

  void Stop();

  // Audio Stream Consumer
  void AddAudioPacket(std::unique_ptr<AudioPacket> packet) override;
  base::WeakPtr<AudioStreamConsumer> AudioStreamConsumerAsWeakPtr() override;

  // Async Audio Frame Supplier
  void AsyncGetAudioFrame(uint32_t buffer_size,
                          void* buffer,
                          const base::Closure& callback) override;

  // Audio Frame Supplier
  uint32_t GetAudioFrame(uint32_t buffer_size, void* buffer) override;
  AudioPacket::SamplingRate buffered_sampling_rate() const override;
  AudioPacket::Channels buffered_channels() const override;
  AudioPacket::BytesPerSample buffered_byes_per_sample() const override;
  uint32_t bytes_per_frame() const override;

  // Return the recommended number of samples to include in a frame.
  uint32_t samples_per_frame() const;

 private:
  friend class AudioPlayerBufferTest;
  struct AudioFrameRequest;

  void ResetQueue();
  void ProcessFrameRequestQueue();

  std::list<std::unique_ptr<AudioPacket>> queued_packets_;
  int queued_bytes_;

  // The number of bytes from |queued_packets_| that have been consumed.
  size_t bytes_consumed_;

  std::list<std::unique_ptr<AudioFrameRequest>> queued_requests_;

  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<AudioPlayerBuffer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AudioPlayerBuffer);
};

}  // namespace remoting

#endif  // REMOTING_IOS_AUDIO_AUDIO_PLAYER_BUFFER_H_
