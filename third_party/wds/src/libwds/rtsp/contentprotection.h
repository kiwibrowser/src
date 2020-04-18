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


#ifndef LIBWDS_RTSP_CONTENTPROTECTION_H_
#define LIBWDS_RTSP_CONTENTPROTECTION_H_

#include "libwds/rtsp/property.h"

namespace wds {
namespace rtsp {

class ContentProtection: public Property {
 public:
  enum HDCPSpec {
    HDCP_SPEC_2_0,
    HDCP_SPEC_2_1
  };

 public:
  ContentProtection();
  ContentProtection(HDCPSpec hdcp_spec, unsigned int port);
  ~ContentProtection() override;

  HDCPSpec hdcp_spec() const;
  unsigned int port() const { return port_; }
  std::string ToString() const override;

 private:
  HDCPSpec hdcp_spec_;
  unsigned int port_;
};

}  // namespace rtsp
}  // namespace wds

#endif  // LIBWDS_RTSP_CONTENTPROTECTION_H_
