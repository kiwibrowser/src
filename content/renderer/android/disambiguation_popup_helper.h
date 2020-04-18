// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ANDROID_DISAMBIGUATION_POPUP_HELPER_H_
#define CONTENT_RENDERER_ANDROID_DISAMBIGUATION_POPUP_HELPER_H_

#include "content/common/content_export.h"
#include "third_party/blink/public/platform/web_vector.h"

namespace gfx {
class Rect;
class Size;
}

namespace blink {
struct WebRect;
}

namespace content {

// Contains functions to calculate proper scaling factor and popup size
class DisambiguationPopupHelper {
 public:
  CONTENT_EXPORT static float ComputeZoomAreaAndScaleFactor(
      const gfx::Rect& tap_rect,
      const blink::WebVector<blink::WebRect>& target_rects,
      const gfx::Size& screen_size,
      const gfx::Size& visible_content_size,
      float total_scale,
      gfx::Rect* zoom_rect);
};

}  // namespace content

#endif  // CONTENT_RENDERER_ANDROID_DISAMBIGUATION_POPUP_HELPER_H_
