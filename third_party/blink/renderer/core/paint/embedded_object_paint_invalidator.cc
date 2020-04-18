// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/embedded_object_paint_invalidator.h"

#include "third_party/blink/renderer/core/exported/web_plugin_container_impl.h"
#include "third_party/blink/renderer/core/layout/layout_embedded_object.h"
#include "third_party/blink/renderer/core/paint/box_paint_invalidator.h"

namespace blink {

PaintInvalidationReason EmbeddedObjectPaintInvalidator::InvalidatePaint() {
  PaintInvalidationReason reason =
      BoxPaintInvalidator(embedded_object_, context_).InvalidatePaint();

  WebPluginContainerImpl* plugin = embedded_object_.Plugin();
  if (plugin)
    plugin->InvalidatePaint();

  return reason;
}

}  // namespace blink
