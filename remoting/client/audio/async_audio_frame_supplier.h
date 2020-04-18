// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_AUDIO_ASYNC_AUDIO_FRAME_SUPPLIER_H_
#define REMOTING_CLIENT_AUDIO_ASYNC_AUDIO_FRAME_SUPPLIER_H_

#include <cstdint>

#include "base/callback.h"
#include "remoting/client/audio/audio_frame_supplier.h"

namespace remoting {

// This interface extends the AudioFrameSupplier interface adding async support
// for audio frame requests. This allows the audio frame supplier to wait until
// a full frame is buffered before returning the audio frame to the caller.
// Audio Pipeline Context:
// Stream -> Decode -> Stream Consumer -> Buffer -> [Frame Supplier] -> Play
class AsyncAudioFrameSupplier : public AudioFrameSupplier {
 public:
  // |buffer| is the destination of the audio frame data, it should be at least
  // |buffer_size|. |done| will be called when |buffer| has been filled.
  virtual void AsyncGetAudioFrame(uint32_t buffer_size,
                                  void* buffer,
                                  const base::Closure& done) = 0;
};

}  // namespace remoting

#endif  // REMOTING_CLIENT_AUDIO_ASYNC_AUDIO_FRAME_SUPPLIER_H_
