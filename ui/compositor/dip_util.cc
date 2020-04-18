// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/dip_util.h"

#include "base/command_line.h"
#include "cc/layers/layer.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/dip_util.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_conversions.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/geometry/size_conversions.h"

#if DCHECK_IS_ON()
#include "ui/compositor/layer_animator.h"
#endif

namespace ui {

float GetDeviceScaleFactor(const Layer* layer) {
  return layer->device_scale_factor();
}

gfx::Point ConvertPointToDIP(const Layer* layer,
                             const gfx::Point& point_in_pixel) {
  return gfx::ConvertPointToDIP(GetDeviceScaleFactor(layer), point_in_pixel);
}

gfx::PointF ConvertPointToDIP(const Layer* layer,
                              const gfx::PointF& point_in_pixel) {
  return gfx::ConvertPointToDIP(GetDeviceScaleFactor(layer), point_in_pixel);
}

gfx::Size ConvertSizeToDIP(const Layer* layer,
                           const gfx::Size& size_in_pixel) {
  return gfx::ConvertSizeToDIP(GetDeviceScaleFactor(layer), size_in_pixel);
}

gfx::Rect ConvertRectToDIP(const Layer* layer,
                           const gfx::Rect& rect_in_pixel) {
  return gfx::ConvertRectToDIP(GetDeviceScaleFactor(layer), rect_in_pixel);
}

gfx::Point ConvertPointToPixel(const Layer* layer,
                               const gfx::Point& point_in_dip) {
  return gfx::ConvertPointToPixel(GetDeviceScaleFactor(layer), point_in_dip);
}

gfx::Size ConvertSizeToPixel(const Layer* layer,
                             const gfx::Size& size_in_dip) {
  return gfx::ConvertSizeToPixel(GetDeviceScaleFactor(layer), size_in_dip);
}

gfx::Rect ConvertRectToPixel(const Layer* layer,
                             const gfx::Rect& rect_in_dip) {
  return gfx::ConvertRectToPixel(GetDeviceScaleFactor(layer), rect_in_dip);
}

#if DCHECK_IS_ON()
namespace {

void CheckSnapped(float snapped_position) {
  // The acceptable error epsilon should be small enough to detect visible
  // artifacts as well as large enough to not cause false crashes when an
  // uncommon device scale factor is applied.
  const float kEplison = 0.003f;
  float diff = std::abs(snapped_position - gfx::ToRoundedInt(snapped_position));
  DCHECK_LT(diff, kEplison);
}

}  // namespace
#endif

void SnapLayerToPhysicalPixelBoundary(ui::Layer* snapped_layer,
                                      ui::Layer* layer_to_snap) {
  DCHECK_NE(snapped_layer, layer_to_snap);
  DCHECK(snapped_layer);
  DCHECK(snapped_layer->Contains(layer_to_snap));

  gfx::PointF view_offset(layer_to_snap->GetTargetBounds().origin());
  ui::Layer::ConvertPointToLayer(layer_to_snap->parent(), snapped_layer,
                                 &view_offset);

  float scale_factor = GetDeviceScaleFactor(layer_to_snap);
  view_offset.Scale(scale_factor);
  gfx::PointF view_offset_snapped(gfx::ToRoundedPoint(view_offset));

  gfx::Vector2dF fudge = view_offset_snapped - view_offset;
  fudge.Scale(1.0 / scale_factor);

  // Apply any scale originating from transforms to the fudge.
  gfx::Transform transform;
  layer_to_snap->parent()->GetTargetTransformRelativeTo(snapped_layer,
                                                        &transform);
  gfx::Vector2dF transform_scale = transform.Scale2d();
  fudge.Scale(1.0 / transform_scale.x(), 1.0 / transform_scale.y());

  layer_to_snap->SetSubpixelPositionOffset(fudge);
#if DCHECK_IS_ON()
  gfx::PointF layer_offset;
  gfx::PointF origin;
  Layer::ConvertPointToLayer(
      layer_to_snap->parent(), snapped_layer, &layer_offset);
  if (layer_to_snap->GetAnimator()->is_animating()) {
    origin = gfx::PointF(layer_to_snap->GetTargetBounds().origin()) +
             layer_to_snap->subpixel_position_offset();
  } else {
    origin = layer_to_snap->position();
  }
  origin.Scale(transform_scale.x(), transform_scale.y());
  CheckSnapped((layer_offset.x() + origin.x()) * scale_factor);
  CheckSnapped((layer_offset.y() + origin.y()) * scale_factor);
#endif
}

}  // namespace ui
