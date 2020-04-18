// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_GRID_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_GRID_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

struct PaintInfo;
class LayoutPoint;
class LayoutGrid;

class GridPainter {
  STACK_ALLOCATED();

 public:
  GridPainter(const LayoutGrid& layout_grid) : layout_grid_(layout_grid) {}

  void PaintChildren(const PaintInfo&, const LayoutPoint&);

 private:
  const LayoutGrid& layout_grid_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_GRID_PAINTER_H_
