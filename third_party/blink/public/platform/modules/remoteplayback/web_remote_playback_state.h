// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_REMOTEPLAYBACK_WEB_REMOTE_PLAYBACK_STATE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_REMOTEPLAYBACK_WEB_REMOTE_PLAYBACK_STATE_H_

namespace blink {

enum class WebRemotePlaybackState {
  kConnecting = 0,
  kConnected,
  kDisconnected,
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_MODULES_REMOTEPLAYBACK_WEB_REMOTE_PLAYBACK_STATE_H_
