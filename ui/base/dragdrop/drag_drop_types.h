// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_DRAGDROP_DRAG_DROP_TYPES_H_
#define UI_BASE_DRAGDROP_DRAG_DROP_TYPES_H_

#include <stdint.h>

#include "build/build_config.h"
#include "ui/base/ui_base_export.h"

namespace ui {

class UI_BASE_EXPORT DragDropTypes {
 public:
  enum DragOperation {
    DRAG_NONE = 0,
    DRAG_MOVE = 1 << 0,
    DRAG_COPY = 1 << 1,
    DRAG_LINK = 1 << 2
  };

  enum DragEventSource {
    DRAG_EVENT_SOURCE_MOUSE = 0,
    DRAG_EVENT_SOURCE_TOUCH,
    DRAG_EVENT_SOURCE_LAST = DRAG_EVENT_SOURCE_TOUCH,
    DRAG_EVENT_SOURCE_COUNT
  };

#if defined(OS_WIN)
  static uint32_t DragOperationToDropEffect(int drag_operation);
  static int DropEffectToDragOperation(uint32_t effect);
#endif

#if defined(OS_MACOSX)
  static uint64_t DragOperationToNSDragOperation(int drag_operation);
  static int NSDragOperationToDragOperation(uint64_t ns_drag_operation);
#endif
};

}  // namespace ui

#endif  // UI_BASE_DRAGDROP_DRAG_DROP_TYPES_H_
