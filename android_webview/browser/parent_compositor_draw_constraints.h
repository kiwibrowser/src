// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_PARENT_COMPOSITOR_DRAW_CONSTRAINTS_H_
#define ANDROID_WEBVIEW_BROWSER_PARENT_COMPOSITOR_DRAW_CONSTRAINTS_H_

#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/transform.h"

namespace android_webview {

class ChildFrame;

struct ParentCompositorDrawConstraints {
  bool is_layer;
  gfx::Transform transform;
  bool surface_rect_empty;

  ParentCompositorDrawConstraints();
  ParentCompositorDrawConstraints(bool is_layer,
                                  const gfx::Transform& transform,
                                  bool surface_rect_empty);
  bool NeedUpdate(const ChildFrame& frame) const;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_PARENT_COMPOSITOR_DRAW_CONSTRAINTS_H_
