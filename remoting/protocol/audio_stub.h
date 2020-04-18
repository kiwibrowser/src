// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_AUDIO_STUB_H_
#define REMOTING_PROTOCOL_AUDIO_STUB_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"

namespace remoting {

class AudioPacket;

namespace protocol {

class AudioStub {
 public:
  virtual ~AudioStub() { }

  virtual void ProcessAudioPacket(std::unique_ptr<AudioPacket> audio_packet,
                                  const base::Closure& done) = 0;

 protected:
  AudioStub() { }

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioStub);
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_AUDIO_STUB_H_
