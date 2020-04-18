// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_DETAILS_MARKER_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_DETAILS_MARKER_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

struct PaintInfo;
class Path;
class LayoutPoint;
class LayoutDetailsMarker;

class DetailsMarkerPainter {
  STACK_ALLOCATED();

 public:
  DetailsMarkerPainter(const LayoutDetailsMarker& layout_details_marker)
      : layout_details_marker_(layout_details_marker) {}

  void Paint(const PaintInfo&, const LayoutPoint& paint_offset);

 private:
  Path GetCanonicalPath() const;
  Path GetPath(const LayoutPoint& origin) const;

  const LayoutDetailsMarker& layout_details_marker_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_DETAILS_MARKER_PAINTER_H_
