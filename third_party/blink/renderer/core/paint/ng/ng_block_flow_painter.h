// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_NG_NG_BLOCK_FLOW_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_NG_NG_BLOCK_FLOW_PAINTER_H_

#include "third_party/blink/renderer/core/layout/api/hit_test_action.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class HitTestResult;
class HitTestLocation;
class LayoutPoint;
struct PaintInfo;
class LayoutBlockFlow;
class NGPaintFragment;
struct PaintInfo;

// Painter for NGBlockFlow which represents the root of a LayoutNG sub-tree.
// Paints the root fragment associated with the NGBlockFlow recursively, walking
// the LayoutNG fragment tree instead of the legacy layout tree.
class NGBlockFlowPainter {
  STACK_ALLOCATED();

 public:
  NGBlockFlowPainter(const LayoutBlockFlow& layout_block_flow)
      : block_(layout_block_flow) {}
  void Paint(const PaintInfo&, const LayoutPoint&);

  bool NodeAtPoint(HitTestResult&,
                   const HitTestLocation& location_in_container,
                   const LayoutPoint& accumulated_offset,
                   const LayoutPoint& accumulated_offset_for_legacy,
                   HitTestAction);

 private:
  void PaintBoxFragment(const NGPaintFragment&,
                        const PaintInfo&,
                        const LayoutPoint&);

  const LayoutBlockFlow& block_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_NG_NG_BLOCK_FLOW_PAINTER_H_
