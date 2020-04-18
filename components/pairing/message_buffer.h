// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAIRING_MESSAGE_BUFFER_H_
#define COMPONENTS_PAIRING_MESSAGE_BUFFER_H_

#include "base/containers/circular_deque.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace net {
class IOBuffer;
}

namespace pairing_chromeos {

// A MessageBuffer is a simple wrapper around an ordered set of buffers received
// from a socket.  It keeps track of the amount of unread data, and allows data
// to be retreived that was split across buffers in a single chunk.
class MessageBuffer {
 public:
  MessageBuffer();
  ~MessageBuffer();

  // Returns the number of bytes currently received, but not yet read.
  int AvailableBytes();

  // Read |size| bytes into |buffer|.  |size| must be smaller than or equal to
  // the current number of available bytes.
  void ReadBytes(char* buffer, int size);

  // Add the data from an IOBuffer.  A reference to the IOBuffer will be
  // maintained until it is drained.
  void AddIOBuffer(scoped_refptr<net::IOBuffer> io_buffer, int size);

 private:
  // Offset of the next byte to be read from the current IOBuffer.
  int buffer_offset_;
  // Total number of bytes in IOBuffers in |pending_data_|, including bytes that
  // have already been read.
  int total_buffer_size_;
  base::circular_deque<std::pair<scoped_refptr<net::IOBuffer>, int>>
      pending_data_;

  DISALLOW_COPY_AND_ASSIGN(MessageBuffer);
};

}  // namespace pairing_chromeos

#endif  // COMPONENTS_PAIRING_MESSAGE_BUFFER_H_
