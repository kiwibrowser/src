// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_DRAGDROP_DROP_TARGET_EVENT_H_
#define UI_BASE_DRAGDROP_DROP_TARGET_EVENT_H_

#include "base/macros.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/events/event.h"

namespace ui {

class UI_BASE_EXPORT DropTargetEvent : public LocatedEvent {
 public:
  DropTargetEvent(const OSExchangeData& data,
                  const gfx::Point& location,
                  const gfx::Point& root_location,
                  int source_operations);

  const OSExchangeData& data() const { return data_; }
  int source_operations() const { return source_operations_; }

 private:
  // Data associated with the drag/drop session.
  const OSExchangeData& data_;

  // Bitmask of supported DragDropTypes::DragOperation by the source.
  int source_operations_;

  DISALLOW_COPY_AND_ASSIGN(DropTargetEvent);
};

}  // namespace ui

#endif  // UI_BASE_DRAGDROP_DROP_TARGET_EVENT_H_

