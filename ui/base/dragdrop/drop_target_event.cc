// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/dragdrop/drop_target_event.h"

#include "ui/events/event_utils.h"

namespace ui {

////////////////////////////////////////////////////////////////////////////////
// DropTargetEvent

DropTargetEvent::DropTargetEvent(const OSExchangeData& data,
                                 const gfx::Point& location,
                                 const gfx::Point& root_location,
                                 int source_operations)
    : LocatedEvent(ET_DROP_TARGET_EVENT,
                   gfx::PointF(location),
                   gfx::PointF(root_location),
                   EventTimeForNow(),
                   0),
      data_(data),
      source_operations_(source_operations) {}

}  // namespace ui

