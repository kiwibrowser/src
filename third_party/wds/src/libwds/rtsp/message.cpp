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


#include "libwds/rtsp/message.h"

#include <cassert>

namespace wds {
namespace rtsp {

namespace {
  const char kDefaultContentType[] = "text/parameters";
}

Message::Message(Type type)
  : type_(type) {
}

Message::~Message() {
}

int Message::cseq() const {
  assert(header_);
  return header_->cseq();
}

Header& Message::header() {
  if (!header_)
    header_.reset(new Header());
  return *header_;
}

std::string Message::ToString() const {
  std::string ret;
  if (payload_)
    ret = payload_->ToString();

  if (header_) {
    header_->set_content_length (ret.length());
    if(ret.length() > 0 && header_->content_type().length() == 0)
        header_->set_content_type (kDefaultContentType);
    ret = header_->ToString() + ret;
  }

  return ret;
}

Request::Request(RTSPMethod method, const std::string& request_uri)
  : Message(REQUEST),
    id_(UNKNOWN),
    method_(method),
    request_uri_(request_uri) {
}

Request::~Request() {
}

} // namespace rtsp
} // namespace wds
