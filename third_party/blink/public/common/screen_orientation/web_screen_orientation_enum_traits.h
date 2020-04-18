// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_SCREEN_ORIENTATION_WEB_SCREEN_ORIENTATION_ENUM_TRAITS_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_SCREEN_ORIENTATION_WEB_SCREEN_ORIENTATION_ENUM_TRAITS_H_

#include "services/device/public/mojom/screen_orientation_lock_types.mojom-shared.h"
#include "third_party/blink/public/common/screen_orientation/web_screen_orientation_lock_type.h"

namespace mojo {

template <>
struct EnumTraits<::device::mojom::ScreenOrientationLockType,
                  ::blink::WebScreenOrientationLockType> {
  static ::device::mojom::ScreenOrientationLockType ToMojom(
      ::blink::WebScreenOrientationLockType lockType) {
    switch (lockType) {
      case ::blink::kWebScreenOrientationLockDefault:
        return ::device::mojom::ScreenOrientationLockType::DEFAULT;
      case ::blink::kWebScreenOrientationLockPortraitPrimary:
        return ::device::mojom::ScreenOrientationLockType::PORTRAIT_PRIMARY;
      case ::blink::kWebScreenOrientationLockPortraitSecondary:
        return ::device::mojom::ScreenOrientationLockType::PORTRAIT_SECONDARY;
      case ::blink::kWebScreenOrientationLockLandscapePrimary:
        return ::device::mojom::ScreenOrientationLockType::LANDSCAPE_PRIMARY;
      case ::blink::kWebScreenOrientationLockLandscapeSecondary:
        return ::device::mojom::ScreenOrientationLockType::LANDSCAPE_SECONDARY;
      case ::blink::kWebScreenOrientationLockAny:
        return ::device::mojom::ScreenOrientationLockType::ANY;
      case ::blink::kWebScreenOrientationLockLandscape:
        return ::device::mojom::ScreenOrientationLockType::LANDSCAPE;
      case ::blink::kWebScreenOrientationLockPortrait:
        return ::device::mojom::ScreenOrientationLockType::PORTRAIT;
      case ::blink::kWebScreenOrientationLockNatural:
        return ::device::mojom::ScreenOrientationLockType::NATURAL;
    }
    NOTREACHED();
    return ::device::mojom::ScreenOrientationLockType::DEFAULT;
  }

  static bool FromMojom(::device::mojom::ScreenOrientationLockType lockType,
                        ::blink::WebScreenOrientationLockType* out) {
    switch (lockType) {
      case ::device::mojom::ScreenOrientationLockType::DEFAULT:
        *out = ::blink::kWebScreenOrientationLockDefault;
        return true;
      case ::device::mojom::ScreenOrientationLockType::PORTRAIT_PRIMARY:
        *out = ::blink::kWebScreenOrientationLockPortraitPrimary;
        return true;
      case ::device::mojom::ScreenOrientationLockType::PORTRAIT_SECONDARY:
        *out = ::blink::kWebScreenOrientationLockPortraitSecondary;
        return true;
      case ::device::mojom::ScreenOrientationLockType::LANDSCAPE_PRIMARY:
        *out = ::blink::kWebScreenOrientationLockLandscapePrimary;
        return true;
      case ::device::mojom::ScreenOrientationLockType::LANDSCAPE_SECONDARY:
        *out = ::blink::kWebScreenOrientationLockLandscapeSecondary;
        return true;
      case ::device::mojom::ScreenOrientationLockType::ANY:
        *out = ::blink::kWebScreenOrientationLockAny;
        return true;
      case ::device::mojom::ScreenOrientationLockType::LANDSCAPE:
        *out = ::blink::kWebScreenOrientationLockLandscape;
        return true;
      case ::device::mojom::ScreenOrientationLockType::PORTRAIT:
        *out = ::blink::kWebScreenOrientationLockPortrait;
        return true;
      case ::device::mojom::ScreenOrientationLockType::NATURAL:
        *out = ::blink::kWebScreenOrientationLockNatural;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

}  // namespace mojo

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_SCREEN_ORIENTATION_WEB_SCREEN_ORIENTATION_ENUM_TRAITS_H_
