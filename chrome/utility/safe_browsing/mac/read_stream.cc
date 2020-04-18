// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/safe_browsing/mac/read_stream.h"

#include <string.h>
#include <unistd.h>

#include <algorithm>

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"

namespace safe_browsing {
namespace dmg {

bool ReadStream::ReadExact(uint8_t* data, size_t size) {
  size_t bytes_read = 0;
  return Read(data, size, &bytes_read) && bytes_read == size;
}

FileReadStream::FileReadStream(int fd) : fd_(fd) {}

FileReadStream::~FileReadStream() {}

bool FileReadStream::Read(uint8_t* data, size_t size, size_t* bytes_read) {
  *bytes_read = 0;
  ssize_t signed_bytes_read = HANDLE_EINTR(read(fd_, data, size));
  if (signed_bytes_read < 0)
    return false;
  *bytes_read = signed_bytes_read;
  return true;
}

off_t FileReadStream::Seek(off_t offset, int whence) {
  return lseek(fd_, offset, whence);
}

MemoryReadStream::MemoryReadStream(const uint8_t* data, size_t size)
    : data_(data), size_(size), offset_(0) {}

MemoryReadStream::~MemoryReadStream() {}

bool MemoryReadStream::Read(uint8_t* data, size_t size, size_t* bytes_read) {
  *bytes_read = 0;

  if (data_ == nullptr)
    return false;

  size_t bytes_remaining = size_ - offset_;
  if (bytes_remaining == 0)
    return true;

  *bytes_read = std::min(size, bytes_remaining);
  memcpy(data, data_ + offset_, *bytes_read);
  offset_ += *bytes_read;
  return true;
}

off_t MemoryReadStream::Seek(off_t offset, int whence) {
  switch (whence) {
    case SEEK_SET:
      offset_ = offset;
      break;
    case SEEK_CUR:
      offset_ += offset;
      break;
    case SEEK_END:
      offset_ = size_ + offset;
      break;
    default:
      NOTREACHED();
      return -1;
  }
  if (static_cast<size_t>(offset_) >= size_)
    offset_ = size_;
  return offset_;
}

}  // namespace dmg
}  // namespace safe_browsing
