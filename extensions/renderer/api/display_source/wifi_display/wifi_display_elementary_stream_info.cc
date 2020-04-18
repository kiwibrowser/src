// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/api/display_source/wifi_display/wifi_display_elementary_stream_info.h"

#include <utility>

namespace extensions {

WiFiDisplayElementaryStreamInfo::WiFiDisplayElementaryStreamInfo(
    ElementaryStreamType type)
    : type_(type) {}

WiFiDisplayElementaryStreamInfo::WiFiDisplayElementaryStreamInfo(
    ElementaryStreamType type,
    DescriptorVector descriptors)
    : descriptors_(std::move(descriptors)), type_(type) {}

WiFiDisplayElementaryStreamInfo::WiFiDisplayElementaryStreamInfo(
    const WiFiDisplayElementaryStreamInfo&) = default;

WiFiDisplayElementaryStreamInfo::WiFiDisplayElementaryStreamInfo(
    WiFiDisplayElementaryStreamInfo&&) = default;

WiFiDisplayElementaryStreamInfo::~WiFiDisplayElementaryStreamInfo() {}

WiFiDisplayElementaryStreamInfo& WiFiDisplayElementaryStreamInfo::operator=(
    WiFiDisplayElementaryStreamInfo&&) = default;

void WiFiDisplayElementaryStreamInfo::AddDescriptor(
    WiFiDisplayElementaryStreamDescriptor descriptor) {
  descriptors_.emplace_back(std::move(descriptor));
}

const WiFiDisplayElementaryStreamDescriptor*
WiFiDisplayElementaryStreamInfo::FindDescriptor(
    DescriptorTag descriptor_tag) const {
  for (const auto& descriptor : descriptors()) {
    if (descriptor.tag() == descriptor_tag)
      return &descriptor;
  }
  return nullptr;
}

}  // namespace extensions
