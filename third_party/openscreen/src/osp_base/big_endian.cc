// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp_base/big_endian.h"

namespace openscreen {

BigEndianReader::BigEndianReader(const uint8_t* buffer, size_t length)
    : begin_(buffer), current_(buffer), end_(buffer + length) {}

bool BigEndianReader::ReadBytes(size_t length, void* out) {
  if (current_ + length > end_) {
    return false;
  }
  memcpy(out, current_, length);
  current_ += length;
  return true;
}

bool BigEndianReader::Skip(size_t length) {
  if (current_ + length > end_) {
    return false;
  }
  current_ += length;
  return true;
}

BigEndianWriter::BigEndianWriter(uint8_t* buffer, size_t length)
    : begin_(buffer), current_(buffer), end_(buffer + length) {}

bool BigEndianWriter::WriteBytes(const void* buffer, size_t length) {
  if (current_ + length > end_) {
    return false;
  }
  memcpy(current_, buffer, length);
  current_ += length;
  return true;
}

bool BigEndianWriter::Skip(size_t length) {
  if (current_ + length > end_) {
    return false;
  }
  current_ += length;
  return true;
}

}  // namespace openscreen