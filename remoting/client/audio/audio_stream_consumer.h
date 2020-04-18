// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_AUDIO_AUDIO_STREAM_CONSUMER_H_
#define REMOTING_CLIENT_AUDIO_AUDIO_STREAM_CONSUMER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "remoting/proto/audio.pb.h"
#include "remoting/protocol/audio_stub.h"

namespace remoting {

// This is the interface that consumes the audio stream from the decoder.
// The decoder is not guaranteed to produce a constant flow of audio, this
// interface sits between the audio decoder and the |AudioFrameSupplier|.
// Audio Pipeline Context:
// Stream -> Decode -> [Stream Consumer] -> Buffer -> Frame Supplier -> Play
class AudioStreamConsumer : public protocol::AudioStub {
 public:
  AudioStreamConsumer() = default;
  ~AudioStreamConsumer() override {}

  // Adds a audio packet from the audio stream to the audio consumer.
  // What happens to the packet is up to implmentation but typically it is
  // expected that it will be buffered and then given to the player when
  // requested.
  virtual void AddAudioPacket(std::unique_ptr<AudioPacket> packet) = 0;
  // Get a weak refrence to the audio consumer.
  virtual base::WeakPtr<AudioStreamConsumer> AudioStreamConsumerAsWeakPtr() = 0;

  // AudioStub implementation. Delegates to AddAudioPacket. Used for
  // integration with |protocol::AudioStub| API users but AddAudioPacket
  // is preferred.
  void ProcessAudioPacket(std::unique_ptr<AudioPacket> packet,
                          const base::Closure& done) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioStreamConsumer);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_AUDIO_AUDIO_STREAM_CONSUMER_H_
