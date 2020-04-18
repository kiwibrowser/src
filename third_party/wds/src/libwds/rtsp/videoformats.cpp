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

#include "libwds/rtsp/videoformats.h"

#include <cassert>

#include "libwds/rtsp/macros.h"

namespace wds {
namespace rtsp {

namespace {
template <typename EnumType>
unsigned int EnumListToMask(const std::vector<EnumType>& from) {
  unsigned int result = 0;

  for (auto item : from) {
    result = result | (1 << item);
  }

  return result;
}
} //namespace

using wds::H264VideoFormat;
using wds::H264VideoCodec;

H264Codec::H264Codec(unsigned char profile, unsigned char level,
    unsigned int cea_support, unsigned int vesa_support,
    unsigned int hh_support, unsigned char latency,
    unsigned short min_slice_size, unsigned short slice_enc_params,
    unsigned char frame_rate_control_support,
    unsigned short max_hres, unsigned short max_vres)
  : profile(profile),
    level(level),
    cea_support(cea_support),
    vesa_support(vesa_support),
    hh_support(hh_support),
    latency(latency),
    min_slice_size(min_slice_size),
    slice_enc_params(slice_enc_params),
    frame_rate_control_support(frame_rate_control_support),
    max_hres(max_hres),
    max_vres(max_vres) {}

H264Codec::H264Codec(const H264VideoFormat& format)
  : profile(1 << format.profile),
    level(1 << format.level),
    cea_support((format.type == CEA) ? 1 << format.rate_resolution : 0),
    vesa_support((format.type == VESA) ? 1 << format.rate_resolution : 0),
    hh_support((format.type == HH) ? 1 << format.rate_resolution : 0),
    latency(0),
    min_slice_size(0),
    slice_enc_params(0),
    frame_rate_control_support(0),
    max_hres(0),
    max_vres(0) {

}

H264Codec::H264Codec(const H264VideoCodec& format)
  : profile(1 << format.profile),
    level(1 << format.level),
    cea_support(format.cea_rr.to_ulong()),
    vesa_support(format.vesa_rr.to_ulong()),
    hh_support(format.hh_rr.to_ulong()),
    latency(0),
    min_slice_size(0),
    slice_enc_params(0),
    frame_rate_control_support(0),
    max_hres(0),
    max_vres(0) {

}


std::string H264Codec::ToString() const {
  std::string ret;
  MAKE_HEX_STRING_2(profile_str, profile);
  MAKE_HEX_STRING_2(level_str, level);
  MAKE_HEX_STRING_8(cea_support_str, cea_support);
  MAKE_HEX_STRING_8(vesa_support_str, vesa_support);
  MAKE_HEX_STRING_8(hh_support_str, hh_support);
  MAKE_HEX_STRING_2(latency_str, latency);
  MAKE_HEX_STRING_4(min_slice_size_str, min_slice_size);
  MAKE_HEX_STRING_4(slice_enc_params_str, slice_enc_params);
  MAKE_HEX_STRING_2(frame_rate_control_support_str, frame_rate_control_support);

  ret = profile_str + std::string(SPACE)
      + level_str + std::string(SPACE)
      + cea_support_str + std::string(SPACE)
      + vesa_support_str + std::string(SPACE)
      + hh_support_str + std::string(SPACE)
      + latency_str + std::string(SPACE)
      + min_slice_size_str + std::string(SPACE)
      + slice_enc_params_str + std::string(SPACE)
      + frame_rate_control_support_str + std::string(SPACE);

  if (max_hres > 0) {
    MAKE_HEX_STRING_4(max_hres_str, max_hres);
    ret += max_hres_str;
  } else {
    ret += NONE;
  }
  ret += std::string(SPACE);

  if (max_vres > 0) {
    MAKE_HEX_STRING_4(max_vres_str, max_vres);
    ret += max_vres_str;
  } else {
    ret += NONE;
  }

  return ret;
}

namespace {

template <typename EnumType, typename ArgType>
EnumType MaskToEnum(ArgType from, EnumType biggest_value) {
  assert(from != 0);
  ArgType copy = from;
  unsigned result = 0;
  while ((copy & 1) == 0 && copy != 0) {
    copy = copy >> 1;
    ++result;
  }
  if (result > static_cast<unsigned>(biggest_value)) {
    assert(false);
    return biggest_value;
  }
  return static_cast<EnumType>(result);
}

template <typename EnumType, typename ArgType>
std::vector<EnumType> MaskToEnumList(ArgType from, EnumType biggest_value) {
  assert(from != 0);
  ArgType copy = from;
  unsigned enum_value = 0;
  std::vector<EnumType> result;

  while (copy != 0) {
    if ((copy & 1) != 0) {
      if (enum_value > static_cast<unsigned>(biggest_value))
        break;

      result.push_back(static_cast<EnumType>(enum_value));
    }
    copy = copy >> 1;
    ++enum_value;
  }

  return result;
}

inline H264Profile ToH264Profile(unsigned char profile) {
  return MaskToEnum<H264Profile>(profile, CHP);
}

inline H264Level ToH264Level(unsigned char level) {
  return MaskToEnum<H264Level>(level, k4_2);
}

}  // namespace

H264VideoCodec H264Codec::ToH264VideoCodec() const {
  H264VideoCodec result;
  result.profile = ToH264Profile(profile);
  result.level = ToH264Level(level);
  result.cea_rr = RateAndResolutionsBitmap(cea_support);
  result.vesa_rr = RateAndResolutionsBitmap(vesa_support);
  result.hh_rr = RateAndResolutionsBitmap(hh_support);
  return result;
}

VideoFormats::VideoFormats() : Property(VideoFormatsPropertyType, true) {
}

VideoFormats::VideoFormats(NativeVideoFormat format,
    bool preferred_display_mode,
    const std::vector<H264VideoFormat>& h264_formats)
  : Property(VideoFormatsPropertyType),
    preferred_display_mode_(preferred_display_mode ? 1 : 0) {
  native_ = (format.rate_resolution << 3) | format.type;
  for(auto h264_format : h264_formats)
    h264_codecs_.push_back(H264Codec(h264_format));
}

VideoFormats::VideoFormats(NativeVideoFormat format,
    bool preferred_display_mode,
    const std::vector<H264VideoCodec>& h264_formats)
  : Property(VideoFormatsPropertyType),
    preferred_display_mode_(preferred_display_mode ? 1 : 0) {
  native_ = (format.rate_resolution << 3) | format.type;
  for(auto h264_format : h264_formats)
    h264_codecs_.push_back(H264Codec(h264_format));
}

VideoFormats::VideoFormats(unsigned char native,
    unsigned char preferred_display_mode,
    const H264Codecs& h264_codecs)
  : Property(VideoFormatsPropertyType),
    native_(native),
    preferred_display_mode_(preferred_display_mode),
    h264_codecs_(h264_codecs) {
}

VideoFormats::~VideoFormats() {
}

namespace {

template <typename EnumType>
NativeVideoFormat GetFormatFromIndex(unsigned index, EnumType biggest_value) {
  if (index <= static_cast<unsigned>(biggest_value))
    return NativeVideoFormat(static_cast<EnumType>(index));
  assert(false);
  return NativeVideoFormat(biggest_value);
}

}

NativeVideoFormat VideoFormats::GetNativeFormat() const {
  unsigned index  = native_ >> 3;
  unsigned selection_bits = native_ & 7;
  switch (selection_bits) {
  case 0: // 0b000 CEA
    return GetFormatFromIndex<CEARatesAndResolutions>(index, CEA1920x1080p24);
  case 1: // 0b001 VESA
    return GetFormatFromIndex<VESARatesAndResolutions>(index, VESA1920x1200p30);
  case 2: // 0b010 HH
    return GetFormatFromIndex<HHRatesAndResolutions>(index, HH848x480p60);
  default:
    assert(false);
    break;
  }
  return NativeVideoFormat(CEA640x480p60);
}

std::vector<H264VideoFormat> VideoFormats::GetH264Formats() const {
  std::vector<H264VideoFormat> result;
  for (const auto& codec : h264_codecs_)
    PopulateVideoFormatList(codec.ToH264VideoCodec(), result);
  return result;
}

std::vector<H264VideoCodec> VideoFormats::GetH264VideoCodecs() const {
  std::vector<H264VideoCodec> result;
  for (const auto& codec : h264_codecs_)
    result.push_back(codec.ToH264VideoCodec());
  return result;
}

std::string VideoFormats::ToString() const {
  std::string ret;

  ret = PropertyName::wfd_video_formats
      + std::string(SEMICOLON)+ std::string(SPACE);

  if (is_none())
    return ret + NONE;

  MAKE_HEX_STRING_2(native, native_);
  MAKE_HEX_STRING_2(preferred_display_mode, preferred_display_mode_);

  ret += native + std::string(SPACE)
      + preferred_display_mode + std::string(SPACE);

  auto it = h264_codecs_.begin();
  auto end = h264_codecs_.end();
  while(it != end) {
    ret += (*it).ToString();
    ++it;
    if (it != end)
      ret += ", ";
  }

  return ret;
}

}  // namespace rtsp
}  // namespace wds
