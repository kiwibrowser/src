// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BLOCK_FLOW_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BLOCK_FLOW_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class LayoutPoint;
struct PaintInfo;
class LayoutBlockFlow;

class BlockFlowPainter {
  STACK_ALLOCATED();

 public:
  BlockFlowPainter(const LayoutBlockFlow& layout_block_flow)
      : layout_block_flow_(layout_block_flow) {}
  void PaintContents(const PaintInfo&, const LayoutPoint&);
  void PaintFloats(const PaintInfo&, const LayoutPoint&);

 private:
  const LayoutBlockFlow& layout_block_flow_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_BLOCK_FLOW_PAINTER_H_
