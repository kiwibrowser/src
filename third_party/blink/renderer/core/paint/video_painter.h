// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_VIDEO_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_VIDEO_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

struct PaintInfo;
class LayoutPoint;
class LayoutVideo;

class VideoPainter {
  STACK_ALLOCATED();

 public:
  VideoPainter(const LayoutVideo& layout_video) : layout_video_(layout_video) {}

  void PaintReplaced(const PaintInfo&, const LayoutPoint&);

 private:
  const LayoutVideo& layout_video_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_VIDEO_PAINTER_H_
