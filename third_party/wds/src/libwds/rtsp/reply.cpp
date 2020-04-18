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


#include "libwds/rtsp/reply.h"

namespace wds {
namespace rtsp {

namespace {
  const char kRTSPHeader[] = "RTSP/1.0 ";
  const char kOK[] = "OK";
}

Reply::Reply(int response_code)
  : Message(Message::REPLY),
    response_code_(response_code) {
}

Reply::~Reply() {
}

std::string Reply::ToString() const {
  std::string ret;
  ret += kRTSPHeader + std::to_string(response_code_)
       + std::string(SPACE) + kOK + std::string(CRLF);
  return ret + Message::ToString();
}

}  // namespace rtsp
}  // namespace wds
