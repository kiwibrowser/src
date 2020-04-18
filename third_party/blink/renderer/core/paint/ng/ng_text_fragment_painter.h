// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_NG_NG_TEXT_FRAGMENT_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_NG_NG_TEXT_FRAGMENT_PAINTER_H_

#include "third_party/blink/renderer/core/style/computed_style_constants.h"
#include "third_party/blink/renderer/platform/geometry/layout_rect.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

class LayoutPoint;
class NGPaintFragment;
struct PaintInfo;

// Text fragment painter for LayoutNG. Operates on NGPhysicalTextFragments and
// handles clipping, selection, etc. Delegates to NGTextPainter to paint the
// text itself.
class NGTextFragmentPainter {
  STACK_ALLOCATED();

 public:
  explicit NGTextFragmentPainter(const NGPaintFragment&);

  void Paint(const PaintInfo&, const LayoutPoint&);

 private:
  const NGPaintFragment& fragment_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_NG_NG_TEXT_FRAGMENT_PAINTER_H_
