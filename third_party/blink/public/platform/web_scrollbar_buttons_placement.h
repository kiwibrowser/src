// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SCROLLBAR_BUTTONS_PLACEMENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SCROLLBAR_BUTTONS_PLACEMENT_H_

namespace blink {

enum WebScrollbarButtonsPlacement {
  kWebScrollbarButtonsPlacementNone,
  kWebScrollbarButtonsPlacementSingle,
  kWebScrollbarButtonsPlacementDoubleStart,
  kWebScrollbarButtonsPlacementDoubleEnd,
  kWebScrollbarButtonsPlacementDoubleBoth,
  kWebScrollbarButtonsPlacementLast = kWebScrollbarButtonsPlacementDoubleBoth
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_SCROLLBAR_BUTTONS_PLACEMENT_H_
