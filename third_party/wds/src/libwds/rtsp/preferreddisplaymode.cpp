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


#include "libwds/rtsp/preferreddisplaymode.h"

#include "libwds/rtsp/macros.h"

namespace wds {
namespace rtsp {

PreferredDisplayMode::PreferredDisplayMode(
    unsigned int p_clock,unsigned short h,
    unsigned short hb, unsigned short hspol_hsoff, unsigned short hsw,
    unsigned short v, unsigned short vb, unsigned short vspol_vsoff,
    unsigned short vsw, unsigned char vbs3d, unsigned char modes_2d_s3d,
    unsigned char p_depth, const H264Codec& h264_codec)
  : Property(PreferredDisplayModePropertyType),
    p_clock_(p_clock),
    h_(h),
    hb_(hb),
    hspol_hsoff_(hspol_hsoff),
    hsw_(hsw),
    v_(v),
    vb_(vb),
    vspol_vsoff_(vspol_vsoff),
    vsw_(vsw),
    vbs3d_(vbs3d),
    modes_2d_s3d_(modes_2d_s3d),
    p_depth_(p_depth),
    h264_codec_(h264_codec) {
}

std::string PreferredDisplayMode::ToString() const {
  MAKE_HEX_STRING_6(p_clock, p_clock_);
  MAKE_HEX_STRING_4(h, h_);
  MAKE_HEX_STRING_4(hb, hb_);
  MAKE_HEX_STRING_4(hspol_hsoff, hspol_hsoff_);
  MAKE_HEX_STRING_4(hsw, hsw_);
  MAKE_HEX_STRING_4(v, v_);
  MAKE_HEX_STRING_4(vb, vb_);
  MAKE_HEX_STRING_4(vspol_vsoff, vspol_vsoff_);
  MAKE_HEX_STRING_4(vsw, vsw_);
  MAKE_HEX_STRING_2(vbs3d, vbs3d_);
  MAKE_HEX_STRING_2(modes_2d_s3d, modes_2d_s3d_);
  MAKE_HEX_STRING_2(p_depth, p_depth_);

  std::string ret =
      PropertyName::wfd_preferred_display_mode + std::string(SEMICOLON)
  + std::string(SPACE) + p_clock
  + std::string(SPACE) + h
  + std::string(SPACE) + hb
  + std::string(SPACE) + hspol_hsoff
  + std::string(SPACE) + hsw
  + std::string(SPACE) + v
  + std::string(SPACE) + vb
  + std::string(SPACE) + vspol_vsoff
  + std::string(SPACE) + vsw
  + std::string(SPACE) + vbs3d
  + std::string(SPACE) + modes_2d_s3d
  + std::string(SPACE) + p_depth
  + std::string(SPACE) + h264_codec_.ToString();
  return ret;
}

PreferredDisplayMode::~PreferredDisplayMode() {
}

}  // namespace rtsp
}  // namespace wds
