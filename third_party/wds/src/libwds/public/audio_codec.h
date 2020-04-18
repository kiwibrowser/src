/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2015 Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#ifndef LIBWDS_PUBLIC_AUDIO_CODEC_H_
#define LIBWDS_PUBLIC_AUDIO_CODEC_H_

#include <bitset>

namespace wds {

enum AudioFormats {
  LPCM,
  AAC,
  AC3
};

enum LPCMModes {
  LPCM_44_1K_16B_2CH,
  LPCM_48K_16B_2CH
};

enum AACModes {
  AAC_48K_16B_2CH,
  AAC_48K_16B_4CH,
  AAC_48K_16B_6CH,
  AAC_48K_16B_8CH
};

enum AC3Modes {
  AC3_48K_16B_2CH,
  AC3_48K_16B_4CH,
  AC3_48K_16B_6CH
};

using AudioModes = std::bitset<32>;

/**
 * @brief The AudioCodec struct
 *
 * Repersents a <audio-format, modes, latency> tuple used in 'wfd-audio-codecs'.
 */
struct AudioCodec {
  AudioCodec()
  : format(LPCM), modes(AudioModes().set(LPCM_48K_16B_2CH)), latency(0) {}
  AudioCodec(AudioFormats format, const AudioModes& modes, unsigned latency)
  : format(format), modes(modes), latency(latency) {}

  AudioFormats format;
  AudioModes modes;
  unsigned latency;
};

}  // namespace wds

#endif  // LIBWDS_PUBLIC_AUDIO_CODEC_H_
