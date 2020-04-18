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


#ifndef LIBWDS_RTSP_FORMATS3D_H_
#define LIBWDS_RTSP_FORMATS3D_H_

#include "libwds/rtsp/property.h"

#include <vector>

namespace wds {
namespace rtsp {

// todo(shalamov): refactor, looks almost similar to VideoFormats

struct H264Codec3d {
 public:
  H264Codec3d(unsigned char profile, unsigned char level,
      unsigned long long int video_capability_3d, unsigned char latency,
      unsigned short min_slice_size, unsigned short slice_enc_params,
      unsigned char frame_rate_control_support,
      unsigned short max_hres, unsigned short max_vres)
    : profile_(profile),
      level_(level),
      video_capability_3d_(video_capability_3d),
      latency_(latency),
      min_slice_size_(min_slice_size),
      slice_enc_params_(slice_enc_params),
      frame_rate_control_support_(frame_rate_control_support),
      max_hres_(max_hres),
      max_vres_(max_vres) {}

  std::string ToString() const;

  unsigned char profile_;
  unsigned char level_;
  unsigned long long int video_capability_3d_;
  unsigned char latency_;
  unsigned short min_slice_size_;
  unsigned short slice_enc_params_;
  unsigned char frame_rate_control_support_;
  unsigned short max_hres_;
  unsigned short max_vres_;
};

typedef std::vector<H264Codec3d> H264Codecs3d;

class Formats3d: public Property {
 public:
  Formats3d();
  Formats3d(unsigned char native,
            unsigned char preferred_display_mode,
            const H264Codecs3d& h264_codecs_3d);
  ~Formats3d() override;

  unsigned char native_resolution() const { return native_; }
  unsigned char preferred_display_mode() const { return preferred_display_mode_;}
  const H264Codecs3d& codecs() const { return h264_codecs_3d_; }

  std::string ToString() const override;

 private:
  unsigned char native_;
  unsigned char preferred_display_mode_;
  H264Codecs3d h264_codecs_3d_;
};

}  // namespace rtsp
}  // namespace wds

#endif  // LIBWDS_RTSP_FORMATS3D_H_
