// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/battor_agent/serial_utils.h"

namespace battor {

std::string ByteVectorToString(const std::vector<uint8_t>& data) {
  std::string s;

  // Reserve enough bytes for '0x', the two data characters, a space, and a null
  // terminating byte.
  char num_buff[6];
  for (uint8_t d : data) {
    // We use sprintf because stringstream's hex support wants to print our
    // characters as signed.
    sprintf(num_buff, "0x%02hhx ", d);
    s += num_buff;
  }

  return s.substr(0, s.size() - 1);
}

std::string CharArrayToString(const char* bytes, size_t len) {
  return ByteVectorToString(std::vector<uint8_t>(bytes, bytes + len));
}

}  // namespace battor
