// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_FLOAT_CLIP_DISPLAY_ITEM_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_FLOAT_CLIP_DISPLAY_ITEM_H_

#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {

class PLATFORM_EXPORT FloatClipDisplayItem final
    : public PairedBeginDisplayItem {
 public:
  FloatClipDisplayItem(const DisplayItemClient& client,
                       Type type,
                       const FloatRect& clip_rect)
      : PairedBeginDisplayItem(client, type, sizeof(*this)),
        clip_rect_(clip_rect) {
    DCHECK(IsFloatClipType(type));
  }

  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;

 private:
#if DCHECK_IS_ON()
  void PropertiesAsJSON(JSONObject&) const override;
#endif
  bool Equals(const DisplayItem& other) const final {
    return DisplayItem::Equals(other) &&
           clip_rect_ ==
               static_cast<const FloatClipDisplayItem&>(other).clip_rect_;
  }

  const FloatRect clip_rect_;
};

class PLATFORM_EXPORT EndFloatClipDisplayItem final
    : public PairedEndDisplayItem {
 public:
  EndFloatClipDisplayItem(const DisplayItemClient& client, Type type)
      : PairedEndDisplayItem(client, type, sizeof(*this)) {
    DCHECK(IsEndFloatClipType(type));
  }

  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;

 private:
#if DCHECK_IS_ON()
  bool IsEndAndPairedWith(DisplayItem::Type other_type) const final {
    return DisplayItem::IsFloatClipType(other_type);
  }
#endif
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_FLOAT_CLIP_DISPLAY_ITEM_H_
