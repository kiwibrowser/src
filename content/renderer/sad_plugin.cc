// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/sad_plugin.h"

#include <algorithm>
#include <memory>

#include "cc/paint/paint_canvas.h"
#include "cc/paint/paint_flags.h"
#include "ui/gfx/geometry/rect.h"

namespace content {

void PaintSadPlugin(blink::WebCanvas* webcanvas,
                    const gfx::Rect& plugin_rect,
                    const SkBitmap& sad_plugin_bitmap) {
  const int width = plugin_rect.width();
  const int height = plugin_rect.height();

  cc::PaintCanvas* canvas = webcanvas;
  cc::PaintCanvasAutoRestore auto_restore(canvas, true);
  // We draw the sad-plugin bitmap at the origin of canvas.
  // Add a translation so that it appears at the origin of plugin rect.
  canvas->translate(plugin_rect.x(), plugin_rect.y());

  cc::PaintFlags flags;
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(SK_ColorBLACK);
  canvas->drawRect(SkRect::MakeIWH(width, height), flags);
  canvas->drawBitmap(
      sad_plugin_bitmap,
      SkIntToScalar(std::max(0, (width - sad_plugin_bitmap.width()) / 2)),
      SkIntToScalar(std::max(0, (height - sad_plugin_bitmap.height()) / 2)));
}

}  // namespace content
