// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_MEDIA_CAPABILITIES_WEB_MEDIA_CONFIGURATION_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_MEDIA_CAPABILITIES_WEB_MEDIA_CONFIGURATION_H_

#include "base/optional.h"
#include "third_party/blink/public/platform/modules/media_capabilities/web_audio_configuration.h"
#include "third_party/blink/public/platform/modules/media_capabilities/web_video_configuration.h"

namespace blink {

enum class MediaConfigurationType {
  kFile,
  kMediaSource,
};

// Represents a MediaConfiguration dictionary to be used outside of Blink. At
// least one of `audioConfiguration` or `videoConfiguration` will be set.
// It is created by Blink and passed to consumers that can assume that all
// required fields are properly set.
struct WebMediaConfiguration {
  MediaConfigurationType type;

  base::Optional<WebAudioConfiguration> audio_configuration;
  base::Optional<WebVideoConfiguration> video_configuration;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_MEDIA_CAPABILITIES_WEB_MEDIA_CONFIGURATION_H_
