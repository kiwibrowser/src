// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BASE_BITSTREAM_AUDIO_CODECS_H_
#define CHROMECAST_BASE_BITSTREAM_AUDIO_CODECS_H_

#include <string>

namespace chromecast {

constexpr int kBitstreamAudioCodecNone = 0b000000;
constexpr int kBitstreamAudioCodecAc3 = 0b000001;
constexpr int kBitstreamAudioCodecDts = 0b000010;
constexpr int kBitstreamAudioCodecDtsHd = 0b000100;
constexpr int kBitstreamAudioCodecEac3 = 0b001000;
constexpr int kBitstreamAudioCodecPcmSurround = 0b010000;
constexpr int kBitstreamAudioCodecMpegHAudio = 0b100000;
constexpr int kBitstreamAudioCodecAll = 0b111111;

std::string BitstreamAudioCodecsToString(int codecs);

}  // namespace chromecast

#endif  // CHROMECAST_BASE_BITSTREAM_AUDIO_CODECS_H_
