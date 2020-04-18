// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_MEDIA_CAPABILITIES_WEB_MEDIA_CAPABILITIES_CLIENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_MEDIA_CAPABILITIES_WEB_MEDIA_CAPABILITIES_CLIENT_H_

#include <memory>

#include "third_party/blink/public/platform/modules/media_capabilities/web_media_capabilities_info.h"

namespace blink {

struct WebMediaConfiguration;

// Interface between Blink and the Media layer.
class WebMediaCapabilitiesClient {
 public:
  virtual ~WebMediaCapabilitiesClient() = default;

  virtual void DecodingInfo(
      const WebMediaConfiguration&,
      std::unique_ptr<WebMediaCapabilitiesQueryCallbacks>) = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_MEDIA_CAPABILITIES_WEB_MEDIA_CAPABILITIES_CLIENT_H_
