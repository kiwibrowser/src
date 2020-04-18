// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_FRAME_SANDBOX_FLAGS_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_FRAME_SANDBOX_FLAGS_H_

namespace blink {

// See http://www.whatwg.org/specs/web-apps/current-work/#attr-iframe-sandbox
// for a list of the sandbox flags.  This enum should be kept in sync with
// Source/core/frame/SandboxFlags.h, as enforced in SandboxFlags.cpp.
enum class WebSandboxFlags : int {
  kNone = 0,
  kNavigation = 1,
  kPlugins = 1 << 1,
  kOrigin = 1 << 2,
  kForms = 1 << 3,
  kScripts = 1 << 4,
  kTopNavigation = 1 << 5,
  kPopups = 1 << 6,
  kAutomaticFeatures = 1 << 7,
  kPointerLock = 1 << 8,
  kDocumentDomain = 1 << 9,
  kOrientationLock = 1 << 10,
  kPropagatesToAuxiliaryBrowsingContexts = 1 << 11,
  kModals = 1 << 12,
  kPresentationController = 1 << 13,
  kTopNavigationByUserActivation = 1 << 14,
  kDownloads = 1 << 15,
  kAll = -1
};

inline constexpr WebSandboxFlags operator&(WebSandboxFlags a,
                                           WebSandboxFlags b) {
  return static_cast<WebSandboxFlags>(static_cast<int>(a) &
                                      static_cast<int>(b));
}

inline constexpr WebSandboxFlags operator|(WebSandboxFlags a,
                                           WebSandboxFlags b) {
  return static_cast<WebSandboxFlags>(static_cast<int>(a) |
                                      static_cast<int>(b));
}

inline WebSandboxFlags& operator|=(WebSandboxFlags& a, WebSandboxFlags b) {
  return a = a | b;
}

inline constexpr WebSandboxFlags operator~(WebSandboxFlags flags) {
  return static_cast<WebSandboxFlags>(~static_cast<int>(flags));
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_FRAME_SANDBOX_FLAGS_H_
