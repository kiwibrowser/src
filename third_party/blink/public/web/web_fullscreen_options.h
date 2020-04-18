// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FULLSCREEN_OPTIONS_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_FULLSCREEN_OPTIONS_H_

namespace blink {

// Options used when requesting fullscreen.
struct WebFullscreenOptions {
  // Prefer that the bottom navigation bar be shown when in fullscreen
  // mode on devices with overlay navigation bars.
  bool prefers_navigation_bar = false;

  bool operator==(const WebFullscreenOptions& rhs) {
    return prefers_navigation_bar == rhs.prefers_navigation_bar;
  }
};

}  // namespace blink

#endif
