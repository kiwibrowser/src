// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/clip_display_item.h"

#include "cc/paint/display_item_list.h"
#include "third_party/blink/renderer/platform/geometry/float_rounded_rect.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/skia/include/core/SkScalar.h"

namespace blink {

void ClipDisplayItem::Replay(GraphicsContext& context) const {
  context.Save();

  // RoundedInnerRectClipper only cares about rounded-rect clips,
  // and passes an "infinite" rect clip; there is no reason to apply this clip.
  // TODO(fmalita): convert RoundedInnerRectClipper to a better suited
  //   DisplayItem so we don't have to special-case its semantics.
  if (clip_rect_ != LayoutRect::InfiniteIntRect())
    context.ClipRect(clip_rect_, kAntiAliased);

  for (const FloatRoundedRect& rounded_rect : rounded_rect_clips_)
    context.ClipRoundedRect(rounded_rect);
}

void ClipDisplayItem::AppendToDisplayItemList(const FloatSize&,
                                              cc::DisplayItemList& list) const {
  list.StartPaint();
  list.push<cc::SaveOp>();
  list.push<cc::ClipRectOp>(clip_rect_, SkClipOp::kIntersect,
                            /*antialias=*/true);
  for (const FloatRoundedRect& rrect : rounded_rect_clips_) {
    SkRRect skrrect = rrect;
    if (skrrect.isRect()) {
      list.push<cc::ClipRectOp>(skrrect.rect(), SkClipOp::kIntersect,
                                /*antialias=*/true);
    } else {
      list.push<cc::ClipRRectOp>(skrrect, SkClipOp::kIntersect,
                                 /*antialias=*/true);
    }
  }
  list.EndPaintOfPairedBegin();
}

void EndClipDisplayItem::Replay(GraphicsContext& context) const {
  context.Restore();
}

void EndClipDisplayItem::AppendToDisplayItemList(
    const FloatSize&,
    cc::DisplayItemList& list) const {
  list.StartPaint();
  list.push<cc::RestoreOp>();
  list.EndPaintOfPairedEnd();
}

#if DCHECK_IS_ON()
void ClipDisplayItem::PropertiesAsJSON(JSONObject& json) const {
  DisplayItem::PropertiesAsJSON(json);
  json.SetString("clipRect", clip_rect_.ToString());
}
#endif

}  // namespace blink
