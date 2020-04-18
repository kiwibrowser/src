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

#ifndef LIBWDS_COMMON_RTSP_INPUT_HANDLER_H_
#define LIBWDS_COMMON_RTSP_INPUT_HANDLER_H_

#include <memory>
#include <string>

namespace wds {

namespace rtsp {
class Message;
}  // namespace rtsp

// An aux class used to obtain Message object from the given raw input.
class RTSPInputHandler {
 protected:
  RTSPInputHandler() = default;
  virtual ~RTSPInputHandler();

  void AddInput(const std::string& input);

  // To be overridden.
  virtual void MessageParsed(std::unique_ptr<rtsp::Message> message) = 0;
  virtual void ParserErrorOccurred(const std::string& invalid_input) {}

 private:
  bool ParseHeader();
  bool ParsePayload();

  std::string rtsp_input_buffer_;
  std::unique_ptr<rtsp::Message> message_;
};

}

#endif // LIBWDS_COMMON_RTSP_INPUT_HANDLER_H_
