// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_REMOTEPLAYBACK_WEB_REMOTE_PLAYBACK_AVAILABILITY_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_REMOTEPLAYBACK_WEB_REMOTE_PLAYBACK_AVAILABILITY_H_

namespace blink {

// GENERATED_JAVA_ENUM_PACKAGE: (
//     org.chromium.blink_public.platform.modules.remoteplayback)
// Various states for the remote playback availability.
enum class WebRemotePlaybackAvailability {
  // The availability is unknown.
  kUnknown,

  // The media source is compatible with some supported device types but
  // no devices were found.
  kDeviceNotAvailable,

  // There're available devices but the current media source is not compatible
  // with any of those.
  kSourceNotCompatible,

  // There're available remote playback devices and the media source is
  // compatible with at least one of them.
  kDeviceAvailable,

  kLast = kDeviceAvailable
};

}  // namespace blink

#endif  // WebRemotePlaybackState_h
