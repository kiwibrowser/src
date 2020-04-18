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

#include "libwds/public/media_manager.h"

#include <assert.h>

#include <algorithm>

#include "libwds/public/logging.h"

namespace wds {

namespace {
// Quality weight is calculated using following formula:
// width * height * fps * 2 for progressive or 1 for interlaced frames
struct QualityInfo {
  unsigned width;
  unsigned height;
  unsigned weight;
};

const QualityInfo cea_info_table[] = {
  {640, 480, 640*480*60*2},     // CEA640x480p60
  {720, 480, 720*480*60*2},     // CEA720x480p60
  {720, 480, 720*480*60},       // CEA720x480i60
  {720, 576, 720*576*50*2},     // CEA720x576p50
  {720, 576, 720*576*50},       // CEA720x576i50
  {1280, 720, 1280*720*30*2},   // CEA1280x720p30
  {1280, 720, 1280*720*60*2},   // CEA1280x720p60
  {1920, 1080, 1920*1080*30*2}, // CEA1920x1080p30
  {1920, 1080, 1920*1080*60*2}, // CEA1920x1080p60
  {1920, 1080, 1920*1080*60},   // CEA1920x1080i60
  {1280, 720, 1280*720*25*2},   // CEA1280x720p25
  {1280, 720, 1280*720*50*2},   // CEA1280x720p50
  {1920, 1080, 1920*1080*25*2}, // CEA1920x1080p25
  {1920, 1080, 1920*1080*50*2}, // CEA1920x1080p50
  {1920, 1080, 1920*1080*50},   // CEA1920x1080i50
  {1280, 720, 1280*720*24*2},   // CEA1280x720p24
  {1920, 1080, 1920*1080*24*2}  // CEA1920x1080p24
};

#define CEA_TABLE_LENGTH  sizeof(cea_info_table) / sizeof(QualityInfo)

const QualityInfo vesa_info_table[] = {
  {800, 600, 800*600*30*2},        // VESA800x600p30
  {800, 600, 800*600*60*2},        // VESA800x600p60
  {1024, 768, 1024*768*30*2},      // VESA1024x768p30
  {1024, 768, 1024*768*60*2},      // VESA1024x768p60
  {1152, 864, 1152*864*30*2},      // VESA1152x864p30
  {1152, 864, 1152*864*60*2},      // VESA1152x864p60
  {1280, 768, 1280*768*30*2},      // VESA1280x768p30
  {1280, 768, 1280*768*60*2},      // VESA1280x768p60
  {1280, 800, 1280*800*30*2},      // VESA1280x800p30
  {1280, 800, 1280*800*60*2},      // VESA1280x800p60
  {1360, 768, 1360*768*30*2},      // VESA1360x768p30
  {1360, 768, 1360*768*60*2},      // VESA1360x768p60
  {1366, 768, 1366*768*30*2},      // VESA1366x768p30
  {1366, 768, 1366*768*60*2},      // VESA1366x768p60
  {1280, 1024, 1280*1024*30*2},    // VESA1280x1024p30
  {1280, 1024, 1280*1024*60*2},    // VESA1280x1024p60
  {1400, 1050, 1400*1050*30*2},    // VESA1400x1050p30
  {1400, 1050, 1400*1050*60*2},    // VESA1400x1050p60
  {1440, 900, 1440*900*30*2},      // VESA1440x900p30
  {1440, 900, 1440*900*60*2},      // VESA1440x900p60
  {1600, 900, 1600*900*30*2},      // VESA1600x900p30
  {1600, 900, 1600*900*60*2},      // VESA1600x900p60
  {1600, 1200, 1600*1200*30*2},    // VESA1600x1200p30
  {1600, 1200, 1600*1200*60*2},    // VESA1600x1200p60
  {1680, 1024, 1680*1024*30*2},    // VESA1680x1024p30
  {1680, 1024, 1680*1024*60*2},    // VESA1680x1024p60
  {1680, 1050, 1680*1050*30*2},    // VESA1680x1050p30
  {1680, 1050, 1680*1050*60*2},    // VESA1680x1050p60
  {1920, 1200, 1920*1200*30*2}     // VESA1920x1200p30
};

#define VESA_TABLE_LENGTH  sizeof(vesa_info_table) / sizeof(QualityInfo)

const QualityInfo hh_info_table[] = {
  {800, 480, 800*480*30*2},         // HH800x480p30
  {800, 480, 800*480*60*2},         // HH800x480p60
  {854, 480, 854*480*30*2},         // HH854x480p30
  {854, 480, 854*480*60*2},         // HH854x480p60
  {864, 480, 864*480*30*2},         // HH864x480p30
  {864, 480, 864*480*60*2},         // HH864x480p60
  {640, 360, 640*360*30*2},         // HH640x360p30
  {640, 360, 640*360*60*2},         // HH640x360p60
  {960, 540, 960*540*30*2},         // HH960x540p30
  {960, 540, 960*540*60*2},         // HH960x540p60
  {848, 480, 848*480*30*2},         // HH848x480p30
  {848, 480, 848*480*60*2}          // HH848x480p60
};

#define HH_TABLE_LENGTH  sizeof(hh_info_table) / sizeof(QualityInfo)

QualityInfo get_cea_info(const H264VideoFormat& format) {
  assert(format.rate_resolution < CEA_TABLE_LENGTH);
  return cea_info_table[format.rate_resolution];
}

QualityInfo get_vesa_info(const H264VideoFormat& format) {
  assert(format.rate_resolution < VESA_TABLE_LENGTH);
  return vesa_info_table[format.rate_resolution];
}

QualityInfo get_hh_info(const H264VideoFormat& format) {
  assert(format.rate_resolution < HH_TABLE_LENGTH);
  return hh_info_table[format.rate_resolution];
}

QualityInfo get_quality_info(const H264VideoFormat& format) {
  QualityInfo info;
  switch (format.type) {
  case CEA:
    info = get_cea_info(format);
    break;
  case VESA:
    info = get_vesa_info(format);
    break;
  case HH:
    info = get_hh_info(format);
    break;
  default:
    assert(false);
    break;
  }
  return info;
}

std::pair<unsigned, unsigned> get_resolution(const H264VideoFormat& format) {
  QualityInfo info = get_quality_info(format);
  return std::pair<unsigned, unsigned>(info.width, info.height);
}

bool video_format_sort_func(const H264VideoFormat& a, const H264VideoFormat& b) {
  if (get_quality_info(a).weight != get_quality_info(b).weight)
    return get_quality_info(a).weight < get_quality_info(b).weight;
  if (a.profile != b.profile)
    return a.profile < b.profile;
  return a.level < b.level;
}

template <typename RREnum>
void PopulateVideoFormatList(
    H264Profile profile,
    H264Level level,
    const RateAndResolutionsBitmap& bitmap,
    RREnum max_value,
    std::vector<H264VideoFormat>& formats) {
  if (bitmap.none())
    return;

  for (RateAndResolution rr = 0; rr <= static_cast<RateAndResolution>(max_value); ++rr) {
    if (bitmap.test(rr))
      formats.push_back(H264VideoFormat(profile, level, static_cast<RREnum>(rr)));
  }
}

}  // namespace

void PopulateVideoFormatList(
    const H264VideoCodec& codec, std::vector<H264VideoFormat>& formats) {
  PopulateVideoFormatList<CEARatesAndResolutions>(
      codec.profile, codec.level, codec.cea_rr, CEA1920x1080p24, formats);
  PopulateVideoFormatList<VESARatesAndResolutions>(
      codec.profile, codec.level, codec.vesa_rr, VESA1920x1200p30, formats);
  PopulateVideoFormatList<HHRatesAndResolutions>(
      codec.profile, codec.level, codec.hh_rr, HH848x480p60, formats);
}

H264VideoFormat FindOptimalVideoFormat(
    const NativeVideoFormat& native,
    const std::vector<H264VideoCodec>& local_codecs,
    const std::vector<H264VideoCodec>& remote_codecs,
    bool* success) {
  std::vector<H264VideoFormat> local_formats, remote_formats;
  for (const auto& codec : local_codecs)
    PopulateVideoFormatList(codec, local_formats);
  for (const auto& codec : remote_codecs)
    PopulateVideoFormatList(codec, remote_formats);

  std::sort(local_formats.begin(), local_formats.end(),
      video_format_sort_func);
  std::sort(remote_formats.begin(), remote_formats.end(),
      video_format_sort_func);

  auto it = local_formats.begin();
  auto end = local_formats.end();

  H264VideoFormat format;

  while(it != end) {
    auto match = std::find_if(
        remote_formats.begin(),
        remote_formats.end(),
        [&it] (const H264VideoFormat& format) {
            return ((*it).type == format.type) &&
                   (get_resolution(*it) == get_resolution(format));
        });

    if (match != remote_formats.end()) {
      format = *match;
      break;
    }
    ++it;
  }

  // Should not happen, 640x480p60 should be always supported!
  if (it == end) {
    WDS_ERROR("Failed to find compatible video format.");
    if (success)
      *success = false;
    return format;
  }

  // if remote device supports higher codec profile / level
  // downgrade them to what we support locally.
  if (format.profile > (*it).profile)
    format.profile = (*it).profile;
  if (format.level > (*it).level)
    format.level = (*it).level;
  if (success)
    *success = true;
  return format;
}

}  // namespace wds
