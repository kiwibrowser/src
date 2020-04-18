// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_EMBEDDED_CONTENT_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_EMBEDDED_CONTENT_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

struct PaintInfo;
class LayoutPoint;
class LayoutEmbeddedContent;

class EmbeddedContentPainter {
  STACK_ALLOCATED();

 public:
  EmbeddedContentPainter(const LayoutEmbeddedContent& layout_embedded_content)
      : layout_embedded_content_(layout_embedded_content) {}

  void Paint(const PaintInfo&, const LayoutPoint&);
  void PaintContents(const PaintInfo&, const LayoutPoint&);

 private:
  bool IsSelected() const;

  const LayoutEmbeddedContent& layout_embedded_content_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_EMBEDDED_CONTENT_PAINTER_H_
