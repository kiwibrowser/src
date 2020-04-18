// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/rounded_inner_rect_clipper.h"

#include "third_party/blink/renderer/core/paint/paint_info.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item_client.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"

namespace blink {

RoundedInnerRectClipper::RoundedInnerRectClipper(
    const DisplayItemClient& display_item,
    const PaintInfo& paint_info,
    const LayoutRect& rect,
    const FloatRoundedRect& clip_rect,
    RoundedInnerRectClipperBehavior behavior)
    : display_item_(display_item),
      paint_info_(paint_info),
      use_paint_controller_(behavior == kApplyToDisplayList),
      clip_type_(use_paint_controller_
                     ? paint_info_.DisplayItemTypeForClipping()
                     : DisplayItem::kClipBoxPaintPhaseFirst) {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
      use_paint_controller_)
    return;

  Vector<FloatRoundedRect> rounded_rect_clips;
  if (clip_rect.IsRenderable()) {
    rounded_rect_clips.push_back(clip_rect);
  } else {
    // We create a rounded rect for each of the corners and clip it, while
    // making sure we clip opposing corners together.
    if (!clip_rect.GetRadii().TopLeft().IsEmpty() ||
        !clip_rect.GetRadii().BottomRight().IsEmpty()) {
      FloatRect top_corner(clip_rect.Rect().X(), clip_rect.Rect().Y(),
                           rect.MaxX() - clip_rect.Rect().X(),
                           rect.MaxY() - clip_rect.Rect().Y());
      FloatRoundedRect::Radii top_corner_radii;
      top_corner_radii.SetTopLeft(clip_rect.GetRadii().TopLeft());
      rounded_rect_clips.push_back(
          FloatRoundedRect(top_corner, top_corner_radii));

      FloatRect bottom_corner(rect.X().ToFloat(), rect.Y().ToFloat(),
                              clip_rect.Rect().MaxX() - rect.X().ToFloat(),
                              clip_rect.Rect().MaxY() - rect.Y().ToFloat());
      FloatRoundedRect::Radii bottom_corner_radii;
      bottom_corner_radii.SetBottomRight(clip_rect.GetRadii().BottomRight());
      rounded_rect_clips.push_back(
          FloatRoundedRect(bottom_corner, bottom_corner_radii));
    }

    if (!clip_rect.GetRadii().TopRight().IsEmpty() ||
        !clip_rect.GetRadii().BottomLeft().IsEmpty()) {
      FloatRect top_corner(rect.X().ToFloat(), clip_rect.Rect().Y(),
                           clip_rect.Rect().MaxX() - rect.X().ToFloat(),
                           rect.MaxY() - clip_rect.Rect().Y());
      FloatRoundedRect::Radii top_corner_radii;
      top_corner_radii.SetTopRight(clip_rect.GetRadii().TopRight());
      rounded_rect_clips.push_back(
          FloatRoundedRect(top_corner, top_corner_radii));

      FloatRect bottom_corner(clip_rect.Rect().X(), rect.Y().ToFloat(),
                              rect.MaxX() - clip_rect.Rect().X(),
                              clip_rect.Rect().MaxY() - rect.Y().ToFloat());
      FloatRoundedRect::Radii bottom_corner_radii;
      bottom_corner_radii.SetBottomLeft(clip_rect.GetRadii().BottomLeft());
      rounded_rect_clips.push_back(
          FloatRoundedRect(bottom_corner, bottom_corner_radii));
    }
  }

  if (use_paint_controller_) {
    paint_info_.context.GetPaintController().CreateAndAppend<ClipDisplayItem>(
        display_item, clip_type_, LayoutRect::InfiniteIntRect(),
        rounded_rect_clips);
  } else {
    paint_info.context.Save();
    for (const auto& rrect : rounded_rect_clips)
      paint_info.context.ClipRoundedRect(rrect);
  }
}

RoundedInnerRectClipper::~RoundedInnerRectClipper() {
  if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled() &&
      use_paint_controller_)
    return;

  DisplayItem::Type end_type = DisplayItem::ClipTypeToEndClipType(clip_type_);
  if (use_paint_controller_) {
    paint_info_.context.GetPaintController().EndItem<EndClipDisplayItem>(
        display_item_, end_type);
  } else {
    paint_info_.context.Restore();
  }
}

}  // namespace blink
