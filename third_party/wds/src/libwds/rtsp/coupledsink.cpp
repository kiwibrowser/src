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


#include "libwds/rtsp/coupledsink.h"

#include <climits>
#include "libwds/rtsp/macros.h"

namespace wds {
namespace rtsp {

CoupledSink::CoupledSink(unsigned char status,
    unsigned long long int sink_address)
    : Property(CoupledSinkPropertyType),
      status_(status),
      sink_address_(sink_address) {
}

CoupledSink::CoupledSink(): Property(CoupledSinkPropertyType, true){
}

CoupledSink::~CoupledSink(){
}

std::string CoupledSink::ToString() const {
  std::string ret =
      PropertyName::wfd_coupled_sink + std::string(SEMICOLON)
    + std::string(SPACE);

  if (is_none()) {
    ret += NONE;
  } else {
    MAKE_HEX_STRING_2(status, status_);
    ret += status + std::string(SPACE);

    if (sink_address_ != ULLONG_MAX) {
      MAKE_HEX_STRING_12(sink_address, sink_address_);
      ret += sink_address;
    } else {
      ret += NONE;
    }
  }

  return ret;
}

}  // namespace rtsp
}  // namespace wds
