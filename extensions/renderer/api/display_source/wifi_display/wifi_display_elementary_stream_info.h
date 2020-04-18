// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_ELEMENTARY_STREAM_INFO_H_
#define EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_ELEMENTARY_STREAM_INFO_H_

#include <stdint.h>
#include <vector>

#include "extensions/renderer/api/display_source/wifi_display/wifi_display_elementary_stream_descriptor.h"

namespace extensions {

// WiFi Display elementary stream info is a container for elementary stream
// information and is used for passing that information to a WiFi Display
// transport stream packetizer.
class WiFiDisplayElementaryStreamInfo {
 public:
  using DescriptorVector = std::vector<WiFiDisplayElementaryStreamDescriptor>;
  using DescriptorTag = WiFiDisplayElementaryStreamDescriptor::DescriptorTag;

  enum ElementaryStreamType : uint8_t {
    AUDIO_AAC = 0x0Fu,
    AUDIO_AC3 = 0x81u,
    AUDIO_LPCM = 0x83u,
    VIDEO_H264 = 0x1Bu,
  };

  explicit WiFiDisplayElementaryStreamInfo(ElementaryStreamType type);
  WiFiDisplayElementaryStreamInfo(ElementaryStreamType type,
                                  DescriptorVector descriptors);
  WiFiDisplayElementaryStreamInfo(const WiFiDisplayElementaryStreamInfo&);
  WiFiDisplayElementaryStreamInfo(WiFiDisplayElementaryStreamInfo&&);
  ~WiFiDisplayElementaryStreamInfo();

  WiFiDisplayElementaryStreamInfo& operator=(WiFiDisplayElementaryStreamInfo&&);

  const DescriptorVector& descriptors() const { return descriptors_; }
  ElementaryStreamType type() const { return type_; }

  void AddDescriptor(WiFiDisplayElementaryStreamDescriptor descriptor);
  const WiFiDisplayElementaryStreamDescriptor* FindDescriptor(
      DescriptorTag descriptor_tag) const;
  template <typename Descriptor>
  const Descriptor* FindDescriptor() const {
    return static_cast<const Descriptor*>(
        FindDescriptor(static_cast<DescriptorTag>(Descriptor::kTag)));
  }

 private:
  DescriptorVector descriptors_;
  ElementaryStreamType type_;
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_API_DISPLAY_SOURCE_WIFI_DISPLAY_WIFI_DISPLAY_ELEMENTARY_STREAM_INFO_H_
