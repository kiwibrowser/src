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

#include "rtsp_input_handler.h"

#include "libwds/rtsp/driver.h"
#include "libwds/rtsp/message.h"

#include <cassert>

namespace wds {

using rtsp::Message;
using rtsp::Driver;

RTSPInputHandler::~RTSPInputHandler() {
}

void RTSPInputHandler::AddInput(const std::string& input) {
  rtsp_input_buffer_ += input;

  // First trying to get payload for the message obtained
  // from the previous input.
  if (message_ && !ParsePayload())
    return;

  while (ParseHeader()) {
    if (!ParsePayload())
      break;
  }
}

bool RTSPInputHandler::ParseHeader() {
  assert(!message_);
  static const char delimiter[] = "\r\n\r\n";
  static const int delimiter_length = 4;
  size_t eom = rtsp_input_buffer_.find(delimiter);
  if (eom == std::string::npos) {
    return false;
  }

  const std::string& header = rtsp_input_buffer_.substr(0, eom + delimiter_length);
  rtsp_input_buffer_.erase(0, eom + delimiter_length);
  Driver::Parse(header, message_);
  if (!message_) {
    ParserErrorOccurred(rtsp_input_buffer_);
    rtsp_input_buffer_.clear();
    return false;
  }
  return true;
}

bool RTSPInputHandler::ParsePayload() {
  assert(message_);
  unsigned content_length = message_->header().content_length();
  if (content_length == 0) {
    MessageParsed(std::move(message_));
    return true;
  }

  if (rtsp_input_buffer_.size() < content_length)
    return false;

  const std::string& payload = rtsp_input_buffer_.substr(0, content_length);
  rtsp_input_buffer_.erase(0, content_length);
  Driver::Parse(payload, message_);
  MessageParsed(std::move(message_));
  return true;
}

}  // namespace wds
