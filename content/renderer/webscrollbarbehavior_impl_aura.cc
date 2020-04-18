// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/webscrollbarbehavior_impl_aura.h"

#include "build/build_config.h"
#include "third_party/blink/public/platform/web_point.h"
#include "third_party/blink/public/platform/web_rect.h"

namespace content {

bool WebScrollbarBehaviorImpl::ShouldCenterOnThumb(
    blink::WebPointerProperties::Button mouseButton,
    bool shiftKeyPressed,
    bool altKeyPressed) {
#if (defined(OS_LINUX) && !defined(OS_CHROMEOS))
  if (mouseButton == blink::WebPointerProperties::Button::kMiddle)
    return true;
#endif
  return (mouseButton == blink::WebPointerProperties::Button::kLeft) &&
         shiftKeyPressed;
}

bool WebScrollbarBehaviorImpl::ShouldSnapBackToDragOrigin(
    const blink::WebPoint& eventPoint,
    const blink::WebRect& scrollbarRect,
    bool isHorizontal) {
// Disable snapback on desktop Linux to better integrate with the desktop
// behavior.  Typically, Linux apps do not implement scrollbar snapback (this is
// true for at least GTK and QT apps).
#if (defined(OS_LINUX) && !defined(OS_CHROMEOS))
  return false;
#endif

  // Constants used to figure the drag rect outside which we should snap the
  // scrollbar thumb back to its origin. These calculations are based on
  // observing the behavior of the MSVC8 main window scrollbar + some
  // guessing/extrapolation.
  static const int kOffEndMultiplier = 3;
  static const int kOffSideMultiplier = 8;
  static const int kDefaultWinScrollbarThickness = 17;

  // Find the rect within which we shouldn't snap, by expanding the track rect
  // in both dimensions.
  gfx::Rect noSnapRect(scrollbarRect);
  int thickness = isHorizontal ? noSnapRect.height() : noSnapRect.width();
  // Even if the platform's scrollbar is narrower than the default Windows one,
  // we still want to provide at least as much slop area, since a slightly
  // narrower scrollbar doesn't necessarily imply that users will drag
  // straighter.
  thickness = std::max(thickness, kDefaultWinScrollbarThickness);
  noSnapRect.Inset(
      (isHorizontal ? kOffEndMultiplier : kOffSideMultiplier) * -thickness,
      (isHorizontal ? kOffSideMultiplier : kOffEndMultiplier) * -thickness);

  return !noSnapRect.Contains(eventPoint);
}

}  // namespace content
