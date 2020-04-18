/*
 * This file is part of Wireless Display Software for Linux OS
 *
 * Copyright (C) 2016 Intel Corporation.
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

#include <string.h>
#include <iostream>
#include <fstream>

#include "libwds/rtsp/driver.h"
#include "libwds/rtsp/message.h"
#include "libwds/rtsp/reply.h"
#include "libwds/rtsp/getparameter.h"

using wds::rtsp::Driver;

namespace {
const char kTestHeaderCommand[] = "--header";
const char kTestPayloadRequestCommand[] = "--payload-request";
const char kTestPayloadReplyCommand[] = "--payload-reply";
const char kTestPayloadErrorCommand[] = "--payload-error";
const char kTestNumLinesCommand[] = "--num-lines";
const char kTestTestCaseCommand[] = "--test-case";

int PrintError(const char* program) {
  std::cerr << "Usage: " << program << " [--num-lines NUM | --test-case ABSOLUTE_PATH_FILE] [--header | --payload-request | --payload-reply | --payload-error]" << std::endl;
  std::cerr << "Example: " << program << " --num-lines 6 --header" << std::endl;
  std::cerr << "Example: " << program << " --test-case test-options-request.txt --header" << std::endl;
  return 1;
}

std::string GetBufferFromStdin(int num_lines) {
  std::string buffer, input;
  while(num_lines--) {
    getline(std::cin, input);
    buffer += input;
    if (num_lines)
      buffer += "\r\n";
  }
  return buffer;
}

std::string GetBufferFromFile(const std::string& file) {
  std::string line, buffer;
  std::ifstream input_stream(file);
  if (input_stream.is_open()) {
    while (getline(input_stream, line))
      buffer += line;
    input_stream.close();
  }
  return buffer;
}

}  // namespace

int main(const int argc, const char **argv)
{
  // Program name, number of lines to be read, type of message
  if (argc < 4)
    return PrintError(argv[0]);

  std::string buffer;
  if (strncmp(argv[1], kTestNumLinesCommand, strlen(kTestNumLinesCommand)) == 0)
    buffer = GetBufferFromStdin(atoi(argv[2])) + "\r\n\r\n";
  else if (strncmp(argv[1], kTestTestCaseCommand, strlen(kTestTestCaseCommand)) == 0)
    buffer = GetBufferFromFile(argv[2]);
  else
    return PrintError(argv[0]);

  std::unique_ptr<wds::rtsp::Message> message;

  if (strcmp(argv[3], kTestHeaderCommand) == 0) {
    Driver::Parse(buffer, message);
  } else if (strcmp(argv[3], kTestPayloadReplyCommand) == 0) {
    message.reset(new wds::rtsp::Reply());
    Driver::Parse(buffer, message);
  } else if (strcmp(argv[3], kTestPayloadRequestCommand) == 0) {
    message.reset(new wds::rtsp::GetParameter("rtsp://localhost/wfd1.0"));
    Driver::Parse(buffer, message);
  } else if (strcmp(argv[3], kTestPayloadErrorCommand) == 0) {
    message.reset(new wds::rtsp::Reply(303));
    Driver::Parse(buffer, message);
  } else {
    return PrintError(argv[0]);
  }

  return 0;
}
