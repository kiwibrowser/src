// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/scroll_display_item.h"

#include "cc/paint/display_item_list.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"

namespace blink {

void BeginScrollDisplayItem::Replay(GraphicsContext& context) const {
  context.Save();
  context.Translate(-current_offset_.Width(), -current_offset_.Height());
}

void BeginScrollDisplayItem::AppendToDisplayItemList(
    const FloatSize&,
    cc::DisplayItemList& list) const {
  list.StartPaint();
  list.push<cc::SaveOp>();
  list.push<cc::TranslateOp>(static_cast<float>(-current_offset_.Width()),
                             static_cast<float>(-current_offset_.Height()));
  list.EndPaintOfPairedBegin();
}

#if DCHECK_IS_ON()
void BeginScrollDisplayItem::PropertiesAsJSON(JSONObject& json) const {
  PairedBeginDisplayItem::PropertiesAsJSON(json);
  json.SetString("currentOffset", current_offset_.ToString());
}
#endif

void EndScrollDisplayItem::Replay(GraphicsContext& context) const {
  context.Restore();
}

void EndScrollDisplayItem::AppendToDisplayItemList(
    const FloatSize&,
    cc::DisplayItemList& list) const {
  list.StartPaint();
  list.push<cc::RestoreOp>();
  list.EndPaintOfPairedEnd();
}

}  // namespace blink
