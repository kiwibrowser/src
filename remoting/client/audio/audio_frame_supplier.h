// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_AUDIO_AUDIO_FRAME_SUPPLIER_H_
#define REMOTING_CLIENT_AUDIO_AUDIO_FRAME_SUPPLIER_H_

#include <cstdint>

#include "base/macros.h"
#include "remoting/proto/audio.pb.h"

namespace remoting {

// This class provides an interface to request audio frames of a given size
// and a way to ask about the currently buffered audio data.
// Audio Pipeline Context:
// Stream -> Decode -> Stream Consumer -> Buffer -> [Frame Supplier] -> Play
class AudioFrameSupplier {
 public:
  AudioFrameSupplier() = default;
  virtual ~AudioFrameSupplier() = default;
  virtual uint32_t GetAudioFrame(uint32_t buffer_size, void* buffer) = 0;

  // Methods to describe buffered data.
  virtual AudioPacket::SamplingRate buffered_sampling_rate() const = 0;
  virtual AudioPacket::Channels buffered_channels() const = 0;
  virtual AudioPacket::BytesPerSample buffered_byes_per_sample() const = 0;
  virtual uint32_t bytes_per_frame() const = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioFrameSupplier);
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_AUDIO_AUDIO_FRAME_SUPPLIER_H_
