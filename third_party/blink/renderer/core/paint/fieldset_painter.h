// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FIELDSET_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FIELDSET_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

struct PaintInfo;
class LayoutPoint;
class LayoutFieldset;

class FieldsetPainter {
  STACK_ALLOCATED();

 public:
  FieldsetPainter(const LayoutFieldset& layout_fieldset)
      : layout_fieldset_(layout_fieldset) {}

  void PaintBoxDecorationBackground(const PaintInfo&, const LayoutPoint&);
  void PaintMask(const PaintInfo&, const LayoutPoint&);

 private:
  const LayoutFieldset& layout_fieldset_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FIELDSET_PAINTER_H_
