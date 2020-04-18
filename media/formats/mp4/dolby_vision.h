// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FORMATS_MP4_DOLBY_VISION_H_
#define MEDIA_FORMATS_MP4_DOLBY_VISION_H_

#include <vector>

#include "base/memory/ref_counted.h"
#include "media/base/media_export.h"
#include "media/formats/mp4/box_definitions.h"

namespace media {

namespace mp4 {

// The structure of the configuration is defined in Dolby Streams Within the ISO
// Base Media File Format v1.1 section 3.2.
struct MEDIA_EXPORT DolbyVisionConfiguration : Box {
  DECLARE_BOX_METHODS(DolbyVisionConfiguration);

  // Parses DolbyVisionConfiguration data encoded in |data|.
  // Note: This method is intended to parse data outside the MP4StreamParser
  //       context and therefore the box header is not expected to be present
  //       in |data|.
  // Returns true if |data| was successfully parsed.
  bool ParseForTesting(const uint8_t* data, int data_size);

  uint8_t dv_version_major;
  uint8_t dv_version_minor;
  uint8_t dv_profile;
  uint8_t dv_level;
  uint8_t rpu_present_flag;
  uint8_t el_present_flag;
  uint8_t bl_present_flag;

  VideoCodecProfile codec_profile;

 private:
  bool ParseInternal(BufferReader* reader, MediaLog* media_log);
};

}  // namespace mp4
}  // namespace media

#endif  // MEDIA_FORMATS_MP4_DOLBY_VISION_H_
