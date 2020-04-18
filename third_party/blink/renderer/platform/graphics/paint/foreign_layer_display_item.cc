// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/foreign_layer_display_item.h"

#include <utility>

#include "cc/layers/layer.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

ForeignLayerDisplayItem::ForeignLayerDisplayItem(
    const DisplayItemClient& client,
    Type type,
    scoped_refptr<cc::Layer> layer,
    const FloatPoint& location,
    const IntSize& bounds)
    : DisplayItem(client, type, sizeof(*this)),
      layer_(std::move(layer)),
      location_(location),
      bounds_(bounds) {
  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV2Enabled() ||
         RuntimeEnabledFeatures::BlinkGenPropertyTreesEnabled());
  DCHECK(IsForeignLayerType(type));
  DCHECK(layer_);
}

ForeignLayerDisplayItem::~ForeignLayerDisplayItem() = default;

void ForeignLayerDisplayItem::Replay(GraphicsContext&) const {
  NOTREACHED();
}

void ForeignLayerDisplayItem::AppendToDisplayItemList(
    const FloatSize&,
    cc::DisplayItemList&) const {
  NOTREACHED();
}

bool ForeignLayerDisplayItem::DrawsContent() const {
  return true;
}

bool ForeignLayerDisplayItem::Equals(const DisplayItem& other) const {
  return DisplayItem::Equals(other) &&
         layer_ == static_cast<const ForeignLayerDisplayItem&>(other).layer_;
}

#if DCHECK_IS_ON()
void ForeignLayerDisplayItem::PropertiesAsJSON(JSONObject& json) const {
  DisplayItem::PropertiesAsJSON(json);
  json.SetInteger("layer", layer_->id());
}
#endif

void RecordForeignLayer(GraphicsContext& context,
                        const DisplayItemClient& client,
                        DisplayItem::Type type,
                        scoped_refptr<cc::Layer> layer,
                        const FloatPoint& location,
                        const IntSize& bounds) {
  PaintController& paint_controller = context.GetPaintController();
  if (paint_controller.DisplayItemConstructionIsDisabled())
    return;

  paint_controller.CreateAndAppend<ForeignLayerDisplayItem>(
      client, type, std::move(layer), location, bounds);
}

}  // namespace blink
