// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SAD_PLUGIN_H_
#define CONTENT_RENDERER_SAD_PLUGIN_H_

#include "third_party/blink/public/platform/web_canvas.h"

class SkBitmap;

namespace gfx {
class Rect;
}

namespace content {

// Paints the sad plugin to the given canvas for the given plugin bounds. This
// is used by PPAPI out-of-process plugin impls.
void PaintSadPlugin(blink::WebCanvas* canvas,
                    const gfx::Rect& plugin_rect,
                    const SkBitmap& sad_plugin_bitmap);

}  // namespace content

#endif  // CONTENT_RENDERER_SAD_PLUGIN_H_
