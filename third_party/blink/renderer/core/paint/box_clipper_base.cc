// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/box_clipper_base.h"

#include "third_party/blink/renderer/core/paint/object_paint_properties.h"
#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

DISABLE_CFI_PERF
void BoxClipperBase::InitializeScopedClipProperty(
    const FragmentData* fragment,
    const DisplayItemClient& client,
    const PaintInfo& paint_info) {
  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV175Enabled());

  if (!fragment)
    return;
  const auto* properties = fragment->PaintProperties();
  if (!properties)
    return;

  const auto* clip = properties->OverflowClip()
                         ? properties->OverflowClip()
                         : properties->InnerBorderRadiusClip();
  if (!clip)
    return;

  scoped_clip_property_.emplace(paint_info.context.GetPaintController(), clip,
                                client,
                                paint_info.DisplayItemTypeForClipping());
}

}  // namespace blink
