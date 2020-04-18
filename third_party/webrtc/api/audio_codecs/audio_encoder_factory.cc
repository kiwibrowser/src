/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/audio_codecs/audio_encoder_factory.h"

namespace webrtc {

std::unique_ptr<AudioEncoder> AudioEncoderFactory::MakeAudioEncoder(
    int payload_type,
    const SdpAudioFormat& format,
    rtc::Optional<AudioCodecPairId> codec_pair_id) {
  return MakeAudioEncoder(payload_type, format);
}

std::unique_ptr<AudioEncoder> AudioEncoderFactory::MakeAudioEncoder(
    int payload_type,
    const SdpAudioFormat& format) {
  return MakeAudioEncoder(payload_type, format, rtc::nullopt);
}

}  // namespace webrtc
