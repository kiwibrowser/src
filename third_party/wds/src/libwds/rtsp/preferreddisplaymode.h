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


#ifndef LIBWDS_RTSP_PREFERREDDISPLAYMODE_H_
#define LIBWDS_RTSP_PREFERREDDISPLAYMODE_H_

#include "libwds/rtsp/property.h"

#include "libwds/rtsp/videoformats.h"

namespace wds {
namespace rtsp {

class PreferredDisplayMode: public Property {
 public:
  PreferredDisplayMode(unsigned int p_clock, unsigned short h,
      unsigned short hb, unsigned short hspol_hsoff, unsigned short hsw,
      unsigned short v, unsigned short vb, unsigned short vspol_vsoff,
      unsigned short vsw, unsigned char vbs3d, unsigned char modes_2d_s3d,
      unsigned char p_depth, const H264Codec& h264_codec);

  ~PreferredDisplayMode() override;

  unsigned int p_clock() const { return p_clock_; }
  unsigned short h() const { return h_; }
  unsigned short hb() const { return hb_; }
  unsigned short hspol_hsoff() const { return hspol_hsoff_; }
  unsigned short hsw() const { return hsw_; }
  unsigned short v() const { return v_; }
  unsigned short vb() const { return vb_; }
  unsigned short vspol_vsoff() const { return vspol_vsoff_; }
  unsigned short vsw() const { return vsw_; }
  unsigned char vbs3d() const { return vbs3d_; }
  unsigned char modes_2d_s3d() const { return modes_2d_s3d_; }
  unsigned char p_depth() const { return p_depth_; }
  const H264Codec& h264_codec() const { return h264_codec_; }

  std::string ToString() const override;

 private:
  unsigned int p_clock_;
  unsigned short h_;
  unsigned short hb_;
  unsigned short hspol_hsoff_;
  unsigned short hsw_;
  unsigned short v_;
  unsigned short vb_;
  unsigned short vspol_vsoff_;
  unsigned short vsw_;
  unsigned char vbs3d_;
  unsigned char modes_2d_s3d_;
  unsigned char p_depth_;
  H264Codec h264_codec_;
};

}  // namespace rtsp
}  // namespace wds

#endif  // LIBWDS_RTSP_PREFERREDDISPLAYMODE_H_
