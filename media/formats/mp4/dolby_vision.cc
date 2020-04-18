// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/formats/mp4/dolby_vision.h"

#include "base/logging.h"
#include "media/base/video_codecs.h"
#include "media/formats/mp4/box_definitions.h"
#include "media/formats/mp4/box_reader.h"

namespace media {
namespace mp4 {

DolbyVisionConfiguration::DolbyVisionConfiguration()
    : dv_version_major(0),
      dv_version_minor(0),
      dv_profile(0),
      dv_level(0),
      rpu_present_flag(0),
      el_present_flag(0),
      bl_present_flag(0),
      codec_profile(VIDEO_CODEC_PROFILE_UNKNOWN) {}

DolbyVisionConfiguration::~DolbyVisionConfiguration() {}

FourCC DolbyVisionConfiguration::BoxType() const {
  return FOURCC_DVCC;
}

bool DolbyVisionConfiguration::Parse(BoxReader* reader) {
  return ParseInternal(reader, reader->media_log());
}

bool DolbyVisionConfiguration::ParseForTesting(const uint8_t* data,
                                               int data_size) {
  BufferReader reader(data, data_size);
  MediaLog media_log;
  return ParseInternal(&reader, &media_log);
}

bool DolbyVisionConfiguration::ParseInternal(BufferReader* reader,
                                             MediaLog* media_log) {
  uint16_t profile_track_indication = 0;
  RCHECK(reader->Read1(&dv_version_major) && reader->Read1(&dv_version_minor) &&
         reader->Read2(&profile_track_indication));

  dv_profile = profile_track_indication >> 9;
  dv_level = (profile_track_indication >> 3) & 0x3F;
  rpu_present_flag = (profile_track_indication >> 2) & 1;
  el_present_flag = (profile_track_indication >> 1) & 1;
  bl_present_flag = profile_track_indication & 1;

  switch (dv_profile) {
    case 0:
      codec_profile = DOLBYVISION_PROFILE0;
      break;
    case 4:
      codec_profile = DOLBYVISION_PROFILE4;
      break;
    case 5:
      codec_profile = DOLBYVISION_PROFILE5;
      break;
    case 7:
      codec_profile = DOLBYVISION_PROFILE7;
      break;
    default:
      DVLOG(2) << "Deprecated or invalid Dolby Vision profile:"
               << static_cast<int>(dv_profile);
      return false;
  }

  DVLOG(2) << "Dolby Vision profile:" << static_cast<int>(dv_profile)
           << " level:" << static_cast<int>(dv_level)
           << " has_bl:" << static_cast<int>(bl_present_flag)
           << " has_el:" << static_cast<int>(el_present_flag)
           << " has_rpu:" << static_cast<int>(rpu_present_flag)
           << " profile type:" << codec_profile;

  return true;
}

}  // namespace mp4
}  // namespace media
