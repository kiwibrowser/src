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


#include "libwds/rtsp/driver.h"

#include <cctype>
#include <sstream>

#include "libwds/public/logging.h"
#include "libwds/rtsp/message.h"
#include "libwds/rtsp/reply.h"

#include "errorscanner.h"
#include "headerscanner.h"
#include "messagescanner.h"

int wds_lex(YYSTYPE* yylval, void* scanner,
    std::unique_ptr<wds::rtsp::Message>& message) {
  if (!message) {
    return header_lex(yylval, scanner);
  } else if (message->is_reply()) {
    wds::rtsp::Reply* reply = static_cast<wds::rtsp::Reply*>(message.get());
    if (reply->response_code() == wds::rtsp::STATUS_SeeOther)
      return error_lex(yylval, scanner);

    return message_lex(yylval, scanner);
  }

  return message_lex(yylval, scanner);
}

void wds_error(void* scanner, std::unique_ptr<wds::rtsp::Message>& message,
    const char* error_message) {
  WDS_ERROR("Parser error: %s", error_message);
  message.reset(nullptr);
}

namespace wds {
namespace rtsp {

void Driver::Parse(const std::string& input, std::unique_ptr<Message>& message) {
  void* scanner = nullptr;
#if YYDEBUG
  bool enable_debug = true;
  wds_debug = 1;
#else
  bool enable_debug = false;
#endif

  if (!message) {
    header_lex_init(&scanner);
    header_set_debug(enable_debug, scanner);
    header__scan_string(input.c_str(), scanner);
    wds_parse(scanner, message);
    header_lex_destroy(scanner);
  } else if (message->is_reply()) {
    Reply* reply = static_cast<Reply*>(message.get());
    if (reply->response_code() == STATUS_SeeOther) {
      error_lex_init(&scanner);
      error_set_debug(enable_debug, scanner);
      error__scan_string(input.c_str(), scanner);
      wds_parse(scanner, message);
      error_lex_destroy(scanner);
    } else {
      message_lex_init(&scanner);
      message_set_debug(enable_debug, scanner);
      message_set_extra(message->is_reply(), scanner);
      message__scan_string(input.c_str(), scanner);
      wds_parse(scanner, message);
      message_lex_destroy(scanner);
    }
  } else {
    message_lex_init(&scanner);
    message_set_debug(enable_debug, scanner);
    message__scan_string(input.c_str(), scanner);
    wds_parse(scanner, message);
    message_lex_destroy(scanner);
  }
}

} // namespace rtsp
} // namespace wds

