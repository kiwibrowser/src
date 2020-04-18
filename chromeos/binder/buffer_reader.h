// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_BINDER_BUFFER_READER_H_
#define CHROMEOS_BINDER_BUFFER_READER_H_

#include <stddef.h>

#include "base/macros.h"
#include "chromeos/chromeos_export.h"

namespace binder {

// BufferReader reads data from the given buffer.
class CHROMEOS_EXPORT BufferReader {
 public:
  BufferReader(const char* data, size_t size);
  ~BufferReader();

  const char* data() const { return data_; }
  size_t size() const { return size_; }
  const char* current() const { return current_; }

  // Returns true when there is some data to read.
  bool HasMoreData() const;

  // Copies the specified number of bytes from the buffer to |out|.
  // |out| shouldn't overlap with |data_|.
  bool Read(void* out, size_t num_bytes);

  // Moves the position |num_bytes| forward.
  bool Skip(size_t num_bytes);

 private:
  const char* data_;
  size_t size_;
  const char* current_;

  DISALLOW_COPY_AND_ASSIGN(BufferReader);
};

}  // namespace binder

#endif  // CHROMEOS_BINDER_BUFFER_READER_H_
