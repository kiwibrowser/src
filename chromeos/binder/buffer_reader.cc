// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/binder/buffer_reader.h"

#include <string.h>

namespace binder {

BufferReader::BufferReader(const char* data, size_t size)
    : data_(data), size_(size), current_(data) {}

BufferReader::~BufferReader() = default;

bool BufferReader::HasMoreData() const {
  return current_ < data_ + size_;
}

bool BufferReader::Read(void* out, size_t num_bytes) {
  if (current_ + num_bytes > data_ + size_)
    return false;
  memcpy(out, current_, num_bytes);
  current_ += num_bytes;
  return true;
}

bool BufferReader::Skip(size_t num_bytes) {
  if (current_ + num_bytes > data_ + size_)
    return false;
  current_ += num_bytes;
  return true;
}

}  // namespace binder
