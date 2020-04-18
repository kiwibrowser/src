// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/fx_string_testhelpers.h"

#include <iomanip>
#include <ios>
#include <string>

std::ostream& operator<<(std::ostream& os, const CFX_DateTime& dt) {
  os << dt.GetYear() << "-" << std::to_string(dt.GetMonth()) << "-"
     << std::to_string(dt.GetDay()) << " " << std::to_string(dt.GetHour())
     << ":" << std::to_string(dt.GetMinute()) << ":"
     << std::to_string(dt.GetSecond()) << "."
     << std::to_string(dt.GetMillisecond());
  return os;
}

CFX_InvalidSeekableReadStream::CFX_InvalidSeekableReadStream(
    FX_FILESIZE data_size)
    : data_size_(data_size) {}

CFX_InvalidSeekableReadStream::~CFX_InvalidSeekableReadStream() = default;

CFX_BufferSeekableReadStream::CFX_BufferSeekableReadStream(
    const unsigned char* src,
    size_t src_size)
    : data_(src), data_size_(src_size) {}

CFX_BufferSeekableReadStream::~CFX_BufferSeekableReadStream() = default;
