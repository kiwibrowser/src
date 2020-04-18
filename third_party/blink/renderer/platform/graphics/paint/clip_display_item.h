// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_CLIP_DISPLAY_ITEM_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_CLIP_DISPLAY_ITEM_H_

#include "SkRegion.h"
#include "third_party/blink/renderer/platform/geometry/float_rounded_rect.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class PLATFORM_EXPORT ClipDisplayItem final : public PairedBeginDisplayItem {
 public:
  ClipDisplayItem(const DisplayItemClient& client,
                  Type type,
                  const IntRect& clip_rect)
      : PairedBeginDisplayItem(client, type, sizeof(*this)),
        clip_rect_(clip_rect) {
    DCHECK(IsClipType(type));
  }

  ClipDisplayItem(const DisplayItemClient& client,
                  Type type,
                  const IntRect& clip_rect,
                  Vector<FloatRoundedRect>& rounded_rect_clips)
      : ClipDisplayItem(client, type, clip_rect) {
    rounded_rect_clips_.swap(rounded_rect_clips);
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
               static_cast<const ClipDisplayItem&>(other).clip_rect_ &&
           rounded_rect_clips_ ==
               static_cast<const ClipDisplayItem&>(other).rounded_rect_clips_;
  }

  const IntRect clip_rect_;
  Vector<FloatRoundedRect> rounded_rect_clips_;
};

class PLATFORM_EXPORT EndClipDisplayItem final : public PairedEndDisplayItem {
 public:
  EndClipDisplayItem(const DisplayItemClient& client, Type type)
      : PairedEndDisplayItem(client, type, sizeof(*this)) {
    DCHECK(IsEndClipType(type));
  }

  void Replay(GraphicsContext&) const override;
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const override;

 private:
#if DCHECK_IS_ON()
  bool IsEndAndPairedWith(DisplayItem::Type other_type) const final {
    return DisplayItem::IsClipType(other_type);
  }
#endif
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_CLIP_DISPLAY_ITEM_H_
