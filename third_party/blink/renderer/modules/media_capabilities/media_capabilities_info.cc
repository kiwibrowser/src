// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/media_capabilities/media_capabilities_info.h"

namespace blink {

// static
MediaCapabilitiesInfo* MediaCapabilitiesInfo::Take(
    ScriptPromiseResolver*,
    std::unique_ptr<WebMediaCapabilitiesInfo> web_media_capabilities_info) {
  DCHECK(web_media_capabilities_info);
  return new MediaCapabilitiesInfo(std::move(web_media_capabilities_info));
}

bool MediaCapabilitiesInfo::supported() const {
  return web_media_capabilities_info_->supported;
}

bool MediaCapabilitiesInfo::smooth() const {
  return web_media_capabilities_info_->smooth;
}

bool MediaCapabilitiesInfo::powerEfficient() const {
  return web_media_capabilities_info_->power_efficient;
}

MediaCapabilitiesInfo::MediaCapabilitiesInfo(
    std::unique_ptr<WebMediaCapabilitiesInfo> web_media_capabilities_info)
    : web_media_capabilities_info_(std::move(web_media_capabilities_info)) {}

}  // namespace blink
