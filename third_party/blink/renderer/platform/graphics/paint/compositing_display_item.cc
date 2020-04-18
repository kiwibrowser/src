// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/compositing_display_item.h"

#include "cc/paint/display_item_list.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/skia/skia_utils.h"

namespace blink {

void BeginCompositingDisplayItem::Replay(GraphicsContext& context) const {
  context.BeginLayer(opacity_, xfer_mode_, has_bounds_ ? &bounds_ : nullptr,
                     color_filter_);
}

void BeginCompositingDisplayItem::AppendToDisplayItemList(
    const FloatSize&,
    cc::DisplayItemList& list) const {
  DCHECK_GE(opacity_, 0.f);
  DCHECK_LE(opacity_, 1.f);

  // TODO(ajuma): This should really be rounding instead of flooring the alpha
  // value, but that breaks slimming paint reftests.
  auto alpha = static_cast<uint8_t>(gfx::ToFlooredInt(255 * opacity_));
  sk_sp<SkColorFilter> color_filter =
      GraphicsContext::WebCoreColorFilterToSkiaColorFilter(color_filter_);
  SkRect sk_bounds = bounds_;
  SkRect* maybe_bounds = has_bounds_ ? &sk_bounds : nullptr;

  if (xfer_mode_ == SkBlendMode::kSrcOver && !color_filter) {
    list.StartPaint();
    list.push<cc::SaveLayerAlphaOp>(maybe_bounds, alpha, false);
    if (maybe_bounds) {
      list.push<cc::ClipRectOp>(sk_bounds, SkClipOp::kIntersect, false);
    }
    list.EndPaintOfPairedBegin();
    return;
  }

  cc::PaintFlags flags;
  flags.setBlendMode(xfer_mode_);
  flags.setAlpha(alpha);
  flags.setColorFilter(std::move(color_filter));

  list.StartPaint();
  list.push<cc::SaveLayerOp>(maybe_bounds, &flags);
  if (maybe_bounds) {
    list.push<cc::ClipRectOp>(sk_bounds, SkClipOp::kIntersect, false);
  }
  list.EndPaintOfPairedBegin();
}

#if DCHECK_IS_ON()
void BeginCompositingDisplayItem::PropertiesAsJSON(JSONObject& json) const {
  DisplayItem::PropertiesAsJSON(json);
  json.SetInteger("xferMode", static_cast<int>(xfer_mode_));
  json.SetDouble("opacity", opacity_);
  if (has_bounds_)
    json.SetString("bounds", bounds_.ToString());
}
#endif

void EndCompositingDisplayItem::Replay(GraphicsContext& context) const {
  context.EndLayer();
}

void EndCompositingDisplayItem::AppendToDisplayItemList(
    const FloatSize&,
    cc::DisplayItemList& list) const {
  list.StartPaint();
  list.push<cc::RestoreOp>();
  list.EndPaintOfPairedEnd();
}

}  // namespace blink
