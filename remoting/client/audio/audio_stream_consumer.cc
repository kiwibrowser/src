// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/audio/audio_stream_consumer.h"

namespace remoting {

void AudioStreamConsumer::ProcessAudioPacket(
    std::unique_ptr<AudioPacket> packet,
    const base::Closure& done) {
  AddAudioPacket(std::move(packet));
  if (done) {
    done.Run();
  }
}

}  // namespace remoting
