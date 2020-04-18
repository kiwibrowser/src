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


#ifndef LIBWDS_RTSP_PRESENTATIONURL_H_
#define LIBWDS_RTSP_PRESENTATIONURL_H_

#include "libwds/rtsp/property.h"

namespace wds {
namespace rtsp {

class PresentationUrl: public Property {
 public:
  PresentationUrl(const std::string& presentation_url_1,
      const std::string presentation_url_2);
  ~PresentationUrl() override;

  const std::string& presentation_url_1() const { return presentation_url_1_; }
  const std::string& presentation_url_2() const { return presentation_url_2_; }
  std::string ToString() const override;

 private:
  std::string presentation_url_1_;
  std::string presentation_url_2_;
};

}  // namespace rtsp
}  // namespace wds

#endif  // LIBWDS_RTSP_PRESENTATIONURL_H_
