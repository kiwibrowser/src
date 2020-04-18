/*
 *  Copyright (c) 2019 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef API_VIDEO_CODECS_VP8_FRAME_CONFIG_H_
#define API_VIDEO_CODECS_VP8_FRAME_CONFIG_H_

#include <stdint.h>

namespace webrtc {

struct Vp8FrameConfig {
  enum BufferFlags : int {
    kNone = 0,
    kReference = 1,
    kUpdate = 2,
    kReferenceAndUpdate = kReference | kUpdate,
  };

  enum FreezeEntropy { kFreezeEntropy };

  // Defined bit-maskable reference to the three buffers available in VP8.
  enum class Vp8BufferReference : uint8_t {
    kNone = 0,
    kLast = 1,
    kGolden = 2,
    kAltref = 4
  };

  Vp8FrameConfig();

  Vp8FrameConfig(BufferFlags last, BufferFlags golden, BufferFlags arf);
  Vp8FrameConfig(BufferFlags last,
                 BufferFlags golden,
                 BufferFlags arf,
                 FreezeEntropy);

  enum class Buffer : int { kLast = 0, kGolden = 1, kArf = 2, kCount };

  bool References(Buffer buffer) const;

  bool Updates(Buffer buffer) const;

  bool drop_frame;
  BufferFlags last_buffer_flags;
  BufferFlags golden_buffer_flags;
  BufferFlags arf_buffer_flags;

  // The encoder layer ID is used to utilize the correct bitrate allocator
  // inside the encoder. It does not control references nor determine which
  // "actual" temporal layer this is. The packetizer temporal index determines
  // which layer the encoded frame should be packetized into.
  // Normally these are the same, but current temporal-layer strategies for
  // screenshare use one bitrate allocator for all layers, but attempt to
  // packetize / utilize references to split a stream into multiple layers,
  // with different quantizer settings, to hit target bitrate.
  // TODO(sprang): Screenshare layers are being reconsidered at the time of
  // writing, we might be able to remove this distinction, and have a temporal
  // layer imply both (the normal case).
  int encoder_layer_id;
  // TODO(eladalon/sprang): Move out of this class.
  int packetizer_temporal_idx;

  // TODO(eladalon/sprang): Move out of this class.
  bool layer_sync;

  bool freeze_entropy;

  // Indicates in which order the encoder should search the reference buffers
  // when doing motion prediction. Set to kNone to use unspecified order. Any
  // buffer indicated here must not have the corresponding no_ref bit set.
  // If all three buffers can be reference, the one not listed here should be
  // searched last.
  Vp8BufferReference first_reference;
  Vp8BufferReference second_reference;

 private:
  Vp8FrameConfig(BufferFlags last,
                 BufferFlags golden,
                 BufferFlags arf,
                 bool freeze_entropy);
};

}  // namespace webrtc

#endif  // API_VIDEO_CODECS_VP8_FRAME_CONFIG_H_
