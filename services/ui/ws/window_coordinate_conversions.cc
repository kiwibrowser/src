// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/window_coordinate_conversions.h"

#include "services/ui/ws/server_window.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/point3_f.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/transform.h"

namespace ui {
namespace ws {
namespace {

gfx::Transform GetTransformToRoot(const ServerWindow* root,
                                  const ServerWindow* window) {
  gfx::Transform transform;
  const ServerWindow* w = window;
  for (; w && w != root; w = w->parent()) {
    gfx::Transform translation;
    translation.Translate(static_cast<float>(w->bounds().x()),
                          static_cast<float>(w->bounds().y()));
    if (!w->transform().IsIdentity())
      transform.ConcatTransform(w->transform());
    transform.ConcatTransform(translation);
  }
  // Allow the root to also have a transform. This mirrors how
  // WindowManagerDisplayRoot works. Ash sets the transform for rotation as well
  // as ui scale.
  if (w == root && !w->transform().IsIdentity())
    transform.ConcatTransform(w->transform());
  return transform;
}

}  // namespace

gfx::Point ConvertPointFromRootForEventDispatch(
    const ServerWindow* root,
    const ServerWindow* window,
    const gfx::Point& location_in_root) {
  // This code should only be called when |window| is connected to a display.
  DCHECK(root);

  if (root == window)
    return location_in_root;

  const gfx::Transform transform = GetTransformToRoot(root, window);
  gfx::Point3F location_in_root3(gfx::PointF{location_in_root});
  transform.TransformPointReverse(&location_in_root3);

  return gfx::ToFlooredPoint(location_in_root3.AsPointF());
}

}  // namespace ws
}  // namespace ui
