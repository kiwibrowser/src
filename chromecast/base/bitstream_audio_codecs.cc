// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/base/bitstream_audio_codecs.h"

#include <vector>

#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"

namespace chromecast {

namespace {

const char* BitstreamAudioCodecToString(int codec) {
  switch (codec) {
    case kBitstreamAudioCodecNone:
      return "None";
    case kBitstreamAudioCodecAc3:
      return "AC3";
    case kBitstreamAudioCodecDts:
      return "DTS";
    case kBitstreamAudioCodecDtsHd:
      return "DTS-HD";
    case kBitstreamAudioCodecEac3:
      return "EAC3";
    case kBitstreamAudioCodecPcmSurround:
      return "PCM";
    case kBitstreamAudioCodecMpegHAudio:
      return "MPEG-H Audio";
    default:
      return "";
  }
}

}  // namespace

std::string BitstreamAudioCodecsToString(int codecs) {
  std::string codec_string = BitstreamAudioCodecToString(codecs);
  if (!codec_string.empty()) {
    return codec_string;
  }
  std::vector<base::StringPiece> codec_strings;
  for (int codec :
       {kBitstreamAudioCodecAc3, kBitstreamAudioCodecDts,
        kBitstreamAudioCodecDtsHd, kBitstreamAudioCodecEac3,
        kBitstreamAudioCodecPcmSurround, kBitstreamAudioCodecMpegHAudio}) {
    if ((codec & codecs) != 0) {
      codec_strings.push_back(BitstreamAudioCodecToString(codec));
    }
  }
  return "[" + base::JoinString(codec_strings, ", ") + "]";
}

}  // namespace chromecast
