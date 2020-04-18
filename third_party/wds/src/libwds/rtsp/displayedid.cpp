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


#include "libwds/rtsp/displayedid.h"

#include "libwds/rtsp/macros.h"

namespace wds {
namespace rtsp {

DisplayEdid::DisplayEdid(): Property(DisplayEdidPropertyType, true) {
}

DisplayEdid::DisplayEdid(unsigned short edid_block_count,
    const std::string& edid_payload)
  : Property(DisplayEdidPropertyType),
    edid_block_count_(edid_block_count),
    edid_payload_(edid_payload.length() ? edid_payload : NONE) {
}

DisplayEdid::~DisplayEdid() {
}

std::string DisplayEdid::ToString() const {

  std::string ret =
      PropertyName::wfd_display_edid + std::string(SEMICOLON)
    + std::string(SPACE);

  if (is_none()) {
    ret += NONE;
  } else {
    MAKE_HEX_STRING_2(edid_block_count, edid_block_count_);
    ret += edid_block_count + std::string(SPACE) + edid_payload_;
  }

  return ret;
}

}  // namespace rtsp
}  // namespace wds
