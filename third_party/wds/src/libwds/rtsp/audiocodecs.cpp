/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2014 Intel Corporation.
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


#include "libwds/rtsp/audiocodecs.h"

#include <assert.h>

#include "libwds/rtsp/macros.h"

namespace wds {
namespace rtsp {

namespace {

std::string ToString(const wds::AudioCodec& codec) {
  MAKE_HEX_STRING_8(audio_modes_str,
      static_cast<unsigned int>(codec.modes.to_ulong()));
  MAKE_HEX_STRING_2(latency_str, codec.latency);
  const char* const name[] = {"LPCM", "AAC", "AC3"};
  std::string ret = name[codec.format]
    + std::string(SPACE) + audio_modes_str
    + std::string(SPACE) + latency_str;
  return ret;
}

}

AudioCodecs::AudioCodecs()
  : Property(AudioCodecsPropertyType, true) {
}

AudioCodecs::AudioCodecs(const std::vector<wds::AudioCodec>& audio_codecs)
  : Property(AudioCodecsPropertyType),
    audio_codecs_(audio_codecs) {
}

AudioCodecs::~AudioCodecs() {
}

std::string AudioCodecs::ToString() const {
  std::string ret = PropertyName::wfd_audio_codecs + std::string(SEMICOLON)
    + std::string(SPACE);

  if (audio_codecs_.empty())
    return ret + NONE;

  auto it = audio_codecs_.begin();
  auto end = audio_codecs_.end();
  while (it != end) {
    ret += wds::rtsp::ToString(*it);
    ++it;
    if (it != end)
      ret += ", ";
  }

  return ret;
}

}  // namespace rtsp
}  // namespace wds
