// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_WEBSCROLLBARBEHAVIOR_IMPL_AURA_H_
#define CONTENT_RENDERER_WEBSCROLLBARBEHAVIOR_IMPL_AURA_H_

#include "third_party/blink/public/platform/web_scrollbar_behavior.h"

namespace content {

class WebScrollbarBehaviorImpl : public blink::WebScrollbarBehavior {
 public:
  bool ShouldCenterOnThumb(blink::WebPointerProperties::Button mouseButton,
                           bool shiftKeyPressed,
                           bool altKeyPressed) override;
  bool ShouldSnapBackToDragOrigin(const blink::WebPoint& eventPoint,
                                  const blink::WebRect& scrollbarRect,
                                  bool isHorizontal) override;
};

}  // namespace content

#endif  // CONTENT_RENDERER_WEBSCROLLBARBEHAVIOR_IMPL_AURA_H_
