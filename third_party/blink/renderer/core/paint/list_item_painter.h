// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_LIST_ITEM_PAINTER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_LIST_ITEM_PAINTER_H_

#include "third_party/blink/renderer/platform/wtf/allocator.h"

namespace blink {

struct PaintInfo;
class LayoutListItem;
class LayoutPoint;

class ListItemPainter {
  STACK_ALLOCATED();

 public:
  ListItemPainter(const LayoutListItem& layout_list_item)
      : layout_list_item_(layout_list_item) {}

  void Paint(const PaintInfo&, const LayoutPoint& paint_offset);

 private:
  const LayoutListItem& layout_list_item_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_LIST_ITEM_PAINTER_H_
