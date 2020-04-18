// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_STREAMS_STREAM_METADATA_H_
#define CONTENT_BROWSER_STREAMS_STREAM_METADATA_H_

#include "base/macros.h"

#include "net/http/http_response_info.h"

namespace content {

class StreamMetadata {
 public:
  explicit StreamMetadata(const net::HttpResponseInfo& response_info)
      : total_received_bytes_(0),
        raw_body_bytes_(0),
        response_info_(response_info) {}

  void set_total_received_bytes(int64_t bytes) {
    total_received_bytes_ = bytes;
  }
  int64_t total_received_bytes() const { return total_received_bytes_; }

  void set_raw_body_bytes(int64_t bytes) { raw_body_bytes_ = bytes; }
  int64_t raw_body_bytes() const { return raw_body_bytes_; }

  const net::HttpResponseInfo& response_info() const { return response_info_; }

 private:
  int64_t total_received_bytes_;
  int64_t raw_body_bytes_;
  net::HttpResponseInfo response_info_;

  DISALLOW_COPY_AND_ASSIGN(StreamMetadata);
};

}  // namespace content

#endif  // CONTENT_BROWSER_STREAMS_STREAM_METADATA_H_