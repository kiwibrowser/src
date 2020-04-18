// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/ng/ng_box_clipper.h"

#include "third_party/blink/renderer/core/paint/ng/ng_paint_fragment.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

NGBoxClipper::NGBoxClipper(const NGPaintFragment& fragment,
                           const PaintInfo& paint_info) {
  DCHECK(paint_info.phase != PaintPhase::kSelfBlockBackgroundOnly &&
         paint_info.phase != PaintPhase::kSelfOutlineOnly);

  if (paint_info.phase == PaintPhase::kMask)
    return;

  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV175Enabled());
  DCHECK(fragment.GetLayoutObject());
  InitializeScopedClipProperty(
      paint_info.FragmentToPaint(*fragment.GetLayoutObject()), fragment,
      paint_info);
}

}  // namespace blink
