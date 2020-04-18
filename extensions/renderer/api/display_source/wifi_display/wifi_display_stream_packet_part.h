// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_STREAM_PACKET_PART_H_
#define EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_STREAM_PACKET_PART_H_

#include <stddef.h>
#include <stdint.h>

#include "base/macros.h"

namespace extensions {

// WiFi Display elementary stream unit packetization consists of multiple
// packetization phases. During those phases, unit buffer bytes are not
// modified but only wrapped in packets.
// This class allows different kind of WiFi Display stream packets to refer to
// unit buffer bytes without copying them.
class WiFiDisplayStreamPacketPart {
 public:
  WiFiDisplayStreamPacketPart(const uint8_t* data, size_t size)
      : data_(data), size_(size) {}
  template <size_t N>
  explicit WiFiDisplayStreamPacketPart(const uint8_t (&data)[N])
      : WiFiDisplayStreamPacketPart(data, N) {}

  const uint8_t* begin() const { return data(); }
  const uint8_t* data() const { return data_; }
  bool empty() const { return size() == 0u; }
  const uint8_t* end() const { return data() + size(); }
  size_t size() const { return size_; }

 private:
  const uint8_t* const data_;
  const size_t size_;

  DISALLOW_COPY_AND_ASSIGN(WiFiDisplayStreamPacketPart);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_STREAM_PACKET_PART_H_
