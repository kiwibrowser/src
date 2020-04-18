// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_BATTOR_AGENT_SERIAL_UTILS_H_
#define TOOLS_BATTOR_AGENT_SERIAL_UTILS_H_

#include <string>
#include <vector>

namespace battor {

// Prints |bytes| as a space separated list of hex numbers (e.g. {'A', 'J'}
// would return "0x41 0x4A").
std::string CharArrayToString(const char* bytes, size_t len);
std::string ByteVectorToString(const std::vector<uint8_t>& bytes);

}  // namespace battor

#endif  // TOOLS_BATTOR_AGENT_SERIAL_UTILS_H_
